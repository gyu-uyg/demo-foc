#ifndef APP_MOTOR_H
#define APP_MOTOR_H

#include <stdbool.h>
#include <stdint.h>

typedef enum
{
    MOTOR_MODE_STANDBY = 0,
    MOTOR_MODE_ENCODER_CALIBRATION,
    MOTOR_MODE_SPEED_CLOSED_LOOP,
    MOTOR_MODE_POSITION_CLOSED_LOOP
} MotorMode;

typedef enum
{
    MOTOR_FAULT_NONE = 0u,
    MOTOR_FAULT_GATE_DRIVER = 1u << 0,
    MOTOR_FAULT_OVER_TEMPERATURE = 1u << 1
} MotorFaultFlags;

typedef struct
{
    int32_t encoder_count;
    int32_t speed_counts_per_cycle;
    float phase_current_a;
    float phase_current_b;
    float phase_current_c;
    float i_alpha;
    float i_beta;
    float i_d;
    float i_q;
    float electrical_angle_rad;
    float v_alpha;
    float v_beta;
    float v_d;
    float v_q;
    float board_temperature_c;
    float bus_voltage_v;
} MotorTelemetry;

typedef struct
{
    float target_position;
    float target_speed;
    float target_torque;
} MotorCommand;

typedef struct
{
    uint16_t pwm_u;
    uint16_t pwm_v;
    uint16_t pwm_w;
} MotorPwmOutput;

typedef struct
{
    MotorMode mode;              /* 当前电机控制模式：待机、编码器校准、速度闭环或位置闭环。 */
    uint32_t fault_flags;        /* 故障标志位，对应 MotorFaultFlags；非 0 时应关闭电机输出。 */
    bool gate_driver_enabled;    /* 软件侧的驱动使能请求，用来控制电机驱动输出级是否打开。 */
    bool foc_loop_ready;         /* FOC 快速控制环至少运行过一次后置位。 */
    uint32_t foc_cycle_counter;  /* FOC 快速中断控制环自启动以来的运行次数。 */
    uint32_t slow_task_counter;  /* 慢速任务/通信任务自启动以来的运行次数。 */
    MotorTelemetry telemetry;    /* 最新采样和计算得到的反馈量，用于控制和调试观察。 */
    MotorCommand command;        /* 用户或外层控制环给出的目标命令，如位置、速度、力矩电流。 */
    MotorPwmOutput pwm;          /* FOC/SVPWM 计算出的三相 PWM 比较值。 */
} MotorControlState;

void AppMotor_Init(void);
MotorControlState *AppMotor_GetState(void);
const MotorControlState *AppMotor_GetStateConst(void);
void AppMotor_SetFault(MotorFaultFlags fault);
void AppMotor_ClearFault(MotorFaultFlags fault);
bool AppMotor_HasFault(MotorFaultFlags fault);
void AppMotor_SetMode(MotorMode mode);
void AppMotor_CycleMode(void);
const char *AppMotor_ModeName(MotorMode mode);

#endif
