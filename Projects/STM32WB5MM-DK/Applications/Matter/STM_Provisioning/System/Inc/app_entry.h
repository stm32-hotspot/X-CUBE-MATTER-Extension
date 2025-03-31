
/**
  ******************************************************************************
  * @file    app_entry.h
  * @author  MCD Application Team
  * @brief   Interface to the application
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2019-2021 STMicroelectronics.
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

#include "stm32wbxx_hal.h"

#ifdef  USE_STM32WBXX_NUCLEO
#include "stm32wbxx_nucleo.h"
#endif
#ifdef  USE_X_NUCLEO_EPD
#include "x_nucleo_epd.h"
#endif
#ifdef USE_STM32WB5M_DK
#include "stm32wb5mm_dk.h"
#endif

#include "tl.h"

#ifdef __cplusplus
extern "C" {
#endif

  /* Includes ------------------------------------------------------------------*/
  /* Exported types ------------------------------------------------------------*/
  /* Exported constants --------------------------------------------------------*/
  /* External variables --------------------------------------------------------*/
	typedef struct
	{
	  uint8_t Pushed_Button;
	  uint8_t State; //1 pushed
	}Push_Button_st;

  typedef void (*PushButtonCallback)(Push_Button_st *aMessage);

  /* Exported macros -----------------------------------------------------------*/
  /* Exported functions ------------------------------------------------------- */
  void APPE_Config(void);
  void APPE_Init(void);
  void APP_ENTRY_PBSetReceiveCallback(PushButtonCallback aCallback);

#ifdef  USE_STM32WBXX_NUCLEO
  void APP_ENTRY_LedBlink(Led_TypeDef Led, uint8_t LedStatus);
#endif /* USE_STM32WBXX_NUCLEO */

#ifdef __cplusplus
}
#endif

#endif /* __APP_ENTRY_H */
