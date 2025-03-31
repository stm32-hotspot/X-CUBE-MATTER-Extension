/*
 *
 *    Copyright (c) 2020 Project CHIP Authors
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

/* this file behaves like a config.h, comes first */
#include <platform/internal/CHIPDeviceLayerInternal.h>

#include <platform/ConnectivityManager.h>

//#include <platform/internal/GenericConnectivityManagerImpl_UDP.ipp>
#include <platform/internal/GenericConnectivityManagerImpl_UDP.cpp>

#if INET_CONFIG_ENABLE_TCP_ENDPOINT
//#include <platform/internal/GenericConnectivityManagerImpl_TCP.ipp>
#include <platform/internal/GenericConnectivityManagerImpl_TCP.cpp>
#endif

#if CHIP_DEVICE_CONFIG_ENABLE_CHIPOBLE
//#include <platform/internal/GenericConnectivityManagerImpl_BLE.ipp>
#include <platform/internal/GenericConnectivityManagerImpl_BLE.cpp>
#endif

#if CHIP_DEVICE_CONFIG_ENABLE_THREAD
//#include <platform/internal/GenericConnectivityManagerImpl_Thread.ipp>
#include <platform/internal/GenericConnectivityManagerImpl_Thread.cpp>
#endif

#if CHIP_DEVICE_CONFIG_ENABLE_WIFI
//#include <platform/internal/GenericConnectivityManagerImpl_WiFi.ipp>
#include <platform/internal/GenericConnectivityManagerImpl_WiFi.cpp>
#endif

#include <lib/support/logging/CHIPLogging.h>

using namespace ::chip;
using namespace ::chip::TLV;

namespace chip {
namespace DeviceLayer {

ConnectivityManagerImpl ConnectivityManagerImpl::sInstance;
// ==================== ConnectivityManager Platform Internal Methods ====================

CHIP_ERROR ConnectivityManagerImpl::_Init()
{
    CHIP_ERROR err = CHIP_NO_ERROR;

    ChipLogDetail(DeviceLayer, "ST => ConnectivityManagerImpl Init");
    // Initialize the generic base classes that require it.
#if CHIP_DEVICE_CONFIG_ENABLE_THREAD
    GenericConnectivityManagerImpl_Thread<ConnectivityManagerImpl>::_Init();
#endif

#if CHIP_DEVICE_CONFIG_ENABLE_WIFI

#endif

    return err;
}

void ConnectivityManagerImpl::_OnPlatformEvent(const ChipDeviceEvent * event)
{
    ChipLogDetail(DeviceLayer, "ST => ConnectivityManagerImpl OnPlatformEvent");
    // Forward the event to the generic base classes as needed.
#if CHIP_DEVICE_CONFIG_ENABLE_THREAD
    GenericConnectivityManagerImpl_Thread<ConnectivityManagerImpl>::_OnPlatformEvent(event);
#endif

#if CHIP_DEVICE_CONFIG_ENABLE_WIFI

#endif
}

} // namespace DeviceLayer
} // namespace chip
