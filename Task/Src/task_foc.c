#include "task_foc.h"

#include "app_motor.h"
#include "adc.h"
#include "bsp_board.h"
#include "tim.h"

#include <math.h>
#include <stdbool.h>
#include <stdint.h>

#define FOC_TWO_PI                     6.28318530718f
#define FOC_SQRT3                      1.73205080757f
#define FOC_INV_SQRT3                  0.57735026919f   /*根号3的倒数*/
#define FOC_ADC_FULL_SCALE             4095.0f
#define FOC_ADC_REF_VOLTAGE            3.3f
#define FOC_CURRENT_ZERO_A_COUNTS      2048.0f
#define FOC_CURRENT_ZERO_B_COUNTS      2048.0f
#define FOC_CURRENT_SENSOR_V_PER_A     0.5f         /*电流每变化 1A，ADC 看到的电压变化 0.5V */
#define FOC_ENCODER_COUNTS_PER_REV     4096.0f
#define FOC_ENCODER_TIMER_COUNTS       65536
#define FOC_ENCODER_TIMER_HALF_COUNTS  32768
#define FOC_MOTOR_POLE_PAIRS           7.0f
#define FOC_ID_KP                      0.8f
#define FOC_ID_KI                      120.0f
#define FOC_IQ_KP                      0.8f
#define FOC_IQ_KI                      120.0f
#define FOC_CURRENT_REF_LIMIT_A        3.0f
#define FOC_MIN_VALID_BUS_VOLTAGE      6.0f
#define FOC_ADC_TIMEOUT_TICKS          1u

typedef struct
{
    float kp;
    float ki;
    float integral;
    float output_min;
    float output_max;
} FocPiController;

typedef struct
{
    float alpha;
    float beta;
} AlphaBeta;

typedef struct
{
    float d;
    float q;
} DqFrame;

static void TaskFoc_SampleFeedback(MotorControlState *state);
static void TaskFoc_RunControl(MotorControlState *state);
static void TaskFoc_ApplyPwm(const MotorControlState *state);
static uint16_t TaskFoc_ReadAdc1Raw(uint32_t channel, uint16_t fallback);
static float TaskFoc_GetTimerPeriodS(const TIM_HandleTypeDef *htim);
static int32_t TaskFoc_EncoderDelta(int32_t current_count, int32_t last_count);
static float TaskFoc_EncoderMechanicalAngle(int32_t encoder_count);
static float TaskFoc_NormalizeAngle(float angle_rad);
static float TaskFoc_Clamp(float value, float min_value, float max_value);
static float TaskFoc_Min(float a, float b);
static float TaskFoc_Max(float a, float b);
static FocPiController TaskFoc_MakePi(float kp, float ki);
static float TaskFoc_RunPi(FocPiController *controller, float error, float dt_s);
static AlphaBeta TaskFoc_Clarke(float ia, float ib);
static DqFrame TaskFoc_Park(AlphaBeta input, float sin_theta, float cos_theta);
static AlphaBeta TaskFoc_RevPark(DqFrame input, float sin_theta, float cos_theta);
static void TaskFoc_Svpwm(float v_alpha,
                          float v_beta,
                          float bus_voltage,
                          uint16_t pwm_period,
                          MotorPwmOutput *output);
static void TaskFoc_DisableOutput(MotorControlState *state);

static FocPiController g_id_pi;
static FocPiController g_iq_pi;
static float g_current_loop_dt_s;

void TaskFoc_Init(void)
{
    g_id_pi = TaskFoc_MakePi(FOC_ID_KP, FOC_ID_KI);
    g_iq_pi = TaskFoc_MakePi(FOC_IQ_KP, FOC_IQ_KI);
    g_current_loop_dt_s = TaskFoc_GetTimerPeriodS(&htim3);

    (void)HAL_ADCEx_Calibration_Start(&hadc1);
    (void)HAL_ADCEx_Calibration_Start(&hadc2);

    HAL_TIM_Encoder_Start(&htim1, TIM_CHANNEL_ALL);
    HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_1);
    HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_2);
    HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_3);
    HAL_TIM_Base_Start_IT(&htim3);
}

void TaskFoc_FastLoopISR(void)
{
    MotorControlState *state = AppMotor_GetState();

    state->foc_cycle_counter++;

    TaskFoc_SampleFeedback(state);
    TaskFoc_RunControl(state);
    TaskFoc_ApplyPwm(state);
}

static void TaskFoc_SampleFeedback(MotorControlState *state)
{
    static int32_t last_encoder_count = 0;
    int32_t encoder_count = (int32_t)__HAL_TIM_GET_COUNTER(&htim1);
    uint16_t adc_a = TaskFoc_ReadAdc1Raw(ADC_CHANNEL_0, (uint16_t)FOC_CURRENT_ZERO_A_COUNTS);
    uint16_t adc_b = TaskFoc_ReadAdc1Raw(ADC_CHANNEL_1, (uint16_t)FOC_CURRENT_ZERO_B_COUNTS);

    /*
    在INA240中，Vout = Vref + (I * Rshunt * Gain)，其中Vref是当电流为0时ADC看到的电压，Rshunt是采样电阻阻值，Gain是INA240的增益倍数。
    Vout = (Vref1 + Vref2)/2 + (I * Rshunt * Gain)
         = (0 + 3.3)/2 + (I * 0.01 * 50)
         = 1.65 + (I * 0.5) 
    这个Vout会被ADC采样到，并转换成数字值，也就是adc_a,adc_b。
    I = (Vout - 1.65) / 0.5
      = ((adc_counts / 4095) * 3.3 - 1.65) / 0.5
      = ((adc_counts - 2048) * 3.3) / (4096 * 0.5)
    */
    float current_a = (((float)adc_a - FOC_CURRENT_ZERO_A_COUNTS) * FOC_ADC_REF_VOLTAGE) /
                      (FOC_ADC_FULL_SCALE * FOC_CURRENT_SENSOR_V_PER_A);
    float current_b = (((float)adc_b - FOC_CURRENT_ZERO_B_COUNTS) * FOC_ADC_REF_VOLTAGE) /
                      (FOC_ADC_FULL_SCALE * FOC_CURRENT_SENSOR_V_PER_A);
    float mechanical_angle = TaskFoc_EncoderMechanicalAngle(encoder_count);

    state->telemetry.encoder_count = encoder_count;

    /*两次采集encoder_count之差*/
    state->telemetry.speed_counts_per_cycle =
        TaskFoc_EncoderDelta(encoder_count, last_encoder_count);

    state->telemetry.phase_current_a = current_a;
    state->telemetry.phase_current_b = current_b;
    state->telemetry.phase_current_c = -(current_a + current_b);

    /*机械角乘以极对数得到电角度，并规范化到0-2π*/
    state->telemetry.electrical_angle_rad =
        TaskFoc_NormalizeAngle(mechanical_angle * FOC_MOTOR_POLE_PAIRS);

    state->telemetry.bus_voltage_v = BSP_Board_ReadBusVoltageV();
    last_encoder_count = encoder_count;
}

static void TaskFoc_RunControl(MotorControlState *state)
{
    float id_ref = 0.0f;
    float iq_ref = TaskFoc_Clamp(state->command.target_torque,
                                 -FOC_CURRENT_REF_LIMIT_A,
                                 FOC_CURRENT_REF_LIMIT_A);
    float voltage_limit = 0.0f;
    float sin_theta = 0.0f;
    float cos_theta = 1.0f;
    AlphaBeta i_ab = {0.0f, 0.0f};
    DqFrame i_dq = {0.0f, 0.0f};
    DqFrame v_dq = {0.0f, 0.0f};
    AlphaBeta v_ab = {0.0f, 0.0f};

    state->foc_loop_ready = true;

    if ((state->fault_flags != 0u) ||
        (state->mode == MOTOR_MODE_STANDBY) ||
        (state->mode == MOTOR_MODE_ENCODER_CALIBRATION) ||
        (state->telemetry.bus_voltage_v < FOC_MIN_VALID_BUS_VOLTAGE))
    {
        state->gate_driver_enabled = false;
        TaskFoc_DisableOutput(state);
        return;
    }

    state->gate_driver_enabled = true;
    voltage_limit = state->telemetry.bus_voltage_v * FOC_INV_SQRT3;/*根据当前母线电压，算出 FOC 电压指令最多允许给6.93v*/
    g_id_pi.output_min = -voltage_limit;
    g_id_pi.output_max = voltage_limit;
    g_iq_pi.output_min = -voltage_limit;
    g_iq_pi.output_max = voltage_limit;

    sin_theta = sinf(state->telemetry.electrical_angle_rad);
    cos_theta = cosf(state->telemetry.electrical_angle_rad);

    /*三个变量转换为分析两个变量*/
    i_ab = TaskFoc_Clarke(state->telemetry.phase_current_a,
                          state->telemetry.phase_current_b);

    /*两个非线性变量转换为分析两个线性变量*/
    i_dq = TaskFoc_Park(i_ab, sin_theta, cos_theta);

    /*运行PI控制器*/
    v_dq.d = TaskFoc_RunPi(&g_id_pi, id_ref - i_dq.d, g_current_loop_dt_s);
    v_dq.q = TaskFoc_RunPi(&g_iq_pi, iq_ref - i_dq.q, g_current_loop_dt_s);

    /*反Park变换*/
    v_ab = TaskFoc_RevPark(v_dq, sin_theta, cos_theta);

    state->telemetry.i_alpha = i_ab.alpha;
    state->telemetry.i_beta = i_ab.beta;
    state->telemetry.i_d = i_dq.d;
    state->telemetry.i_q = i_dq.q;
    state->telemetry.v_d = v_dq.d;
    state->telemetry.v_q = v_dq.q;
    state->telemetry.v_alpha = v_ab.alpha;
    state->telemetry.v_beta = v_ab.beta;

    TaskFoc_Svpwm(v_ab.alpha,
                  v_ab.beta,
                  state->telemetry.bus_voltage_v,
                  (uint16_t)__HAL_TIM_GET_AUTORELOAD(&htim3),
                  &state->pwm);
}

static void TaskFoc_ApplyPwm(const MotorControlState *state)
{
    __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_1, state->pwm.pwm_u);
    __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_2, state->pwm.pwm_v);
    __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_3, state->pwm.pwm_w);
}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    if (htim->Instance == TIM3)
    {
        TaskFoc_FastLoopISR();
    }
}

static uint16_t TaskFoc_ReadAdc1Raw(uint32_t channel, uint16_t fallback)
{
    ADC_ChannelConfTypeDef config = {
        .Channel = channel,
        .Rank = ADC_REGULAR_RANK_1,
        .SamplingTime = ADC_SAMPLETIME_7CYCLES_5,
    };
    uint16_t value = fallback;

    if (HAL_ADC_ConfigChannel(&hadc1, &config) == HAL_OK)
    {
        if (HAL_ADC_Start(&hadc1) == HAL_OK)
        {
            if (HAL_ADC_PollForConversion(&hadc1, FOC_ADC_TIMEOUT_TICKS) == HAL_OK)
            {
                value = (uint16_t)HAL_ADC_GetValue(&hadc1);
            }
            (void)HAL_ADC_Stop(&hadc1);
        }
    }

    return value;
}

static float TaskFoc_GetTimerPeriodS(const TIM_HandleTypeDef *htim)
{
    uint32_t timer_clock_hz = HAL_RCC_GetPCLK1Freq();
    uint32_t hclk_hz = HAL_RCC_GetHCLKFreq();
    float ticks_per_period = (float)(htim->Init.Period + 1u) * (float)(htim->Init.Prescaler + 1u);

    if (timer_clock_hz != hclk_hz)
    {
        timer_clock_hz *= 2u;
    }

    return ticks_per_period / (float)timer_clock_hz;
}

static int32_t TaskFoc_EncoderDelta(int32_t current_count, int32_t last_count)
{
    int32_t delta = current_count - last_count;

    /*
    last = 65530
    current = 5
    delta = 5 - 65530 = -65525
    这种情况就是变量反转了，实际增量应该是 +11，而不是 -65525
     delta < -32768, 说明delta过大了，说明发生了反转了，应该加上65536，得到正确的增量11
    反过来，如果last=5，current=65530，delta=65525
    这种情况就是变量反转了，实际增量应该是 -11，而不是65525
     delta > 32768, 说明delta过大了，说明发生了反转了，应该减去65536，得到正确的增量-11

    */

    if (delta > FOC_ENCODER_TIMER_HALF_COUNTS)
    {
        delta -= FOC_ENCODER_TIMER_COUNTS;
    }
    else if (delta < -FOC_ENCODER_TIMER_HALF_COUNTS)
    {
        delta += FOC_ENCODER_TIMER_COUNTS;
    }

    return delta;
}

static float TaskFoc_EncoderMechanicalAngle(int32_t encoder_count)
{
    /*
    在AS5047中，以下为一个周期
    A   B
    0   0   0-90度
    0   1   90-180度
    1   1   180-270度
    1   0   270-360度
    此时，我们把分辨率改小，一圈为1024个周期，那么每个周期就是4个计数（00,01,11,10），所以每转4096个计数。
    */
    float count_in_turn = fmodf((float)encoder_count, FOC_ENCODER_COUNTS_PER_REV);/*encoder_count除以FOC_ENCODER_COUNTS_PER_REV的余数，将count_in_turn限制在0-4096*/

    if (count_in_turn < 0.0f)
    {
        count_in_turn += FOC_ENCODER_COUNTS_PER_REV;//说明是反转
    }

    return (count_in_turn / FOC_ENCODER_COUNTS_PER_REV) * FOC_TWO_PI;//将count_in_turn映射到0-2π，也就是当前机械角度
}

static float TaskFoc_NormalizeAngle(float angle_rad)
{
    while (angle_rad >= FOC_TWO_PI)
    {
        angle_rad -= FOC_TWO_PI;
    }
    while (angle_rad < 0.0f)
    {
        angle_rad += FOC_TWO_PI;
    }
    return angle_rad;
}

static float TaskFoc_Clamp(float value, float min_value, float max_value)
{
    if (value < min_value)
    {
        return min_value;
    }
    if (value > max_value)
    {
        return max_value;
    }
    return value;
}

static float TaskFoc_Min(float a, float b)
{
    return (a < b) ? a : b;
}

static float TaskFoc_Max(float a, float b)
{
    return (a > b) ? a : b;
}

static FocPiController TaskFoc_MakePi(float kp, float ki)
{
    FocPiController controller = {
        .kp = kp,
        .ki = ki,
        .integral = 0.0f,
        .output_min = -1.0f,
        .output_max = 1.0f,
    };

    return controller;
}

static float TaskFoc_RunPi(FocPiController *controller, float error, float dt_s)
{
    float proportional = controller->kp * error;
    float integral_candidate = controller->integral + (controller->ki * error * dt_s);
    float output = proportional + integral_candidate;
    float clamped = TaskFoc_Clamp(output, controller->output_min, controller->output_max);

    if (output == clamped)
    {
        controller->integral = integral_candidate;
    }

    return clamped;
}

static AlphaBeta TaskFoc_Clarke(float ia, float ib)
{
    AlphaBeta output = {
        .alpha = ia,
        .beta = (ia + (2.0f * ib)) * FOC_INV_SQRT3,
    };

    return output;
}

static DqFrame TaskFoc_Park(AlphaBeta input, float sin_theta, float cos_theta)
{
    DqFrame output = {
        .d = (input.alpha * cos_theta) + (input.beta * sin_theta),
        .q = (-input.alpha * sin_theta) + (input.beta * cos_theta),
    };

    return output;
}

static AlphaBeta TaskFoc_RevPark(DqFrame input, float sin_theta, float cos_theta)
{
    AlphaBeta output = {
        .alpha = (input.d * cos_theta) - (input.q * sin_theta),
        .beta = (input.d * sin_theta) + (input.q * cos_theta),
    };

    return output;
}

static void TaskFoc_Svpwm(float v_alpha,
                          float v_beta,
                          float bus_voltage,
                          uint16_t pwm_period,
                          MotorPwmOutput *output)
{
    float va = v_alpha;
    float vb = (-0.5f * v_alpha) + ((FOC_SQRT3 * 0.5f) * v_beta);
    float vc = (-0.5f * v_alpha) - ((FOC_SQRT3 * 0.5f) * v_beta);
    float v_max = TaskFoc_Max(va, TaskFoc_Max(vb, vc));
    float v_min = TaskFoc_Min(va, TaskFoc_Min(vb, vc));
    float v_offset = 0.5f * (v_max + v_min);//把三相电压整体平移，让最大值和最小值关于 0 对称。电机真正感受到的是线电压，所以可以平移
    float duty_a = 0.5f;
    float duty_b = 0.5f;
    float duty_c = 0.5f;

    if (bus_voltage > FOC_MIN_VALID_BUS_VOLTAGE)
    {
        /*
        由于电压有负值，但占空比必须在0-1之间。
        */
        duty_a = ((va - v_offset) / bus_voltage) + 0.5f;
        duty_b = ((vb - v_offset) / bus_voltage) + 0.5f;
        duty_c = ((vc - v_offset) / bus_voltage) + 0.5f;
    }

    duty_a = TaskFoc_Clamp(duty_a, 0.0f, 1.0f);
    duty_b = TaskFoc_Clamp(duty_b, 0.0f, 1.0f);
    duty_c = TaskFoc_Clamp(duty_c, 0.0f, 1.0f);

    output->pwm_u = (uint16_t)(duty_a * (float)pwm_period);
    output->pwm_v = (uint16_t)(duty_b * (float)pwm_period);
    output->pwm_w = (uint16_t)(duty_c * (float)pwm_period);
}

static void TaskFoc_DisableOutput(MotorControlState *state)
{
    state->pwm.pwm_u = 0u;
    state->pwm.pwm_v = 0u;
    state->pwm.pwm_w = 0u;
    g_id_pi.integral = 0.0f;
    g_iq_pi.integral = 0.0f;
}
