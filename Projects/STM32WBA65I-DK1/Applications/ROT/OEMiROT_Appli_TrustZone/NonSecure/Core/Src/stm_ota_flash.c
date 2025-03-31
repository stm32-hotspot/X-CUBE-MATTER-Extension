/**
 ******************************************************************************
 * @file    stm_ota_flash.c
 * @author  MCD Application Team
 * @brief   Write new image in internal flash
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
#include "stm_ota_flash.h"
#include "app_common.h"
#include "cmsis_os.h"

/* Private defines -----------------------------------------------------------*/

/* Private macros ------------------------------------------------------------*/
#include "flash_manager.h"
#include "flash_wb.h"

///* Private variables ---------------------------------------------------------*/
/* FlashOperationCompletedSema goal is to return to upper layer only when flash
 * operation requested is complete.
 */
osSemaphoreId_t FlashOperationCompletedSema;

///* Global variables ----------------------------------------------------------*/
///* Private function prototypes -----------------------------------------------*/
static void FM_EraseCallback(FM_FlashOp_Status_t Status);
static void FM_WriteCallback(FM_FlashOp_Status_t Status);

static FM_CallbackNode_t FM_EraseStatusCallback = {
/* Header for chained list */
.NodeList = { .next = NULL, .prev = NULL },
/* Callback for request status */
.Callback = FM_EraseCallback };

static FM_CallbackNode_t FM_WriteStatusCallback = {
/* Header for chained list */
.NodeList = { .next = NULL, .prev = NULL },
/* Callback for request status */
.Callback = FM_WriteCallback };

static void FM_EraseCallback(FM_FlashOp_Status_t Status) {
	/* Update status */

  /* flash operation is complete */
  osSemaphoreRelease(FlashOperationCompletedSema);
  /* free FM semaphore for other FM users */
	UnLockFMThread();
}

static void FM_WriteCallback(FM_FlashOp_Status_t Status) {
	/* Update status */

  /* flash operation is complete */
  osSemaphoreRelease(FlashOperationCompletedSema);
  /* free FM semaphore for other FM users */
	UnLockFMThread();
}

/* Public functions ----------------------------------------------------------*/

STM_OTA_StatusTypeDef STM_OTA_FLASH_Init(void)
{
	//APP_DBG("STM_OTA_FLASH_Init.\n");

  // Semaphore initialization (initial count = 0)
  FlashOperationCompletedSema = osSemaphoreNew( 1, 0, NULL );
	if ( FlashOperationCompletedSema == NULL )
	{ 
		APP_DBG( "ERROR FREERTOS : FLASH OP COMPLETED SEMAPHORE CREATION FAILED" );
	}

	return STM_OTA_FLASH_OK;
}

STM_OTA_StatusTypeDef STM_OTA_FLASH_Delete_Image(uint32_t Address, uint32_t Length)
{
	//APP_DBG("STM_OTA_FLASH_Delete_Image @=0x%x, Length=%lu (=0x%x).\n", Address, Length, Length);	

  FM_Cmd_Status_t error = FM_ERROR;

	LockFMThread();

  do
  {
    /* Flash manager erase */
    error = FM_Erase( SLOT_DWL_A_START_SECTOR, 
                      SLOT_DWL_A_NB_SECTORS, 
                      &FM_EraseStatusCallback );
    if (error == FM_OK)
    {    
      osSemaphoreAcquire(FlashOperationCompletedSema, osWaitForever);
    }                      
  }  while (error == FM_BUSY);

  return ((error == FM_OK) ? STM_OTA_FLASH_OK : STM_OTA_FLASH_DELETE_FAILED);
}

STM_OTA_StatusTypeDef STM_OTA_FLASH_WriteChunk(uint32_t *pDestAddress, uint32_t *pSrcBuffer, uint32_t Length)
{	
	//APP_DBG("STM_OTA_FLASH_WriteChunk to @=0x%p, from=0x%p, Length=%lu (=0x%x).\n", pDestAddress, pSrcBuffer, Length, Length);

  FM_Cmd_Status_t error = FM_ERROR;

  /* check buffers pointers */
  if (pSrcBuffer == NULL) 
  {
      return STM_OTA_FLASH_INVALID_PARAM;
  }
  if (pDestAddress == NULL) 
  {
      return STM_OTA_FLASH_INVALID_PARAM;
  }  

  /* Do nothing if Length equal to 0 */
  if (Length == 0U) 
  {
      return STM_OTA_FLASH_OK;
  }

	LockFMThread();

  do
  {
    /* warning, the Length is a multiple of 32bits (size = 1 means 32bits), we need to divide it by 4 */ 
    error = FM_Write_ext (  pSrcBuffer, 
                            pDestAddress,
                            ((int32_t) (Length / 4)), 
                            &FM_WriteStatusCallback);
    if (error == FM_OK)
    {    
      osSemaphoreAcquire(FlashOperationCompletedSema, osWaitForever);
    }    
  }  while (error == FM_BUSY);

  return ((error == FM_OK) ? STM_OTA_FLASH_OK : STM_OTA_FLASH_WRITE_FAILED);
}

