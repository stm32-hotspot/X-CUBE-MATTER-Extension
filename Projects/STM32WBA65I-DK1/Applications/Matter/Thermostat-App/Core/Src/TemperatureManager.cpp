/*
 *
 *    Copyright (c) 2020 Project CHIP Authors
 *    Copyright (c) 2019 Google LLC.
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

/**********************************************************
 * Includes
 *********************************************************/

#include "TemperatureManager.h"
#include <lib/support/logging/CHIPLogging.h>
#include <platform/CHIPDeviceLayer.h>
#include "platform/KeyValueStoreManager.h"

/**********************************************************
 * Defines and Constants
 *********************************************************/

using namespace chip;
using namespace ::chip::DeviceLayer;
using namespace ::chip::DeviceLayer::PersistedStorage;

namespace ThermAttr = chip::app::Clusters::Thermostat::Attributes;
namespace ThermMode = chip::app::Clusters::Thermostat;

constexpr EndpointId kThermostatEndpoint = 1;
app::DataModel::Nullable<int16_t> mTemp, mOutTemp;
int16_t mHeatSet, mCoolSet;

// persisted storage keys
const char skCoolSet[]         = "a/cs";
const char skHeatSet[]       = "a/hs";
const char skSystemMode[]     = "a/sm";


/**********************************************************
 * Variable declarations
 *********************************************************/

TemperatureManager TemperatureManager::sTempMgr;

CHIP_ERROR TemperatureManager::Init()
{
	RestoreSetup();

    mCurrentTempCelsius = -127;

    mOutdoorTempCelsius = -127;

	mHeatingCelsiusSetPoint = ConvertToPrintableTemp(mHeatSet);
	mCoolingCelsiusSetPoint = ConvertToPrintableTemp(mCoolSet);

	mExternal_read_ok = false;
	mInternal_read_ok = false;

    ChipLogProgress(NotSpecified, "sTempMgr.Init, Local temp %d", mCurrentTempCelsius);
    ChipLogProgress(NotSpecified, "sTempMgr.Init, Outdoor temp %d", mOutdoorTempCelsius);
    ChipLogProgress(NotSpecified, "sTempMgr.Init, CoolingSetpoint %d", mCoolingCelsiusSetPoint);
    ChipLogProgress(NotSpecified, "sTempMgr.Init, HeatingSetpoint %d", mHeatingCelsiusSetPoint);
    ChipLogProgress(NotSpecified, "sTempMgr.Init, SystemMode %d", mSystemMode);

    return CHIP_NO_ERROR;
}

void TemperatureManager::RestoreSetup(void)
{
	CHIP_ERROR err;
	size_t outLen;

	// try to read values from NVM. If not yet present, get the (default) value from stack and save it to NVM.
	// If present, send the saved value to the stack
	err = KeyValueStoreMgr().Get(skCoolSet, reinterpret_cast<void *>(&mCoolSet), sizeof(int16_t), &outLen);
	if (err != CHIP_NO_ERROR)
	{
		PlatformMgr().LockChipStack();
		ThermAttr::OccupiedCoolingSetpoint::Get(kThermostatEndpoint,&mCoolSet);
		PlatformMgr().UnlockChipStack();

		err = KeyValueStoreMgr().Put(skCoolSet, reinterpret_cast<void *>(&mCoolSet), sizeof(int16_t));
	}
	else
	{
		PlatformMgr().LockChipStack();
		ThermAttr::OccupiedCoolingSetpoint::Set(kThermostatEndpoint, mCoolSet);
		PlatformMgr().UnlockChipStack();
	}

	err = KeyValueStoreMgr().Get(skHeatSet, reinterpret_cast<void *>(&mHeatSet), sizeof(int16_t), &outLen);
	if (err != CHIP_NO_ERROR)
	{
		PlatformMgr().LockChipStack();
		ThermAttr::OccupiedHeatingSetpoint::Get(kThermostatEndpoint, &mHeatSet);
		PlatformMgr().UnlockChipStack();

		err = KeyValueStoreMgr().Put(skHeatSet, reinterpret_cast<void *>(&mHeatSet), sizeof(int16_t));
	}
	else
	{
		PlatformMgr().LockChipStack();
		ThermAttr::OccupiedHeatingSetpoint::Set(kThermostatEndpoint, mHeatSet);
		PlatformMgr().UnlockChipStack();
	}

	err = KeyValueStoreMgr().Get(skSystemMode, reinterpret_cast<void *>(&mSystemMode), sizeof(ThermMode::SystemModeEnum), &outLen);
	if (err != CHIP_NO_ERROR)
	{
		PlatformMgr().LockChipStack();
		ThermAttr::SystemMode::Get(kThermostatEndpoint, &mSystemMode);
		PlatformMgr().UnlockChipStack();

		err = KeyValueStoreMgr().Put(skSystemMode, reinterpret_cast<void *>(&mSystemMode), sizeof(ThermMode::SystemModeEnum));
	}
	else
	{
		PlatformMgr().LockChipStack();
		ThermAttr::SystemMode::Set(kThermostatEndpoint, mSystemMode);
		PlatformMgr().UnlockChipStack();
	}


}

void TemperatureManager::SetCallbacks(ThermostatCallback_fn aActionCompleted_CB)
{
	mActionCompleted_CB = aActionCompleted_CB;
}

int8_t TemperatureManager::ConvertToPrintableTemp(int16_t temperature)
{
    constexpr uint8_t kRoundUpValue = 50;

    // Round up the temperature as we won't print decimals on LCD
    // Is it a negative temperature
    if (temperature < 0)
    {
        temperature -= kRoundUpValue;
    }
    else
    {
        temperature += kRoundUpValue;
    }

    return static_cast<int8_t>(temperature / 100);
}

void TemperatureManager::AttributeChangeHandler(Action_t aAction, uint8_t * value, uint16_t size)
{
    switch (aAction)
    {
    case LOCAL_T_ACTION: {
    	if (mInternal_read_ok)
    	{
    		int8_t Temp = ConvertToPrintableTemp(*((int16_t *) value));
			ChipLogProgress(NotSpecified, "TempMgr: Local temp %d", Temp);
			mCurrentTempCelsius = Temp;
    	}
    	else
    	{
    		ChipLogProgress(NotSpecified, "TempMgr: Local temp NULL");
			mCurrentTempCelsius = -127;
    	}
    }
    break;

    case OUTDOOR_T_ACTION: {
    	if (mExternal_read_ok)
		{
    		int8_t Temp = ConvertToPrintableTemp(*((int16_t *) value));
			ChipLogProgress(NotSpecified, "TempMgr: Outdoor temp %d", Temp);
			mOutdoorTempCelsius = Temp;
		}
		else
		{
			ChipLogProgress(NotSpecified, "TempMgr: Outdoor temp NULL");
			mOutdoorTempCelsius = -127;
		}
	}
	break;

    case COOL_SET_ACTION: {
    	mCoolingCelsiusSetPoint = ConvertToPrintableTemp(*((int16_t *) value));

        CHIP_ERROR err = KeyValueStoreMgr().Put(skCoolSet, reinterpret_cast<void *>(value), sizeof(int16_t));
        ChipLogProgress(NotSpecified, "TempMgr: CoolingSetpoint %d. NVM.Put-> %d", *((int16_t *) value), err);
    }
    break;

    case HEAT_SET_ACTION: {
    	mHeatingCelsiusSetPoint = ConvertToPrintableTemp(*((int16_t *) value));

    	CHIP_ERROR err = KeyValueStoreMgr().Put(skHeatSet, reinterpret_cast<void *>(value), sizeof(int16_t));
        ChipLogProgress(NotSpecified, "TempMgr: HeatingSetpoint %d. NVM.Put-> %d", *((int16_t *) value), err);
    }
    break;

    case MODE_SET_ACTION: {
    	mSystemMode = static_cast<ThermMode::SystemModeEnum>(*value);

    	CHIP_ERROR err = KeyValueStoreMgr().Put(skSystemMode, reinterpret_cast<void *>(value), sizeof(ThermMode::SystemModeEnum));
    	ChipLogProgress(NotSpecified, "TempMgr: SystemMode %d. NVM.Put-> %d", mSystemMode, err);
    }
    break;

    default: {
        return;
    }
    break;
    }

    if (mActionCompleted_CB) {
		mActionCompleted_CB(aAction);
	}

}

ThermMode::SystemModeEnum TemperatureManager::GetMode()
{
    return mSystemMode;
}

int8_t TemperatureManager::GetCurrentTemp()
{
    return mCurrentTempCelsius;
}

int8_t TemperatureManager::GetOutdoorTemp()
{
    return mOutdoorTempCelsius;
}

int8_t TemperatureManager::GetHeatingSetPoint()
{
    return mHeatingCelsiusSetPoint;
}

int8_t TemperatureManager::GetCoolingSetPoint()
{
    return mCoolingCelsiusSetPoint;
}

void TemperatureManager::SetCurrentTemp(int16_t temp)
{
//	mCurrentTempCelsius = ConvertToPrintableTemp(temp);
	PlatformMgr().LockChipStack();
	ThermAttr::LocalTemperature::Set(kThermostatEndpoint, temp);
	PlatformMgr().UnlockChipStack();
}

void TemperatureManager::SetMode(ThermMode::SystemModeEnum mode)
{
	PlatformMgr().LockChipStack();
	ThermAttr::SystemMode::Set(kThermostatEndpoint, mode);
	PlatformMgr().UnlockChipStack();
}


void TemperatureManager::SetSetpoints(int16_t cool_change, int16_t heat_change)
{
	int16_t heatingSetpoint, coolingSetpoint;

	if (cool_change != 0)
	{
		PlatformMgr().LockChipStack();
		ThermAttr::OccupiedCoolingSetpoint::Get(kThermostatEndpoint, &coolingSetpoint);
		coolingSetpoint += cool_change;
		ThermAttr::OccupiedCoolingSetpoint::Set(kThermostatEndpoint, coolingSetpoint);
		PlatformMgr().UnlockChipStack();
	}
	if (heat_change != 0)
	{
		PlatformMgr().LockChipStack();
		ThermAttr::OccupiedHeatingSetpoint::Get(kThermostatEndpoint, &heatingSetpoint);
		heatingSetpoint += heat_change;
		ThermAttr::OccupiedHeatingSetpoint::Set(kThermostatEndpoint, heatingSetpoint);
		PlatformMgr().UnlockChipStack();
	}
}

void TemperatureManager::SetExtReadOk(bool ok)
{
	mExternal_read_ok = ok;
}

void TemperatureManager::SetIntReadOk(bool ok)
{
	mInternal_read_ok = ok;
}
