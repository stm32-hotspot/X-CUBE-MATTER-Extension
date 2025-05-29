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

#include "BridgedClustersManager.h"

/* ------------------------ Includes C++ MATTER ------------------------ */
#include "CHIPDevicePlatformConfig.h"

#include "AppLog.h"
#include "AppEntry.h"
#include "Device.h"
#include "stm32_nvm_implement.h"

#include <app-common/zap-generated/ids/Attributes.h>
#include <app-common/zap-generated/ids/Clusters.h>
#include <app-common/zap-generated/callback.h>
#include <app/ConcreteAttributePath.h>
#include <app/clusters/identify-server/identify-server.h>
#include <app/reporting/reporting.h>
#include <app/util/attribute-storage.h>
#include <app/util/ember-strings.h>
#include <app/util/endpoint-config-api.h>

#include <lib/core/CHIPError.h>
#include <lib/core/ErrorStr.h>
#include <lib/support/CHIPMem.h>
#include <lib/support/CHIPMemString.h>
#include <lib/support/ZclString.h>

#include <app/InteractionModelEngine.h>
#include <app/server/Server.h>

extern const char TAG[] = "bridge-app";

using namespace ::chip;
using namespace ::chip::Platform;
using namespace ::chip::DeviceLayer;
using namespace ::chip::app::Clusters;

namespace {

#define BRIDGE_STORAGE_SIZE      (((CHIP_DEVICE_CONFIG_DYNAMIC_ENDPOINT_COUNT + 7) / 8) + sizeof(uint16_t))
#define DEVICES_STORAGE_SIZE_MAX (Device::kGenericDataStorageSizeMax)
#define DEVICE_STORAGE_SIZE_MAX  (DeviceOnOff::kDataStorageSizeMax) // max of data size for any device

static const uint8_t kNodeLabelSize = Device::kNameSize;
static const uint8_t kUniqueIdSize  = Device::kUniqueIdSize;
// Current ZCL implementation of Struct uses a max-size array of 254 bytes
static const int kDescriptorAttributeArraySize = 254;

static EndpointId gCurrentEndpointId;
static EndpointId gFirstDynamicEndpointId;
static Device * gDevices[CHIP_DEVICE_CONFIG_DYNAMIC_ENDPOINT_COUNT]; // number of dynamic endpoints count
static uint8_t gBridgeDescription[BRIDGE_STORAGE_SIZE];
static uint8_t gDeviceDescription[DEVICE_STORAGE_SIZE_MAX];
static uint8_t gDevicesDescription[DEVICES_STORAGE_SIZE_MAX];

// Size of used KeyName to save parameters
#define BRIDGE_KEYNAME_SZ        (8)

#define DEVICES_LO_ON_OFF_LIGHT_MAX_NB  (CHIP_DEVICE_CONFIG_DYNAMIC_ENDPOINT_COUNT) // max CHIP_DEVICE_CONFIG_DYNAMIC_ENDPOINT_COUNT
#define DEVICES_LO_ON_OFF_LIGHT_DEMO_NB (DEVICES_LO_ON_OFF_LIGHT_MAX_NB - 2)        // max DEVICES_LO_ON_OFF_LIGHT_MAX_NB

// Device types for dynamic endpoints: Need a generated file from ZAP to define these!
// (taken from matter-devices.xml)
#define DEVICE_TYPE_ROOT_NODE       0x0016
#define DEVICE_TYPE_BRIDGE          0x000e
#define DEVICE_TYPE_BRIDGED_NODE    0x0013
#define DEVICE_TYPE_GENERIC         0x0000

// Device Version for dynamic endpoints:
#define DEVICE_VERSION_DEFAULT (1u)
// Cluster revision and Feature map definition:
#define ZCL_DESCRIPTOR_CLUSTER_REVISION (2u)
#define ZCL_BRIDGED_DEVICE_BASIC_INFORMATION_CLUSTER_REVISION (4u)
#define ZCL_BRIDGED_DEVICE_BASIC_INFORMATION_FEATURE_MAP (0u)
#define ZCL_ON_OFF_CLUSTER_REVISION (6u)
#define ZCL_ON_OFF_FEATURE_MAP (0u)

/* BRIDGED DEVICE ENDPOINT: contains the following clusters:
   - On/Off
   - Descriptor
   - Bridged Device Basic Information
 */

// Declare On/Off cluster attributes
DECLARE_DYNAMIC_ATTRIBUTE_LIST_BEGIN(onOffAttrs) \
    DECLARE_DYNAMIC_ATTRIBUTE(OnOff::Attributes::OnOff::Id, BOOLEAN, 1, 0),       /* on/off */
    DECLARE_DYNAMIC_ATTRIBUTE(OnOff::Attributes::FeatureMap::Id, BITMAP32, 4, 0), /* FeatureMap */
DECLARE_DYNAMIC_ATTRIBUTE_LIST_END();

// Declare Descriptor cluster attributes
DECLARE_DYNAMIC_ATTRIBUTE_LIST_BEGIN(descriptorAttrs)
    DECLARE_DYNAMIC_ATTRIBUTE(Descriptor::Attributes::DeviceTypeList::Id, ARRAY, kDescriptorAttributeArraySize, 0), /* device list */
    DECLARE_DYNAMIC_ATTRIBUTE(Descriptor::Attributes::ServerList::Id, ARRAY, kDescriptorAttributeArraySize, 0),     /* server list */
    DECLARE_DYNAMIC_ATTRIBUTE(Descriptor::Attributes::ClientList::Id, ARRAY, kDescriptorAttributeArraySize, 0),     /* client list */
    DECLARE_DYNAMIC_ATTRIBUTE(Descriptor::Attributes::PartsList::Id, ARRAY, kDescriptorAttributeArraySize, 0),      /* parts list */
    DECLARE_DYNAMIC_ATTRIBUTE(Descriptor::Attributes::TagList::Id, ARRAY, kDescriptorAttributeArraySize, 0),        /* tag list */
DECLARE_DYNAMIC_ATTRIBUTE_LIST_END();

// Declare Bridged Device Basic information cluster attributes
DECLARE_DYNAMIC_ATTRIBUTE_LIST_BEGIN(bridgedDeviceBasicAttrs)
    DECLARE_DYNAMIC_ATTRIBUTE(BridgedDeviceBasicInformation::Attributes::NodeLabel::Id, CHAR_STRING, kNodeLabelSize, 1), /* NodeLabel */
    DECLARE_DYNAMIC_ATTRIBUTE(BridgedDeviceBasicInformation::Attributes::Reachable::Id, BOOLEAN, 1, 0),                  /* Reachable */
    DECLARE_DYNAMIC_ATTRIBUTE(BridgedDeviceBasicInformation::Attributes::UniqueID::Id, CHAR_STRING, kUniqueIdSize, 0),   /* UniqueID */
    DECLARE_DYNAMIC_ATTRIBUTE(BridgedDeviceBasicInformation::Attributes::FeatureMap::Id, BITMAP32, 4, 0),                /* FeatureMap */
DECLARE_DYNAMIC_ATTRIBUTE_LIST_END();

// Declare Cluster List for Bridged Light endpoint
// It's not clear whether it would be better to get the command lists from
// the ZAP config on our last fixed endpoint instead.
constexpr CommandId onOffIncomingCommands[] = {
    app::Clusters::OnOff::Commands::Off::Id,
    app::Clusters::OnOff::Commands::On::Id,
    app::Clusters::OnOff::Commands::Toggle::Id,
    kInvalidCommandId,
};

DECLARE_DYNAMIC_CLUSTER_LIST_BEGIN(bridgedLightClusters)
    DECLARE_DYNAMIC_CLUSTER(OnOff::Id, onOffAttrs, ZAP_CLUSTER_MASK(SERVER), onOffIncomingCommands, nullptr),
    DECLARE_DYNAMIC_CLUSTER(Descriptor::Id, descriptorAttrs, ZAP_CLUSTER_MASK(SERVER), nullptr, nullptr),
    DECLARE_DYNAMIC_CLUSTER(BridgedDeviceBasicInformation::Id, bridgedDeviceBasicAttrs, ZAP_CLUSTER_MASK(SERVER),
                            nullptr, nullptr)
DECLARE_DYNAMIC_CLUSTER_LIST_END;

// Declare Bridged Light endpoint
DECLARE_DYNAMIC_ENDPOINT(bridgedLightEndpoint, bridgedLightClusters);

DataVersion gLightsDataVersions[DEVICES_LO_ON_OFF_LIGHT_MAX_NB][ArraySize(bridgedLightClusters)];

// Bridged devices On/Off endpoints
static DeviceOnOff gLights[DEVICES_LO_ON_OFF_LIGHT_MAX_NB];

const EmberAfDeviceType gRootDeviceTypes[]          = { { DEVICE_TYPE_ROOT_NODE, DEVICE_VERSION_DEFAULT } };
const EmberAfDeviceType gAggregatorNodeDeviceTypes[] = { { DEVICE_TYPE_BRIDGE, DEVICE_VERSION_DEFAULT } };

const EmberAfDeviceType gBridgedOnOffDeviceTypes[] = { { DEVICE_TYPE_LO_ON_OFF_LIGHT, DEVICE_VERSION_DEFAULT },
                                                       { DEVICE_TYPE_BRIDGED_NODE, DEVICE_VERSION_DEFAULT } };

} // namespace

/* MATTER BRIDGE APPLICATION -------------------------------------------------------------*/

/* Private user code ---------------------------------------------------------*/

/* ------------------------ Matter function prototypes C++ ------------------------ */
void NVMFormatKeyName(uint16_t deviceType, uint16_t deviceId, uint8_t *bufferKeyName);
CHIP_ERROR NVMRead(uint16_t deviceType, uint16_t deviceId, void *ptDataValue, uint32_t dataValueSize, size_t *ptReadDataValueSize);
CHIP_ERROR NVMWrite(uint16_t deviceType, uint16_t deviceId, const void *ptDataValue, uint32_t dataValueSize);
CHIP_ERROR NVMDelete(uint16_t deviceType, uint16_t deviceId);

bool ReadBridgeDescription(void);
void WriteBridgeDescription(void);
void ReadGenericDevicesDescription(void);
void WriteGenericDevicesDescription(void);
bool ReadDeviceDescription(uint16_t index);
void WriteDeviceDescription(uint16_t index);
void WriteDeviceDescription(void);
void RemoveDeviceDescription(uint16_t index);

int SearchDeviceEndpoint(char * deviceName, uint8_t deviceNameLength);
bool AddDeviceEndpoint(Device * dev, EmberAfEndpointType * ep, const Span<const EmberAfDeviceType> & deviceTypeList,
                       const Span<DataVersion> & dataVersionStorage, uint16_t * index, chip::EndpointId parentEndpointId);
bool AddDeviceEndpoint(Device * dev, EmberAfEndpointType * ep, const Span<const EmberAfDeviceType> & deviceTypeList,
                       const Span<DataVersion> & dataVersionStorage, uint16_t index, chip::EndpointId parentEndpointId);

int RemoveDeviceEndpoint(Device * dev);

Protocols::InteractionModel::Status HandleReadBridgedDeviceBasicAttribute(Device * dev, chip::AttributeId attributeId,
                                                                          uint8_t * buffer, uint16_t maxReadLength);
Protocols::InteractionModel::Status HandleReadOnOffAttribute(DeviceOnOff * dev, chip::AttributeId attributeId, uint8_t * buffer,
                                                             uint16_t maxReadLength);
Protocols::InteractionModel::Status HandleWriteBridgedDeviceBasicAttribute(Device * dev, chip::AttributeId attributeId, uint8_t * buffer);
Protocols::InteractionModel::Status HandleWriteOnOffAttribute(DeviceOnOff * dev, chip::AttributeId attributeId, uint8_t * buffer);

Protocols::InteractionModel::Status emberAfExternalAttributeReadCallback(EndpointId endpoint, ClusterId clusterId,
                                                                         const EmberAfAttributeMetadata * attributeMetadata,
                                                                         uint8_t * buffer, uint16_t maxReadLength);

Protocols::InteractionModel::Status emberAfExternalAttributeWriteCallback(EndpointId endpoint, ClusterId clusterId,
                                                                          const EmberAfAttributeMetadata * attributeMetadata,
                                                                          uint8_t * buffer);

void HandleDeviceStatusChanged(Device * dev, Device::Changed_t itemChangedMask);
void HandleDeviceOnOffStatusChanged(DeviceOnOff * dev, DeviceOnOff::Changed_t itemChangedMask);

/* ------------------------ Matter function prototypes C++ ------------------------ */
bool ReadBridgeDescription(void)
{
    bool ret = false;
    size_t bridgeDescriptionReadSize = 0;

    NVMRead(DEVICE_TYPE_BRIDGE, 0x0000, gBridgeDescription, sizeof(gBridgeDescription), &bridgeDescriptionReadSize);
    if (bridgeDescriptionReadSize == BRIDGE_STORAGE_SIZE)
    {
        bool exit = false;
        uint8_t mask;
        uint8_t i = 0U;
        uint16_t deviceId;

        ReadGenericDevicesDescription();

        gCurrentEndpointId = (((uint16_t)gBridgeDescription[i]) << 8) + (uint16_t)gBridgeDescription[i + 1U];
        i = 2U;
        ret = true;

        while ((i < bridgeDescriptionReadSize) && (exit == false))
        {
            for (uint8_t j = 0U; ((j < 8U) && (exit == false)); j++)
            {
                mask = 0x80 >> j;
                deviceId = j + ((i - 2U) * 8);
                if (deviceId < CHIP_DEVICE_CONFIG_DYNAMIC_ENDPOINT_COUNT)
                {
                    if ((gBridgeDescription[i] & mask) == mask)
                    {
                        if (ReadDeviceDescription(deviceId) == true)
                        {
                            // STM32_LOG("BridgedNode gDevice[%04x] added", deviceId);
                        }
                    }
                    else
                    {
                        // STM32_LOG("BridgedNode gDevice[%04x]:empty", deviceId);
                    }
                }
                else
                {
                    exit = true;
                }
            }
            i++;
        }
    }

    return ret;
}

void WriteBridgeDescription(void)
{
    uint8_t devicesList[((CHIP_DEVICE_CONFIG_DYNAMIC_ENDPOINT_COUNT + 7) / 8)];
    uint8_t mask;

    memset(devicesList, 0, sizeof(devicesList));
    for (uint16_t i = 0U; i < CHIP_DEVICE_CONFIG_DYNAMIC_ENDPOINT_COUNT; i++)
    {
        
        if (gDevices[i] != NULL)
        {
            mask = 0x80 >> (i % 8U);
            devicesList[(i / 8U)] |= mask;
        }
    }
    gBridgeDescription[0] = (gCurrentEndpointId & 0xFF00) >> 8;
    gBridgeDescription[1] = (gCurrentEndpointId & 0x00FF);
    memcpy(&gBridgeDescription[2], devicesList, ((CHIP_DEVICE_CONFIG_DYNAMIC_ENDPOINT_COUNT + 7) / 8));
    if (NVMWrite(DEVICE_TYPE_BRIDGE, 0x0000, gBridgeDescription, sizeof(gBridgeDescription)) != CHIP_NO_ERROR)
    {
        STM32_LOG("Bridge description write NOK !");
    }
    else
    {
        STM32_LOG("Bridge description write OK");
    }
}

void ReadGenericDevicesDescription(void)
{
    size_t devicesDescriptionReadSize = 0;

    memset(gDevicesDescription, '\0', sizeof(gDevicesDescription));
    if (NVMRead(DEVICE_TYPE_GENERIC, 0x0000, gDevicesDescription, sizeof(gDevicesDescription), &devicesDescriptionReadSize) == CHIP_NO_ERROR)
    {
        if (Device::SetGenericDataStorage(gDevicesDescription, devicesDescriptionReadSize) == false)
        {
            STM32_LOG("Bridge: Generic Devices description read NOK ! Reset them !");
            (void)NVMDelete(DEVICE_TYPE_GENERIC, 0x0000);
        }
    }
    else
    {
        //STM32_LOG("Bridge: no GenericDevicesData available");
    }
}

void WriteGenericDevicesDescription(void)
{
    uint8_t devicesDescriptionLength = sizeof(gDevicesDescription);

    if (Device::GetGenericDataStorage(gDevicesDescription, &devicesDescriptionLength) == true)
    {
        if (NVMWrite(DEVICE_TYPE_GENERIC, 0x0000, gDevicesDescription, devicesDescriptionLength) != CHIP_NO_ERROR)
        {
            STM32_LOG("Bridge: Generic Devices description write NOK !");
        }
        else
        {
            STM32_LOG("Bridge: Generic Devices description write OK");
        }
    }
    else
    {
        //No GenericDevicesData to save but maybe some to reset
        (void)NVMDelete(DEVICE_TYPE_GENERIC, 0x0000);
    }
}

bool ReadDeviceDescription(uint16_t index)
{
    bool ret = false;
    size_t deviceDescriptionReadSize = 0;

    NVMRead(DEVICE_TYPE_BRIDGED_NODE, index, gDeviceDescription, sizeof(gDeviceDescription), &deviceDescriptionReadSize);
    if (deviceDescriptionReadSize != 0)
    {
        if (deviceDescriptionReadSize >= (size_t)(Device::kDataStorageSizeMin))
        {
            chip::DeviceTypeId deviceTypeId;

            deviceTypeId = Device::GetDeviceTypeId(gDeviceDescription, deviceDescriptionReadSize);
            if (((uint16_t)(deviceTypeId & 0x0000FFFF) == DEVICE_TYPE_LO_ON_OFF_LIGHT)
                && (deviceDescriptionReadSize >= (size_t)(DeviceOnOff::kDataStorageSizeMin)))
            {
                ret = gLights[index].SetDataStorage(gDeviceDescription, deviceDescriptionReadSize);
                if (ret == true)
                {
                    ret = AddDeviceEndpoint(&gLights[index], &bridgedLightEndpoint, Span<const EmberAfDeviceType>(gBridgedOnOffDeviceTypes),
                                            Span<DataVersion>(gLightsDataVersions[index]), index, gLights[index].GetParentEndpointId());
                    if (ret == true)
                    {
                        gLights[index].SetChangeCallback(&HandleDeviceOnOffStatusChanged);
                    }
                    else
                    {
                        gLights[index].Reset();
                    }
                }
            }
            else
            {
                STM32_LOG("Device:%04x typeId:%04x unknown !", index, (deviceTypeId & 0x0000FFFF));
            }
        }
        else
        {
            STM32_LOG("Device:%04x data length:%d too short !", index, deviceDescriptionReadSize);
        }
    }

    return ret;
}

void WriteDeviceDescription(uint16_t index)
{
    uint8_t deviceDescriptionLength = sizeof(gDeviceDescription);

    if (gDevices[index]->GetDeviceTypeId() == DEVICE_TYPE_LO_ON_OFF_LIGHT)
    {
        static_cast<DeviceOnOff *>(gDevices[index])->GetDataStorage(gDeviceDescription, &deviceDescriptionLength);

        if (NVMWrite(DEVICE_TYPE_BRIDGED_NODE, index, gDeviceDescription, deviceDescriptionLength) != CHIP_NO_ERROR)
        {
            STM32_LOG("Device description write NOK !");
        }
        else
        {
            STM32_LOG("Device description write OK");
        }
    }
    else
    {
        STM32_LOG("Device description write NOK ! Unknown type.");
    }
}

void WriteDeviceDescription(void)
{
    for (uint16_t i = 0U; i < CHIP_DEVICE_CONFIG_DYNAMIC_ENDPOINT_COUNT; i++)
    {
        if (gDevices[i] != nullptr)
        {
            WriteDeviceDescription(i);
        }
    }
}

void RemoveDeviceDescription(uint16_t index)
{
    if (NVMDelete(DEVICE_TYPE_BRIDGED_NODE, index) != CHIP_NO_ERROR)
    {
        STM32_LOG("Device description remove NOK !");
    }
    else
    {
        STM32_LOG("Device description remove OK");
    }
}

int SearchDeviceEndpoint(char * deviceName, uint8_t deviceNameLength)
{
    uint16_t index = 0U;
    char * gDeviceName;

    while (index < CHIP_DEVICE_CONFIG_DYNAMIC_ENDPOINT_COUNT)
    {
        if (gDevices[index] != nullptr)
        {
            gDeviceName = gDevices[index]->GetName();
            if ((strlen(gDeviceName) == deviceNameLength) && (strcmp(deviceName, gDeviceName) == 0))
            {
                return index;
            }
        }
        index++;
    }
    return -1;
}

bool AddDeviceEndpoint(Device * dev, EmberAfEndpointType * ep, const Span<const EmberAfDeviceType> & deviceTypeList,
                       const Span<DataVersion> & dataVersionStorage, uint16_t * index, chip::EndpointId parentEndpointId = chip::kInvalidEndpointId)
{
    uint16_t i = 0U;

    while (i < CHIP_DEVICE_CONFIG_DYNAMIC_ENDPOINT_COUNT)
    {
        if (gDevices[i] == NULL)
        {
            gDevices[i] = dev;
            CHIP_ERROR err;
            while (true)
            {
                err = emberAfSetDynamicEndpoint(i, gCurrentEndpointId, ep, dataVersionStorage, deviceTypeList, parentEndpointId);
                if (err == CHIP_NO_ERROR)
                {
                    *index = i;
                    dev->SetEndpointId(gCurrentEndpointId);
                    dev->SetParentEndpointId(parentEndpointId);
                    ChipLogProgress(DeviceLayer, "Added device to dynamic endpointId:%d (index=%d)", gCurrentEndpointId, *index);
                    return true;
                }
                else if (err != CHIP_ERROR_ENDPOINT_EXISTS)
                {
                    return false;
                }
                // Handle wrap condition
                if (++gCurrentEndpointId < gFirstDynamicEndpointId)
                {
                    gCurrentEndpointId = gFirstDynamicEndpointId;
                }
            }
        }
        i++;
    }
    ChipLogProgress(DeviceLayer, "Failed to add dynamic endpoint: No endpoints available!");
    return false;
}

bool AddDeviceEndpoint(Device * dev, EmberAfEndpointType * ep, const Span<const EmberAfDeviceType> & deviceTypeList,
                      const Span<DataVersion> & dataVersionStorage, uint16_t index, chip::EndpointId parentEndpointId = chip::kInvalidEndpointId)
{
    bool ret = false;

    if (gDevices[index] == NULL)
    {
        CHIP_ERROR err;
        err = emberAfSetDynamicEndpoint(index, dev->GetEndpointId(), ep, dataVersionStorage, deviceTypeList, parentEndpointId);
        if (err == CHIP_NO_ERROR)
        {
            ChipLogProgress(DeviceLayer, "Added device:%s to dynamic endpointId:%d (index=%d)",
                            dev->GetName(), dev->GetEndpointId(), index);
            gDevices[index] = dev;
            ret = true;
        }
        else
        {
            ChipLogProgress(DeviceLayer, "Failed to add device:%s to dynamic endpointId:%d !",
                            dev->GetName(), dev->GetEndpointId());
        }
    }
    else
    {
        ChipLogProgress(DeviceLayer, "Failed to add device:%s to dynamic endpoint - index:%d already used !",
                        dev->GetName(), index);
    }

    return ret;
}

int RemoveDeviceEndpoint(Device * dev)
{
    uint16_t index = 0U;
    while (index < CHIP_DEVICE_CONFIG_DYNAMIC_ENDPOINT_COUNT)
    {
        if (gDevices[index] == dev)
        {
            char nameDevice[Device::kNameSize];
            [[maybe_unused]] EndpointId ep;
            // Silence complaints about unused ep when progress logging disabled.
            CopyString(nameDevice, Device::kNameSize, dev->GetName());
            if (dev->GetDeviceTypeId() == DEVICE_TYPE_LO_ON_OFF_LIGHT)
            {
                (static_cast<DeviceOnOff *>(dev))->Reset();
            }
            else
            {
                dev->Reset();
            }
            ep = emberAfClearDynamicEndpoint(index);
            ChipLogProgress(DeviceLayer, "Removed device %s from dynamic endpointId:%d (index=%d)", nameDevice, ep, index);
            gDevices[index] = nullptr;
            return index;
        }
        index++;
    }
    return -1;
}

Protocols::InteractionModel::Status HandleReadBridgedDeviceBasicAttribute(Device * dev, chip::AttributeId attributeId,
                                                                          uint8_t * buffer, uint16_t maxReadLength)
{
    using namespace BridgedDeviceBasicInformation::Attributes;
    Protocols::InteractionModel::Status ret = Protocols::InteractionModel::Status::Success;

    if ((attributeId == Reachable::Id) && (maxReadLength == 1U))
    {
        *buffer = dev->IsReachable() ? 1 : 0;
    }
    else if ((attributeId == NodeLabel::Id) && (maxReadLength >= kNodeLabelSize))
    {
        MutableByteSpan zclNameSpan(buffer, maxReadLength);
        MakeZclCharString(zclNameSpan, dev->GetName());
    }
    else if ((attributeId == UniqueID::Id) && (maxReadLength >= kUniqueIdSize))
    {
        MutableByteSpan zclNameSpan(buffer, maxReadLength);
        MakeZclCharString(zclNameSpan, dev->GetUniqueId());
    }
    else if ((attributeId == ClusterRevision::Id) && (maxReadLength == 2U))
    {
        uint16_t rev = ZCL_BRIDGED_DEVICE_BASIC_INFORMATION_CLUSTER_REVISION;
        memcpy(buffer, &rev, sizeof(rev));
    }
    else if ((attributeId == FeatureMap::Id) && (maxReadLength == 4U))
    {
        uint32_t featureMap = ZCL_BRIDGED_DEVICE_BASIC_INFORMATION_FEATURE_MAP;
        memcpy(buffer, &featureMap, sizeof(featureMap));
    }
    else
    {
        ret = Protocols::InteractionModel::Status::Failure;
    }

    return ret;
}

Protocols::InteractionModel::Status HandleReadOnOffAttribute(DeviceOnOff * dev, chip::AttributeId attributeId, uint8_t * buffer,
                                                             uint16_t maxReadLength)
{
    Protocols::InteractionModel::Status ret = Protocols::InteractionModel::Status::Success;

    if ((attributeId == OnOff::Attributes::OnOff::Id) && (maxReadLength == 1U))
    {
        *buffer = dev->IsOn() ? 1 : 0;
    }
    else if ((attributeId == OnOff::Attributes::ClusterRevision::Id) && (maxReadLength == 2U))
    {
        uint16_t rev = ZCL_ON_OFF_CLUSTER_REVISION;
        memcpy(buffer, &rev, sizeof(rev));
    }
    else if ((attributeId == OnOff::Attributes::FeatureMap::Id) && (maxReadLength == 4U))
    {
        uint32_t featureMap = ZCL_ON_OFF_FEATURE_MAP;
        memcpy(buffer, &featureMap, sizeof(featureMap));
    }
    else
    {
        ret = Protocols::InteractionModel::Status::Failure;
    }

    return ret;
}

Protocols::InteractionModel::Status HandleWriteBridgedDeviceBasicAttribute(Device * dev, chip::AttributeId attributeId, uint8_t * buffer)
{
    Protocols::InteractionModel::Status ret = Protocols::InteractionModel::Status::Failure;

    if ((buffer != nullptr) && (dev->IsReachable()))
    {
        if (attributeId == BridgedDeviceBasicInformation::Attributes::NodeLabel::Id)
        {
            uint8_t bufferLength = emberAfStringLength(buffer);
            if (bufferLength != 0U)
            {
                dev->SetName((char *)&buffer[1], bufferLength);
                ret = Protocols::InteractionModel::Status::Success;
            }
        }
    }

    return ret;
}

Protocols::InteractionModel::Status HandleWriteOnOffAttribute(DeviceOnOff * dev, chip::AttributeId attributeId, uint8_t * buffer)
{
    Protocols::InteractionModel::Status ret = Protocols::InteractionModel::Status::Failure;

    if ((buffer != nullptr) && (dev->IsReachable()))
    {
        if (attributeId == OnOff::Attributes::OnOff::Id)
        {
            dev->SetOnOff((*buffer == 1) ? true : false);
            ret = Protocols::InteractionModel::Status::Success;
        }
    }

    return ret;
}

Protocols::InteractionModel::Status emberAfExternalAttributeReadCallback(EndpointId endpoint, ClusterId clusterId,
                                                                         const EmberAfAttributeMetadata * attributeMetadata,
                                                                         uint8_t * buffer, uint16_t maxReadLength)
{
    uint16_t endpointId = emberAfGetDynamicIndexFromEndpoint(endpoint);
    Protocols::InteractionModel::Status ret = Protocols::InteractionModel::Status::Failure;

    if ((endpointId < CHIP_DEVICE_CONFIG_DYNAMIC_ENDPOINT_COUNT) && (gDevices[endpointId] != NULL))
    {
        Device * dev = gDevices[endpointId];

        if (clusterId == BridgedDeviceBasicInformation::Id)
        {
            ret = HandleReadBridgedDeviceBasicAttribute(dev, attributeMetadata->attributeId, buffer, maxReadLength);
        }
        else if (clusterId == OnOff::Id)
        {
            ret = HandleReadOnOffAttribute(static_cast<DeviceOnOff *>(dev), attributeMetadata->attributeId, buffer, maxReadLength);
        }
        else
        {
        }
    }

    return ret;
}

Protocols::InteractionModel::Status emberAfExternalAttributeWriteCallback(EndpointId endpoint, ClusterId clusterId,
                                                                          const EmberAfAttributeMetadata * attributeMetadata,
                                                                          uint8_t * buffer)
{
    uint16_t endpointId = emberAfGetDynamicIndexFromEndpoint(endpoint);
    Protocols::InteractionModel::Status ret = Protocols::InteractionModel::Status::Failure;

    if ((endpointId < CHIP_DEVICE_CONFIG_DYNAMIC_ENDPOINT_COUNT) && (gDevices[endpointId] != NULL))
    {
        Device * dev = gDevices[endpointId];

        if (clusterId == BridgedDeviceBasicInformation::Id)
        {
            ret = HandleWriteBridgedDeviceBasicAttribute(dev, attributeMetadata->attributeId, buffer);
        }
        else if (clusterId == OnOff::Id)
        {
            ret= HandleWriteOnOffAttribute(static_cast<DeviceOnOff *>(dev), attributeMetadata->attributeId, buffer);
        }
        else
        {
        }
    }

    return ret;
}

namespace {
void CallReportingCallback(intptr_t closure)
{
    auto path = reinterpret_cast<app::ConcreteAttributePath *>(closure);
    MatterReportingAttributeChangeCallback(*path);
    Platform::Delete(path);
}

void ScheduleReportingCallback(Device * dev, ClusterId cluster, AttributeId attribute)
{
    auto * path = Platform::New<app::ConcreteAttributePath>(dev->GetEndpointId(), cluster, attribute);
    DeviceLayer::PlatformMgr().ScheduleWork(CallReportingCallback, reinterpret_cast<intptr_t>(path));
}
} // anonymous namespace

void HandleDeviceStatusChanged(Device * dev, Device::Changed_t itemChangedMask)
{
    if (itemChangedMask & (Device::kChanged_Name))
    {
        ScheduleReportingCallback(dev, BridgedDeviceBasicInformation::Id, BridgedDeviceBasicInformation::Attributes::NodeLabel::Id);
    }
    if (itemChangedMask & (Device::kChanged_Reachable))
    {
        ScheduleReportingCallback(dev, BridgedDeviceBasicInformation::Id, BridgedDeviceBasicInformation::Attributes::Reachable::Id);
    }
    if (itemChangedMask & ((Device::kChanged_Location) | (DeviceOnOff::kChanged_ChannelCom)))
    {
        /* Nothing to report */
    }
}

void HandleDeviceOnOffStatusChanged(DeviceOnOff * dev, DeviceOnOff::Changed_t itemChangedMask)
{
    uint16_t endpointId = emberAfGetDynamicIndexFromEndpoint(dev->GetEndpointId());

    if (itemChangedMask & (DeviceOnOff::kChanged_Name | DeviceOnOff::kChanged_Reachable
                           | DeviceOnOff::kChanged_Location | DeviceOnOff::kChanged_ChannelCom))
    {
        WriteDeviceDescription(endpointId);
        HandleDeviceStatusChanged(static_cast<Device *>(dev), (Device::Changed_t)itemChangedMask);
    }
    if (itemChangedMask & (DeviceOnOff::kChanged_OnOff))
    {
        WriteDeviceDescription(endpointId);
        ScheduleReportingCallback(dev, OnOff::Id, OnOff::Attributes::OnOff::Id);
    }
}

bool emberAfActionsClusterInstantActionCallback(app::CommandHandler * commandObj, const app::ConcreteCommandPath & commandPath,
    const Actions::Commands::InstantAction::DecodableType & commandData)
{
    // No actions are implemented, just return status NotFound.
    commandObj->AddStatus(commandPath, Protocols::InteractionModel::Status::NotFound);
    return true;
}

void NVMFormatKeyName(uint16_t deviceType, uint16_t deviceId, char *bufferKeyName)
{
    sprintf(bufferKeyName, "BR%03x%03x", (deviceType & 0x0FFF), (deviceId & 0x0FFF));
}

CHIP_ERROR NVMRead(uint16_t deviceType, uint16_t deviceId, void *ptDataValue, uint32_t dataValueSize,
                   size_t *ptReadDataValueSize)
{
    char bufferKeyName[BRIDGE_KEYNAME_SZ + 1];
    NVM_StatusTypeDef err;

    *ptReadDataValueSize = 0;
    NVMFormatKeyName(deviceType, deviceId, bufferKeyName);
    err = NVM_GetKeyValue(bufferKeyName, ptDataValue, dataValueSize, ptReadDataValueSize, NVM_SECTOR_APPLICATION);

    return ((err == NVM_OK) ? CHIP_NO_ERROR : CHIP_ERROR_PERSISTED_STORAGE_VALUE_NOT_FOUND);
}

CHIP_ERROR NVMWrite(uint16_t deviceType, uint16_t deviceId, const void *ptDataValue, uint32_t dataValueSize)
{
    char bufferKeyName[BRIDGE_KEYNAME_SZ + 1];
    NVM_StatusTypeDef err = NVM_OK;

    NVMFormatKeyName(deviceType, deviceId, bufferKeyName);
    err = NVM_SetKeyValue(bufferKeyName, ptDataValue, dataValueSize, NVM_SECTOR_APPLICATION);

    return ((err == NVM_OK) ? CHIP_NO_ERROR : CHIP_ERROR_WRITE_FAILED);
}

CHIP_ERROR NVMDelete(uint16_t deviceType, uint16_t deviceId)
{
    char bufferKeyName[BRIDGE_KEYNAME_SZ + 1];
    NVM_StatusTypeDef err;

    NVMFormatKeyName(deviceType, deviceId, bufferKeyName);
    err = NVM_DeleteKey(bufferKeyName, NVM_SECTOR_APPLICATION);

    return ((err == NVM_OK) ? CHIP_NO_ERROR : CHIP_ERROR_WRITE_FAILED);
}

bool BridgedClustersManager::Init()
{
    bool ret = true;

    NVM_Init(BRIDGE_KEYNAME_SZ, sizeof(uint8_t), NVM_SECTOR_APPLICATION);

    for (uint16_t i = 0U; i < CHIP_DEVICE_CONFIG_DYNAMIC_ENDPOINT_COUNT; i++)
    {
        gDevices[i] = NULL;
    }

    // Set starting endpoint id where dynamic endpoints will be assigned, which
    // will be the next consecutive endpoint id after the last fixed endpoint.
    gFirstDynamicEndpointId = static_cast<chip::EndpointId>(
            static_cast<int>(emberAfEndpointFromIndex(static_cast<uint16_t>(emberAfFixedEndpointCount() - 1))) + 1);
    gCurrentEndpointId = gFirstDynamicEndpointId;

    // Disable last fixed endpoint, which is used as a placeholder for all of the
    // supported clusters so that ZAP will generated the requisite code.
    emberAfEndpointEnableDisable(emberAfEndpointFromIndex(static_cast<uint16_t>(emberAfFixedEndpointCount() - 1)), false);

    // A bridge has root node device type on EP0 and aggregate node device type (bridge) at EP1
    emberAfSetDeviceTypeList(0, Span<const EmberAfDeviceType>(gRootDeviceTypes));
    emberAfSetDeviceTypeList(1, Span<const EmberAfDeviceType>(gAggregatorNodeDeviceTypes));

    // See for devices list
    if (ReadBridgeDescription() == false)
    {
        // No data - Setup Light devices
        char gLightsDefaultName[9] = "Light   ";
        uint16_t index;
        // Add lights 1..X --> will be mapped to ZCL endpoints 3..(X+3)
        for (uint16_t i = 0U;  i < DEVICES_LO_ON_OFF_LIGHT_DEMO_NB; i++)
        {
            bool ret = AddDeviceEndpoint(&gLights[i], &bridgedLightEndpoint, Span<const EmberAfDeviceType>(gBridgedOnOffDeviceTypes),
                                         Span<DataVersion>(gLightsDataVersions[i]), &index, 1);
            if (ret == true)
            {
                snprintf(&gLightsDefaultName[5], 4, "%d", (i + 1U));
                // Set bridged Light device name
                gLights[i].SetName(gLightsDefaultName, strlen(gLightsDefaultName));
                // Set bridged Light uniqueId
                gLights[i].SetUniqueId();
                // Set bridged Light device location
                gLights[i].SetLocation((Device::Location_t)((i % (uint8_t)(Device::kLocation_Last)) + 1U));
                // Set bridged Light device as Reachable
                gLights[i].SetReachable(true);
                // Set bridged Light device as Off
                gLights[i].SetOnOff(false);
                // Whenever bridged Light device changes its state
                gLights[i].SetChangeCallback(&HandleDeviceOnOffStatusChanged);
            }
        }
        WriteBridgeDescription();
        WriteGenericDevicesDescription();
        WriteDeviceDescription();
    }

    ConsoleSetCallback(ChangeRequested, ChangeSupported);
    ChipLogProgress(DeviceLayer, "BridgeManager initialized");

    return ret;
}

bool BridgedClustersManager::ChangeRequested(uint8_t *ptBuffer, uint8_t BufferLen)
{
    bool treated = true;
    uint8_t bufferLenTmp = BufferLen;

    if (ptBuffer[bufferLenTmp - 1U] == (uint8_t)'\r')
    {
        bufferLenTmp --;
        ptBuffer[BufferLen - 1U] = (uint8_t)'\0';
    }

    if  (bufferLenTmp > 9U)
    {
        if ((bufferLenTmp >= 12U) && strncmp((const char *)ptBuffer, "set custloc", 11) == 0)
        {
            char * newName;
            char emptyName = '\0';

            if (bufferLenTmp >= 14)
            {
                newName = (char *)&ptBuffer[13];
            }
            else
            {
                newName = &emptyName;
            }
            if (ptBuffer[11] == (char)'1')
            {
                if (Device::SetLocationName(Device::kLocationCustom1, newName) == true)
                {
                    WriteGenericDevicesDescription();
                }
            }
            else if (ptBuffer[11] == (char)'2')
            {
                if (Device::SetLocationName(Device::kLocationCustom2, newName) == true)
                {
                    WriteGenericDevicesDescription();
                }
            }
            else
            {
                STM32_LOG("Unknown custom location !");
            }
        }
        else if ((bufferLenTmp >= 11U) && (strncmp((const char *)ptBuffer, "bridge ", 7) == 0))
        {
            if ((bufferLenTmp == 13U) && (strncmp((const char *)ptBuffer, "bridge status", 13) == 0))
            {
                char * gDeviceName;

                STM32_LOG("Lights list:");
                for (uint16_t i = 0U; i < DEVICES_LO_ON_OFF_LIGHT_MAX_NB; i++)
                {
                    gDeviceName = gLights[i].GetName();
                    if (strlen(gDeviceName) != 0)
                    {
                        STM32_LOG("   gLights[%d]:%s - endpointId:%d", i, gDeviceName, gLights[i].GetEndpointId());
                    }
                }
                STM32_LOG("Locations list:");
                for (uint8_t i = 0; i < Device::kLocation_Last; i++)
                {
                    if ((i != Device::kLocationCustom1) && (i != Device::kLocationCustom2))
                    {
                        STM32_LOG("   Location[%d]:%s", i, Device::GetLocationName((Device::Location_t)i));
                    }
                }
                STM32_LOG("   Custom[%d/1]:%s", Device::kLocationCustom1, Device::GetLocationName(Device::kLocationCustom1));
                STM32_LOG("   Custom[%d/2]:%s", Device::kLocationCustom2, Device::GetLocationName(Device::kLocationCustom2));
            }
            else if ((bufferLenTmp == 11U) && (strncmp((const char *)ptBuffer, "bridge save", 11) == 0))
            {
                WriteDeviceDescription();
                WriteGenericDevicesDescription();
                WriteBridgeDescription();
            }
            else if ((bufferLenTmp == 12U) && (strncmp((const char *)ptBuffer, "bridge reset", 12) == 0))
            {
                if (NVM_Reset(NVM_SECTOR_APPLICATION) == NVM_OK)
                {
                    STM32_LOG("Bridge NVM reset OK");
                }
                else
                {
                    STM32_LOG("Bridge NVM reset NOK !");
                }
            }
            else
            {
                treated = false;
            }
        }
        else
        {
            bool exit = false;
            uint8_t order_length = 4U;
            uint8_t index = order_length; // skip order and ' '
            char name[Device::kNameSize]="";
            int deviceNb;

            while ((index < bufferLenTmp) && (exit == false) && ((index - order_length)< Device::kNameSize))
            {
                if ((ptBuffer[index] == (uint8_t)' ') || (ptBuffer[index] == (uint8_t)'\n'))
                {
                    exit = true;
                }
                else
                {
                    name[index - order_length] = ptBuffer[index];
                    index++;
                }
            }
            deviceNb = SearchDeviceEndpoint(name, strlen((const char *)name));

            if (strncmp((const char *)ptBuffer, "new Light", 9) == 0)
            {
                if (deviceNb == -1)
                {
                    // New light device
                    bool empty = false;
                    uint16_t i = 0U;
    
                    while ((i < DEVICES_LO_ON_OFF_LIGHT_MAX_NB) && (empty == false))
                    {
                        if (gLights[i].GetEndpointId() == 0)
                        {
                            empty = true;
                        }
                        else
                        {
                            i++;
                        }
                    }
                    if (empty == true)
                    {
                        STM32_LOG("Empty place to add Device:%s found:%i", (const char *)&ptBuffer[4], i);
                        uint16_t index;
                        bool ret = AddDeviceEndpoint(&gLights[i], &bridgedLightEndpoint, Span<const EmberAfDeviceType>(gBridgedOnOffDeviceTypes),
                                                     Span<DataVersion>(gLightsDataVersions[i]), &index, 1);
                        if (ret == true)
                        {
                            // Initialize bridged Light device
                            gLights[i].SetName((char *)&ptBuffer[4], strlen((char *)&ptBuffer[4]));
                            gLights[i].SetUniqueId();
                            gLights[i].SetChannelCom(Device::kUART1);
                            gLights[i].SetReachable(true);
                            gLights[i].SetOnOff(); // ask to check the hardware
                            gLights[i].SetChangeCallback(&HandleDeviceOnOffStatusChanged);
                            WriteDeviceDescription(index);
                            WriteBridgeDescription();
                        }
                        else
                        {
                            STM32_LOG("Failed to add Device %s in the list", (const char *)&ptBuffer[4]);
                        }
                    }
                    else
                    {
                        STM32_LOG("No more place to add Device: %s", (const char *)&ptBuffer[4]);
                    }
                }
                else
                {
                    STM32_LOG("Device: %s already available", (const char *)&ptBuffer[4]);
                }
            }
            else if (strncmp((const char *)ptBuffer, "del Light", 9) == 0)
            {
                if (deviceNb != -1)
                {
                    // Delete a Light
                    int endpointId = RemoveDeviceEndpoint(&gLights[(uint16_t)deviceNb]);
                    if (endpointId >= 0)
                    {
                        RemoveDeviceDescription(endpointId);
                        WriteBridgeDescription();
                    }
                }
                else
                {
                    STM32_LOG("Device: %s not found", (const char *)&ptBuffer[4]);
                }
            }
            else if (strncmp((const char *)ptBuffer, "nam Light", 9) == 0)
            {
                if (deviceNb != -1)
                {
                    // Change a Light Name
                    char deviceNewName[Device::kNameSize];
                    CopyString(deviceNewName, Device::kNameSize, gLights[(uint16_t)deviceNb].GetName());
                    size_t deviceNameLength = strlen(deviceNewName);
                    // Add 'b' or Remove 'b' at the end of the name
                    if (deviceNewName[deviceNameLength - 1U] == 'b')
                    {
                        deviceNewName[deviceNameLength - 1U] = '\0';
                    }
                    else
                    {
                        if (deviceNameLength == (Device::kNameSize - 1))
                        {
                            deviceNewName[deviceNameLength - 1U] = 'b';
                            deviceNewName[deviceNameLength] = '\0';
                        }
                        else
                        {
                            deviceNewName[deviceNameLength] = 'b';
                            deviceNewName[deviceNameLength + 1U] = '\0';
                        }
                    }
                    gLights[(uint16_t)deviceNb].SetName(deviceNewName, strlen(deviceNewName));
                }
                else
                {
                    STM32_LOG("Device: %s not found", (const char *)&ptBuffer[4]);
                }
            }
            else if (strncmp((const char *)ptBuffer, "tog Light", 9) == 0)
            {
                if (deviceNb != -1)
                {
                    // Change a Light On/Off state
                    gLights[(uint16_t)deviceNb].SetOnOff(!(gLights[(uint16_t)deviceNb].IsOn()));
                }
                else
                {
                    STM32_LOG("Device: %s not found", (const char *)&ptBuffer[4]);
                }
            }
            else if (strncmp((const char *)ptBuffer, "rea Light", 9) == 0)
            {
                if (deviceNb != -1)
                {
                    // Change a Light Reachable/Unreachable state
                    gLights[(uint16_t)deviceNb].SetReachable(!(gLights[(uint16_t)deviceNb].IsReachable()));
                }
                else
                {
                    STM32_LOG("Device: %s not found", (const char *)&ptBuffer[4]);
                }
            }
            else if (strncmp((const char *)ptBuffer, "inc Light", 9) == 0)
            {
                if (deviceNb != -1)
                {
                    STM32_LOG("Bridge inc Light:%d index:%d bufferLenTmp:%d ptBuffer[i]:%c", deviceNb, index, bufferLenTmp, ptBuffer[index]);
                    // Change a Light parameter
                    if ((index + 4U) <= bufferLenTmp)
                    {
                         if (strncmp((const char *)&ptBuffer[index], " loc", 4) == 0)
                         {
                             Device::Location_t location = gLights[(uint16_t)deviceNb].GetLocation();
                             location = (location == Device::kLocation_Last) ? Device::kLocationUnknown : Device::Location_t(location + 1);
                             gLights[(uint16_t)deviceNb].SetLocation(location);
                         }
                    }
                }
                else
                {
                    STM32_LOG("Device: %s not found", (const char *)&ptBuffer[4]);
                }
            }
            else
            {
                treated = false;
            }
        }
    }
    else
    {
        treated = false;
    }

    return treated;
}

void BridgedClustersManager::ChangeSupported()
{
    STM32_LOG("BridgeManager commands help:");
    STM32_LOG("bridge save   : save bridge infos to NVM");
    STM32_LOG("bridge reset  : reset NVM BRIDGE part");
    STM32_LOG("bridge status : status bridge infos");
    STM32_LOG("new LightX    : add LightX");
    STM32_LOG("del LightX    : remove LightX");
    STM32_LOG("nam LightX    : toggle name from LightX to LightXb");
    STM32_LOG("tog LightX    : toggle state of LightX from On to Off");
    STM32_LOG("rea LightX    : toggle state of LightX from reachable to unreachable");
    STM32_LOG("inc LightX loc: increment location LightX to (loc+1)");
    STM32_LOG("set custlocY name: change name custom location Y");
    STM32_LOG("with X = [1..%d]{b} Y = [1..%d]", DEVICES_LO_ON_OFF_LIGHT_MAX_NB, DEVICE_LOCATION_CUSTOM_MAX_NB);
}
