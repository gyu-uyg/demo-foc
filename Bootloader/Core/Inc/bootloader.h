#ifndef FOC_BOOTLOADER_H
#define FOC_BOOTLOADER_H

#include <stdint.h>
#include <stdbool.h>
#include "stm32f1xx_hal.h"

#define BOOTLOADER_FLASH_BASE     0x08000000UL
#define BOOTLOADER_FLASH_SIZE     (16UL * 1024UL)
#define APP_FLASH_BASE            0x08004000UL
#define APP_FLASH_END             0x08010000UL
#define APP_FLASH_PAGE_SIZE       1024UL
#define APP_FLASH_TIMEOUT_MS      1500UL

bool Bootloader_AppIsValid(void);
void Bootloader_JumpToApp(void);
HAL_StatusTypeDef Bootloader_FlashErasePage(uint32_t address);
HAL_StatusTypeDef Bootloader_FlashWrite(uint32_t address, const uint8_t *data, uint16_t length);
HAL_StatusTypeDef Bootloader_FlashRead(uint32_t address, uint8_t *data, uint16_t length);

#endif
