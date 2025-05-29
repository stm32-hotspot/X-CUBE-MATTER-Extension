/*
 *
 *    Copyright (c) 2021 Project CHIP Authors
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

/* These are the bridged devices */
#include <app/util/attribute-storage.h>
#include <functional>
#include <stdbool.h>
#include <stdint.h>

#include "main.h"

namespace {
#define DEVICE_TYPE_LO_ON_OFF_LIGHT    0x0100
#define DEVICE_LOCATION_CUSTOM_MAX_NB  2
};

class Device
{
protected:

    typedef struct
    {
        uint8_t deviceTypeId[2];
        uint8_t endpointId[2];
        uint8_t parentEndpointId[2];
        uint8_t reachable:1;
        uint8_t channelCom:3;
        uint8_t location:4;
        uint8_t nameLength;
        uint8_t uniqueIdLength;
    } DataStorage_t;

public:

    enum Location_t
    {
        kLocationUnknown = 0,
        kEntrance,
        kKitchen,
        kLivingRoom,
        kDiningRoom,
        kHomeOffice,
        kBathroom,
        kMasterBedroom,
        kBedroom1,
        kBedroom2,
        kGarage,
        kGarden,
        kLocationCustom1,
        kLocationCustom2,
        kLocation_Last = kLocationCustom2
    };

    enum ChannelCom_t
    {
        kChannelComUnknown = 0,
        kUART1,
        kUART3,
        kChannelCom_Last = kUART3
    };

    enum Changed_t
    {
        kChanged_Name       = (0x01 << 0),
        kChanged_Reachable  = (0x01 << 1),
        kChanged_Location   = (0x01 << 2),
        kChanged_ChannelCom = (0x01 << 3),
        kChanged_Last       = kChanged_ChannelCom
    };

    static const uint8_t kNameSize     = 16U; /* 1 reserved for '\0' */
    static const uint8_t kUniqueIdSize = ((8U * 2U) + 1U); /* default:uint64_t + 1 reserved for '\0' */
    static const uint8_t kLocationSize = 16U; /* 1 reserved for '\0' */
    static const uint8_t kDataStorageSizeMin = sizeof(DataStorage_t);
    static const uint8_t kDataStorageSizeMax = kDataStorageSizeMin + (kNameSize - 1U) + (kUniqueIdSize - 1U);
    static const uint8_t kGenericDataStorageSizeMax = ((1U + (kLocationSize - 1U)) * 2U);

    static constexpr char kLocation[kLocation_Last + 1][kLocationSize] = {
        "Unknown", "Entrance", "Kitchen", "LivingRoom", "DiningRoom", "HomeOffice", "Bathroom",
        "MasterBedroom", "Bedroom1", "Bedroom2", "Garage", "Garden", "Custom1", "Custom2"
    }; 

    Device(const char * szName, const char * szUniqueId, chip::DeviceTypeId aDeviceTypeId,
           Location_t aLocation, ChannelCom_t aChannelCom);
    virtual ~Device() {}

    void Reset();

    inline char * GetName() { return mName; };
    void SetName(const char * szName, uint8_t nameLength);

    inline char * GetUniqueId() { return mUniqueId; };
    void SetUniqueId(const char * szUniqueId = nullptr, uint8_t uniqueIdLength = 0U);

    inline chip::DeviceTypeId GetDeviceTypeId() { return mDeviceTypeId; };
    static chip::DeviceTypeId GetDeviceTypeId(uint8_t * ptDataRead, uint8_t dataReadLength);

    inline void SetEndpointId(chip::EndpointId id) { mEndpointId = id; };
    inline chip::EndpointId GetEndpointId() { return mEndpointId; };
    inline void SetParentEndpointId(chip::EndpointId id) { mParentEndpointId = id; };
    inline chip::EndpointId GetParentEndpointId() { return mParentEndpointId; };

    bool IsReachable() const;
    void SetReachable(bool aReachable);

    inline Location_t GetLocation() { return mLocation; };
    void SetLocation(Location_t aLocation);
    char * GetLocationName();
    static char * GetLocationName(Location_t aLocation);
    static bool SetLocationName(Location_t aLocation, const char * szLocationName);

    inline ChannelCom_t GetChannelCom() { return mChannelCom; };
    void SetChannelCom(ChannelCom_t aChannelCom);

    static bool GetGenericDataStorage(uint8_t * ptDataToSave, uint8_t * ptDataToSaveLength);
    static bool SetGenericDataStorage(uint8_t * ptDataRead, uint8_t dataReadLength);
    bool GetDataStorage(uint8_t * ptDataToSave, uint8_t * ptDataToSaveLength);
    uint8_t SetDataStorage(uint8_t * ptDataRead, uint8_t dataReadLength);

private:
    virtual void HandleDeviceChange(Device * device, Device::Changed_t changeMask) = 0;

protected:
    char mName[kNameSize];
    char mUniqueId[kUniqueIdSize];
    chip::DeviceTypeId mDeviceTypeId;
    chip::EndpointId mEndpointId;
    chip::EndpointId mParentEndpointId;
    bool mReachable;
    Location_t mLocation;
    ChannelCom_t mChannelCom;
    static char mLocationCustom1[kLocationSize];
    static char mLocationCustom2[kLocationSize];
};

class DeviceOnOff : public Device
{

protected:
    /* Provide an example how to manage extra data for a specific Device as DeviceOnOff */
    typedef struct
    {
        uint8_t onOff:1;
    } DataStorage_t;

public:
    enum Changed_t
    {
        kChanged_OnOff  = kChanged_Last << 1,
    } Changed;

    static const uint8_t kDataStorageSizeMin = Device::kDataStorageSizeMin + 1U;
    static const uint8_t kDataStorageSizeMax = Device::kDataStorageSizeMax + 1U;

    DeviceOnOff();
    ~DeviceOnOff();

    void Reset();
    bool IsOn() const;
    void SetOnOff(bool aOn);
    void SetOnOff();

    bool GetDataStorage(uint8_t * ptDataToSave, uint8_t * ptDataToSaveLength);
    bool SetDataStorage(uint8_t * ptDataRead, uint8_t dataReadLength);
    using DeviceCallback_fn = std::function<void(DeviceOnOff *, DeviceOnOff::Changed_t)>;
    void SetChangeCallback(DeviceCallback_fn aChanged_CB);

private:
    void HandleDeviceChange(Device * device, Device::Changed_t changeMask);
    bool IsLocal(Led_TypeDef * ledId);

protected:
    bool mOn;
    DeviceCallback_fn mChanged_CB;
};
