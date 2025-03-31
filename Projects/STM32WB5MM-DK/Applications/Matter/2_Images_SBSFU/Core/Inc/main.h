/**
  ******************************************************************************
  * @file    main.h
  * @author  MCD Application Team
  * @brief   Header for main.c module
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2017 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file in
  * the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef MAIN_H
#define MAIN_H

/* Includes ------------------------------------------------------------------*/
#include "stm32wbxx_hal.h"
#include "stm32wb5mm_dk.h"
#include "app_sfu.h"
#include "app_hw.h"
typedef struct {
/**
 * Wireless Info
 */
  uint8_t VersionMajor;
  uint8_t VersionMinor;
  uint8_t VersionSub;
  uint8_t VersionBranch;
  uint8_t VersionReleaseType;
  uint8_t MemorySizeSram2B;     /*< Multiple of 1K */
  uint8_t MemorySizeSram2A;     /*< Multiple of 1K */
  uint8_t MemorySizeSram1;      /*< Multiple of 1K */
  uint8_t MemorySizeFlash;      /*< Multiple of 4K */
  uint8_t StackType;
/**
 * Fus Info
 */
  uint8_t FusVersionMajor;
  uint8_t FusVersionMinor;
  uint8_t FusVersionSub;
  uint8_t FusMemorySizeSram2B;  /*< Multiple of 1K */
  uint8_t FusMemorySizeSram2A;  /*< Multiple of 1K */
  uint8_t FusMemorySizeFlash;   /*< Multiple of 4K */
}WirelessFwInfo_t;
#endif /* MAIN_H */
