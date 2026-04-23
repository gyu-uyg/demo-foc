#include "task_key.h"

#include "app_motor.h"
#include "bsp_board.h"

typedef struct
{
    unsigned char stable_count;
    bool stable_pressed;
    bool last_sample;
} KeyFilter;

static KeyFilter g_key_filter;

void TaskKey_Init(void)
{
    g_key_filter.stable_count = 0u;
    g_key_filter.stable_pressed = false;
    g_key_filter.last_sample = false;
}

void TaskKey_Run(void)
{
    bool sample = BSP_Board_IsKeyPressed();

    if (sample == g_key_filter.last_sample)
    {
        if (g_key_filter.stable_count < 3u)
        {
            g_key_filter.stable_count++;
        }
    }
    else
    {
        g_key_filter.stable_count = 0u;
    }

    if ((g_key_filter.stable_count >= 2u) && (sample != g_key_filter.stable_pressed))
    {
        g_key_filter.stable_pressed = sample;
        if (sample)
        {
            AppMotor_CycleMode();
        }
    }

    g_key_filter.last_sample = sample;
}
