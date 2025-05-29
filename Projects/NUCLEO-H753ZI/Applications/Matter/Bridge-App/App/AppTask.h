/*
 *
 *    Copyright (c) 2022 Project CHIP Authors
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

#include <stdint.h>
#include <platform/CHIPDeviceLayer.h>

#include <app/server/AppDelegate.h>
#include <app/util/config.h>

#include "timers.h"
#include "AppEvent.h"
#include "AppEntry.h"

#ifdef MATTER_DM_PLUGIN_IDENTIFY_SERVER
#include <app/clusters/identify-server/identify-server.h>
#endif /* MATTER_DM_PLUGIN_IDENTIFY_SERVER */

class AppTask
{
public:
    CHIP_ERROR Init(AppDelegate * context = nullptr);
    CHIP_ERROR Start(void);
    static void AppTaskMain(void *pvParameter);

    void PostEvent(const AppEvent *aEvent);
    static void ButtonEventHandler(ButtonState_t *Button);

#ifdef MATTER_DM_PLUGIN_IDENTIFY_SERVER
    /* Identify server command callbacks. */
    static void OnIdentifyStart(Identify * identify);
    static void OnIdentifyStop(Identify * identify);
    static void OnTriggerIdentifyEffectCompleted(chip::System::Layer * systemLayer, void * appState);
    static void OnTriggerIdentifyEffect(Identify * identify);
#endif /* MATTER_DM_PLUGIN_IDENTIFY_SERVER */

private:
    static AppTask sAppTask;
    friend AppTask & GetAppTask(void);

    static void DispatchEvent(AppEvent *aEvent);
    static void UpdateEventHandler(AppEvent *aEvent);
    static void TimerUserButtonEventHandler(TimerHandle_t xTimer);
    static void ButtonHandler(AppEvent *aEvent);
    static void TimerDnssdEventHandler(TimerHandle_t xTimer);
    static void StartDnssdServer(intptr_t context);
    static void MatterEventHandler(const chip::DeviceLayer::ChipDeviceEvent * event, intptr_t arg);
    static void OnInternetConnectivityChange(const chip::DeviceLayer::ChipDeviceEvent * event);

    enum Function_t
    {
        kFunction_NoneSelected   = 0,
        kFunction_SaveNvm        = 2,
        kFunction_FactoryReset   = 3,
        kFunction_Invalid
    } Function;
    Function_t mFunction;
};

inline AppTask & GetAppTask(void)
{
    return AppTask::sAppTask;
}
