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

class TempSensorManager
{
public:
	enum Action_t
	{
		INVALID_ACTION = 0,
		LOCAL_T_ACTION
	} Action;

    CHIP_ERROR Init();

    static void TempMeasureTask(void * pvParameter);
    
    void StartInternalMeasurement();
    void StopInternalMeasurement();
    
    bool IsInternalActive();
    
    void MeasureTemperature();

    int8_t GetCurrentTemp();

    using TempMeasureCallback_fn = std::function<void(Action_t)>;
    void SetCallbacks(TempMeasureCallback_fn aActionCompleted_CB);

private:
    friend TempSensorManager & TempSensMgr();
    
    static void TimerTemperatureIntEventHandler(TimerHandle_t xTimer);
    
    int8_t ConvertToPrintableTemp(int16_t temperature);
    
    int8_t mCurrentTempCelsius;

    TempMeasureCallback_fn mActionCompleted_CB;

    static TempSensorManager sTempSensMgr;
};

inline TempSensorManager & TempSensMgr()
{
    return TempSensorManager::sTempSensMgr;
}
