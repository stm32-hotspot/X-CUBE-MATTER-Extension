/*
 *
 *    Copyright (c) 2020 Project CHIP Authors
 *    Copyright (c) 2018 Nest Labs, Inc.
 *    All rights reserved.
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 */

/**
 *    @file
 *          Platform-specific configuration overrides for CHIP on
 *          the STM32 platforms.
 */

#pragma once

#ifndef CHIP_PLATFORM_CONFIG_H
#define CHIP_PLATFORM_CONFIG_H

// ==================== General Platform Adaptations ====================

#define CHIP_CONFIG_ABORT() abort()

#define CHIP_CONFIG_PERSISTED_STORAGE_KEY_TYPE         uint32_t
#define CHIP_CONFIG_PERSISTED_STORAGE_ENC_MSG_CNTR_ID  1
#define CHIP_CONFIG_PERSISTED_STORAGE_MAX_KEY_LENGTH   2

#ifndef CHIP_LOG_FILTERING
#define CHIP_LOG_FILTERING 0
#endif // CHIP_LOG_FILTERING

// ==================== Security Adaptations ====================

#define CHIP_CONFIG_USE_OPENSSL_ECC 0
#define CHIP_CONFIG_USE_MICRO_ECC 0

#define CHIP_CONFIG_HASH_IMPLEMENTATION_OPENSSL 0
//#define CHIP_CONFIG_HASH_IMPLEMENTATION_MINCRYPT 1

#define CHIP_CONFIG_RNG_IMPLEMENTATION_OPENSSL 0
#define CHIP_CONFIG_RNG_IMPLEMENTATION_CHIPDRBG 1

#define CHIP_CONFIG_AES_IMPLEMENTATION_OPENSSL 0
#define CHIP_CONFIG_AES_IMPLEMENTATION_AESNI 0
//#define CHIP_CONFIG_AES_IMPLEMENTATION_PLATFORM 1

#define CHIP_CONFIG_SUPPORT_PASE_CONFIG0 0
#define CHIP_CONFIG_SUPPORT_PASE_CONFIG1 0
#define CHIP_CONFIG_SUPPORT_PASE_CONFIG2 0
#define CHIP_CONFIG_SUPPORT_PASE_CONFIG3 0
#define CHIP_CONFIG_SUPPORT_PASE_CONFIG4 1

#define CHIP_CONFIG_ENABLE_KEY_EXPORT_INITIATOR 0

#define CHIP_CONFIG_ENABLE_PROVISIONING_BUNDLE_SUPPORT 0

#define CHIP_DEVICE_CONFIG_DYNAMIC_ENDPOINT_COUNT 8

#ifndef CHIP_CONFIG_MAX_UNSOLICITED_MESSAGE_HANDLERS
#define CHIP_CONFIG_MAX_UNSOLICITED_MESSAGE_HANDLERS 8
#endif // CHIP_CONFIG_MAX_UNSOLICITED_MESSAGE_HANDLERS

#ifndef CHIP_CONFIG_MAX_EXCHANGE_CONTEXTS
#define CHIP_CONFIG_MAX_EXCHANGE_CONTEXTS 8
#endif // CHIP_CONFIG_MAX_EXCHANGE_CONTEXTS

#define CHIP_CONFIG_ENABLE_SERVER_IM_EVENT 1

#ifndef CHIP_CONFIG_MAX_FABRICS
#define CHIP_CONFIG_MAX_FABRICS 5
#endif // CHIP_CONFIG_MAX_FABRICS

#endif /* CHIP_PLATFORM_CONFIG_H */
