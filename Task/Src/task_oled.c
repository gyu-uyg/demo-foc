#include "task_oled.h"

#include "app_motor.h"

#include <stdio.h>

void TaskOled_Init(void)
{
}

void TaskOled_Run(void)
{
    const MotorControlState *state = AppMotor_GetStateConst();
    char line_buffer[64];

    (void)snprintf(line_buffer,
                   sizeof(line_buffer),
                   "Mode:%s Temp:%.1fC",
                   AppMotor_ModeName(state->mode),
                   (double)state->telemetry.board_temperature_c);

    /* Placeholder:
     * - render telemetry into OLED line buffers
     * - push the frame through I2C
     */
    (void)line_buffer;
}
