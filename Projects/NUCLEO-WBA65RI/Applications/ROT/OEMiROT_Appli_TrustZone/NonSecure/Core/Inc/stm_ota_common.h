/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file    stm_ota_common.h
 * @author  MCD Application Team
 * @brief   Header file for common OTA api
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
#ifndef STM_OTA_COMMON_H
#define STM_OTA_COMMON_H

#ifdef __cplusplus
extern "C" {
#endif


typedef enum {
    STM_OTA_FLASH_OK,
    STM_OTA_FLASH_INIT_FAILED,
    STM_OTA_FLASH_WRITE_FAILED,
    STM_OTA_FLASH_READ_FAILED,
    STM_OTA_FLASH_DELETE_FAILED,
    STM_OTA_FLASH_INVALID_PARAM,
    STM_OTA_FLASH_SIZE_FULL
} STM_OTA_StatusTypeDef;

#ifdef __cplusplus
}
#endif

#endif /*STM_OTA_COMMON_H */
