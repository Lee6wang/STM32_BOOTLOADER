#ifndef __BOOTLOADER_H
#define __BOOTLOADER_H

#include "stm32f4xx_hal.h"
#include "FlashCV.h"

// Bootloader 主入口，供 main.c 调用
void Bootloader_Run(void);

#endif /* __BOOTLOADER_H */
