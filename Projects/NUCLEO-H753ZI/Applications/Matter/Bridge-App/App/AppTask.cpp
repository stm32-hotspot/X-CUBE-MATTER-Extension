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

#include "AppTask.h"
#include "AppLog.h"
#include "main.h"
#include <app/clusters/network-commissioning/network-commissioning.h>
#include <app/server/Dnssd.h>
#include <app/server/Server.h>

#include "FreeRTOS.h"
#include "task.h"
#include "portable.h"

#include <credentials/DeviceAttestationCredsProvider.h>

#include <setup_payload/QRCodeSetupPayloadGenerator.h>
#include <setup_payload/SetupPayload.h>
#include <app/server/OnboardingCodesUtil.h>

#include "DeviceInfoProviderImpl.h"
#include "FactoryDataProvider.h"
#include "NetworkCommissioningDriver.h"
#include "STM32FreeRtosHooks.h"

#include <lib/dnssd/MinimalMdnsServer.h>

#include "stm32_nvm_implement.h"

#include "BridgedClustersManager.h"

#define TIMER_DEFAULT_TIMEOUT         (1000)     // in ms: timer default timeout

#define APPTASK_THREAD_NAME           "AppTask"
#define APPTASK_THREAD_STACK_DEPTH    (configMINIMAL_STACK_SIZE * 2)
#define APPTASK_THREAD_PRIORITY       (4)
#define APPTASK_EVENT_QUEUE_SIZE      (10)

TimerHandle_t sUserButtonTimeoutTimer;
TimerHandle_t sDnssdInitTimer;
TaskHandle_t  sAppTaskHandle;
static QueueHandle_t sAppTaskEventQueue;

AppTask AppTask::sAppTask;

static uint8_t TimerUserButton;
static uint8_t UserButtonStateCpt;

using namespace ::chip;
using namespace ::chip::DeviceLayer;
using namespace ::chip::Platform;

chip::DeviceLayer::DeviceInfoProviderImpl gExampleDeviceInfoProvider;
chip::DeviceLayer::FactoryDataProvider mFactoryDataProvider;
chip::DeviceLayer::NetworkCommissioning::STM32EthernetDriver sEthernetDriver;
app::Clusters::NetworkCommissioning::Instance sEthernetNetworkCommissioningInstance(kRootEndpointId, &sEthernetDriver);

BridgedClustersManager gExampleBridgedClustersManager;

CHIP_ERROR AppTask::Init(AppDelegate * sAppDelegate)
{
    CHIP_ERROR err = CHIP_NO_ERROR;

    // Create FreeRTOS resources
    // Create Timer for Push button timeouts.
    sUserButtonTimeoutTimer = xTimerCreate("UserButtonTimer", // Just a text name, not used by the RTOS kernel
                                           pdMS_TO_TICKS(TIMER_DEFAULT_TIMEOUT), // == default timer period (mS)
                                           true, // no timer reload (==one-shot)
                                           (void *)this, // init timer id
                                           TimerUserButtonEventHandler); // timer callback handler
    TimerUserButton = 0U;
    UserButtonStateCpt = 0U;

    sDnssdInitTimer = xTimerCreate("DnssdTimer",                         // Just a text name, not used by the RTOS kernel
                                   pdMS_TO_TICKS(TIMER_DEFAULT_TIMEOUT), // == default timer period (mS)
                                   true,               // no timer reload (==one-shot)
                                   (void *)this,       // init timer id
                                   TimerDnssdEventHandler); // timer callback handler

    sAppTaskEventQueue = xQueueCreate(APPTASK_EVENT_QUEUE_SIZE, sizeof(AppEvent));
    // Check resources creation
    if ((sUserButtonTimeoutTimer == NULL) || (sDnssdInitTimer == NULL) || (sAppTaskEventQueue == NULL))
    {
        STM32_LOG("Failed to allocate AppTask resources !");
        err = CHIP_ERROR_NO_MEMORY;
    }

    if (err == CHIP_NO_ERROR)
    {
      // Continue with Matter initialization
      ReturnErrorOnFailure(Platform::MemoryInit());

      // Initialize the CHIP stack.
      ReturnErrorOnFailure(PlatformMgr().InitChipStack());

      // Register a function to receive events from the CHIP device layer.  Note that calls to
      // this function will happen on the CHIP event loop thread, not AppTask thread
      PlatformMgr().AddEventHandler(MatterEventHandler, 0);

      ReturnErrorOnFailure(mFactoryDataProvider.Init());
      chip::DeviceLayer::SetDeviceInstanceInfoProvider(&mFactoryDataProvider);
      chip::DeviceLayer::SetCommissionableDataProvider(&mFactoryDataProvider);
      chip::Credentials::SetDeviceAttestationCredentialsProvider(&mFactoryDataProvider);

      chip::DeviceLayer::SetDeviceInfoProvider(&gExampleDeviceInfoProvider);

      // Initialization ZCL Data Model and CHIP AppServer
      static chip::CommonCaseDeviceServerInitParams initParams;
      (void) initParams.InitializeStaticResourcesBeforeServerInit();
      if (sAppDelegate != nullptr)
      {
        initParams.appDelegate = sAppDelegate;
      }
      chip::Server::GetInstance().Init(initParams);

      gExampleDeviceInfoProvider.SetStorageDelegate(&Server::GetInstance().GetPersistentStorage());

      chip::DeviceLayer::ConfigurationMgr().LogDeviceConfig();

      sEthernetNetworkCommissioningInstance.Init();

      // Open commissioning after boot if no fabric was available
      if (chip::Server::GetInstance().GetFabricTable().FabricCount() == 0)
      {
        ConnectivityMgr().SetBLEAdvertisingEnabled(false);
        ConnectivityMgr().SetWiFiAPMode(ConnectivityManager::kWiFiAPMode_Enabled);
        PrintOnboardingCodes(chip::RendezvousInformationFlags(chip::RendezvousInformationFlag::kOnNetwork));
      }

      if(!chip::Dnssd::GlobalMinimalMdnsServer::Server().IsListening())
      {
        xTimerStart(sDnssdInitTimer, 0);
      }

      // Start a task to run the CHIP Device event loop.
      err = PlatformMgr().StartEventLoopTask();
      if (err != CHIP_NO_ERROR)
      {
          STM32_LOG("Failed PlatformMgr().StartEventLoopTask() !");
      }
    }

    return err;
}

CHIP_ERROR AppTask::Start(void)
{
    CHIP_ERROR err = CHIP_NO_ERROR;;

    ButtonSetCallback(ButtonEventHandler);

    // Initialization Matter application
    gExampleBridgedClustersManager.Init();

    // Create AppTask.
    xTaskCreate(AppTaskMain, APPTASK_THREAD_NAME, APPTASK_THREAD_STACK_DEPTH, NULL, APPTASK_THREAD_PRIORITY,
                &sAppTaskHandle);
    if (sAppTaskHandle == NULL)
    {
        STM32_LOG("!!! Failed to create AppTask !!!");
        err = CHIP_ERROR_NO_MEMORY;
    }

    return err;
}

void AppTask::AppTaskMain(void *pvParameter)
{
    AppEvent event;
    BaseType_t eventReceived;

    while (true)
    {
        eventReceived = xQueueReceive(sAppTaskEventQueue, &event, portMAX_DELAY);
        while (eventReceived == pdTRUE)
        {
            sAppTask.DispatchEvent(&event);
            eventReceived = xQueueReceive(sAppTaskEventQueue, &event, 0);
        }
    }
}

void AppTask::PostEvent(const AppEvent *aEvent) {
    if (sAppTaskEventQueue != NULL) {
        if (!xQueueSend(sAppTaskEventQueue, aEvent, 1)) {
            ChipLogError(NotSpecified, "Failed to post event to AppTask event queue");
        }
    } else {
        ChipLogError(NotSpecified, "Event Queue is NULL should never happen");
    }
}

void AppTask::DispatchEvent(AppEvent *aEvent) {
    if (aEvent->Handler) {
        aEvent->Handler(aEvent);
    } else {
        ChipLogError(NotSpecified, "Event received with no handler. Dropping event.");
    }
}

void AppTask::TimerUserButtonEventHandler(TimerHandle_t xTimer) {
    TimerUserButton++;
    if (BSP_PB_GetState(BUTTON_USER) == 1) {
        UserButtonStateCpt++;
    }
    // else xTimerStop ?
    if (TimerUserButton >= 10) {
        xTimerStop(sUserButtonTimeoutTimer, 0);
        if (UserButtonStateCpt >= 9) {
            AppEvent event;
            TimerUserButton = 0;
            event.Type = AppEvent::kEventType_Timer;
            event.Handler = UpdateEventHandler;
            sAppTask.mFunction = kFunction_FactoryReset;
            sAppTask.PostEvent(&event);
        }
    } else if ((TimerUserButton > UserButtonStateCpt) && (TimerUserButton <= 2)) {
        AppEvent event;
        xTimerStop(sUserButtonTimeoutTimer, 0);
        TimerUserButton = 0;
        event.Type = AppEvent::kEventType_Timer;
        event.Handler = UpdateEventHandler;
        sAppTask.mFunction = kFunction_SaveNvm;
        sAppTask.PostEvent(&event);
    } else {
    }
}

void AppTask::ButtonHandler(AppEvent *aEvent) {
    if (aEvent->Type == AppEvent::kEventType_Button) {
            if (xTimerIsTimerActive(sUserButtonTimeoutTimer) == 0) {
                xTimerStart(sUserButtonTimeoutTimer, 0);
                TimerUserButton = 0;
                UserButtonStateCpt = 0;
            }
    }
}

void AppTask::ButtonEventHandler(ButtonState_t *Button) {
    AppEvent button_event = { };
    button_event.Type = AppEvent::kEventType_Button;
    button_event.ButtonEvent.ButtonIdx = Button->Button;
    button_event.ButtonEvent.Action = Button->State;
    button_event.Handler = ButtonHandler;
    sAppTask.PostEvent(&button_event);
}

void AppTask::UpdateEventHandler(AppEvent *aEvent) {
    if (sAppTask.mFunction == kFunction_SaveNvm) {
        NVM_StatusTypeDef nvmStatus;
        nvmStatus = NVM_Save();
        STM32_LOG("NVM %s", (nvmStatus == NVM_OK) ? "saved" : "NOT saved !!!");
    } else if (sAppTask.mFunction == kFunction_FactoryReset) {
        STM32_LOG("FACTORY RESET");
        NVM_FactoryReset();
    } else {
        STM32_LOG("AppTask: Unknown Event received !!!");
    }
}

void AppTask::TimerDnssdEventHandler(TimerHandle_t xTimer) {
    chip::DeviceLayer::PlatformMgr().ScheduleWork(StartDnssdServer, reinterpret_cast<intptr_t>(nullptr));
}

void AppTask::StartDnssdServer(intptr_t context) {
    if(chip::Dnssd::GlobalMinimalMdnsServer::Server().IsListening())
    {
        xTimerStop(sDnssdInitTimer, 0);
        if (chip::Server::GetInstance().GetFabricTable().FabricCount() == 0)
        {
            STM32_LOG("Server is ready. Waiting for Pairing.");
        }
        else
        {
            STM32_LOG("Server is ready. Pairing already done. Waiting for command.");
        }
    }
    else
    {
       chip::app::DnssdServer::Instance().StartServer();
    }
}

void AppTask::MatterEventHandler(const ChipDeviceEvent *event, intptr_t arg)
{
    switch (event->Type)
    {
    case DeviceEventType::kInternetConnectivityChange:
        OnInternetConnectivityChange(event);
        break;

    case DeviceEventType::kCHIPoBLEConnectionEstablished:
        STM32_LOG("CHIPoBLE connection established");
        break;

    case DeviceEventType::kCHIPoBLEConnectionClosed:
        STM32_LOG("CHIPoBLE disconnected");
        break;

    case DeviceEventType::kCommissioningComplete:
    {
        NVM_StatusTypeDef nvmStatus;
        STM32_LOG("Commissioning complete");
        nvmStatus = NVM_Save();
        STM32_LOG("NVM %s", (nvmStatus == NVM_OK) ? "Saved" : "NOT Saved !!!");
        break;
    }

    case DeviceEventType::kInterfaceIpAddressChanged:
        if ((event->InterfaceIpAddressChanged.Type == InterfaceIpChangeType::kIpV4_Assigned) ||
            (event->InterfaceIpAddressChanged.Type == InterfaceIpChangeType::kIpV6_Assigned))
        {
            // MDNS server restart on any IP assignment: if link local ipv6 is configured, that
            // will not trigger a 'Internet connectivity change' as there is no Internet
            // connectivity. MDNS still wants to refresh its listening interfaces to include the
            // newly selected address.
            chip::app::DnssdServer::Instance().StartServer();
        }
        if (event->InterfaceIpAddressChanged.Type == InterfaceIpChangeType::kIpV6_Assigned)
        {
        }
        break;
    }
}

void AppTask::OnInternetConnectivityChange(const chip::DeviceLayer::ChipDeviceEvent * event)
{
    if (event->InternetConnectivityChange.IPv4 == kConnectivity_Established)
    {
        STM32_LOG("IPv4 Server ready...");
        chip::app::DnssdServer::Instance().StartServer();
    }
    else if (event->InternetConnectivityChange.IPv4 == kConnectivity_Lost)
    {
        STM32_LOG("Lost IPv4 connectivity...");
    }
    if (event->InternetConnectivityChange.IPv6 == kConnectivity_Established)
    {
        STM32_LOG("IPv6 Server ready...");
        chip::app::DnssdServer::Instance().StartServer();
    }
    else if (event->InternetConnectivityChange.IPv6 == kConnectivity_Lost)
    {
        STM32_LOG("Lost IPv6 connectivity...");
    }
}
