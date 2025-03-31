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
 *          Provides an implementation of the NetworkCommissioning object.
 */

#pragma once

#include <platform/NetworkCommissioning.h>

namespace chip {
namespace DeviceLayer {
namespace NetworkCommissioning {

/**
 * Concrete implementation of the EthernetDriver object for STM32 platforms.
 */

class STM32EthernetDriver final : public EthernetDriver
{
public:
    class EthernetNetworkIterator final : public NetworkIterator
    {
    public:
        EthernetNetworkIterator(STM32EthernetDriver * aDriver);
        size_t Count() override { return interfaceNameLen > 0 ? 1 : 0; }
        bool Next(Network & item) override;
        void Release() override { delete this; }
        ~EthernetNetworkIterator() override = default;

    private:
        STM32EthernetDriver * mDriver;
        uint8_t interfaceName[kMaxNetworkIDLen];
        uint8_t interfaceNameLen = 0U;
        bool exhausted = false;
    };

    uint8_t GetMaxNetworks() override { return 1; };
    NetworkIterator * GetNetworks() override { return new EthernetNetworkIterator(this); };
};

} // namespace NetworkCommissioning
} // namespace DeviceLayer
} // namespace chip
