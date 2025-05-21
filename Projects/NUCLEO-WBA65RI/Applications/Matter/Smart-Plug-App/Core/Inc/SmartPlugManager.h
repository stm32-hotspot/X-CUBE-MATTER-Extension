/*
 *
 *    Copyright (c) 2021 Project CHIP Authors
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

#include <lib/core/CHIPError.h>

#include <lib/core/DataModelTypes.h>

#include "FreeRTOS.h"

#include <app/clusters/electrical-energy-measurement-server/electrical-energy-measurement-server.h>
#include <app/clusters/power-topology-server/power-topology-server.h>
#include <ElectricalPowerMeasurementDelegate.h>
#include <PowerTopologyDelegate.h>

typedef uint16_t EndpointId;

class SmartPlugManager
{
public:
    enum Action_t
    {
        ON_ACTION = 0,
        OFF_ACTION,
        INVALID_ACTION
    } Action;

    enum State_t
    {
        kState_On = 0,
        kState_Off,
    } State;

    CHIP_ERROR Init();
    bool IsTurnedOn();
    bool InitiateAction(Action_t aAction, int32_t aActor, uint16_t size, uint8_t * value);

    void PowerMeasureStart();
    void PowerMeasureStop();
    static void PowerMeasureTask(void * pvParameter);

    using SmartPlugCallback_fn = std::function<void(Action_t)>;

    void SetCallbacks(SmartPlugCallback_fn aActionInitiated_CB, SmartPlugCallback_fn aActionCompleted_CB);

private:
    friend SmartPlugManager & SmartPlugMgr(void);
    State_t mState;


    SmartPlugCallback_fn mActionInitiated_CB;
    SmartPlugCallback_fn mActionCompleted_CB;

    void Set(bool aOn);
    void UpdatePlug();

    CHIP_ERROR PowerTopologyInit();
    CHIP_ERROR PowerTopologyShutdown();
    CHIP_ERROR EnergyMeterInit();
    CHIP_ERROR EnergyMeterShutdown();

    static SmartPlugManager sPlug;

};

inline SmartPlugManager & SmartPlugMgr(void)
{
    return SmartPlugManager::sPlug;
}
