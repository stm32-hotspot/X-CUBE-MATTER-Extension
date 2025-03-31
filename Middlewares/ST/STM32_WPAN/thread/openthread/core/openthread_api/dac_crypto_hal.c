/**
 ******************************************************************************
 * @file    dac_crypto_hal.c
 * @author  MCD Application Team
 * @brief   This file contains the cks crypto interface shared between M0 and
 *          M4.
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2018-2021 STMicroelectronics.
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 *
 ******************************************************************************
 */

#include "app_conf.h"
#if (USE_STM32WBXX_DAC_CRYPTO == 1)

/* Includes ------------------------------------------------------------------*/
#include "stm32wbxx_hal.h"

#include "stm32wbxx_core_interface_def.h"
#include "tl_thread_hci.h"

#include "dac_crypto_hal.h"


otError otCksDacInitialize(const uint8_t *DAC_private_key)
{
  otError rspData;
  
  Pre_OtCmdProcessing();
  /* prepare buffer */
  Thread_OT_Cmd_Request_t* p_ot_req = THREAD_Get_OTCmdPayloadBuffer();

  p_ot_req->ID = MSG_M4TOM0_OT_CKS_DAC_PRIVATE_KEY_SET;

  p_ot_req->Size=1;
  p_ot_req->Data[0] = (uint32_t) DAC_private_key;

  Ot_Cmd_Transfer();

  p_ot_req = THREAD_Get_OTCmdRspPayloadBuffer();
  
  rspData = (otError)p_ot_req->Data[0];
  
  Post_OtCmdProcessing();
  
  return rspData;
}


otError otCksDacSignature(const uint8_t *message_to_sign, const size_t msg_length, const uint8_t *DAC_public_key, uint8_t *out_signature)
{
  otError rspData;
  
  Pre_OtCmdProcessing();
  /* prepare buffer */
  Thread_OT_Cmd_Request_t* p_ot_req = THREAD_Get_OTCmdPayloadBuffer();

  p_ot_req->ID = MSG_M4TOM0_OT_CKS_DAC_SIGNATURE;

  //Use Data[3] to carry out signature - Need to match with P256ECDSASignature struct
  p_ot_req->Size=4;
  p_ot_req->Data[0] = (uint32_t) message_to_sign;
  p_ot_req->Data[1] = (uint32_t) msg_length;
  p_ot_req->Data[2] = (uint32_t) DAC_public_key;
  p_ot_req->Data[3] = (uint32_t) out_signature;

  Ot_Cmd_Transfer();

  p_ot_req = THREAD_Get_OTCmdRspPayloadBuffer();

  rspData = (otError)p_ot_req->Data[0];
  
  Post_OtCmdProcessing();
  
  return rspData;  
}

#endif
