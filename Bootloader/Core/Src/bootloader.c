#include "bootloader.h"
#include "main.h"
#include <string.h>

/* 定义 App 起始地址，需与链接脚本一致 */
#ifndef APP_FLASH_BASE
#define APP_FLASH_BASE    0x08004000UL
#endif

#ifndef APP_FLASH_END
#define APP_FLASH_END     0x08020000UL  /* 假设 128KB Flash */
#endif

typedef void (*AppEntryPoint)(void);

/**
  * @brief 检查地址是否在合法范围内
  */
static bool Bootloader_AddressInRange(uint32_t address, uint32_t start, uint32_t end)
{
    return (address >= start) && (address < end);
}

/**
  * @brief 检查 App 是否有效 (检查栈顶和复位向量)
  */
bool Bootloader_AppIsValid(void)
{
    const uint32_t app_stack = *(volatile uint32_t *)APP_FLASH_BASE;
    const uint32_t app_reset = *(volatile uint32_t *)(APP_FLASH_BASE + 4UL);

    /* 检查栈顶是否在 RAM 范围内 (假设 20K RAM) */
    if (!Bootloader_AddressInRange(app_stack, SRAM_BASE, SRAM_BASE + 0x5000UL + 1UL))
    {
        return false;
    }

    /* 检查复位向量是否在 Flash 范围内 */
    if (!Bootloader_AddressInRange(app_reset, APP_FLASH_BASE, APP_FLASH_END))
    {
        return false;
    }

    return true;
}

/**
  * @brief 跳转到主程序
  */
void Bootloader_JumpToApp(void)
{
    const uint32_t app_stack = *(volatile uint32_t *)APP_FLASH_BASE;
    const uint32_t app_reset = *(volatile uint32_t *)(APP_FLASH_BASE + 4UL);
    AppEntryPoint app_entry = (AppEntryPoint)app_reset;

    /* 1. 彻底关闭中断，防止跳转瞬间触发中断导致 HardFault */
    __disable_irq();

    /* 2. 停止 SysTick 定时器 */
    SysTick->CTRL = 0U;
    SysTick->LOAD = 0U;
    SysTick->VAL = 0U;

    /* 3. 清除所有 NVIC 中断挂起标志，关闭所有 NVIC 中断使能 */
    for (uint32_t i = 0; i < 8U; ++i)
    {
        NVIC->ICER[i] = 0xFFFFFFFFUL;
        NVIC->ICPR[i] = 0xFFFFFFFFUL;
    }

    /* 4. 反初始化所有 HAL 外设和时钟切换回 HSI */
    HAL_DeInit();

    /* 5. 重定向向量表到 App 起始位置 */
    SCB->VTOR = APP_FLASH_BASE;

    /* 6. 设置主堆栈指针 (MSP) 并跳转 */
    __set_MSP(app_stack);
    app_entry();

    /* 永远不应该执行到这里 */
    while (1);
}

/**
  * @brief 擦除指定页
  */
HAL_StatusTypeDef Bootloader_FlashErasePage(uint32_t address)
{
    FLASH_EraseInitTypeDef erase = {0};
    uint32_t page_error = 0U;
    HAL_StatusTypeDef status;

    if (!Bootloader_AddressInRange(address, APP_FLASH_BASE, APP_FLASH_END))
    {
        return HAL_ERROR;
    }

    HAL_FLASH_Unlock(); /* 🚨 必须解锁 */

    erase.TypeErase = FLASH_TYPEERASE_PAGES;
    erase.PageAddress = address;
    erase.NbPages = 1U;

    status = HAL_FLASHEx_Erase(&erase, &page_error);

    HAL_FLASH_Lock(); /* 🚨 操作完立即锁定 */
    return status;
}

/**
  * @brief 写入数据到 Flash
  */
HAL_StatusTypeDef Bootloader_FlashWrite(uint32_t address, const uint8_t *data, uint16_t length)
{
    uint32_t write_address = address;
    uint16_t index = 0U;
    HAL_StatusTypeDef status = HAL_OK;

    if ((length == 0U) || (data == NULL)) return HAL_OK;

    /* 地址合法性校验 */
    if (!Bootloader_AddressInRange(address, APP_FLASH_BASE, APP_FLASH_END) ||
        !Bootloader_AddressInRange(address + length - 1, APP_FLASH_BASE, APP_FLASH_END))
    {
        return HAL_ERROR;
    }

    HAL_FLASH_Unlock(); /* 🚨 开启写权限 */

    while (index < length)
    {
        /* 组装 16 位半字 (HalfWord) */
        uint16_t halfword = data[index];
        if ((index + 1U) < length)
        {
            halfword |= (uint16_t)((uint16_t)data[index + 1U] << 8);
        }
        else
        {
            halfword |= 0xFF00U; /* 奇数长度补齐 */
        }

        /* 写入 Flash */
        status = HAL_FLASH_Program(FLASH_TYPEPROGRAM_HALFWORD, write_address, halfword);
        if (status != HAL_OK) break;

        /* 读出校验 */
        if (*(volatile uint16_t *)write_address != halfword)
        {
            status = HAL_ERROR;
            break;
        }

        write_address += 2UL;
        index += 2U;
    }

    HAL_FLASH_Lock(); /* 🚨 锁定 */
    return status;
}

/**
  * @brief 从 Flash 读取数据
  */
HAL_StatusTypeDef Bootloader_FlashRead(uint32_t address, uint8_t *data, uint16_t length)
{
    if ((data == NULL) || (length == 0U)) return HAL_OK;

    if (!Bootloader_AddressInRange(address, APP_FLASH_BASE, APP_FLASH_END))
    {
        return HAL_ERROR;
    }

    /* Flash 读取可以直接通过指针访问 */
    memcpy(data, (const void *)address, length);

    return HAL_OK;
}