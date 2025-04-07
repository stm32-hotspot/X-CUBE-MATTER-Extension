#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "AppEvent.h"

#include "FreeRTOS.h"
#include "timers.h" // provides FreeRTOS timer support
#include <app-common/zap-generated/attributes/Accessors.h>

#include "BindingHandler.h"

#include <lib/core/CHIPError.h>

using namespace chip;

class TempSensorManager
{
public:

    CHIP_ERROR Init();

    static void TempMeasureTask(void * pvParameter);
    
    void StartInternalMeasurement();
    void StartExternalMeasurement();
    void StopInternalMeasurement();
    void StopExternalMeasurement();
    
    bool IsInternalActive();
    bool IsExternalActive();
    
    void MeasureTemperature(bool internal_temp);

private:
    friend TempSensorManager & TempSensMgr();
    
    static void TimerTemperatureIntEventHandler(TimerHandle_t xTimer);
    static void TimerTemperatureExtEventHandler(TimerHandle_t xTimer);
    static void TimerSetExtEventHandler(TimerHandle_t xTimer);
    
    static void SetExternal();

    static void ExternalTemperatureMeasurementReadHandler(const EmberBindingTableEntry &binding,
    			  OperationalDeviceProxy *deviceProxy, BindingCommandData &bindingData);
    
    static TempSensorManager sTempSensMgr;
};

inline TempSensorManager & TempSensMgr()
{
    return TempSensorManager::sTempSensMgr;
}
