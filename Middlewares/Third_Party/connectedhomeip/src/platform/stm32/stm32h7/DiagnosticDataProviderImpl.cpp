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
 *          Provides an implementation of the DiagnosticDataProvider object
 *          for STM32 platforms.
 */

#include <platform/internal/CHIPDeviceLayerInternal.h>

#include <platform/DiagnosticDataProvider.h>
#include <lib/support/CHIPMemString.h>
#include "DiagnosticDataProviderImpl.h"

#include "FreeRTOS.h"

using namespace chip::app::Clusters::GeneralDiagnostics;

namespace {

InterfaceTypeEnum GetInterfaceType(const char * if_desc)
{
    InterfaceTypeEnum ret;

    if (strncmp(if_desc, "AR", strnlen(if_desc, 2)) == 0) {
        ret = InterfaceTypeEnum::kEthernet;
    }
    // add here analysis for thread or wifi
    else {
        ret = InterfaceTypeEnum::kUnspecified;
        ChipLogError(DeviceLayer, "Failed to get interface type");
    }

    return ret;
}

} // namespace

namespace chip {
namespace DeviceLayer {

DiagnosticDataProviderImpl & DiagnosticDataProviderImpl::GetDefaultInstance()
{
    static DiagnosticDataProviderImpl sInstance;
    return sInstance;
}

CHIP_ERROR DiagnosticDataProviderImpl::GetCurrentHeapFree(uint64_t & currentHeapFree)
{
    size_t freeHeapSize = xPortGetFreeHeapSize();
    currentHeapFree     = static_cast<uint64_t>(freeHeapSize);
    return CHIP_NO_ERROR;
}

CHIP_ERROR DiagnosticDataProviderImpl::GetCurrentHeapUsed(uint64_t & currentHeapUsed)
{
    // Calculate the Heap used = Total heap - Free heap
    int64_t heapUsed = (configTOTAL_HEAP_SIZE - xPortGetFreeHeapSize());

    // Something went wrong, this should not happen
    VerifyOrReturnError(heapUsed >= 0, CHIP_ERROR_INVALID_INTEGER_VALUE);
    currentHeapUsed = static_cast<uint64_t>(heapUsed);
    return CHIP_NO_ERROR;
}

CHIP_ERROR DiagnosticDataProviderImpl::GetCurrentHeapHighWatermark(uint64_t & currentHeapHighWatermark)
{
    // Calculate the  highest heap used
    int64_t highestHeapUsageRecorded = (configTOTAL_HEAP_SIZE - xPortGetMinimumEverFreeHeapSize());

    // Something went wrong, this should not happen
    VerifyOrReturnError(highestHeapUsageRecorded >= 0, CHIP_ERROR_INVALID_INTEGER_VALUE);
    currentHeapHighWatermark = static_cast<uint64_t>(highestHeapUsageRecorded);
    return CHIP_NO_ERROR;
}

CHIP_ERROR DiagnosticDataProviderImpl::ResetWatermarks()
{
    // If implemented, the server SHALL set the value of the CurrentHeapHighWatermark attribute to the
    // value of the CurrentHeapUsed.

    // overide with non-op, no way to write CurrentHeapHighWatermark provided by portable.h
    return CHIP_NO_ERROR;
}

CHIP_ERROR DiagnosticDataProviderImpl::GetRebootCount(uint16_t & rebootCount)
{
    uint32_t count = 0U;
    CHIP_ERROR err = ConfigurationMgr().GetRebootCount(count);

    if (err == CHIP_NO_ERROR)
    {
        VerifyOrReturnError(count <= UINT16_MAX, CHIP_ERROR_INVALID_INTEGER_VALUE);
        rebootCount = static_cast<uint16_t>(count);
    }

    return err;
}

CHIP_ERROR DiagnosticDataProviderImpl::GetUpTime(uint64_t & upTime)
{
    CHIP_ERROR err = CHIP_ERROR_INVALID_TIME;
    System::Clock::Timestamp currentTime = System::SystemClock().GetMonotonicTimestamp();
    System::Clock::Timestamp startTime   = PlatformMgrImpl().GetStartTime();

    if (currentTime >= startTime)
    {
        upTime = std::chrono::duration_cast<System::Clock::Seconds64>(currentTime - startTime).count();
        err = CHIP_NO_ERROR;
    }

    return err;
}

CHIP_ERROR DiagnosticDataProviderImpl::GetTotalOperationalHours(uint32_t & totalOperationalHours)
{
    return ConfigurationMgr().GetTotalOperationalHours(totalOperationalHours);
}

CHIP_ERROR DiagnosticDataProviderImpl::GetBootReason(BootReasonType & bootReason)
{
    uint32_t reason = 0U;
    CHIP_ERROR err = ConfigurationMgr().GetBootReason(reason);

    if (err == CHIP_NO_ERROR)
    {
        VerifyOrReturnError(reason <= UINT8_MAX, CHIP_ERROR_INVALID_INTEGER_VALUE);
        bootReason = static_cast<BootReasonType>(reason);
    }

    return err;
}

CHIP_ERROR DiagnosticDataProviderImpl::GetNetworkInterfaces(NetworkInterface ** netifpp)
{
    NetworkInterface * head = NULL;

    for (Inet::InterfaceIterator interfaceIterator; interfaceIterator.HasCurrent(); interfaceIterator.Next())
    {
        uint8_t addressSize;
        uint8_t ipv6AddressesCount = 0;
        Inet::InterfaceType interfaceType = Inet::InterfaceType::Unknown;
        // Assuming IPv6-only support
        Inet::InterfaceAddressIterator interfaceAddressIterator;
        CHIP_ERROR err;
        NetworkInterface * ifp = new NetworkInterface();

        interfaceIterator.GetInterfaceName(ifp->Name, Inet::InterfaceId::kMaxIfNameLength);
        ifp->name = CharSpan::fromCharString(ifp->Name);
        ifp->isOperational = true;
        err = interfaceIterator.GetInterfaceType(interfaceType);
        if (err == CHIP_NO_ERROR || err == CHIP_ERROR_NOT_IMPLEMENTED)
        {
            switch (interfaceType)
            {
            case Inet::InterfaceType::WiFi:
                ifp->type = InterfaceTypeEnum::kWiFi;
                break;
            case Inet::InterfaceType::Ethernet:
                ifp->type = InterfaceTypeEnum::kEthernet;
                break;
            case Inet::InterfaceType::Cellular:
                ifp->type = InterfaceTypeEnum::kCellular;
                break;
            case Inet::InterfaceType::Thread:
                ifp->type = InterfaceTypeEnum::kThread;
                break;
            case Inet::InterfaceType::Unknown:
                ifp->type = GetInterfaceType(ifp->Name);
                break;
            default:
                ifp->type = InterfaceTypeEnum::kUnspecified;
                break;
            }
        }
        else
        {
            ifp->type = InterfaceTypeEnum::kUnspecified;
            ChipLogError(DeviceLayer, "Failed to get interface type");
        }

        ifp->offPremiseServicesReachableIPv4.SetNull();
        ifp->offPremiseServicesReachableIPv6.SetNull();

        if (interfaceIterator.GetHardwareAddress(ifp->MacAddress, addressSize, sizeof(ifp->MacAddress)) == CHIP_NO_ERROR)
        {
            ifp->hardwareAddress = ByteSpan(ifp->MacAddress, addressSize);
        }
        else
        {
            ChipLogError(DeviceLayer, "Failed to get network hardware address");
        }

        while (interfaceAddressIterator.HasCurrent() && ipv6AddressesCount < kMaxIPv6AddrCount)
        {
            if (interfaceAddressIterator.GetInterfaceId() == interfaceIterator.GetInterfaceId())
            {
                chip::Inet::IPAddress ipv6Address;
                if (interfaceAddressIterator.GetAddress(ipv6Address) == CHIP_NO_ERROR)
                {
                    memcpy(ifp->Ipv6AddressesBuffer[ipv6AddressesCount], ipv6Address.Addr, kMaxIPv6AddrSize);
                    ifp->Ipv6AddressSpans[ipv6AddressesCount] = ByteSpan(ifp->Ipv6AddressesBuffer[ipv6AddressesCount]);
                    ipv6AddressesCount++;
                }
            }
            interfaceAddressIterator.Next();
        }

        ifp->IPv6Addresses = chip::app::DataModel::List<chip::ByteSpan>(ifp->Ipv6AddressSpans, ipv6AddressesCount);
        ifp->Next = head;
        head = ifp;
    }
    *netifpp = head;

    return CHIP_NO_ERROR;
}

void DiagnosticDataProviderImpl::ReleaseNetworkInterfaces(NetworkInterface * netifp)
{
    while (netifp)
    {
        NetworkInterface * del = netifp;
        netifp = netifp->Next;
        delete del;
    }
}

DiagnosticDataProvider & GetDiagnosticDataProviderImpl()
{
    return DiagnosticDataProviderImpl::GetDefaultInstance();
}

} // namespace DeviceLayer
} // namespace chip
