#include "bsp_board.h"

#include "main.h"

void BSP_Board_Init(void)
{
    BSP_Board_SetMotorEnable(false);
}

bool BSP_Board_IsKeyPressed(void)
{
    return HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_13) == GPIO_PIN_RESET;
}

bool BSP_Board_IsDrvFaultActive(void)
{
    return HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_14) == GPIO_PIN_RESET;
}

void BSP_Board_SetMotorEnable(bool enable)
{
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, enable ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

float BSP_Board_ReadTemperatureC(void)
{
    return 25.0f;
}

float BSP_Board_ReadBusVoltageV(void)
{
    return 24.0f;
}
