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

/**
 *    @file
 *          Provides an implementation of the NetworkCommissioningDriver object
 *          for STM32 platforms with Ethernet.
 */

#include "NetworkCommissioningDriver.h"
#include <lib/support/SafePointerCast.h>
#include <inet/InetInterface.h>

using namespace chip::DeviceLayer::Internal;

/**
 * Concrete implementation of the EthernetDriver object for STM32 platforms.
 */

namespace chip {
namespace DeviceLayer {
namespace NetworkCommissioning {

STM32EthernetDriver::EthernetNetworkIterator::EthernetNetworkIterator(STM32EthernetDriver * aDriver) : mDriver(aDriver)
{
    Inet::InterfaceIterator interfaceIterator;
    memset(interfaceName, (uint8_t)'\0', sizeof(interfaceName));
    if (interfaceIterator.GetInterfaceName((char *)interfaceName, sizeof(interfaceName)) == CHIP_NO_ERROR)
    {
        interfaceNameLen = strlen((const char *)interfaceName);
    }
}

bool STM32EthernetDriver::EthernetNetworkIterator::Next(Network & item)
{
    bool ret = false;

    if (exhausted == false)
    {
        exhausted = true;
        memcpy(item.networkID, interfaceName, interfaceNameLen);
        item.networkIDLen = interfaceNameLen;
        item.connected = true;
        ret = true;
    }

    return ret;
}

} // namespace NetworkCommissioning
} // namespace DeviceLayer
} // namespace chip
