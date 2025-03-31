/*
 *
 *    Copyright (c) 2023 Project CHIP Authors
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
 *          Utilities for interacting with the the STM32 "NVS" key-value store.
 */

/* this file behaves like a config.h, comes first */
#include <platform/internal/CHIPDeviceLayerInternal.h>

#include "STM32Config.h"
#include <platform/KeyValueStoreManager.h>

#include <lib/support/logging/CHIPLogging.h>

#define BUFFER_KEY_LEN_MAX (32)

namespace chip {
namespace DeviceLayer {
namespace Internal {

CHIP_ERROR STM32Config::Init()
{
    CHIP_ERROR err;

    ChipLogProgress(DeviceLayer, "STM32 ConfigInit");
    err = ConvertNVMStatus(NVM_Init(BUFFER_KEY_LEN_MAX));

    return err;
}

void STM32Config::DeInit()
{
    ChipLogProgress(DeviceLayer, "STM32 ConfigDeInit");
}

CHIP_ERROR STM32Config::ReadConfigValue(Key key, bool &val)
{
    CHIP_ERROR err;
    char bufferKey[BUFFER_KEY_LEN_MAX] = {0,};
    bool tmpVal;
    size_t outLen;

    // ChipLogProgress(DeviceLayer, "STM32 ReadConfigValueBool %s", ProvideKeyString(key));
    // VerifyOrExit(ValidConfigKey(key), err = CHIP_DEVICE_ERROR_CONFIG_NOT_FOUND); // Verify key id.
    // sprintf(bufferKey, "%07x", key);
    sprintf((char*) bufferKey, "Config%" PRIu32, key);
    err = ConvertNVMStatus(NVM_GetKeyValue(bufferKey, reinterpret_cast<uint8_t *>(&tmpVal), sizeof(bool),
                                           &outLen, NVM_SECTOR_CONFIG));
    SuccessOrExit(err);
    val = tmpVal;

exit:
    return err;
}

CHIP_ERROR STM32Config::ReadConfigValue(Key key, uint32_t &val)
{
    CHIP_ERROR err;
    char bufferKey[BUFFER_KEY_LEN_MAX] = {0,};
    uint32_t tmpVal;
    size_t outLen;

    // ChipLogProgress(DeviceLayer, "STM32 ReadConfigValueUINT32 %s", ProvideKeyString(key));
    // VerifyOrExit(ValidConfigKey(key), err = CHIP_DEVICE_ERROR_CONFIG_NOT_FOUND); // Verify key id.
    // sprintf(bufferKey, "%07x", key);
    sprintf(bufferKey, "Config%" PRIu32, key);
    err = ConvertNVMStatus(NVM_GetKeyValue(bufferKey, reinterpret_cast<uint8_t *>(&tmpVal), sizeof(uint32_t),
                                           &outLen, NVM_SECTOR_CONFIG));
    SuccessOrExit(err);
    val = tmpVal;

exit:
    return err;
}

CHIP_ERROR STM32Config::ReadConfigValue(Key key, uint64_t &val)
{
    CHIP_ERROR err;
    char bufferKey[BUFFER_KEY_LEN_MAX] = {0,};
    uint64_t tmpVal;
    size_t outLen;

    // ChipLogProgress(DeviceLayer, "STM32 ReadConfigValueUINT64 %s", ProvideKeyString(key));
    // VerifyOrExit(ValidConfigKey(key), err = CHIP_DEVICE_ERROR_CONFIG_NOT_FOUND); // Verify key id.
    // sprintf(bufferKey, "%07x", key);
    sprintf(bufferKey, "Config%" PRIu32, key);
    err = ConvertNVMStatus(NVM_GetKeyValue(bufferKey, reinterpret_cast<uint8_t *>(&tmpVal), sizeof(uint64_t),
                                           &outLen, NVM_SECTOR_CONFIG));
    SuccessOrExit(err);
    val = tmpVal;

exit:
    return err;
}

CHIP_ERROR STM32Config::ReadConfigValueStr(Key key, char *buf, size_t bufSize, size_t &outLen)
{
    CHIP_ERROR err;

    // ChipLogProgress(DeviceLayer, "STM32 ReadConfigValueStr %s", ProvideKeyString(key));
    err = ReadConfigValueBin(key, reinterpret_cast<uint8_t*>(buf), bufSize, outLen);
    // or go through a uint8_t *tmpBuf = NULL and tmpBufLen = strlen(tmpBuf)
    // if (tmpBufLen <= 0) err = CHIP_ERROR_INVALID_STRING_LENGTH;
    // if (bufSize <= tmpBufLen) err = CHIP_ERROR_BUFFER_TOO_SMALL;
    // outLen = ((tmpBufLen == 1) && (tmpBuf[0] == 0)) ? 0 : tmpBufLen;
    // memcpy(buf, tmpBuf, outLen);
    // buf[outLen] = 0;

    return err;
}

CHIP_ERROR STM32Config::ReadConfigValueBin(Key key, uint8_t *buf, size_t bufSize, size_t &outLen)
{
    CHIP_ERROR err;
    char bufferKey[BUFFER_KEY_LEN_MAX] = {0,};

    // ChipLogProgress(DeviceLayer, "STM32 ReadConfigValueBin %s", ProvideKeyString(key));
    // VerifyOrExit(ValidConfigKey(key), err = CHIP_DEVICE_ERROR_CONFIG_NOT_FOUND); // Verify key id.
    // sprintf(bufferKey, "%07x", key);
    sprintf(bufferKey, "Config%" PRIu32, key);
    err = ConvertNVMStatus(NVM_GetKeyValue(bufferKey, buf, bufSize, &outLen, NVM_SECTOR_CONFIG));

    return err;
}

#if 0
CHIP_ERROR STM32Config::ReadConfigValueCounter(uint8_t counterIdx, uint32_t & val)
{
    CHIP_ERROR err;
    char bufferKey[BUFFER_KEY_LEN_MAX] = {0,};
    Key key = kMinConfigKey_MatterCounter + counterIdx;
    uint32_t tmpVal;
    size_t outLen;

    // ChipLogProgress(DeviceLayer, "STM32 ReadConfigValueCounter %lu", key);
    // VerifyOrExit(ValidConfigKey(key), err = CHIP_DEVICE_ERROR_CONFIG_NOT_FOUND); // Verify key id.
    // sprintf(bufferKey, "%07x", key);
    sprintf(bufferKey, "Config%" PRIu32, key);
    err = ConvertNVMStatus(NVM_GetKeyValue(bufferKey, reinterpret_cast<uint8_t *>(&tmpVal), sizeof(uint32_t),
                                           &outLen, NVM_SECTOR_CONFIG));
    SuccessOrExit(err);
    val = tmpVal;

exit:
    return err;
}
#endif

CHIP_ERROR STM32Config::WriteConfigValue(Key key, bool val)
{
    CHIP_ERROR err;
    char bufferKey[BUFFER_KEY_LEN_MAX] = {0,};
    uint8_t tmpVal;

    // ChipLogProgress(DeviceLayer, "STM32 WriteConfigValue: %s %s", ProvideKeyString(key), val ? "true" : "false");
    // VerifyOrExit(ValidConfigKey(key), err = CHIP_DEVICE_ERROR_CONFIG_NOT_FOUND); // Verify key id.
    // sprintf(bufferKey, "%07x", key);
    sprintf(bufferKey, "Config%" PRIu32, key);
    tmpVal = val;
    err = ConvertNVMStatus(NVM_SetKeyValue(bufferKey, reinterpret_cast<uint8_t *>(&tmpVal), sizeof(bool), NVM_SECTOR_CONFIG));

    return err;
}

CHIP_ERROR STM32Config::WriteConfigValue(Key key, uint32_t val)
{
    CHIP_ERROR err;
    char bufferKey[BUFFER_KEY_LEN_MAX] = {0,};
    uint8_t tmpVal[sizeof(uint32_t)];

    // ChipLogProgress(DeviceLayer, "STM32 WriteConfigValue: %s %" PRIu32 " (0x%" PRIX32 ")", ProvideKeyString(key), val, val);
    // VerifyOrExit(ValidConfigKey(key), err = CHIP_DEVICE_ERROR_CONFIG_NOT_FOUND); // Verify key id.
    // sprintf(bufferKey, "%07x", key);
    sprintf(bufferKey, "Config%" PRIu32, key);
    for (uint8_t i = 0; i < sizeof(uint32_t); i++)
    {
        tmpVal[i] = (val >> (8 * i)) & 0xFF;
    }
    err = ConvertNVMStatus(NVM_SetKeyValue(bufferKey, reinterpret_cast<uint8_t *>(&tmpVal), sizeof(uint32_t), NVM_SECTOR_CONFIG));

    return err;
}

CHIP_ERROR STM32Config::WriteConfigValue(Key key, uint64_t val)
{
    CHIP_ERROR err;
    char bufferKey[BUFFER_KEY_LEN_MAX] = {0,};
    uint8_t tmpVal[sizeof(uint64_t)];

    // ChipLogProgress(DeviceLayer, "STM32 WriteConfigValue: %s %" PRIu64 " (0x%" PRIX64 ")", ProvideKeyString(key), val, val);
    // VerifyOrExit(ValidConfigKey(key), err = CHIP_DEVICE_ERROR_CONFIG_NOT_FOUND); // Verify key id.
    // sprintf(bufferKey, "%07x", key);
    sprintf(bufferKey, "Config%" PRIu32, key);
    for (uint8_t i = 0; i < sizeof(uint64_t); i++)
    {
        tmpVal[i] = (val >> (8 * i)) & 0xFF;
    }
    err = ConvertNVMStatus(NVM_SetKeyValue(bufferKey, reinterpret_cast<uint8_t *>(&tmpVal), sizeof(uint64_t), NVM_SECTOR_CONFIG));

    return err;
}

CHIP_ERROR STM32Config::WriteConfigValueStr(Key key, const char *str)
{
    // ChipLogProgress(DeviceLayer, "STM32 WriteConfigValueStr: %s = \"%s\"", ProvideKeyString(key), str);
    return WriteConfigValueStr(key, str, (str != NULL) ? strlen(str) : 0);
}

CHIP_ERROR STM32Config::WriteConfigValueStr(Key key, const char *str, size_t strLen)
{
    // ChipLogProgress(DeviceLayer, "STM32 WriteConfigValueStrlen: %s = \"%s\" %" PRIu32 "", ProvideKeyString(key), str, (uint32_t)strLen);
    return WriteConfigValueBin(key, reinterpret_cast<const uint8_t*>(str), strLen);
}

CHIP_ERROR STM32Config::WriteConfigValueBin(Key key, const uint8_t * data, size_t dataLen)
{
    CHIP_ERROR err;
    char bufferKey[BUFFER_KEY_LEN_MAX] = {0,};

    // ChipLogProgress(DeviceLayer, "STM32 WriteConfigValueBin: %s = (blob length %" PRId32 ")", ProvideKeyString(key), (uint32_t)dataLen);
    // VerifyOrExit(ValidConfigKey(key), err = CHIP_DEVICE_ERROR_CONFIG_NOT_FOUND); // Verify key id.
    // sprintf(bufferKey, "%07x", key);
    sprintf(bufferKey, "Config%" PRIu32, key);
    err = ConvertNVMStatus(NVM_SetKeyValue(bufferKey, data, dataLen, NVM_SECTOR_CONFIG));

    return err;
}

#if 0
CHIP_ERROR STM32Config::WriteConfigValueCounter(uint8_t counterIdx, uint32_t val)
{
    CHIP_ERROR err;
    char bufferKey[BUFFER_KEY_LEN_MAX] = {0,};
    Key key = kMinConfigKey_MatterCounter + counterIdx;
    uint8_t tmpVal[sizeof(uint32_t)];

    // ChipLogProgress(DeviceLayer, "STM32 WriteConfigValueCounter %lu", key);
    // VerifyOrExit(ValidConfigKey(key), err = CHIP_DEVICE_ERROR_CONFIG_NOT_FOUND); // Verify key id.
    // sprintf(bufferKey, "%07x", key);
    sprintf(bufferKey, "Config%" PRIu32, key);
    for (uint8_t i = 0; i < sizeof(uint32_t); i++)
    {
        tmpVal[i] = (val >> (8 * i)) & 0xFF;
    }
    err = ConvertNVMStatus(NVM_SetKeyValue(bufferKey, reinterpret_cast<uint8_t *>(&tmpVal), sizeof(uint32_t), NVM_SECTOR_CONFIG));

    return err;
}
#endif

CHIP_ERROR STM32Config::ClearConfigValue(Key key)
{
    CHIP_ERROR err;
    char bufferKey[BUFFER_KEY_LEN_MAX] = {0,};

    // ChipLogProgress(DeviceLayer, "STM32 ClearConfigValue %s", ProvideKeyString(key));
    // VerifyOrExit(ValidConfigKey(key), err = CHIP_DEVICE_ERROR_CONFIG_NOT_FOUND); // Verify key id.
    // sprintf(bufferKey, "%07x", key);
    sprintf(bufferKey, "Config%" PRIu32, key);
    err = ConvertNVMStatus(NVM_DeleteKey(bufferKey, NVM_SECTOR_CONFIG));

    return err;
}

bool STM32Config::ConfigValueExists(Key key)
{
    NVM_StatusTypeDef err;
    char bufferKey[BUFFER_KEY_LEN_MAX] = {0,};

    // ChipLogProgress(DeviceLayer, "STM32 ConfigValueExists %s", ProvideKeyString(key));
    // VerifyOrExit(ValidConfigKey(key), err = CHIP_DEVICE_ERROR_CONFIG_NOT_FOUND); // Verify key id.
    // sprintf(bufferKey, "%07x", key);
    sprintf(bufferKey, "Config%" PRIu32, key);
    err = NVM_GetKeyExists(bufferKey, NVM_SECTOR_CONFIG);

    return (err == NVM_OK);
}

CHIP_ERROR STM32Config::FactoryResetConfig(void)
{
    return CHIP_ERROR_UNSUPPORTED_CHIP_FEATURE;
}

void STM32Config::RunConfigUnitTest(void)
{
}


CHIP_ERROR STM32Config::ConvertNVMStatus(NVM_StatusTypeDef nvmStatus)
{
    CHIP_ERROR err;

    switch (nvmStatus)
    {
    case NVM_OK:
        ChipLogDetail(DataManagement, "NVM_OK");
        err = CHIP_NO_ERROR;
        break;

      case NVM_KEY_NOT_FOUND:
        ChipLogDetail(DataManagement, "NVM_KEY_NOT_FOUND");
        err = CHIP_ERROR_PERSISTED_STORAGE_VALUE_NOT_FOUND;
        break;

      case NVM_WRITE_FAILED:
        ChipLogDetail(DataManagement, "NVM_WRITE_FAILED");
        err = CHIP_ERROR_PERSISTED_STORAGE_FAILED;
        break;

      case NVM_READ_FAILED:
        ChipLogDetail(DataManagement, "NVM_READ_FAILED");
        err = CHIP_ERROR_PERSISTED_STORAGE_FAILED;
        break;

      case NVM_DELETE_FAILED:
        ChipLogDetail(DataManagement, "NVM_DELETE_FAILED");
        err = CHIP_ERROR_PERSISTED_STORAGE_FAILED;
        break;

      case NVM_LACK_SPACE:
        ChipLogDetail(DataManagement, "NVM_LACK_SPACE");
        err = CHIP_ERROR_PERSISTED_STORAGE_FAILED;
        break;

      case NVM_PARAMETER_ERROR:
        ChipLogDetail(DataManagement, "NVM_PARAMETER_ERROR");
        err = CHIP_ERROR_INVALID_ARGUMENT;
        break;

      case NVM_BLOCK_SIZE_OVERFLOW:
        ChipLogDetail(DataManagement, "NVM_BLOCK_SIZE_OVERFLOW");
        err = CHIP_ERROR_PERSISTED_STORAGE_FAILED;
        break;

      case NVM_ERROR_BLOCK_ALIGN:
        ChipLogDetail(DataManagement, "NVM_ERROR_BLOCK_ALIGN");
        err = CHIP_ERROR_PERSISTED_STORAGE_FAILED;
        break;

      case NVM_BUFFER_TOO_SMALL:
        ChipLogDetail(DataManagement, "NVM_BUFFER_TOO_SMALL");
        err = CHIP_ERROR_BUFFER_TOO_SMALL;
        break;

      case NVM_CORRUPTION:
        ChipLogDetail(DataManagement, "NVM_CORRUPTION");
        err = CHIP_ERROR_INTEGRITY_CHECK_FAILED;
        break;

      default:
        ChipLogDetail(DataManagement, "NVM_UNKNOWN_ERROR");
        err = CHIP_ERROR_PERSISTED_STORAGE_FAILED;
        break;
    }

    return err;
}

#if 0
bool STM32Config::ValidConfigKey(Key key)
{
    bool result = false;
    // Returns true if the key is in the Matter key range.
    // Additional check validates that the user consciously defined the expected key range
    if (((key >= kMatterNvramKeyLoLimit) && (key <= kMatterNvramKeyHiLimit))
        && (((key >= kMinConfigKey_MatterFactory) && (key <= kMaxConfigKey_MatterFactory))
            || ((key >= kMinConfigKey_MatterConfig) && (key <= kMaxConfigKey_MatterConfig))))
    {
        result = true;
    }

    return result;
}
#endif /* unused */

} // namespace Internal
} // namespace DeviceLayer
} // namespace chip

