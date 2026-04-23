#include "task_manager.h"

#include "sys_time.h"
#include "task_comm.h"
#include "task_key.h"
#include "task_oled.h"
#include "task_safety.h"

typedef void (*TaskCallback)(void);

typedef struct
{
    unsigned long period_ms;
    unsigned long next_release_ms;
    TaskCallback callback;
} SoftTask;

static SoftTask g_soft_tasks[] = {
    {1u, 0u, TaskSafety_Run},
    {2u, 0u, TaskComm_Run},
    {20u, 0u, TaskKey_Run},
    {50u, 0u, TaskOled_Run},
};

void TaskManager_Init(void)
{
    unsigned long now_ms = SysTime_GetMillis();
    unsigned long index = 0u;

    TaskSafety_Init();
    TaskComm_Init();
    TaskKey_Init();
    TaskOled_Init();

    for (index = 0u; index < (sizeof(g_soft_tasks) / sizeof(g_soft_tasks[0])); index++)
    {
        g_soft_tasks[index].next_release_ms = now_ms + g_soft_tasks[index].period_ms;
    }
}

void TaskManager_RunOnce(void)
{
    unsigned long now_ms = SysTime_GetMillis();
    unsigned long index = 0u;

    for (index = 0u; index < (sizeof(g_soft_tasks) / sizeof(g_soft_tasks[0])); index++)
    {
        SoftTask *task = &g_soft_tasks[index];

        if (SysTime_IsDue(now_ms, task->next_release_ms))
        {
            task->next_release_ms += task->period_ms;
            task->callback();
        }
    }
}
