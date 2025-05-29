/**
  ******************************************************************************
  * @file           : AppEntry.h
  * @brief          : Header for AppEntry.c materials file.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2024 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __APP_ENTRY_H
#define __APP_ENTRY_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* Includes ------------------------------------------------------------------*/
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

/* Exported types ------------------------------------------------------------*/
typedef struct
{
  uint8_t Button; /* BUTTON_USER */
  bool State; /* 0: released - 1: pressed */
} ButtonState_t;

typedef void (*ButtonCallback)(ButtonState_t *ptMessage);
typedef bool (*UartRxCallback)(uint8_t *ptUartString, uint8_t UartStringLen);
typedef void (*UartHelpCallback)(void);

/* Exported constants --------------------------------------------------------*/
/* Exported macros -----------------------------------------------------------*/
/* External variables --------------------------------------------------------*/

/* Exported functions prototypes -------------------------------------------- */
bool AppEntryInit(void);
bool AppEntryStart(void);

void ButtonSetCallback(ButtonCallback aCallback);
void ConsoleSetCallback(UartRxCallback aUartRxCb, UartHelpCallback aUartHelpCb);

bool AppTreatInput(uint8_t *ptUartString, uint8_t UartStringLen);
void AppHelpInput(void);

uint8_t stm32_GetMacAddr(uint8_t *buf_data, size_t buf_size);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __APP_ENTRY_H */
