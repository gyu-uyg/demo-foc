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
    MotorMode mode;
    uint32_t fault_flags;
    bool gate_driver_enabled;
    bool foc_loop_ready;
    uint32_t foc_cycle_counter;
    uint32_t slow_task_counter;
    MotorTelemetry telemetry;
    MotorCommand command;
    MotorPwmOutput pwm;
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
