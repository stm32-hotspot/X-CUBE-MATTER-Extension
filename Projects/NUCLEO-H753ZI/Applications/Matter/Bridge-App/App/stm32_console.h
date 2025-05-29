/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    stm32_console.h
  * @author  MCD Application Team
  * @brief   Header for stm32_console.c materials file.
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
/* USER CODE END Header */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef CONSOLE_H
#define CONSOLE_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* Includes ------------------------------------------------------------------*/
#include <stdbool.h>

/* Exported types ------------------------------------------------------------*/
/* Exported constants --------------------------------------------------------*/
/* Exported macros -----------------------------------------------------------*/
/* External variables --------------------------------------------------------*/

/* Exported functions prototypes -------------------------------------------- */
bool CONSOLE_Init(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* CONSOLE_H */
