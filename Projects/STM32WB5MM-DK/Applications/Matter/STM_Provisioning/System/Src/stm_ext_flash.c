/**
 ******************************************************************************
 * @file    stm_ota.c
 * @author  MCD Application Team
 * @brief   Write new image in external flash
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
#include "stm_ext_flash.h"
#include "stm32wb5mm_dk_qspi.h"
#include "cmsis_os.h"

/* Private defines -----------------------------------------------------------*/
#define ERASE_BLOC_SIZE 0x10000U /*!< 64 Kbytes */

/* Private macros ------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/
osSemaphoreId_t SemExtFlashId;

/* Global variables ----------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/
static STM_OTA_StatusTypeDef check_addr(uint32_t Address, uint32_t Length);
/* Public functions ----------------------------------------------------------*/

STM_OTA_StatusTypeDef STM_EXT_FLASH_Init(void) {
    BSP_QSPI_Init_t init;

    SemExtFlashId = osSemaphoreNew(1, 1, NULL); /*< Create the semaphore and make it available at initialization */

    init.TransferRate = BSP_QSPI_STR_TRANSFER;
    init.DualFlashMode = BSP_QSPI_DUALFLASH_DISABLE;
    init.InterfaceMode = BSP_QSPI_SPI_MODE;
    if (BSP_QSPI_Init(0, &init) != BSP_ERROR_NONE) {
        return STM_EXT_FLASH_INIT_FAILED;
    } else {
        return STM_EXT_FLASH_OK;
    }

}

STM_OTA_StatusTypeDef STM_EXT_FLASH_Delete_Image(uint32_t Address, uint32_t Length) {
    uint32_t loop_flash;

    // check if the address is in the external flash and if the length is < flash size
    if (check_addr(Address, Length) != STM_EXT_FLASH_OK) {
        return STM_EXT_FLASH_INVALID_PARAM;
    }

    /* Do nothing if Length equal to 0 */
    if (Length == 0U) {
        return STM_EXT_FLASH_OK;
    }

    /* flash address to erase is the offset from begin of external flash */
    Address -= EXTERNAL_FLASH_ADDRESS;

    osSemaphoreAcquire(SemExtFlashId, osWaitForever);
    /* Loop on 64KBytes block */
    for (loop_flash = 0U; loop_flash < (((Length - 1U) / ERASE_BLOC_SIZE) + 1U); loop_flash++) {
        if (BSP_QSPI_EraseBlock(0, Address, BSP_QSPI_ERASE_64K) != BSP_ERROR_NONE) {

            osSemaphoreRelease(SemExtFlashId);
            return STM_EXT_FLASH_DELETE_FAILED;
        }

        /* next 64KBytes block */
        Address += ERASE_BLOC_SIZE;
    }

    osSemaphoreRelease(SemExtFlashId);
    return STM_EXT_FLASH_OK;
}

STM_OTA_StatusTypeDef STM_EXT_FLASH_WriteChunk(uint32_t DestAddress, uint8_t *pSrcBuffer,
        uint32_t Length) {
    int32_t error = 0;
    if (pSrcBuffer == NULL) {
        return STM_EXT_FLASH_INVALID_PARAM;
    }
    // check if the address is in the external flash and if the length is < flash size
    if (check_addr(DestAddress, Length) != STM_EXT_FLASH_OK) {
        return STM_EXT_FLASH_INVALID_PARAM;
    }
    /* Do nothing if Length equal to 0 */
    if (Length == 0U) {
        return STM_EXT_FLASH_OK;
    }
    osSemaphoreAcquire(SemExtFlashId, osWaitForever);

    error = BSP_QSPI_Write(0, pSrcBuffer, DestAddress - EXTERNAL_FLASH_ADDRESS, Length);
    if (error != BSP_ERROR_NONE) {

        osSemaphoreRelease(SemExtFlashId);
        return STM_EXT_FLASH_WRITE_FAILED;
    } else {

        osSemaphoreRelease(SemExtFlashId);
        return STM_EXT_FLASH_OK;
    }

}

STM_OTA_StatusTypeDef STM_EXT_FLASH_ReadChunk(uint32_t DestAddress, uint8_t *pSrcBuffer,
        uint32_t Length) {
    int32_t error = 0;
    if (pSrcBuffer == NULL) {
        return STM_EXT_FLASH_INVALID_PARAM;
    }
    // check if the address is in the external flash and if the length is < flash size
    if (check_addr(DestAddress, Length) != STM_EXT_FLASH_OK) {
        return STM_EXT_FLASH_INVALID_PARAM;
    }

    /* Do nothing if Length equal to 0 */
    if (Length == 0U) {
        return STM_EXT_FLASH_OK;
    }
    osSemaphoreAcquire(SemExtFlashId, osWaitForever);
    error = BSP_QSPI_Read(0, pSrcBuffer, DestAddress - EXTERNAL_FLASH_ADDRESS, Length);
    if (error != BSP_ERROR_NONE) {
        osSemaphoreRelease(SemExtFlashId);
        return STM_EXT_FLASH_READ_FAILED;
    } else {
        osSemaphoreRelease(SemExtFlashId);
        return STM_EXT_FLASH_OK;
    }

}

/*************************************************************
 *
 * LOCAL FUNCTIONS
 *
 *************************************************************/
static STM_OTA_StatusTypeDef check_addr(uint32_t Address, uint32_t Length) {
    // check if the address is in the external flash and if the length is < flash size
    if ((Address < EXTERNAL_FLASH_ADDRESS) || ( S25FL128S_FLASH_SIZE < Length)
            || (Address + Length > EXTERNAL_FLASH_ADDRESS + S25FL128S_FLASH_SIZE)) {
        return STM_EXT_FLASH_INVALID_PARAM;
    } else {
        return STM_EXT_FLASH_OK;
    }
}

