#include "task_foc.h"

#include "app_motor.h"
#include "bsp_board.h"
#include "tim.h"

static void TaskFoc_SampleFeedback(MotorControlState *state);
static void TaskFoc_RunControl(MotorControlState *state);
static void TaskFoc_ApplyPwm(const MotorControlState *state);

void TaskFoc_Init(void)
{
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

    state->telemetry.encoder_count = encoder_count;
    state->telemetry.speed_counts_per_cycle = encoder_count - last_encoder_count;
    state->telemetry.phase_current_a = 0.0f;
    state->telemetry.phase_current_b = 0.0f;
    state->telemetry.bus_voltage_v = BSP_Board_ReadBusVoltageV();

    last_encoder_count = encoder_count;
}

static void TaskFoc_RunControl(MotorControlState *state)
{
    state->foc_loop_ready = true;

    /* Keep PWM output disabled until the real Clarke/Park/PID/SVPWM chain is wired in. */
    state->pwm.pwm_u = 0u;
    state->pwm.pwm_v = 0u;
    state->pwm.pwm_w = 0u;

    (void)state;
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
