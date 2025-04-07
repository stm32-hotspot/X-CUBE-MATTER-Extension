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
#include "app_entry.h"
#include "app_thread.h"
#include "stm32_lpm.h"
#include "dbg_trace.h"
#include "cmsis_os.h"
#include "AppEvent.h"
#include "AppTask.h"
#include "flash_wb.h"

#ifdef USE_STM32WB5M_DK
#if (CFG_LCD_SUPPORTED == 1)
#include "stm32wb5mm_dk_lcd.h"
#include "stm32_lcd.h"
#include "ssd1315.h"
#endif /* (CFG_LCD_SUPPORTED == 1) */
#endif /* USE_STM32WB5M_DK */

#if (OTA_SUPPORT == 1)
#include "ota.h"
#endif /* (OTA_SUPPORT == 1) */

#if HIGHWATERMARK
#include "memory_buffer_alloc.h"
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

#include "qr_code.h"

#if CHIP_ENABLE_OPENTHREAD
#include <platform/OpenThread/OpenThreadUtils.h>
#include <platform/ThreadStackManager.h>
#endif

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

#ifdef USE_STM32WB5M_DK
#define APP_FUNCTION_BUTTON1 BUTTON_USER1
#define APP_FUNCTION_BUTTON2 BUTTON_USER2
#endif /* USE_STM32WB5M_DK */

#ifdef USE_STM32WBXX_NUCLEO
#define APP_FUNCTION_BUTTON1 BUTTON_SW1
#define APP_FUNCTION_BUTTON2 BUTTON_SW2
#endif /* USE_STM32WBXX_NUCLEO */


#define APP_EVENT_QUEUE_SIZE 10
#define NVM_TIMEOUT 1000  // timer to handle PB to save data in nvm or do a factory reset
#define BUTTON_TIMEOUT 1000  // timer to handle short and long press for command buttons

static QueueHandle_t sAppEventQueue;
TimerHandle_t sPushButtonTimeoutTimer;
TimerHandle_t sPushButton2TimeoutTimer;
const osThreadAttr_t AppTask_attr = { .name = APPTASK_NAME, .attr_bits =
APP_ATTR_BITS, .cb_mem = APP_CB_MEM, .cb_size = APP_CB_SIZE, .stack_mem =
APP_STACK_MEM, .stack_size = APP_STACK_SIZE, .priority =
APP_PRIORITY };

static bool sIsThreadProvisioned = false;
static bool sIsThreadEnabled = false;
static bool sHaveBLEConnections = false;
static bool sFabricNeedSaved = false;
static bool sFailCommissioning = false;
static bool sHaveFabric = false;
static uint8_t NvmTimerCpt = 0;
static uint8_t NvmButtonStateCpt = 0;
static uint8_t B2TimerCpt = 0;
static uint8_t B2ButtonStateCpt = 0;

#define B2TIMERCPLT_LIMIT 10 // 3
#undef BUTTON_TIMEOUT
#define BUTTON_TIMEOUT 200  // 1000

chip::DeviceLayer::DeviceInfoProviderImpl gExampleDeviceInfoProvider;

CHIP_ERROR AppTask::StartAppTask() {
    sAppEventQueue = xQueueCreate(APP_EVENT_QUEUE_SIZE, sizeof(AppEvent));
    if (sAppEventQueue == NULL) {
        APP_DBG("Failed to allocate app event queue");
        return CHIP_ERROR_NO_MEMORY;
    }

    // Start App task.
    osThreadNew(AppTaskMain, NULL, &AppTask_attr);

    return CHIP_NO_ERROR;
}

void LockOpenThreadTask(void) {
    chip::DeviceLayer::ThreadStackMgr().LockThreadStack();
}

void UnlockOpenThreadTask(void) {
    chip::DeviceLayer::ThreadStackMgr().UnlockThreadStack();
}

CHIP_ERROR AppTask::Init() {

    CHIP_ERROR err = CHIP_NO_ERROR;

    // Setup button handler
    APP_ENTRY_PBSetReceiveCallback(ButtonEventHandler);

    // Create FreeRTOS sw timer for Push button timeouts.
    sPushButtonTimeoutTimer = xTimerCreate("PushButtonTimer", // Just a text name, not used by the RTOS kernel
            pdMS_TO_TICKS(NVM_TIMEOUT),          // == default timer period (mS)
            true,               // no timer reload (==one-shot)
            (void*) this,       // init timer id
            TimerEventHandler // timer callback handler
            );

    // Create FreeRTOS sw timer for button 2 timeouts.
    sPushButton2TimeoutTimer = xTimerCreate("PushCommandButtonTimer", // Just a text name, not used by the RTOS kernel
            pdMS_TO_TICKS(BUTTON_TIMEOUT),          // == default timer period (mS)
            true,               // no timer reload (==one-shot)
            (void*) this,       // init timer id
            TimerCommandEventHandler // timer callback handler
            );


    ThreadStackMgr().InitThreadStack();

#if ( CHIP_DEVICE_CONFIG_ENABLE_SED == 1)
    ConnectivityMgr().SetThreadDeviceType(ConnectivityManager::kThreadDeviceType_SleepyEndDevice);
#else
    ConnectivityMgr().SetThreadDeviceType(ConnectivityManager::kThreadDeviceType_Router);
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

    ConfigurationMgr().LogDeviceConfig();

    err = BoltLockMgr().Init();
	if (err != CHIP_NO_ERROR) {
		APP_DBG("BoltLockMgr().Init() failed");
		return err;
	}
	BoltLockMgr().SetCallbacks(ActionInitiated, ActionCompleted);

    // Open commissioning after boot if no fabric was available
    if (chip::Server::GetInstance().GetFabricTable().FabricCount() == 0) {
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
		UTIL_LCD_DisplayStringAt(70, 12, (uint8_t*) "Lock", LEFT_MODE);
		UTIL_LCD_DisplayStringAt(70, 24, (uint8_t*) "App", LEFT_MODE);
		BSP_LCD_Refresh(0);

        PrintOnboardingCodes(
                chip::RendezvousInformationFlags(chip::RendezvousInformationFlag::kBLE));
        // Enable BLE advertisements
        chip::Server::GetInstance().GetCommissioningWindowManager().OpenBasicCommissioningWindow();
        APP_DBG("BLE advertising started. Waiting for Pairing.");
    } else {  // try to attach to the thread network
        sHaveFabric = true;
    }

    err = PlatformMgr().StartEventLoopTask();
    if (err != CHIP_NO_ERROR) {
        APP_DBG("PlatformMgr().StartEventLoopTask() failed");
    }

    return err;
}

CHIP_ERROR AppTask::InitMatter() {
    CHIP_ERROR err = CHIP_NO_ERROR;

    err = chip::Platform::MemoryInit();
    if (err != CHIP_NO_ERROR) {
        APP_DBG("Platform::MemoryInit() failed");
    } else {
        APP_DBG("Init CHIP stack");        
        err = PlatformMgr().InitChipStack();
        if (err != CHIP_NO_ERROR) {
            APP_DBG("PlatformMgr().InitChipStack() failed");
        }
    }

    APPE_Lcd_Init();
#ifdef USE_STM32WB5M_DK
#if (CFG_LCD_SUPPORTED == 1)
    if (sHaveFabric)
       {
       char Message[20];
              snprintf(Message, sizeof(Message), "Fabric Found: %d",
                      chip::Server::GetInstance().GetFabricTable().FabricCount());
              UTIL_LCD_DisplayStringAt(0, LINE(1), (uint8_t*) Message, LEFT_MODE);
              BSP_LCD_Refresh(0);
       }
#endif /* (CFG_LCD_SUPPORTED == 1) */
#endif /* USE_STM32WB5M_DK */

    return err;
}

void AppTask::AppTaskMain(void *pvParameter) {
    AppEvent event;

    CHIP_ERROR err = sAppTask.Init();
#if HIGHWATERMARK
	UBaseType_t uxHighWaterMark;
	HeapStats_t HeapStatsInfo;
	size_t max_used;
	size_t max_blocks;
#endif // endif HIGHWATERMARK
    if (err != CHIP_NO_ERROR) {
        APP_DBG("App task init failed ");
    }

    APP_DBG("App Task started");
    while (true) {

        BaseType_t eventReceived = xQueueReceive(sAppEventQueue, &event, portMAX_DELAY);
        while (eventReceived == pdTRUE) {
            sAppTask.DispatchEvent(&event);
            eventReceived = xQueueReceive(sAppEventQueue, &event, 0);
        }
#if HIGHWATERMARK
		uxHighWaterMark = uxTaskGetStackHighWaterMark( NULL );
		vPortGetHeapStats(&HeapStatsInfo);
		mbedtls_memory_buffer_alloc_max_get(&max_used, &max_blocks );

#endif // endif HIGHWATERMARK
    }

}

void AppTask::ButtonEventHandler(Push_Button_st *Button) {

    AppEvent button_event = { };
    button_event.Type = AppEvent::kEventType_Button;
    button_event.ButtonEvent.ButtonIdx = Button->Pushed_Button;
    button_event.ButtonEvent.Action = Button->State;

    if (Button->Pushed_Button == APP_FUNCTION_BUTTON1) {
        // Hand off to Functionality handler - depends on duration of press
        button_event.Handler = FunctionHandler;
    }
    else if (Button->Pushed_Button == APP_FUNCTION_BUTTON2) {
        button_event.Handler = UpFunctionHandler;
    } else {
        return;
    }

    sAppTask.PostEvent(&button_event);
}

void AppTask::TimerEventHandler(TimerHandle_t xTimer) {

    NvmTimerCpt++;
    if (BSP_PB_GetState(APP_FUNCTION_BUTTON1) == 0) {
        NvmButtonStateCpt++;
    }
    if (NvmTimerCpt >= 10) {
        xTimerStop(sPushButtonTimeoutTimer, 0);
        if (NvmButtonStateCpt >= 9) {
            AppEvent event;
            event.Type = AppEvent::kEventType_Timer;
            event.Handler = UpdateNvmEventHandler;
            sAppTask.mFunction = kFunction_FactoryReset;
            sAppTask.PostEvent(&event);
        }
    } else if ((NvmTimerCpt > NvmButtonStateCpt) && (NvmTimerCpt <= 2)) {
        xTimerStop(sPushButtonTimeoutTimer, 0);
        if (sHaveFabric == true)
        {
            AppEvent event;
            event.Type = AppEvent::kEventType_Timer;
            event.Handler = UpdateNvmEventHandler;  
            sAppTask.mFunction = kFunction_SaveNvm;
            sAppTask.PostEvent(&event);                  
        }
    }
}

void AppTask::TimerCommandEventHandler(TimerHandle_t xTimer) {

    B2TimerCpt++;
    if (BSP_PB_GetState(BUTTON_USER2) == 0) {
        if (++B2ButtonStateCpt == B2TIMERCPLT_LIMIT) {
            AppEvent event;
            event.Type = AppEvent::kEventType_Timer;
            event.Handler = ButtonRelease;
            sAppTask.mFunction = kFunction_LongPress;
            sAppTask.PostEvent(&event);
        }
    } else if (B2ButtonStateCpt > 2) {
        xTimerStop(sPushButton2TimeoutTimer, 0);
        if (sHaveFabric == true) {
            AppEvent event;
            event.Type = AppEvent::kEventType_Timer;
            event.Handler = ButtonRelease;
            sAppTask.mFunction = kFunction_LongRelease;
            sAppTask.PostEvent(&event);
        }
    } else if ((B2TimerCpt > B2ButtonStateCpt) && (B2TimerCpt <= B2TIMERCPLT_LIMIT)) {
        xTimerStop(sPushButton2TimeoutTimer, 0);
        if (sHaveFabric == true) {
            AppEvent event;
            event.Type = AppEvent::kEventType_Timer;
            event.Handler = ButtonRelease;
            sAppTask.mFunction = kFunction_ShortRelease;
            sAppTask.PostEvent(&event);
        }
    }
}


void AppTask::UpFunctionHandler(AppEvent *aEvent) {
    if (xTimerIsTimerActive(sPushButton2TimeoutTimer) == 0) {
        xTimerStart(sPushButton2TimeoutTimer, 0);
        B2TimerCpt = 0;
        B2ButtonStateCpt = 0;
    }
}

void AppTask::FunctionHandler(AppEvent *aEvent) {
    if (xTimerIsTimerActive(sPushButtonTimeoutTimer) == 0) {
        xTimerStart(sPushButtonTimeoutTimer, 0);
        NvmTimerCpt = 0;
        NvmButtonStateCpt = 0;
    }
}

void AppTask::ActionInitiated(BoltLockManager::Action_t aAction, int32_t aActor) {
    if (aAction == BoltLockManager::LOCK_ACTION) {
        APP_DBG("Locking initiated");
    } else if (aAction == BoltLockManager::UNLOCK_ACTION) {
        APP_DBG("Unlocking initiated");
    }

   if (aActor == AppEvent::kEventType_Button || aActor == AppEvent::kEventType_Lock)
   {
        sAppTask.mSyncClusterToButtonAction = true;
   }
}

void AppTask::ActionCompleted(BoltLockManager::Action_t aAction) {
    if (aAction == BoltLockManager::LOCK_ACTION) {
        APP_DBG("Locking completed");
#if (CFG_LCD_SUPPORTED == 1)
        UTIL_LCD_DisplayStringAt(0, 48, (uint8_t*) "Locked  ", LEFT_MODE);
        BSP_LCD_Refresh(0);
#endif
    } else if (aAction == BoltLockManager::UNLOCK_ACTION) {
        APP_DBG("Unlocking completed");
#if (CFG_LCD_SUPPORTED == 1)
        UTIL_LCD_DisplayStringAt(0, 48, (uint8_t*) "Unlocked", LEFT_MODE);
        BSP_LCD_Refresh(0);
#endif
    }
    if (sAppTask.mSyncClusterToButtonAction) {
        sAppTask.UpdateClusterState();
        sAppTask.mSyncClusterToButtonAction = false;
    }
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
 * Update cluster status after application changes
 */
void AppTask::UpdateClusterState(void) {
    Protocols::InteractionModel::Status status;

    bool unlocked        = BoltLockMgr().IsUnlocked();
    DlLockState newState = unlocked ? DlLockState::kUnlocked : DlLockState::kLocked;

	PlatformMgr().LockChipStack();

    status = DoorLockServer::Instance().SetLockState(1, newState)
        ? Protocols::InteractionModel::Status::Success
        : Protocols::InteractionModel::Status::Failure;

    if (status != Protocols::InteractionModel::Status::Success)
    {
        ChipLogError(NotSpecified, "ERR: updating lock state %d return %x", newState, to_underlying(status));
    }

	PlatformMgr().UnlockChipStack();
}

void AppTask::DelayNvmHandler(void) {
#ifdef USE_STM32WB5M_DK    
    AppEvent event;
    event.Type = AppEvent::kEventType_Timer;
    event.Handler = UpdateNvmEventHandler;
    sAppTask.mFunction = kFunction_SaveNvm;
    sAppTask.PostEvent(&event);
#endif /* USE_STM32WB5M_DK */    
}

#ifdef USE_STM32WB5M_DK
#if (CFG_LCD_SUPPORTED == 1)
void AppTask::UpdateLCD(void) {
    if (sIsThreadProvisioned && sIsThreadEnabled) {
        UTIL_LCD_DisplayStringAt(0, LINE(4), (uint8_t*) "Network Joined", LEFT_MODE);
    } else if ((sIsThreadProvisioned == false) || (sIsThreadEnabled == false)) {
    }
    if (sHaveBLEConnections) {
        BSP_LCD_Clear(0, SSD1315_COLOR_BLACK);
        BSP_LCD_Refresh(0);
        UTIL_LCD_DisplayStringAt(0, LINE(1), (uint8_t*) "BLE Connected", LEFT_MODE);
    }
    if (sHaveFabric) {
        UTIL_LCD_ClearStringLine(1);
        BSP_LCD_Refresh(0);
        UTIL_LCD_DisplayStringAt(0, LINE(1), (uint8_t*) "Fabric Created", LEFT_MODE);
    }
    if (sFailCommissioning == true) {
        UTIL_LCD_DisplayStringAt(0, LINE(1), (uint8_t*) "", LEFT_MODE);
        BSP_LCD_Refresh(0);
        UTIL_LCD_DisplayStringAt(0, LINE(1), (uint8_t*) "Fabric Failed", LEFT_MODE);
    }
    BSP_LCD_Refresh(0);
}
#else
void AppTask::UpdateLCD(void) 
{
}
#endif /* (CFG_LCD_SUPPORTED == 1) */
void AppTask::UpdateLEDs(void) 
{
    /* nothing for DK */
}

#endif /* USE_STM32WB5M_DK */

#ifdef USE_STM32WBXX_NUCLEO
void AppTask::UpdateLCD(void) 
{
    /* nothing for Nucleo */
}
void AppTask::UpdateLEDs(void) {
    if (sIsThreadProvisioned && sIsThreadEnabled) {
        APP_ENTRY_LedBlink(LED2, 1);
    } else if ((sIsThreadProvisioned == false) || (sIsThreadEnabled == false)) {
        APP_ENTRY_LedBlink(LED2, 0);
    }
    if (sHaveBLEConnections) {
        APP_ENTRY_LedBlink(LED1, 1);
    } else if (sHaveBLEConnections == false) {
        APP_ENTRY_LedBlink(LED1, 0);
    }
}
#endif /* USE_STM32WBXX_NUCLEO */

void AppTask::UpdateNvmEventHandler(AppEvent *aEvent) {
    uint8_t err = 0;

    if (sAppTask.mFunction == kFunction_SaveNvm) {
        err = NM_Dump();
        if (err == 0) {
            APP_DBG("SAVE NVM");
        } else {
            APP_DBG("Failed to SAVE NVM");
        }
    } else if (sAppTask.mFunction == kFunction_FactoryReset) {
        APP_DBG("FACTORY RESET");
        NM_ResetFactory();
    }
}

#if (CFG_RGB_LED_SUPPORTED == 1U)
void AppTask::UpdateRgbLed(uint8_t led_r, uint8_t led_g, uint8_t led_b)
{
    aPwmLedGsData_TypeDef  aPwmLedGsData;
    aPwmLedGsData[PWM_LED_RED] = led_r;
    aPwmLedGsData[PWM_LED_GREEN] = led_g;
    aPwmLedGsData[PWM_LED_BLUE] = led_b;
    LL_GPIO_SetPinMode(GPIOA, LL_GPIO_PIN_7, LL_GPIO_MODE_OUTPUT);
    BSP_PWM_LED_On(aPwmLedGsData);
    LL_GPIO_SetPinMode(GPIOA, LL_GPIO_PIN_7, LL_GPIO_MODE_ALTERNATE);
}
#endif

void AppTask::ButtonRelease(AppEvent *aEvent) {
    Protocols::InteractionModel::Status status;

    bool unlocked        = BoltLockMgr().IsUnlocked();
    DlLockState newState = unlocked ? DlLockState::kLocked : DlLockState::kUnlocked;

	PlatformMgr().LockChipStack();

    status = DoorLockServer::Instance().SetLockState(1, newState)
        ? Protocols::InteractionModel::Status::Success
        : Protocols::InteractionModel::Status::Failure;

    if (status != Protocols::InteractionModel::Status::Success)
    {
        APP_DBG("ERR: updating lock state %x", to_underlying(status));
    }

	PlatformMgr().UnlockChipStack();
}


void AppTask::MatterEventHandler(const ChipDeviceEvent *event, intptr_t) {
    switch (event->Type) {
    case DeviceEventType::kServiceProvisioningChange: {
        sIsThreadProvisioned = event->ServiceProvisioningChange.IsServiceProvisioned;
        UpdateLCD();
        UpdateLEDs();        
        break;
    }

    case DeviceEventType::kThreadConnectivityChange: {
        sIsThreadEnabled = (event->ThreadConnectivityChange.Result == kConnectivity_Established);
        UpdateLCD();
        UpdateLEDs();            
        break;
    }

    case DeviceEventType::kCHIPoBLEConnectionEstablished: {
        sHaveBLEConnections = true;
        APP_DBG("kCHIPoBLEConnectionEstablished");
        UpdateLCD();
        UpdateLEDs();            
        break;
    }

    case DeviceEventType::kCHIPoBLEConnectionClosed: {
        sHaveBLEConnections = false;
        APP_DBG("kCHIPoBLEConnectionClosed");
        UpdateLCD();
        UpdateLEDs();            
        if (sFabricNeedSaved) {
            APP_DBG("Start timer to save nvm after commissioning finish");
            DelayNvmHandler();
            sFabricNeedSaved = false;
        }
        break;
    }

    case DeviceEventType::kCommissioningComplete: {
        sFabricNeedSaved = true;
        sHaveFabric = true;
        // check if ble is on, since before save in nvm we need to stop m0, Better to write in nvm when m0 is less busy
        if (sHaveBLEConnections == false) {
            APP_DBG("Start timer to save nvm after commissioning finish");
            DelayNvmHandler();
            sFabricNeedSaved = false; // put to false to avoid save in nvm 2 times
        }
        UpdateLCD();
        break;
    }
    case DeviceEventType::kFailSafeTimerExpired: {
        UpdateLCD();
        sFailCommissioning = true;
        break;
    }

    case DeviceEventType::kDnssdInitialized:
#if (OTA_SUPPORT == 1)
        InitializeOTARequestor();
#endif
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

