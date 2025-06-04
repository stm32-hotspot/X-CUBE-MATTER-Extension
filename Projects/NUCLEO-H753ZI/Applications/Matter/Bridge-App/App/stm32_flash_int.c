/**
  ******************************************************************************
  * @file    stm32_flash_int.c
  * @author  MCD Application Team
  * @brief   Manage internal flash
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

/* Includes ------------------------------------------------------------------*/
#include "stm32_flash_int.h"
#include "main.h"

/* Private defines -----------------------------------------------------------*/

/* Private macros ------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/

/* Global variables ----------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/
static uint32_t GetSector(uint32_t Address);

/* Private function prototypes -----------------------------------------------*/
/**
  * @brief  Gets the sector of a given address
  * @param  Address : Address of the FLASH Memory
  * @retval The sector of a given address
  */
static uint32_t GetSector(uint32_t Address)
{
  uint32_t sector = 0U;

  if(((Address < FLASH_ADDR_SECTOR_1_BANK1) && (Address >= FLASH_ADDR_SECTOR_0_BANK1)) || \
     ((Address < FLASH_ADDR_SECTOR_1_BANK2) && (Address >= FLASH_ADDR_SECTOR_0_BANK2)))
  {
    sector = FLASH_SECTOR_0;
  }
  else if(((Address < FLASH_ADDR_SECTOR_2_BANK1) && (Address >= FLASH_ADDR_SECTOR_1_BANK1)) || \
          ((Address < FLASH_ADDR_SECTOR_2_BANK2) && (Address >= FLASH_ADDR_SECTOR_1_BANK2)))
  {
    sector = FLASH_SECTOR_1;
  }
  else if(((Address < FLASH_ADDR_SECTOR_3_BANK1) && (Address >= FLASH_ADDR_SECTOR_2_BANK1)) || \
          ((Address < FLASH_ADDR_SECTOR_3_BANK2) && (Address >= FLASH_ADDR_SECTOR_2_BANK2)))
  {
    sector = FLASH_SECTOR_2;
  }
  else if(((Address < FLASH_ADDR_SECTOR_4_BANK1) && (Address >= FLASH_ADDR_SECTOR_3_BANK1)) || \
          ((Address < FLASH_ADDR_SECTOR_4_BANK2) && (Address >= FLASH_ADDR_SECTOR_3_BANK2)))
  {
    sector = FLASH_SECTOR_3;
  }
  else if(((Address < FLASH_ADDR_SECTOR_5_BANK1) && (Address >= FLASH_ADDR_SECTOR_4_BANK1)) || \
          ((Address < FLASH_ADDR_SECTOR_5_BANK2) && (Address >= FLASH_ADDR_SECTOR_4_BANK2)))
  {
    sector = FLASH_SECTOR_4;
  }
  else if(((Address < FLASH_ADDR_SECTOR_6_BANK1) && (Address >= FLASH_ADDR_SECTOR_5_BANK1)) || \
          ((Address < FLASH_ADDR_SECTOR_6_BANK2) && (Address >= FLASH_ADDR_SECTOR_5_BANK2)))
  {
    sector = FLASH_SECTOR_5;
  }
  else if(((Address < FLASH_ADDR_SECTOR_7_BANK1) && (Address >= FLASH_ADDR_SECTOR_6_BANK1)) || \
          ((Address < FLASH_ADDR_SECTOR_7_BANK2) && (Address >= FLASH_ADDR_SECTOR_6_BANK2)))
  {
    sector = FLASH_SECTOR_6;
  }
  else if(((Address < FLASH_ADDR_SECTOR_0_BANK2) && (Address >= FLASH_ADDR_SECTOR_7_BANK1)) || \
          ((Address < FLASH_ADDR_END) && (Address >= FLASH_ADDR_SECTOR_7_BANK2)))
  {
     sector = FLASH_SECTOR_7;
  }
  else
  {
    sector = FLASH_SECTOR_7;
  }

  return sector;
}

/* Public functions ----------------------------------------------------------*/

FLASH_StatusTypeDef FLASH_INT_Init(void) {
    /* Clear pending flags (if any) */
    __HAL_FLASH_CLEAR_FLAG(
            FLASH_FLAG_EOP | FLASH_FLAG_OPERR | FLASH_FLAG_WRPERR
                    | FLASH_FLAG_PGSERR | FLASH_FLAG_WRPERR);

    return FLASH_OK;
}

FLASH_StatusTypeDef FLASH_INT_Delete(uint32_t Address, uint32_t Length) {
    uint32_t SectorError;
    FLASH_EraseInitTypeDef pEraseInit;
    FLASH_StatusTypeDef ret = FLASH_OK;
    /* Unlock the Flash to enable the flash control register access *************/
    HAL_FLASH_Unlock();
    (void)FLASH_INT_Init();

    pEraseInit.TypeErase = FLASH_TYPEERASE_SECTORS;
    pEraseInit.VoltageRange = FLASH_VOLTAGE_RANGE_3;
    pEraseInit.Banks = FLASH_BANK_2;
    pEraseInit.Sector = GetSector(Address);
    pEraseInit.NbSectors = (GetSector(Address + Length) - pEraseInit.Sector + 1);
    SCB_DisableICache();
    if (HAL_FLASHEx_Erase(&pEraseInit, &SectorError) != HAL_OK) {
        /* Error occurred while sector erase */
        ret = FLASH_DELETE_FAILED;
    }
    SCB_EnableICache();
    HAL_FLASH_Lock();

    return ret;
}

FLASH_StatusTypeDef FLASH_INT_Write(uint32_t Address, uint32_t *ptBuffer, uint32_t BufferLength) {
    uint32_t i = 0;

    /* Unlock the Flash to enable the flash control register access *************/
    HAL_FLASH_Unlock();
    SCB_DisableICache();
    for (i = 0;
         (i < BufferLength) && (Address <= (FLASH_ADDR_SECTOR_1_BANK2 - 32));
         i += 8) {
        /* Device voltage range supposed to be [2.7V to 3.6V], the operation will
         be done by word */
        if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_FLASHWORD, Address, ((uint32_t)(ptBuffer + i))) == HAL_OK) {
            /* Check the written value */
            if (*(uint32_t *) Address != *(uint32_t *) (ptBuffer + i)) {
                /* Flash content doesn't match SRAM content */
                SCB_EnableICache();
                HAL_FLASH_Lock();
                return FLASH_READ_FAILED;
            }
            /* Increment FLASH destination address */
            Address += 32;
        } else {
            /* Error occurred while writing data in Flash memory */
            SCB_EnableICache();
            HAL_FLASH_Lock();
            return FLASH_WRITE_FAILED;
        }
    }
    SCB_EnableICache();
    HAL_FLASH_Lock();

    return FLASH_OK;
}
