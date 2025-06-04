/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    stm32_flash_int.h
  * @author  MCD Application Team
  * @brief   Header file for stm32_flash_int.c
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2023 STMicroelectronics.
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
#ifndef FLASH_INT_H
#define FLASH_INT_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include <stdint.h>

/* Defines -------------------------------------------------------------------*/
/* Base address of the Flash sectors */
#define FLASH_ADDR_SECTOR_0_BANK1     ((uint32_t)0x08000000) /* Base @ of Sector 0, Bank1, 128 Kbyte */
#define FLASH_ADDR_SECTOR_1_BANK1     ((uint32_t)0x08020000) /* Base @ of Sector 1, Bank1, 128 Kbyte */
#define FLASH_ADDR_SECTOR_2_BANK1     ((uint32_t)0x08040000) /* Base @ of Sector 2, Bank1, 128 Kbyte */
#define FLASH_ADDR_SECTOR_3_BANK1     ((uint32_t)0x08060000) /* Base @ of Sector 3, Bank1, 128 Kbyte */
#define FLASH_ADDR_SECTOR_4_BANK1     ((uint32_t)0x08080000) /* Base @ of Sector 4, Bank1, 128 Kbyte */
#define FLASH_ADDR_SECTOR_5_BANK1     ((uint32_t)0x080A0000) /* Base @ of Sector 5, Bank1, 128 Kbyte */
#define FLASH_ADDR_SECTOR_6_BANK1     ((uint32_t)0x080C0000) /* Base @ of Sector 6, Bank1, 128 Kbyte */
#define FLASH_ADDR_SECTOR_7_BANK1     ((uint32_t)0x080E0000) /* Base @ of Sector 7, Bank1, 128 Kbyte */

#define FLASH_ADDR_SECTOR_0_BANK2     ((uint32_t)0x08100000) /* Base @ of Sector 0, Bank2, 128 Kbyte */
#define FLASH_ADDR_SECTOR_1_BANK2     ((uint32_t)0x08120000) /* Base @ of Sector 1, Bank2, 128 Kbyte */
#define FLASH_ADDR_SECTOR_2_BANK2     ((uint32_t)0x08140000) /* Base @ of Sector 2, Bank2, 128 Kbyte */
#define FLASH_ADDR_SECTOR_3_BANK2     ((uint32_t)0x08160000) /* Base @ of Sector 3, Bank2, 128 Kbyte */
#define FLASH_ADDR_SECTOR_4_BANK2     ((uint32_t)0x08180000) /* Base @ of Sector 4, Bank2, 128 Kbyte */
#define FLASH_ADDR_SECTOR_5_BANK2     ((uint32_t)0x081A0000) /* Base @ of Sector 5, Bank2, 128 Kbyte */
#define FLASH_ADDR_SECTOR_6_BANK2     ((uint32_t)0x081C0000) /* Base @ of Sector 6, Bank2, 128 Kbyte */
#define FLASH_ADDR_SECTOR_7_BANK2     ((uint32_t)0x081E0000) /* Base @ of Sector 7, Bank2, 128 Kbyte */

#define FLASH_ADDR_END                ((uint32_t)0x081FFFFF)

typedef enum
{
  FLASH_OK = 0,
  FLASH_INIT_FAILED,
  FLASH_WRITE_FAILED,
  FLASH_READ_FAILED,
  FLASH_DELETE_FAILED,
  FLASH_INVALID_PARAM,
  FLASH_SIZE_FULL
} FLASH_StatusTypeDef;

/* Exported variables ------------------------------------------------------- */
/* Exported functions ------------------------------------------------------- */

/**
  * @brief  init
  */
FLASH_StatusTypeDef FLASH_INT_Init(void);

/**
  * @brief  Delete in internal flash at Address
  */
FLASH_StatusTypeDef FLASH_INT_Delete(uint32_t Address, uint32_t Length);

/**
  * @brief  Write in internal flash at Address a ptBuffer
  */
FLASH_StatusTypeDef FLASH_INT_Write(uint32_t Address, uint32_t *ptBuffer, uint32_t BufferLength);

#ifdef __cplusplus
}
#endif

#endif /* FLASH_INT_H */
