#include "sys_time.h"

#include "main.h"

uint32_t SysTime_GetMillis(void)
{
    return HAL_GetTick();
}

bool SysTime_IsDue(uint32_t now_ms, uint32_t deadline_ms)
{
    return (int32_t)(now_ms - deadline_ms) >= 0;
}
