/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    stm32_nvm_implement.h
  * @author  MCD Application Team
  * @brief   Header file for stm32_nvm_implement.c
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
#ifndef NVM_IMPLEMENT_H
#define NVM_IMPLEMENT_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

/* Defines -------------------------------------------------------------------*/
typedef enum
{
  NVM_OK = 0,
  NVM_PARAMETER_ERROR,
  NVM_KEY_NOT_FOUND,
  NVM_WRITE_FAILED,
  NVM_READ_FAILED,
  NVM_DELETE_FAILED,
  NVM_LACK_SPACE,
  NVM_BLOCK_SIZE_OVERFLOW,
  NVM_ERROR_BLOCK_ALIGN,
  NVM_BUFFER_TOO_SMALL,
  NVM_CORRUPTION
} NVM_StatusTypeDef;

typedef enum
{
  NVM_SECTOR_1 = 0,
  NVM_SECTOR_2
} NVM_SectorTypeDef;

#define NVM_SECTOR_CONFIG        (NVM_SECTOR_2)
#define NVM_SECTOR_KEYSTORE      (NVM_SECTOR_2)
#define NVM_SECTOR_APPLICATION   (NVM_SECTOR_1)

/* Exported functions ------------------------------------------------------- */

/**
  * @brief  RAM NVM initialization (copy FLASH to RAM NVM)
  *
  * @param  KeyNameMaxSize:     Size max for key name
  * @param  KeyValueLengthSize: Size for key value length
  * @param  Sector:             Sector to initialize
  */
NVM_StatusTypeDef NVM_Init(uint8_t KeyNameMaxSize, uint8_t KeyValueLengthSize, NVM_SectorTypeDef Sector);

/**
  * @brief  Save RAM NVM to Flash
  */
NVM_StatusTypeDef NVM_Save(void);

/**
  * @brief  Reset RAM NVM for the specific sector
  *
  * @param  Sector: Sector to reset
  */
NVM_StatusTypeDef NVM_Reset(NVM_SectorTypeDef Sector);

/**
  * @brief  Erase all persistent data and reboot program
  */
void NVM_FactoryReset(void);

/**
  * @brief  Search a key using its name in RAM NVM and return if the key exists
  *
  * @param  ptKeyName:    Name of searched Key
  * @param  Sector:       Sector where to search the Key
  * @retval return state of function
  */
NVM_StatusTypeDef NVM_GetKeyExists(const char *ptKeyName, NVM_SectorTypeDef Sector);

/**
  * @brief   Search a key using its name in RAM NVM and return its size if the key exists
  *
  * @param  ptKeyName:    Name of searched Key
  * @param  ptKeySize:    Return the size of searched Key
  * @param  Sector:       Sector where to search the Key
  * @retval return state of function
  */
NVM_StatusTypeDef NVM_GetKeySize(const char *ptKeyName, size_t *ptKeySize, NVM_SectorTypeDef Sector);

/**
  * @brief  Get the Key value using key name for the research
  *
  * @param  ptKeyName:    Name of searched Key
  * @param  ptKeyValue:   Address of the buffer changed (value of key) if the key is found
  * @param  KeyValueSize: Size allowed for ptKeyValue
  * @param  ptReadKeyValueSize: Return size of ptKeyValue found
  * @param  Sector:       Sector where to search the Key
  * @retval return state of function
  */
NVM_StatusTypeDef NVM_GetKeyValue(const char *ptKeyName, void *ptKeyValue, uint32_t KeyValueSize,
                                  size_t *ptReadKeyValueSize, NVM_SectorTypeDef Sector);

/**
  * @brief  Create in RAM NVM a Key (name and value) or modify its value if key already exists
  *
  * @param  ptKeyName:    Name of the Key
  * @param  ptKeyValue:   Address of the buffer (value of key)
  * @param  KeyValueSize: Size pointed by ptKeyValue
  * @param  Sector:       Sector where to create modify the Key
  * @retval return state of function
  */
NVM_StatusTypeDef NVM_SetKeyValue(const char *ptKeyName, const void *ptKeyValue, uint32_t KeyValueSize,
                                  NVM_SectorTypeDef Sector);

/**
  * @brief  Delete in RAM NVM a Key
  *
  * @param  ptKeyName:    Name of the Key
  * @param  Sector:       Sector where to search the Key
  * @retval return state of function
  */

NVM_StatusTypeDef NVM_DeleteKey(const char *ptKeyName, NVM_SectorTypeDef Sector);

/**
  * @brief  RAM NVM usage status
  *
  * @param  ptMemoryUsed: Return memory used
  * @param  ptMemoryFree: Return memory free
  * @param  Sector:       Sector to analyze
  * @retval return state of function
  */
NVM_StatusTypeDef NVM_MemoryStatus(uint32_t *ptMemoryUsed, uint32_t *ptMemoryFree, NVM_SectorTypeDef Sector);

#ifdef __cplusplus
}
#endif

#endif /* NVM_IMPLEMENT_H */

