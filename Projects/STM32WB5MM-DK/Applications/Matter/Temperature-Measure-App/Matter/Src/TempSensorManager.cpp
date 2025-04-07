/**********************************************************
 * Includes
 *********************************************************/

#include "app_common.h"
#include "stm32wb5mm_dk_env_sensors.h"

#include "TempSensorManager.h"
#include <lib/support/logging/CHIPLogging.h>
#include <platform/CHIPDeviceLayer.h>

#include <app/ConcreteAttributePath.h>

/**********************************************************
 * Defines and Constants
 *********************************************************/
#define TEMP_MEAS_INT_PERIOD 15000

using namespace chip;
using namespace ::chip::DeviceLayer;
using namespace ::chip::app;
namespace TempAttr = chip::app::Clusters::TemperatureMeasurement::Attributes;

constexpr EndpointId kEndpoint = 1;
TimerHandle_t sTemperatureMeasureIntTimer;
TempSensorManager TempSensorManager::sTempSensMgr;

float sensor_temperature_float;
int16_t  sensor_temperature;


CHIP_ERROR TempSensorManager::Init() {
	ChipLogProgress(NotSpecified, "TempSensorManager::Init");

	BSP_ENV_SENSOR_Init(ENV_SENSOR_STTS22H_0, ENV_TEMPERATURE);
	BSP_ENV_SENSOR_Enable(ENV_SENSOR_STTS22H_0, ENV_TEMPERATURE);

	sTemperatureMeasureIntTimer = xTimerCreate("TemperatureMeasureIntTimer", // Just a text name, not used by the RTOS kernel
		pdMS_TO_TICKS(TEMP_MEAS_INT_PERIOD),          // == default timer period (mS)
		true,               //
		(void*) this,       // init timer id
		TimerTemperatureIntEventHandler // timer callback handler
		);

	isInternalActive = false;

	return CHIP_NO_ERROR;
}

void TempSensorManager::StartInternalMeasurement() {
	isInternalActive = true;
	xTimerStart(sTemperatureMeasureIntTimer, 0);
}

void TempSensorManager::StopInternalMeasurement() {
	xTimerStop(sTemperatureMeasureIntTimer, 0);
	isInternalActive = false;
}

bool TempSensorManager::IsInternalActive() {
	return isInternalActive;
}

void TempSensorManager::MeasureTemperature() {
		BSP_ENV_SENSOR_GetValue(ENV_SENSOR_STTS22H_0, ENV_TEMPERATURE, &sensor_temperature_float);

		sensor_temperature = (int16_t) (sensor_temperature_float*100);
		ChipLogProgress(NotSpecified, "Measured temperature %d", (int16_t) sensor_temperature);

		mCurrentTempCelsius = TempSensMgr().ConvertToPrintableTemp(sensor_temperature);

		PlatformMgr().LockChipStack();
		TempAttr::MeasuredValue::Set(kEndpoint, sensor_temperature);
		PlatformMgr().UnlockChipStack();

		if (mActionCompleted_CB) {
			Action_t aAction = LOCAL_T_ACTION;
			mActionCompleted_CB(aAction);
		}
}


void TempSensorManager::TimerTemperatureIntEventHandler(TimerHandle_t xTimer) {
	TempSensMgr().MeasureTemperature();
}

void TempSensorManager::SetCallbacks(TempMeasureCallback_fn aActionCompleted_CB)
{
	mActionCompleted_CB = aActionCompleted_CB;
}

int8_t TempSensorManager::GetCurrentTemp()
{
    return mCurrentTempCelsius;
}

int8_t TempSensorManager::ConvertToPrintableTemp(int16_t temperature)
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

