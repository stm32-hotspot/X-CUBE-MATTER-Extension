/**
 ******************************************************************************
 * @file    dac_crypto_hal.h
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
 
/**
 * @file
 *   This file includes function prototype to sign DAC & store DAC private key
 *
 */

#ifndef DAC_CRYPTO_HAL_H_
#define DAC_CRYPTO_HAL_H_

//Function prototype to be added
#ifdef __cplusplus
extern "C" {
#endif

    otError otCksDacInitialize(const uint8_t *DAC_private_key);
	otError otCksDacSignature(const uint8_t *message_to_sign, const size_t msg_length, const uint8_t *DAC_public_key, uint8_t *out_signature);

#ifdef __cplusplus
}
#endif

#endif // DAC_CRYPTO_HAL_H_
