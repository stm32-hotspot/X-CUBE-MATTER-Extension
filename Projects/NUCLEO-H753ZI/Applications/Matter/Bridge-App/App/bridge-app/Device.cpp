/*
 *
 *    Copyright (c) 2021-2022 Project CHIP Authors
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

#include "Device.h"

#include "main.h"
#include <string.h>

#include <lib/support/CHIPMemString.h>
#include <platform/CHIPDeviceLayer.h>
#include <crypto/RandUtils.h>
#include <lib/support/BytesToHex.h>

#include "AppLog.h"

using namespace ::chip::Platform;

char Device::mLocationCustom1[Device::kLocationSize] = "";
char Device::mLocationCustom2[Device::kLocationSize] = "";

Device::Device(const char * szName = nullptr, const char * szUniqueId = nullptr,
               chip::DeviceTypeId aDeviceTypeId = 0x0000,
               Location_t aLocation = kLocationUnknown, ChannelCom_t aChannelCom = kChannelComUnknown)
{
    if (szName != nullptr)
    {
        CopyString(mName, kNameSize, szName);
    }
    else
    {
        memset(mName, 0, kNameSize);
    }
    if (szUniqueId != nullptr)
    {
        CopyString(mUniqueId, kUniqueIdSize, szUniqueId);
    }
    else
    {
        memset(mUniqueId, 0, kUniqueIdSize);
    }
    mDeviceTypeId = 0x00000000 | (aDeviceTypeId & 0x0000FFFF);
    mLocation = aLocation;
    mChannelCom = aChannelCom;
    mReachable  = false;
    mEndpointId = 0;
    mParentEndpointId = 0;
}

void Device::Reset()
{
    memset(mName, 0, kNameSize);
    memset(mUniqueId, 0, kUniqueIdSize);
    mEndpointId = 0;
    mParentEndpointId = 0;
    mLocation   = kLocationUnknown;
    mReachable  = false;
    mChannelCom = kChannelComUnknown;
}

chip::DeviceTypeId Device::GetDeviceTypeId(uint8_t * ptDataRead, uint8_t dataReadLength)
{
    chip::DeviceTypeId deviceTypeId = 0x00000000;

    if ((ptDataRead != nullptr) && (dataReadLength >= kDataStorageSizeMin))
    {
        DataStorage_t *ptDataStorage = (DataStorage_t *)ptDataRead;
        deviceTypeId |=  ((ptDataStorage->deviceTypeId[0] << 8) + ptDataStorage->deviceTypeId[1]);
    }

    return deviceTypeId;
}

void Device::SetName(const char * szName, uint8_t nameLength)
{
    if ((szName != nullptr) && (nameLength != 0U) && (strncmp(mName, szName, kNameSize) != 0))
    {
        if (nameLength < kNameSize)
        {
            ChipLogProgress(DeviceLayer, "Device[%s]: New Name:%.*s", mName, nameLength, szName);
            CopyString(mName, (nameLength + 1), szName);
            mName[nameLength]='\0';
        }
        else
        {
            ChipLogProgress(DeviceLayer, "Device[%s]: New Name:%.*s", mName, (kNameSize -1), szName);
            CopyString(mName, kNameSize, szName);
        }
        HandleDeviceChange(this, kChanged_Name);
    }
}

void Device::SetUniqueId(const char * szUniqueId, uint8_t uniqueIdLength)
{
    if ((szUniqueId != nullptr) && (uniqueIdLength != 0U))
    {
        if (uniqueIdLength < kUniqueIdSize)
        {
            CopyString(mUniqueId, (uniqueIdLength + 1), szUniqueId);
            mUniqueId[uniqueIdLength]='\0';
        }
        else
        {
            CopyString(mUniqueId, kUniqueIdSize, szUniqueId);
        }
    }
    else
    {
        // example: generate a random UniqueId
        uint64_t randomUniqueId = chip::Crypto::GetRandU64();
        CHIP_ERROR err;
        err = chip::Encoding::BytesToUppercaseHexString(reinterpret_cast<uint8_t *>(&randomUniqueId), sizeof(uint64_t),
                                                        mUniqueId, kUniqueIdSize);
        if (err != CHIP_NO_ERROR)
        {
            ChipLogProgress(DeviceLayer, "Device[%s]: ERROR in UniqueId generation !", mName);
        }
    }
}

bool Device::IsReachable() const
{
    return mReachable;
}

void Device::SetReachable(bool aReachable)
{
    bool changed = (mReachable != aReachable);

    mReachable = aReachable;
    if (aReachable)
    {
        ChipLogProgress(DeviceLayer, "Device[%s]: ONLINE", mName);
    }
    else
    {
        ChipLogProgress(DeviceLayer, "Device[%s]: OFFLINE", mName);
    }
    if (changed)
    {
        HandleDeviceChange(this, kChanged_Reachable);
    }
}

void Device::SetLocation(Location_t aLocation)
{
    if ((aLocation <= kLocation_Last) && (mLocation != aLocation))
    {
        mLocation = aLocation;
        ChipLogProgress(DeviceLayer, "Device[%s]: New Location:%s", mName, GetLocationName());
        HandleDeviceChange(this, kChanged_Location);
    }
}

char * Device::GetLocationName()
{
    if ((mLocation == kLocationCustom1) && (mLocationCustom1[0] != '\0'))
    {
        return mLocationCustom1;
    }
    else if ((mLocation == kLocationCustom2) && (mLocationCustom2[0] != '\0'))
    {
        return mLocationCustom2;
    }
    else
    {
        return (char *)kLocation[mLocation];
    }
};

char * Device::GetLocationName(Location_t aLocation)
{
    if ((aLocation == kLocationCustom1) && (mLocationCustom1[0] != '\0'))
    {
        return mLocationCustom1;
    }
    else if ((aLocation == kLocationCustom2) && (mLocationCustom2[0] != '\0'))
    {
        return mLocationCustom2;
    }
    else
    {
        return (char *)kLocation[aLocation];
    }
};

bool Device::SetLocationName(Location_t aLocation, const char * szLocationName)
{
    bool ret = false;

    if (szLocationName != nullptr)
    {
        if (aLocation == kLocationCustom1)
        {
            CopyString(mLocationCustom1, kLocationSize, szLocationName);
            ChipLogProgress(DeviceLayer, "New name LocationCustom1:%s", mLocationCustom1);
            ret = true;
        }
        else if (aLocation == kLocationCustom2)
        {
            CopyString(mLocationCustom2, kLocationSize, szLocationName);
            ChipLogProgress(DeviceLayer, "New name LocationCustom2:%s", mLocationCustom2);
            ret = true;
        }
        else
        {
            /* Nothing to do */
        }
    }

    return ret;
}

void Device::SetChannelCom(ChannelCom_t aChannelCom)
{
    if ((aChannelCom <= kChannelCom_Last) && (aChannelCom != mChannelCom))
    {
        mChannelCom = aChannelCom;
        HandleDeviceChange(this, kChanged_ChannelCom);
    }
}

bool Device::GetGenericDataStorage(uint8_t * ptDataToSave, uint8_t * ptDataToSaveLength)
{
    bool ret = false;

    if ((ptDataToSave != nullptr) && (ptDataToSaveLength != nullptr)
        && (*ptDataToSaveLength >= kGenericDataStorageSizeMax))
    {
        uint8_t i = 0U;
        uint8_t nameLocationSize;

        memset(ptDataToSave, 0, *ptDataToSaveLength);
        *ptDataToSaveLength  = 0U;
        if (mLocationCustom1[0] != '\0')
        {
            nameLocationSize = strlen(mLocationCustom1);
            ptDataToSave[0] = kLocationCustom1;
            ptDataToSave[1] = nameLocationSize;
            memcpy(&ptDataToSave[2], mLocationCustom1, nameLocationSize); /* Don't copy '\0' */
            i += 2U + nameLocationSize;
            *ptDataToSaveLength = i;
            ret = true;
        }
        if (mLocationCustom2[0] != '\0')
        {
            nameLocationSize = strlen(mLocationCustom2);
            ptDataToSave[i] = kLocationCustom2;
            ptDataToSave[i + 1U] = nameLocationSize;
            memcpy(&ptDataToSave[i + 2U], mLocationCustom2, nameLocationSize); /* Don't copy '\0' */
            i += 2U + nameLocationSize;
            *ptDataToSaveLength = i;
            ret = true;
        }
    }

    return ret;
}

bool Device::SetGenericDataStorage(uint8_t * ptDataRead, uint8_t dataReadLength)
{
    bool ret = false;

    if ((ptDataRead != nullptr) && (dataReadLength > 2U))
    {
        uint8_t i = 0U;
        uint8_t nameLocationLength;
        Device::Location_t mLocation;
        char mLocationName[Device::kLocationSize] = "";

        mLocation = (Device::Location_t)ptDataRead[i];
        nameLocationLength = ptDataRead[i + 1U];
        if (((nameLocationLength + 2U) <= dataReadLength) && (nameLocationLength < kLocationSize))
        {
            memset(mLocationName, '\0', sizeof(mLocationName));
            memcpy(mLocationName, &ptDataRead[i + 2U], nameLocationLength);
            ret = SetLocationName(mLocation, mLocationName);
            i = nameLocationLength + 2U;
        }
        if ((ret == true) && (i < dataReadLength))
        {
            if ((i + 2U) < dataReadLength)
            {
                mLocation = (Device::Location_t)ptDataRead[i];
                nameLocationLength = ptDataRead[i + 1U];
                if (((nameLocationLength + i + 2U) <= dataReadLength) && (nameLocationLength < kLocationSize))
                {
                    memset(mLocationName, '\0', sizeof(mLocationName));
                    memcpy(mLocationName, &ptDataRead[i + 2U], nameLocationLength);
                    ret = SetLocationName(mLocation, mLocationName);
                }
            }
        }
    }

    return ret;
}

bool Device::GetDataStorage(uint8_t * ptDataToSave, uint8_t * ptDataToSaveLength)
{
    bool ret = false;

    if ((ptDataToSave != nullptr) && (ptDataToSaveLength != nullptr)
        && (*ptDataToSaveLength >= kDataStorageSizeMax))
    {
        uint8_t nameLength = strlen(GetName());
        uint8_t uniqueIdLength = strlen(GetUniqueId());
        DataStorage_t dataStorage;

        dataStorage.deviceTypeId[0]     = (uint8_t)((GetDeviceTypeId() & 0x0000FF00) >> 8);
        dataStorage.deviceTypeId[1]     = (uint8_t) (GetDeviceTypeId() & 0x000000FF);
        dataStorage.endpointId[0]       = (uint8_t)((GetEndpointId() & 0xFF00) >> 8);
        dataStorage.endpointId[1]       = (uint8_t) (GetEndpointId() & 0x00FF);
        dataStorage.parentEndpointId[0] = (uint8_t)((GetParentEndpointId() & 0xFF00) >> 8);
        dataStorage.parentEndpointId[1] = (uint8_t) (GetParentEndpointId() & 0x00FF);
        dataStorage.location = GetLocation();
        dataStorage.channelCom = GetChannelCom();
        dataStorage.reachable = (IsReachable() == true ? 1 : 0);
        dataStorage.nameLength = nameLength;
        dataStorage.uniqueIdLength = uniqueIdLength;
        memcpy(ptDataToSave, &dataStorage, sizeof(DataStorage_t));
        memcpy(&ptDataToSave[sizeof(DataStorage_t)], GetName(), nameLength); /* Don't copy '\0' */
        memcpy(&ptDataToSave[(sizeof(DataStorage_t) + nameLength)], GetUniqueId(), uniqueIdLength); /* Don't copy '\0' */
        *ptDataToSaveLength = sizeof(DataStorage_t) + nameLength + uniqueIdLength;
        ret = true;
    }

    return ret;
}

uint8_t Device::SetDataStorage(uint8_t * ptDataRead, uint8_t dataReadLength)
{
    uint8_t lengthRead = 0U;

    if ((ptDataRead != nullptr) && (dataReadLength >= kDataStorageSizeMin))
    {
        chip::DeviceTypeId deviceTypeId;
        chip::EndpointId endpointId;
        chip::EndpointId parentEndpointId;
        char name[kNameSize] = "";
        char uniqueId[kUniqueIdSize] = "";
        Location_t location;
        ChannelCom_t channelCom;
        bool reachable;
        uint8_t nameLength;
        uint8_t uniqueIdLength;

        DataStorage_t *ptDataStorage = (DataStorage_t *)ptDataRead;
        deviceTypeId     = 0x00000000 | ((ptDataStorage->deviceTypeId[0] << 8) + ptDataStorage->deviceTypeId[1]);
        endpointId       = (ptDataStorage->endpointId[0] << 8) + ptDataStorage->endpointId[1];
        parentEndpointId = (ptDataStorage->parentEndpointId[0] << 8) + ptDataStorage->parentEndpointId[1];
        reachable        = ptDataStorage->reachable;
        channelCom       = (ChannelCom_t)ptDataStorage->channelCom;
        location         = (Location_t)ptDataStorage->location;
        nameLength       = ptDataStorage->nameLength;
        uniqueIdLength   = ptDataStorage->uniqueIdLength;
        if (dataReadLength >= (kDataStorageSizeMin + nameLength + uniqueIdLength))
        {
            memcpy(name, &ptDataRead[sizeof(DataStorage_t)], nameLength);
            memcpy(uniqueId, &ptDataRead[(sizeof(DataStorage_t) + nameLength)], uniqueIdLength);
            lengthRead = kDataStorageSizeMin + nameLength + uniqueIdLength;
            mDeviceTypeId = deviceTypeId;
            SetEndpointId(endpointId);
            SetParentEndpointId(parentEndpointId);
            SetName(name, nameLength);
            SetUniqueId(uniqueId, uniqueIdLength);
            SetReachable(reachable);
            SetLocation(location);
            SetChannelCom(channelCom);
        }
    }

    return lengthRead;
}

DeviceOnOff::DeviceOnOff() : Device(nullptr, nullptr, DEVICE_TYPE_LO_ON_OFF_LIGHT, kLocationUnknown, kUART1)
{
    mOn = false;
    mChanged_CB = NULL;
}

DeviceOnOff::~DeviceOnOff()
{
}

void DeviceOnOff::Reset()
{
    SetChangeCallback(NULL);
    Device::Reset();
}

bool DeviceOnOff::IsOn() const
{
    return mOn;
}

void DeviceOnOff::SetOnOff(bool aOn)
{
    bool changed;

    changed = aOn ^ mOn;
    mOn     = aOn;

    if (changed)
    {
        ChipLogProgress(DeviceLayer, "Device[%s]:%s", mName, aOn ? "ON" : "OFF");
        if (strncmp(mName, "Light", 5) == 0)
        {
            UART_HandleTypeDef * huart = NULL;
            switch (mChannelCom) {
            case kUART1 :
                huart= &hlpuart1;
                break;
            case kUART3 :
                huart= &huart3;
                break;
            case kChannelComUnknown :
            default :
                break;
            }
            if (huart != NULL)
            {
                Led_TypeDef ledId;
                char stringToTransmit[kNameSize + 4];
                char end[3] = "\r\n";

                CopyString(stringToTransmit, kNameSize, mName);
                if (mOn == true)
                {
                    strcat(stringToTransmit,":ON");
                }
                else
                {
                    strcat(stringToTransmit,":OFF");
                }
                STM32_LOG("Trame transmitted:%s", stringToTransmit);
                (void)HAL_UART_Transmit(huart, (const uint8_t *)&stringToTransmit, strlen(stringToTransmit), 3000);
                (void)HAL_UART_Transmit(huart, (const uint8_t *)&end, 2, 3000);
                if (IsLocal(&ledId) == true)
                {
                    (mOn == true) ? BSP_LED_On(ledId) : BSP_LED_Off(ledId);
                }
            }
        }
        if (mChanged_CB)
        {
            mChanged_CB(this, kChanged_OnOff);
        }
    }
    else
    {
        ChipLogProgress(DeviceLayer, "Device[%s] already %s", mName, aOn ? "ON" : "OFF");
    }
}

bool DeviceOnOff::IsLocal(Led_TypeDef * ledId)
{
    bool ret = true;

    if ((strncmp(mName, "Light1", strlen(mName)) == 0) || (strncmp(mName, "Light1b", strlen(mName)) == 0))
    {
        *ledId = LED1;
    }
    else if ((strncmp(mName, "Light2", strlen(mName)) == 0) || (strncmp(mName, "Light2b", strlen(mName)) == 0))
    {
        *ledId = LED2;
    }
    else
    {
        ret = false;
    }

    return ret;
}

void DeviceOnOff::SetOnOff()
{
    Led_TypeDef ledId;

    if (IsLocal(&ledId) == true)
    {
        if (BSP_LED_GetState(ledId) == GPIO_PIN_SET)
        {
            mOn = true;
        }
        else
        {
            mOn = false;
        }
    }
    else
    {
        /* No hardware to know the status */
        mOn = false;
    }
}

void DeviceOnOff::SetChangeCallback(DeviceCallback_fn aChanged_CB)
{
    mChanged_CB = aChanged_CB;
}

void DeviceOnOff::HandleDeviceChange(Device * device, Device::Changed_t changeMask)
{
    if (mChanged_CB)
    {
        mChanged_CB(this, (DeviceOnOff::Changed_t) changeMask);
    }
}

bool DeviceOnOff::GetDataStorage(uint8_t * ptDataToSave, uint8_t * ptDataToSaveLength)
{
    bool ret = false;

    if ((ptDataToSave != nullptr) && (ptDataToSaveLength != nullptr)
        && (*ptDataToSaveLength >= kDataStorageSizeMax))
    {
        memset(ptDataToSave, 0, *ptDataToSaveLength);
        ret = Device::GetDataStorage(ptDataToSave, ptDataToSaveLength);
        if (ret == true)
        {
            uint8_t i = *ptDataToSaveLength;
            ((DataStorage_t *)&ptDataToSave[i])->onOff = (IsOn() == true ? 1 : 0);
            *ptDataToSaveLength = i + 1;
        }
    }

    return ret;
}

bool DeviceOnOff::SetDataStorage(uint8_t * ptDataRead, uint8_t dataReadLength)
{
    bool ret = false;

    if ((ptDataRead != nullptr) && (dataReadLength >= kDataStorageSizeMin))
    {
        uint8_t lengthRead = Device::SetDataStorage(ptDataRead, dataReadLength);
        if ((lengthRead != 0U) && (dataReadLength > lengthRead))
        {
            bool on = (((DataStorage_t *)&ptDataRead[lengthRead])->onOff == 1) ? true : false;
            SetOnOff(on);
            ret = true;
        }
    }

    return ret;
}
