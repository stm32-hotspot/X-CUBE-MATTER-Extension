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

/**
 *    @file
 *          Provides the implementation of the Device Layer ConfigurationManager object
 *          for the STM32 platforms.
 */
/* this file behaves like a config.h, comes first */
#include <platform/internal/CHIPDeviceLayerInternal.h>

#include <platform/ConfigurationManager.h>
//#include <platform/internal/GenericConfigurationManagerImpl.ipp>
#include <platform/internal/GenericConfigurationManagerImpl.cpp>

#include <lib/core/CHIPVendorIdentifiers.hpp>

//#if CHIP_DEVICE_CONFIG_ENABLE_FACTORY_PROVISIONING
//#include <platform/internal/FactoryProvisioning.cpp>
//#endif // CHIP_DEVICE_CONFIG_ENABLE_FACTORY_PROVISIONING

#include <lib/support/CodeUtils.h>
#include <lib/support/logging/CHIPLogging.h>

#include "STM32Config.h"

#if defined(STM32H753xx)
  #include "stm32h7xx.h"
  #include "stm32h7xx_hal.h"
  #include "stm32h7xx_hal_def.h"
  #include "stm32h7xx_hal_rcc.h"
#endif

extern "C" uint8_t stm32_GetMACAddr(uint8_t *buf_data, size_t buf_size);

namespace chip {
namespace DeviceLayer {

using namespace ::chip::DeviceLayer::Internal;

#define TOTAL_OPERATIONAL_HOURS_PERIOD (60 * 60 * 1000) // every hour in milliseconds
uint32_t ConfigurationManagerImpl::mTotalOperationalHours = 0U;
TimerHandle_t ConfigurationManagerImpl::sTotalOperationalHoursTimer;

void ConfigurationManagerImpl::TimerTotalOperationalHoursEventHandler(TimerHandle_t xTimer)
{
    // Do not increment total operational hours if the value is going to overflow UINT32.
    mTotalOperationalHours = (mTotalOperationalHours < UINT32_MAX) ? (mTotalOperationalHours + 1U) : UINT32_MAX;
    PlatformMgr().ScheduleWork(ConfigurationManagerImpl().UpdateTotalOperationalHours);
}

void ConfigurationManagerImpl::UpdateTotalOperationalHours(intptr_t arg)
{
    CHIP_ERROR err;
    (void)arg;

    err = ConfigurationMgrImpl().StoreTotalOperationalHours(mTotalOperationalHours);
    if (err != CHIP_NO_ERROR)
    {
        ChipLogError(DeviceLayer, "Store total operational hours NOK !");
    }
}

ConfigurationManagerImpl & ConfigurationManagerImpl::GetDefaultInstance()
{
    static ConfigurationManagerImpl sInstance;
    return sInstance;
}

CHIP_ERROR ConfigurationManagerImpl::Init()
{
    CHIP_ERROR err;

    bootReasonRead = false;

    err = STM32Config::Init();
    SuccessOrExit(err);

    if (STM32Config::ConfigValueExists(STM32Config::kCounterKey_RebootCount) == false)
    {
        // The first boot after factory reset of the Node.
        err = StoreRebootCount(1U);
        SuccessOrExit(err);
    }
    else
    {
        uint32_t rebootCount;
        err = GetRebootCount(rebootCount);
        SuccessOrExit(err);

        // Do not increment reboot count if the value is going to overflow UINT32.
        err = StoreRebootCount((rebootCount < UINT32_MAX) ? (rebootCount + 1U) : rebootCount);
        SuccessOrExit(err);
    }

    if (STM32Config::ConfigValueExists(STM32Config::kCounterKey_TotalOperationalHours) == false)
    {
        err = StoreTotalOperationalHours(mTotalOperationalHours);
        SuccessOrExit(err);
    }
    else
    {
        err = GetTotalOperationalHours(mTotalOperationalHours);
        SuccessOrExit(err);
    }

    // Create and Start the timer to manage TotalOperationalHours increment
    sTotalOperationalHoursTimer = xTimerCreate("TotalOperationalHoursTimer", // Just a text name, not used by the RTOS kernel
                                               pdMS_TO_TICKS(TOTAL_OPERATIONAL_HOURS_PERIOD), // == default timer period (mS)
                                               pdTRUE, //timer will auto-reload themselves when they expire
                                               (void *)this, // init timer id
                                               TimerTotalOperationalHoursEventHandler); // timer callback handler
    if (sTotalOperationalHoursTimer == nullptr)
    {
        ChipLogError(DeviceLayer, "TotalOperationalHours timer creation NOK !")
        SuccessOrExit(CHIP_ERROR_NO_MEMORY);
    }
    if (xTimerStart(sTotalOperationalHoursTimer, 0) == pdFAIL)
    {
        ChipLogError(DeviceLayer, "TotalOperationalHours timer start NOK !")
        SuccessOrExit(CHIP_ERROR_INTERNAL);
    }

    // Initialize the generic implementation base class.
    err = Internal::GenericConfigurationManagerImpl<STM32Config>::Init();
    SuccessOrExit(err);

    err = CHIP_NO_ERROR;

exit:
    return err;
}

CHIP_ERROR ConfigurationManagerImpl::GetRebootCount(uint32_t & rebootCount)
{
    return ReadConfigValue(STM32Config::kCounterKey_RebootCount, rebootCount);
}

CHIP_ERROR ConfigurationManagerImpl::StoreRebootCount(uint32_t rebootCount)
{
    return WriteConfigValue(STM32Config::kCounterKey_RebootCount, rebootCount);
}

CHIP_ERROR ConfigurationManagerImpl::GetTotalOperationalHours(uint32_t & totalOperationalHours)
{
    return ReadConfigValue(STM32Config::kCounterKey_TotalOperationalHours, totalOperationalHours);
}

CHIP_ERROR ConfigurationManagerImpl::StoreTotalOperationalHours(uint32_t totalOperationalHours)
{
    return WriteConfigValue(STM32Config::kCounterKey_TotalOperationalHours, totalOperationalHours);
}

bool ConfigurationManagerImpl::CanFactoryReset()
{
    // query the application to determine if factory reset is allowed.
    return true;
}

CHIP_ERROR ConfigurationManagerImpl::GetPrimaryMACAddress(MutableByteSpan buf)
{
    CHIP_ERROR err;

    // Copies the primary MAC into a mutable span, which must be of size kPrimaryMACAddressLength.
    // Upon success, the span will be reduced to the size of the MAC address being returned, which
    // can be less than kPrimaryMACAddressLength on a device that supports Thread.
    if (buf.size() >= ConfigurationManager::kPrimaryMACAddressLength)
    {
        uint8_t macAddrLength;
        memset(buf.data(), 0, buf.size());
        macAddrLength = stm32_GetMACAddr(buf.data(), buf.size());
        if (macAddrLength != 0U)
        {
            buf.reduce_size(ConfigurationManager::kPrimaryMACAddressLength);
            err = CHIP_NO_ERROR;
        }
        else
        {
            err = CHIP_ERROR_NOT_IMPLEMENTED;
        }
    }
    else
    {
        err = CHIP_ERROR_BUFFER_TOO_SMALL;
    }

    return err;
}

void ConfigurationManagerImpl::InitiateFactoryReset()
{
    PlatformMgr().ScheduleWork(DoFactoryReset);
}

CHIP_ERROR ConfigurationManagerImpl::GetBootReason(uint32_t & bootReason)
{
    CHIP_ERROR err = CHIP_NO_ERROR;

#if defined(STM32H753xx)
    // rebootCause is obtained at bootup.
    if (bootReasonRead == false)
    {
        if (__HAL_RCC_GET_FLAG(RCC_FLAG_LPWR1RST))
        {
            /* RESET_CAUSE_LOW_POWER_RESET */
            matterBootCause = BootReasonType::kUnspecified;
        }
        else if (__HAL_RCC_GET_FLAG(RCC_FLAG_WWDG1RST))
        {
            /* RESET_CAUSE_WINDOW_WATCHDOG_RESET */
            matterBootCause = BootReasonType::kSoftwareWatchdogReset;
        }
        else if (__HAL_RCC_GET_FLAG(RCC_FLAG_IWDG1RST))
        {
            /* RESET_CAUSE_INDEPENDENT_WATCHDOG_RESET */
            matterBootCause = BootReasonType::kSoftwareWatchdogReset;
        }
        else if (__HAL_RCC_GET_FLAG(RCC_FLAG_SFTRST))
        {
            /* This reset is induced by calling NVIC_SystemReset() */
            /*  RESET_CAUSE_SOFTWARE_RESET */
            matterBootCause = BootReasonType::kSoftwareReset;
        }
        else if (__HAL_RCC_GET_FLAG(RCC_FLAG_PORRST))
        {
            /* RESET_CAUSE_POWER_ON_POWER_DOWN_RESET */
            matterBootCause = BootReasonType::kPowerOnReboot;
        }
        else if (__HAL_RCC_GET_FLAG(RCC_FLAG_PINRST))
        {
            /* RESET_CAUSE_EXTERNAL_RESET_PIN_RESET */
            matterBootCause = BootReasonType::kPowerOnReboot;
        }
        /* As to come *after* checking the `RCC_FLAG_PORRST` flag in order to
         * ensure first that the reset cause is NOT a POR/PDR reset. */
        else if (__HAL_RCC_GET_FLAG(RCC_FLAG_BORRST))
        {
            /* RESET_CAUSE_BROWNOUT_RESET */
            matterBootCause = BootReasonType::kBrownOutReset;
        }
        else
        {
            /* RESET_CAUSE_UNKNOWN */
            matterBootCause = BootReasonType::kUnspecified;
        }

        bootReasonRead = true;
        // Clear all the reset flags or else they will remain set during future
        // resets until system power is fully removed.
        __HAL_RCC_CLEAR_RESET_FLAGS();
    }
#else
    matterBootCause = BootReasonType::kUnspecified;
    err = CHIP_ERROR_NOT_IMPLEMENTED;
#endif /* defined(STM32H753xx) */

    bootReason = to_underlying(matterBootCause);

    return err;
}

CHIP_ERROR ConfigurationManagerImpl::ReadPersistedStorageValue(::chip::Platform::PersistedStorage::Key key, uint32_t & value)
{
    CHIP_ERROR err  = CHIP_NO_ERROR;
    ChipLogProgress(DataManagement, "ReadPersistedStorageValue");
    ChipLogProgress(DataManagement, "!!! NOT IMT STM32 config !!!");
//    err = STM32Config::ReadConfigValueCounter(key, value);
//    if (err == CHIP_DEVICE_ERROR_CONFIG_NOT_FOUND)
//    {
//        err = CHIP_ERROR_PERSISTED_STORAGE_VALUE_NOT_FOUND;
//    }

    return err;
}

CHIP_ERROR ConfigurationManagerImpl::WritePersistedStorageValue(::chip::Platform::PersistedStorage::Key key, uint32_t value)
{
    CHIP_ERROR err = CHIP_NO_ERROR;
    ChipLogProgress(DataManagement, "WritePersistedStorageValue");
    ChipLogProgress(DataManagement, "!!! NOT IMT STM32 config !!!");
//    ChipLogProgress(DataManagement, "WritePersistedStorageValue");
//    err = STM32Config::WriteConfigValueCounter(key, value);
    return err;
}

CHIP_ERROR ConfigurationManagerImpl::ReadConfigValue(Key key, bool & val)
{
    return STM32Config::ReadConfigValue(key, val);
}

CHIP_ERROR ConfigurationManagerImpl::ReadConfigValue(Key key, uint32_t & val)
{
    return STM32Config::ReadConfigValue(key, val);
}

CHIP_ERROR ConfigurationManagerImpl::ReadConfigValue(Key key, uint64_t & val)
{
    return STM32Config::ReadConfigValue(key, val);
}

CHIP_ERROR ConfigurationManagerImpl::ReadConfigValueStr(Key key, char * buf, size_t bufSize, size_t & outLen)
{
    return STM32Config::ReadConfigValueStr(key, buf, bufSize, outLen);
}

CHIP_ERROR ConfigurationManagerImpl::ReadConfigValueBin(Key key, uint8_t * buf, size_t bufSize, size_t & outLen)
{
    return STM32Config::ReadConfigValueBin(key, buf, bufSize, outLen);
}

CHIP_ERROR ConfigurationManagerImpl::WriteConfigValue(Key key, bool val)
{
    return STM32Config::WriteConfigValue(key, val);
}

CHIP_ERROR ConfigurationManagerImpl::WriteConfigValue(Key key, uint32_t val)
{
    return STM32Config::WriteConfigValue(key, val);
}

CHIP_ERROR ConfigurationManagerImpl::WriteConfigValue(Key key, uint64_t val)
{
    return STM32Config::WriteConfigValue(key, val);
}

CHIP_ERROR ConfigurationManagerImpl::WriteConfigValueStr(Key key, const char * str)
{
    return STM32Config::WriteConfigValueStr(key, str);
}

CHIP_ERROR ConfigurationManagerImpl::WriteConfigValueStr(Key key, const char * str, size_t strLen)
{
    return STM32Config::WriteConfigValueStr(key, str, strLen);
}

CHIP_ERROR ConfigurationManagerImpl::WriteConfigValueBin(Key key, const uint8_t * data, size_t dataLen)
{
    return STM32Config::WriteConfigValueBin(key, data, dataLen);
}

void ConfigurationManagerImpl::RunConfigUnitTest(void)
{
    STM32Config::RunConfigUnitTest();
}

void ConfigurationManagerImpl::DoFactoryReset(intptr_t arg)
{
    ChipLogProgress(DeviceLayer, "Factory reset not implemented");
}

ConfigurationManager & ConfigurationMgrImpl()
{
    return ConfigurationManagerImpl::GetDefaultInstance();
}

} // namespace DeviceLayer
} // namespace chip
