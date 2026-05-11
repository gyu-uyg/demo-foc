#include "app_motor.h"

#include <string.h>

static MotorControlState g_motor_state;

void AppMotor_Init(void)
{
    (void)memset(&g_motor_state, 0, sizeof(g_motor_state));
    g_motor_state.mode = MOTOR_MODE_STANDBY;
    g_motor_state.telemetry.board_temperature_c = 25.0f;
    g_motor_state.telemetry.bus_voltage_v = 12.0f;
}

MotorControlState *AppMotor_GetState(void)
{
    return &g_motor_state;
}

const MotorControlState *AppMotor_GetStateConst(void)
{
    return &g_motor_state;
}

void AppMotor_SetFault(MotorFaultFlags fault)
{
    g_motor_state.fault_flags |= (uint32_t)fault;
    g_motor_state.gate_driver_enabled = false;
}

void AppMotor_ClearFault(MotorFaultFlags fault)
{
    g_motor_state.fault_flags &= ~((uint32_t)fault);
}

bool AppMotor_HasFault(MotorFaultFlags fault)
{
    return (g_motor_state.fault_flags & (uint32_t)fault) != 0u;
}

void AppMotor_SetMode(MotorMode mode)
{
    g_motor_state.mode = mode;

    if ((mode == MOTOR_MODE_STANDBY) || (g_motor_state.fault_flags != 0u))
    {
        g_motor_state.gate_driver_enabled = false;
    }
}

void AppMotor_CycleMode(void)
{
    switch (g_motor_state.mode)
    {
    case MOTOR_MODE_STANDBY:
        AppMotor_SetMode(MOTOR_MODE_ENCODER_CALIBRATION);
        break;
    case MOTOR_MODE_ENCODER_CALIBRATION:
        AppMotor_SetMode(MOTOR_MODE_SPEED_CLOSED_LOOP);
        break;
    case MOTOR_MODE_SPEED_CLOSED_LOOP:
        AppMotor_SetMode(MOTOR_MODE_POSITION_CLOSED_LOOP);
        break;
    case MOTOR_MODE_POSITION_CLOSED_LOOP:
    default:
        AppMotor_SetMode(MOTOR_MODE_STANDBY);
        break;
    }
}

const char *AppMotor_ModeName(MotorMode mode)
{
    switch (mode)
    {
    case MOTOR_MODE_STANDBY:
        return "Standby";
    case MOTOR_MODE_ENCODER_CALIBRATION:
        return "Calib";
    case MOTOR_MODE_SPEED_CLOSED_LOOP:
        return "Speed";
    case MOTOR_MODE_POSITION_CLOSED_LOOP:
        return "Position";
    default:
        return "Unknown";
    }
}
