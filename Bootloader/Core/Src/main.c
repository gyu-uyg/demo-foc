#include "main.h"
#include "bootloader.h"
#include "usb_device.h"
#include "usbd_core.h"

extern USBD_HandleTypeDef hUsbDeviceFS;

/* USB连接检测超时时间（单位：ms）*/
#define USB_DETECT_TIMEOUT_MS  2000U

/**
  * @brief  检测USB是否连接（通过检查USB配置状态）
  * @retval 1: USB已枚举/配置完成  0: USB未连接或超时
  */
static uint8_t USB_IsConnected(void)
{
    return (hUsbDeviceFS.dev_state == USBD_STATE_CONFIGURED) ? 1U : 0U;
}

void SystemClock_Config(void)
{
    RCC_OscInitTypeDef osc = {0};
    RCC_ClkInitTypeDef clk = {0};
    RCC_PeriphCLKInitTypeDef periph = {0};

    osc.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    osc.HSEState = RCC_HSE_ON;
    osc.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
    osc.HSIState = RCC_HSI_ON;
    osc.PLL.PLLState = RCC_PLL_ON;
    osc.PLL.PLLSource = RCC_PLLSOURCE_HSE;
    osc.PLL.PLLMUL = RCC_PLL_MUL9;
    if (HAL_RCC_OscConfig(&osc) != HAL_OK)
    {
        Error_Handler();
    }

    clk.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK |
                    RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    clk.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    clk.AHBCLKDivider = RCC_SYSCLK_DIV1;
    clk.APB1CLKDivider = RCC_HCLK_DIV2;
    clk.APB2CLKDivider = RCC_HCLK_DIV1;
    if (HAL_RCC_ClockConfig(&clk, FLASH_LATENCY_2) != HAL_OK)
    {
        Error_Handler();
    }

    periph.PeriphClockSelection = RCC_PERIPHCLK_USB;
    periph.UsbClockSelection = RCC_USBCLKSOURCE_PLL_DIV1_5;
    if (HAL_RCCEx_PeriphCLKConfig(&periph) != HAL_OK)
    {
        Error_Handler();
    }
}

uint8_t g_upgrade_complete = 0; // 升级完成标志位

int main(void)
{
    uint32_t usb_detect_tick = 0U;

    HAL_Init();
    SystemClock_Config();

    GPIO_InitTypeDef GPIO_InitStruct = {0};

    /* 1. 开启 GPIOC 时钟 */
    __HAL_RCC_GPIOC_CLK_ENABLE();

    /* 2. 配置 PC13 引脚属性 */
    GPIO_InitStruct.Pin = GPIO_PIN_13;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;     // 输入模式
    GPIO_InitStruct.Pull = GPIO_PULLUP;        // 开启内部上拉（如果按键按下是接地）
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

    /* 1. 检查App是否有效 */
    uint8_t app_valid = Bootloader_AppIsValid();

    if (HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_13) != GPIO_PIN_RESET)
    {
        /* 如果没有按下按键，且 App 有效，则直接跳转 */
        if (app_valid)
        {
            Bootloader_JumpToApp();
        }
        /* 如果 App 无效，即使没按键也会向下运行进入 DFU */
    }

    /* 2. 初始化USB设备 */
    MX_USB_DEVICE_Init();


    /* 6. 进入DFU升级循环 */
    while (1)
    {
        /* 检查升级是否完成 */
        if (g_upgrade_complete == 1)
        {
            HAL_Delay(500);

            /* 验证升级的App是否有效 */
            if (Bootloader_AppIsValid())
            {
                Bootloader_JumpToApp();
            }
            else
            {
                /* 升级失败，重新等待 */
                g_upgrade_complete = 0;
            }
        }

        HAL_Delay(200);
    }
}

void Error_Handler(void)
{
    __disable_irq();
    while (1)
    {
    }
}