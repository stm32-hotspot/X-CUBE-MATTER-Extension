/*
 *
 *    Copyright (c) 2021 Project CHIP Authors
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
 *          Platform-specific key value storage implementation for STM32
 */

#include <platform/KeyValueStoreManager.h>

#include <lib/support/logging/CHIPLogging.h>

namespace chip {
namespace DeviceLayer {
namespace PersistedStorage {

/** Singleton instance of the KeyValueStoreManager implementation object.
 */
KeyValueStoreManagerImpl KeyValueStoreManagerImpl::sInstance;

CHIP_ERROR KeyValueStoreManagerImpl::_Get(const char * key, void * value, size_t value_size,
                                          size_t * read_bytes_size, size_t offset)
{
    CHIP_ERROR err = CHIP_ERROR_INVALID_ARGUMENT;

    if ((key != NULL) && (value != NULL) && (read_bytes_size != NULL)) {
        ChipLogProgress(DataManagement, "Get Key '%s'", key);
        err = this->ConvertNVMStatus(NVM_GetKeyValue(key, value, (uint32_t)value_size,
                                                     read_bytes_size, NVM_SECTOR_KEYSTORE));
    }

    return err;
}

CHIP_ERROR KeyValueStoreManagerImpl::_Delete(const char *key)
{
    CHIP_ERROR err = CHIP_ERROR_INVALID_ARGUMENT;

    if (key != NULL) {
        ChipLogProgress(DataManagement, "Delete Key '%s'", key);
        err = this->ConvertNVMStatus(NVM_DeleteKey(key, NVM_SECTOR_KEYSTORE));
    }

    return err;
}

CHIP_ERROR KeyValueStoreManagerImpl::_Put(const char * key, const void * value, size_t value_size) 
{
    CHIP_ERROR err = CHIP_ERROR_INVALID_ARGUMENT;

    if ((value_size != 0) && (key != NULL) && (value != NULL)) {
        ChipLogProgress(DataManagement, "Put Key '%s'",key);
        err = this->ConvertNVMStatus(NVM_SetKeyValue(key, value, (uint32_t)value_size, NVM_SECTOR_KEYSTORE));
    }

    return err;
}

CHIP_ERROR KeyValueStoreManagerImpl::ConvertNVMStatus(NVM_StatusTypeDef nvmStatus)
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

} // namespace PersistedStorage
} // namespace DeviceLayer
} // namespace chip
