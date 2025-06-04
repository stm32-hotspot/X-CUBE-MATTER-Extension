/*
 *
 *    Copyright (c) 2020 Project CHIP Authors
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

/*STM32 includes*/
#include "app_common.h"
#include "app_thread.h"
#include "stm32_lpm.h"
#include "dbg_trace.h"
#include "cmsis_os2.h"
#include "AppEvent.h"
#include "AppTask.h"
#include "flash_wb.h"

#if (OTA_SUPPORT == 1)
#include "ota.h"
#endif /* (OTA_SUPPORT == 1) */

#if HIGHWATERMARK
#include "mbedtls/memory_buffer_alloc.h"
#endif

/*Matter includes*/
#include <DeviceInfoProviderImpl.h>
#include <app/server/OnboardingCodesUtil.h>
#include <app-common/zap-generated/attribute-type.h>
#include <app-common/zap-generated/attributes/Accessors.h>
#include <app/server/Dnssd.h>
#include <app/server/Server.h>
#include <app/util/attribute-storage.h>
#include <credentials/DeviceAttestationCredsProvider.h>
#include <inet/EndPointStateOpenThread.h>
#include <setup_payload/QRCodeSetupPayloadGenerator.h>
#include <setup_payload/SetupPayload.h>
#include <platform/CHIPDeviceLayer.h>

#if CHIP_ENABLE_OPENTHREAD
#include <platform/OpenThread/OpenThreadUtils.h>
#include <platform/ThreadStackManager.h>
#endif

#include "stm32wba65i_discovery.h"
#include "app_conf.h"
#include "app_bsp.h"
#include "qr_code.h"

using namespace ::chip;
using namespace ::chip::app;
using namespace chip::TLV;
using namespace chip::Credentials;
using namespace chip::DeviceLayer;
using namespace ::chip::Platform;
using namespace ::chip::Credentials;
using namespace ::chip::app::Clusters;
using chip::DeviceLayer::PersistedStorage::KeyValueStoreMgr;
namespace ThermMode = chip::app::Clusters::Thermostat;

AppTask AppTask::sAppTask;
chip::DeviceLayer::FactoryDataProvider mFactoryDataProvider;

#define APP_EVENT_QUEUE_SIZE 10
#define NVM_TIMEOUT 1000  // timer to handle PB to save data in nvm or do a factory reset
#define STM32_THERMOSTAT_ENDPOINT_ID 1

static QueueHandle_t sAppEventQueue;


const osThreadAttr_t AppTask_attr =
{
  .name = APPTASK_NAME,
  .attr_bits = APP_ATTR_BITS,
  .cb_mem = APP_CB_MEM,
  .cb_size = APP_CB_SIZE,
  .stack_mem = APP_STACK_MEM,
  .stack_size = APP_STACK_SIZE,
  .priority =  APP_PRIORITY,
};

static bool sIsThreadProvisioned = false;
static bool sIsThreadEnabled = false;
static bool sHaveBLEConnections = false;
static bool sFabricNeedSaved = false;
static bool sFailCommissioning = false;
static bool sHaveFabric = false;

chip::DeviceLayer::DeviceInfoProviderImpl gExampleDeviceInfoProvider;

CHIP_ERROR AppTask::StartAppTask() {
    sAppEventQueue = xQueueCreate(APP_EVENT_QUEUE_SIZE, sizeof(AppEvent));
    if (sAppEventQueue == NULL) {
        APP_DBG("Failed to allocate app event queue\n");
        return CHIP_ERROR_NO_MEMORY;
    }

    // Start App task.
    osThreadNew(AppTaskMain, NULL, &AppTask_attr);

    return CHIP_NO_ERROR;
}

void LockOpenThreadTask(void) {
	APP_THREAD_LockThreadStack();
}

void UnlockOpenThreadTask(void) {
	APP_THREAD_UnLockThreadStack();
}

CHIP_ERROR AppTask::Init() {

    CHIP_ERROR err = CHIP_NO_ERROR;
    ChipLogProgress(NotSpecified, "Current Software Version: %s",
            CHIP_DEVICE_CONFIG_DEVICE_SOFTWARE_VERSION_STRING);

    /* Setup button handler */
    APPE_PushButtonSetReceiveCb(ButtonEventHandler);

    ThreadStackMgr().InitThreadStack();

#if ( CHIP_DEVICE_CONFIG_ENABLE_SED == 1)
    ConnectivityMgr().SetThreadDeviceType(ConnectivityManager::kThreadDeviceType_SleepyEndDevice);
#else
    ConnectivityMgr().SetThreadDeviceType(ConnectivityManager::kThreadDeviceType_FullEndDevice);
#endif

    PlatformMgr().AddEventHandler(MatterEventHandler, 0);

#if CHIP_DEVICE_CONFIG_ENABLE_EXTENDED_DISCOVERY
    chip::app::DnssdServer::Instance().SetExtendedDiscoveryTimeoutSecs(extDiscTimeoutSecs);
#endif

    // Init ZCL Data Model
    static chip::CommonCaseDeviceServerInitParams initParams;
    (void) initParams.InitializeStaticResourcesBeforeServerInit();
    ReturnErrorOnFailure(mFactoryDataProvider.Init());
    SetDeviceInstanceInfoProvider(&mFactoryDataProvider);
    SetCommissionableDataProvider(&mFactoryDataProvider);
    SetDeviceAttestationCredentialsProvider(&mFactoryDataProvider);

    chip::Inet::EndPointStateOpenThread::OpenThreadEndpointInitParam nativeParams;
    nativeParams.lockCb = LockOpenThreadTask;
    nativeParams.unlockCb = UnlockOpenThreadTask;
    nativeParams.openThreadInstancePtr = chip::DeviceLayer::ThreadStackMgrImpl().OTInstance();
    initParams.endpointNativeParams = static_cast<void*>(&nativeParams);
    chip::Server::GetInstance().Init(initParams);

    gExampleDeviceInfoProvider.SetStorageDelegate(&Server::GetInstance().GetPersistentStorage());
    chip::DeviceLayer::SetDeviceInfoProvider(&gExampleDeviceInfoProvider);

    err = TempSensMgr().Init();
    if (err != CHIP_NO_ERROR) {
		APP_DBG("TempSensMgr().Init() failed");
		return err;
	}

    err = TempMgr().Init();
    if (err != CHIP_NO_ERROR) {
        APP_DBG("TempMgr().Init() failed");
        return err;
    }
    TempMgr().SetCallbacks(ActionCompleted);

    InitBindingHandler();

    // Open commissioning after boot if no fabric was available
    if (chip::Server::GetInstance().GetFabricTable().FabricCount() == 0) {
#if (CFG_LCD_SUPPORTED == 1)
        chip::RendezvousInformationFlags aRendezvousFlags;
		aRendezvousFlags = chip::RendezvousInformationFlags(chip::RendezvousInformationFlag::kBLE);
		chip::PayloadContents payload;
		GetPayloadContents(payload, aRendezvousFlags);

		char payloadBuffer[chip::QRCodeBasicSetupPayloadGenerator::kMaxQRCodeBase38RepresentationLength + 1];
		chip::MutableCharSpan qrCode(payloadBuffer);
		GetQRCode(qrCode, payload);

		create_qr_img(qrCode.data());
		draw_qr_img();
		UTIL_LCD_DisplayStringAt(70, 0, (uint8_t*) "Matter", LEFT_MODE);
		UTIL_LCD_DisplayStringAt(70, 12, (uint8_t*) "Thermostat", LEFT_MODE);
		UTIL_LCD_DisplayStringAt(70, 24, (uint8_t*) "App", LEFT_MODE);
		BSP_LCD_Refresh(LCD1);
#endif

        PrintOnboardingCodes(
                chip::RendezvousInformationFlags(chip::RendezvousInformationFlag::kBLE));
        // Enable BLE advertisements
        chip::Server::GetInstance().GetCommissioningWindowManager().OpenBasicCommissioningWindow();
        APP_DBG("BLE advertising started. Waiting for Pairing.\n");
    } else {  // try to attach to the thread network
        sHaveFabric = true;
        char Message[20];
		snprintf(Message, sizeof(Message), "Fabric Found: %d", chip::Server::GetInstance().GetFabricTable().FabricCount());
        UTIL_LCD_DisplayStringAt(0, LINE(1), (uint8_t*) Message, LEFT_MODE);
		BSP_LCD_Refresh(LCD1);
    }



    err = PlatformMgr().StartEventLoopTask();
    if (err != CHIP_NO_ERROR) {
        APP_DBG("PlatformMgr().StartEventLoopTask() failed\n");
    }

    return err;
}

CHIP_ERROR AppTask::InitMatter() {
    CHIP_ERROR err = CHIP_NO_ERROR;

    err = chip::Platform::MemoryInit();
    if (err != CHIP_NO_ERROR) {
        APP_DBG("Platform::MemoryInit() failed\n");
    } else {
        err = PlatformMgr().InitChipStack();
        if (err != CHIP_NO_ERROR) {
            APP_DBG("PlatformMgr().InitChipStack() failed\n");
        }
    }
    return err;
}

void AppTask::AppTaskMain(void *pvParameter) {
    AppEvent event;
#if (CFG_LCD_SUPPORTED == 1)
    APP_BSP_LcdInit();
    BSP_LCD_Clear(LCD1,LCD_COLOR_BLACK);
#endif /* (CFG_LCD_SUPPORTED == 1) */

    CHIP_ERROR err = sAppTask.Init();

#if (CFG_LCD_SUPPORTED == 1)
    if(sHaveFabric)
    {
      	char Message[20];
      	snprintf(Message, sizeof(Message), "Fabric Found: %d",
       	chip::Server::GetInstance().GetFabricTable().FabricCount());
       	UTIL_LCD_DisplayStringAt(0, LINE(1), (uint8_t*) Message, LEFT_MODE);
       	BSP_LCD_Refresh(LCD1);
    }
#endif /* (CFG_LCD_SUPPORTED == 1) */

#if HIGHWATERMARK
    UBaseType_t uxHighWaterMark;
    HeapStats_t HeapStatsInfo;
#endif // endif HIGHWATERMARK
    if (err != CHIP_NO_ERROR) {
        APP_DBG("App task init failed\n");
    }

    APP_DBG("App Task started\n");
    while (true) {

        BaseType_t eventReceived = xQueueReceive(sAppEventQueue, &event, portMAX_DELAY);
        while (eventReceived == pdTRUE) {
            sAppTask.DispatchEvent(&event);
            eventReceived = xQueueReceive(sAppEventQueue, &event, 0);
        }
#if HIGHWATERMARK
        uxHighWaterMark = uxTaskGetStackHighWaterMark( NULL );
        APP_DBG ("\x1b[34m" "AppTask_stack_HighWaterMark %lu \n" "\x1b[0m",uxHighWaterMark);
        vPortGetHeapStats(&HeapStatsInfo);
        APP_DBG ("\x1b[34m" "AppTask_FreeRTOS_heap.freespace %u bytes\n" "\x1b[0m",HeapStatsInfo.xAvailableHeapSpaceInBytes);
        APP_DBG ("\x1b[34m" "AppTask_FreeRTOS_heap.minimum_ever_free %u bytes\n" "\x1b[0m",HeapStatsInfo.xMinimumEverFreeBytesRemaining);

#endif // endif HIGHWATERMARK
    }
}

void AppTask::ButtonEventHandler(ButtonDesc_t *Button) {


    if (Button->button == B1) { /* JOY_UP */
            // Hand off to Functionality handler - depends on duration of press
    	 AppEvent event;
    	 event.Type = AppEvent::kEventType_Timer;
    	 event.Handler = UpdateNvmEventHandler;
    	 sAppTask.mFunction = kFunction_SaveNvm;
    	 sAppTask.PostEvent(&event);
    }
    if (Button->button == B2) {/* JOY_SELECT */
    	 AppEvent event;
    	 event.Type = AppEvent::kEventType_Timer;
    	 event.Handler = UpdateNvmEventHandler;
    	 if (Button->longPressed) {
    	   sAppTask.mFunction = kFunction_FactoryReset;
      	   sAppTask.PostEvent(&event);
    	 }
    }
    if (Button->button == B3) { /* JOY_RIGHT */
        AppEvent event;
        event.Type = AppEvent::kEventType_Timer;
        event.Handler = UserButton2EventHandler;
        sAppTask.PostEvent(&event);
    }
    if (Button->button == B4) {/* JOY_LEFT */
    	 AppEvent event;
    	 event.Type = AppEvent::kEventType_Timer;
        event.Handler = UserButton3EventHandler;
        sAppTask.PostEvent(&event);
    }
    if (Button->button == B5) {/* JOY_DOWN */
        AppEvent event;
        event.Type = AppEvent::kEventType_Timer;
        event.Handler = UserButton1EventHandler;
      sAppTask.PostEvent(&event);
    }
    else {
        return;
    }
}

void AppTask::UserButton1EventHandler(AppEvent *aEvent) {
    ThermMode::SystemModeEnum mode = TempMgr().GetMode();

    if (mode == ThermMode::SystemModeEnum::kAuto) {
        TempMgr().SetMode(ThermMode::SystemModeEnum::kCool);
    } else if (mode == ThermMode::SystemModeEnum::kCool) {
        TempMgr().SetMode(ThermMode::SystemModeEnum::kHeat);
    } else if (mode == ThermMode::SystemModeEnum::kHeat) {
        TempMgr().SetMode(ThermMode::SystemModeEnum::kAuto);
    }
}

void AppTask::UserButton2EventHandler(AppEvent *aEvent) {
    ThermMode::SystemModeEnum mode = TempMgr().GetMode();

    if (mode == ThermMode::SystemModeEnum::kAuto) {
        TempMgr().SetSetpoints(100, 100);
    } else if (mode == ThermMode::SystemModeEnum::kCool) {
        TempMgr().SetSetpoints(100, 0);
    } else if (mode == ThermMode::SystemModeEnum::kHeat) {
        TempMgr().SetSetpoints(0, 100);
    }
}

void AppTask::UserButton3EventHandler(AppEvent *aEvent) {
    ThermMode::SystemModeEnum mode = TempMgr().GetMode();

    if (mode == ThermMode::SystemModeEnum::kAuto) {
        TempMgr().SetSetpoints(-100, -100);
    } else if (mode == ThermMode::SystemModeEnum::kCool) {
        TempMgr().SetSetpoints(-100, 0);
    } else if (mode == ThermMode::SystemModeEnum::kHeat) {
        TempMgr().SetSetpoints(0, -100);
    }
}


void AppTask::ActionCompleted(TemperatureManager::Action_t aAction) {
    if (aAction == TemperatureManager::COOL_SET_ACTION) {
        APP_DBG("Cooling setpoint completed");
    } else if (aAction == TemperatureManager::HEAT_SET_ACTION) {
        APP_DBG("Heating setpoint completed");
    } else if (aAction == TemperatureManager::MODE_SET_ACTION) {
        APP_DBG("Mode set completed");
    } else if (aAction == TemperatureManager::LOCAL_T_ACTION) {
        APP_DBG("Temperature set completed");
    }else if (aAction == TemperatureManager::OUTDOOR_T_ACTION) {
        APP_DBG("Outdoor temperature set completed");
    }

    UpdateTempGUI();
}

void AppTask::PostEvent(const AppEvent *aEvent) {
    if (sAppEventQueue != NULL) {
        if (!xQueueSend(sAppEventQueue, aEvent, 1)) {
            ChipLogError(NotSpecified, "Failed to post event to app task event queue");
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

/**
 * Update cluster status after application level changes
 */
void AppTask::UpdateClusterState(void) {
}

void AppTask::UpdateNvmEventHandler(AppEvent *aEvent) {
    uint8_t err = 0;

    if (sAppTask.mFunction == kFunction_SaveNvm) {
        err = NM_Dump();
        if (err == 0) {
            APP_DBG("SAVE NVM\n");
        } else {
            APP_DBG("Failed to SAVE NVM\n");
        }
    } else if (sAppTask.mFunction == kFunction_FactoryReset) {
        APP_DBG("FACTORY RESET\n");
        NM_ResetFactory();
    }
}

void AppTask::MatterEventHandler(const ChipDeviceEvent *event, intptr_t) {
    switch (event->Type) {
    case DeviceEventType::kServiceProvisioningChange: {
        sIsThreadProvisioned = event->ServiceProvisioningChange.IsServiceProvisioned;
        UpdateLCD();
        break;
    }

    case DeviceEventType::kThreadConnectivityChange: {
        sIsThreadEnabled = (event->ThreadConnectivityChange.Result == kConnectivity_Established);
        if (event->ThreadConnectivityChange.Result == kConnectivity_Established)
        {
       	    TempSensMgr().StartInternalMeasurement();
			TempSensMgr().StartExternalMeasurement();
        }
        UpdateLCD();
        break;
    }

    case DeviceEventType::kCHIPoBLEConnectionEstablished: {
        sHaveBLEConnections = true;
        APP_DBG("kCHIPoBLEConnectionEstablished\n");
        UpdateLCD();
        break;
    }

    case DeviceEventType::kCHIPoBLEConnectionClosed: {
        sHaveBLEConnections = false;
        APP_DBG("kCHIPoBLEConnectionClosed\n");
        UpdateLCD();
        if (sFabricNeedSaved) {
            AppEvent event;
            event.Type = AppEvent::kEventType_Timer;
            event.Handler = UpdateNvmEventHandler;
            sAppTask.mFunction = kFunction_SaveNvm;
            sAppTask.PostEvent(&event);
            sFabricNeedSaved = false;
        }
        break;
    }

    case DeviceEventType::kCommissioningComplete: {
        sFabricNeedSaved = true;
        sHaveFabric = true;
        
        if (sHaveBLEConnections == false) {
            sFabricNeedSaved = false; // put to false to avoid save in nvm 2 times
            AppEvent event;
            event.Type = AppEvent::kEventType_Timer;
            event.Handler = UpdateNvmEventHandler;
            sAppTask.mFunction = kFunction_SaveNvm;
            sAppTask.PostEvent(&event);
        }

        TempSensMgr().StartInternalMeasurement();
        TempSensMgr().StartExternalMeasurement();

        UpdateLCD();
        break;
    }
    case DeviceEventType::kFailSafeTimerExpired: {
        sFailCommissioning = true;
        UpdateLCD();
        break;
    }
    case DeviceEventType::kDnssdInitialized:

#if (OTA_SUPPORT == 1)
        InitializeOTARequestor();
#endif /* (OTA_SUPPORT == 1) */
        break;

    case DeviceEventType::kBindingsChangedViaCluster: {
        APP_DBG("kBindingsChangedViaCluster");
        AppEvent event;
        event.Type = AppEvent::kEventType_Timer;
        event.Handler = UpdateNvmEventHandler;
        sAppTask.mFunction = kFunction_SaveNvm;
        sAppTask.PostEvent(&event);
        break;
    }
    default:
        break;
    }
}


#if (CFG_LCD_SUPPORTED == 1)
void AppTask::UpdateLCD(void) {
    if (sIsThreadProvisioned && sIsThreadEnabled) {
        UTIL_LCD_DisplayStringAt(0, LINE(4), (uint8_t*) "Network Joined", LEFT_MODE);
    } else if ((sIsThreadProvisioned == false) || (sIsThreadEnabled == false)) {
    }
    if (sHaveBLEConnections) {
        BSP_LCD_Clear(LCD1, LCD_COLOR_BLACK);
        BSP_LCD_Refresh(LCD1);
		UTIL_LCD_DisplayStringAt(0, 0, (uint8_t*) "Matter Thermostat", CENTER_MODE);
        UTIL_LCD_DisplayStringAt(0, LINE(1), (uint8_t*) "BLE Connected", LEFT_MODE);
    }
    if (sHaveFabric) {
        UTIL_LCD_ClearStringLine(1);
        BSP_LCD_Refresh(LCD1);
        UTIL_LCD_DisplayStringAt(0, LINE(1), (uint8_t*) "Fabric Created", LEFT_MODE);
    }
    if (sFailCommissioning == true) {
        UTIL_LCD_DisplayStringAt(0, LINE(1), (uint8_t*) "", LEFT_MODE);
        BSP_LCD_Refresh(LCD1);
        UTIL_LCD_DisplayStringAt(0, LINE(1), (uint8_t*) "Fabric Failed", LEFT_MODE);
    }
    BSP_LCD_Refresh(LCD1);
}

void AppTask::UpdateTempGUI(void) {
	char Message[18];
	int8_t tempi, tempo, cool_set, heat_set;
	ThermMode::SystemModeEnum mode;

	tempi = TempMgr().GetCurrentTemp();
	tempo = TempMgr().GetOutdoorTemp();
	mode = TempMgr().GetMode();
	cool_set = TempMgr().GetCoolingSetPoint();
	heat_set = TempMgr().GetHeatingSetPoint();

	UTIL_LCD_ClearStringLine(1);
	UTIL_LCD_ClearStringLine(2);
	BSP_LCD_Refresh(LCD1);

	snprintf(Message, sizeof(Message), "Ti:%d,To:%d", tempi, tempo);
	UTIL_LCD_DisplayStringAt(0, LINE(1), (uint8_t*) Message, CENTER_MODE);
	snprintf(Message, sizeof(Message), "M:%d", mode);
	UTIL_LCD_DisplayStringAt(0, LINE(2), (uint8_t*) Message, CENTER_MODE);
	snprintf(Message, sizeof(Message), "H:%d,C:%d", heat_set, cool_set);
	UTIL_LCD_DisplayStringAt(0, LINE(3), (uint8_t*) Message, CENTER_MODE);
    BSP_LCD_Refresh(LCD1);
}
#else
void AppTask::UpdateLCD(void)
{
}
void AppTask::UpdateTempGUI(void) {}
#endif /* CFG_LCD_SUPPORTED == 1 */





