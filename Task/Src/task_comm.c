#include "task_comm.h"

#include "app_motor.h"


void TaskComm_Init(void)
{
}

void TaskComm_Run(void)
{
    MotorControlState *state = AppMotor_GetState();

    state->slow_task_counter++;
}
