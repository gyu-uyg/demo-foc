#ifndef SYS_TIME_H
#define SYS_TIME_H

#include <stdbool.h>
#include <stdint.h>

uint32_t SysTime_GetMillis(void);
bool SysTime_IsDue(uint32_t now_ms, uint32_t deadline_ms);

#endif
