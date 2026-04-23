#include "task_safety.h"

#include "app_motor.h"
#include "bsp_board.h"

#define BOARD_OVER_TEMPERATURE_C 85.0f

void TaskSafety_Init(void)
{
    BSP_Board_SetMotorEnable(false);
}

void TaskSafety_Run(void)
{
    MotorControlState *state = AppMotor_GetState();

    state->telemetry.board_temperature_c = BSP_Board_ReadTemperatureC();

    if (BSP_Board_IsDrvFaultActive())
    {
        AppMotor_SetFault(MOTOR_FAULT_GATE_DRIVER);
    }

    if (state->telemetry.board_temperature_c >= BOARD_OVER_TEMPERATURE_C)
    {
        AppMotor_SetFault(MOTOR_FAULT_OVER_TEMPERATURE);
    }

    if (state->fault_flags != 0u)
    {
        BSP_Board_SetMotorEnable(false);
        state->gate_driver_enabled = false;
    }
    else
    {
        BSP_Board_SetMotorEnable(state->gate_driver_enabled);
    }
}
