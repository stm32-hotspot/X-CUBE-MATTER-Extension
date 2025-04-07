/*
 *
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

#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <functional>

#include "AppEvent.h"

#include "FreeRTOS.h"
#include "timers.h" // provides FreeRTOS timer support
#include <app-common/zap-generated/attributes/Accessors.h>

#include <lib/core/CHIPError.h>

using namespace chip;
namespace ThermMode = chip::app::Clusters::Thermostat;

// AppCluster Spec Table 85.
//enum ThermMode
//{
//    OFF = 0,
//    AUTO,
//    NOT_USED,
//    COOL,
//    HEAT,
//};

class TemperatureManager
{
public:
	enum Action_t
	{
		INVALID_ACTION = 0,
		COOL_SET_ACTION,
		HEAT_SET_ACTION,
		MODE_SET_ACTION,
		LOCAL_T_ACTION,
		OUTDOOR_T_ACTION
	} Action;

    CHIP_ERROR Init();
    void AttributeChangeHandler(Action_t aAction, uint8_t * value, uint16_t size);
    ThermMode::SystemModeEnum GetMode();
    void SetMode(ThermMode::SystemModeEnum mode);
    int8_t GetCurrentTemp();
    int8_t GetOutdoorTemp();
    int8_t GetHeatingSetPoint();
    int8_t GetCoolingSetPoint();
    void SetCurrentTemp(int16_t temp);
    void SetSetpoints(int16_t cool_change, int16_t heat_change);

    using ThermostatCallback_fn = std::function<void(Action_t)>;
    void SetCallbacks(ThermostatCallback_fn aActionCompleted_CB);

    void SetExtReadOk(bool ok);
    void SetIntReadOk(bool ok);


private:
    friend TemperatureManager & TempMgr();

    int8_t mCurrentTempCelsius, mOutdoorTempCelsius;
    int8_t mCoolingCelsiusSetPoint;
    int8_t mHeatingCelsiusSetPoint;
    ThermMode::SystemModeEnum mSystemMode;

    ThermostatCallback_fn mActionCompleted_CB;

    bool mExternal_read_ok, mInternal_read_ok;

    void RestoreSetup(void);

    int8_t ConvertToPrintableTemp(int16_t temperature);
    static TemperatureManager sTempMgr;
};

inline TemperatureManager & TempMgr()
{
    return TemperatureManager::sTempMgr;
}
