/**
  ******************************************************************************
  * @file    storage_interface.h
  * @author  MCD Application Team
  * @brief   Header for storage_interface.c module
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
#ifndef STORAGE_INTERFACE_H
#define STORAGE_INTERFACE_H

/* Includes ------------------------------------------------------------------*/
#include "stm32wbaxx.h"
#include "crypto.h"
#include "psa_its_alt.h"

/* Exported types ------------------------------------------------------------*/
/* Exported constants --------------------------------------------------------*/
/* Device Attestation PSA key ID */
#define DEVICE_ATTESTATION_PRIVATE_KEY_ID_USER   ((psa_key_id_t)0x1fff0001)
#define ITS_ENCRYPTION_SECRET_KEY_ID             ((psa_key_id_t)0x2FFFAAAA)

#define ITS_LOCATION                   0X081EA000 /* ITS start address */
#define ITS_ENCRYPTION_KEY_LOCATION    0X081EB000 /*Encr ITS secret key */
#define FLASH_IF_MIN_WRITE_LEN    (8U)     /* Flash programming by 64 bits */
#define NB_PAGE_SECTOR_PER_ERASE  (1U)  /* Nb page erased per erase */
#define ITS_MAX_SIZE         (8*1024U)  /* 8 KBytes */
#define ITS_SLOT_MAX_NUMBER  (16U)
#define ITS_SLOT_OFFSET      0x00000080 /* 128 words (32 bits)) */

/* Exported functions ------------------------------------------------------- */
psa_status_t storage_set(uint64_t obj_uid,
                         uint32_t obj_length,
                         const void *p_obj);

psa_status_t storage_get(uint64_t obj_uid,
                         uint32_t obj_offset,
                         uint32_t obj_length,
                         void *p_obj);

psa_status_t storage_get_info(uint64_t obj_uid,
                              void *p_obj_info,
                              uint32_t obj_info_size);

psa_status_t storage_remove(uint64_t obj_uid, uint32_t obj_size);

psa_status_t storage_DA_private_Key(const uint8_t* DA_Private_Key,uint32_t DA_Private_Key_Size);

#endif  /* STORAGE_INTERFACE_H */

