/**********************************************************
 * Includes
 *********************************************************/

#include "app_common.h"

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
int16_t sensor_demo_vals[6] = {2000, 2100, 2200, 2300, 2200, 2100};
bool isInternalActive;

const osThreadAttr_t TempMeasure_attr =
{
  .name = "temp_measure",
  .attr_bits = 0,
  .cb_mem = 0,
  .cb_size = 0,
  .stack_mem = 0,
  .stack_size = 1024,
  .priority =  osPriorityLow,
};

void TempSensorManager::TempMeasureTask(void * pvParameter) {
	while(1) {
		if (isInternalActive) TempSensMgr().MeasureTemperature();
		vTaskDelay(pdMS_TO_TICKS(TEMP_MEAS_INT_PERIOD));
	}
}

CHIP_ERROR TempSensorManager::Init() {
	ChipLogProgress(NotSpecified, "TempSensorManager::Init");

	osThreadNew(TempMeasureTask, NULL, &TempMeasure_attr);

	isInternalActive = false;

	return CHIP_NO_ERROR;
}

void TempSensorManager::StartInternalMeasurement() {
	isInternalActive = true;
}

void TempSensorManager::StopInternalMeasurement() {
	isInternalActive = false;
}

bool TempSensorManager::IsInternalActive() {
	return isInternalActive;
}

void TempSensorManager::MeasureTemperature() {
	static uint8_t demo_idx = 0;

	if (0) // if cannot read sensor
	{
		PlatformMgr().LockChipStack();
		TempAttr::MeasuredValue::SetNull(kEndpoint);
		PlatformMgr().UnlockChipStack();
	}
	else
	{
		sensor_temperature = sensor_demo_vals[demo_idx++];
		if (demo_idx == 6) demo_idx = 0;
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

