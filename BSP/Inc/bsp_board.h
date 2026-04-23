#ifndef BSP_BOARD_H
#define BSP_BOARD_H

#include <stdbool.h>

void BSP_Board_Init(void);
bool BSP_Board_IsKeyPressed(void);
bool BSP_Board_IsDrvFaultActive(void);
void BSP_Board_SetMotorEnable(bool enable);
float BSP_Board_ReadTemperatureC(void);
float BSP_Board_ReadBusVoltageV(void);

#endif
