/**********************************************************
 * Includes
 *********************************************************/

#include "app_common.h"

#include "TempSensorManager.h"
#include "TemperatureManager.h"
#include <lib/support/logging/CHIPLogging.h>
#include <platform/CHIPDeviceLayer.h>

#include <app/ConcreteAttributePath.h>
#include "controller/ReadInteraction.h"
#include "stm32_rtos.h"

/**********************************************************
 * Defines and Constants
 *********************************************************/
#define TEMP_MEAS_INT_PERIOD 15000
#define TEMP_MEAS_EXT_PERIOD 15000
#define TEMP_SET_EXT_PERIOD 1000

using namespace chip;
using namespace ::chip::DeviceLayer;
using namespace ::chip::app;
namespace ThermAttr = chip::app::Clusters::Thermostat::Attributes;
namespace ExtTemp = chip::app::Clusters::TemperatureMeasurement::Attributes;

constexpr EndpointId kThermostatEndpoint = 1;
TimerHandle_t sTemperatureMeasureIntTimer, sTemperatureMeasureExtTimer, sTemperatureSetExtTimer;
TempSensorManager TempSensorManager::sTempSensMgr;

int16_t sensor_demo_vals[6] = {2000, 2100, 2200, 2300, 2200, 2100};

int16_t  sensor_temperature;
int16_t external_temperature;
bool external_read_ok = false;
bool isInternalActive, isExternalActive;

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
		if (isInternalActive) TempSensMgr().MeasureTemperature(true);
		if (isExternalActive) TempSensMgr().MeasureTemperature(false);
		vTaskDelay(pdMS_TO_TICKS(TEMP_MEAS_INT_PERIOD));
	}
}

CHIP_ERROR TempSensorManager::Init() {
	ChipLogProgress(NotSpecified, "TempSensorManager::Init");

	osThreadNew(TempMeasureTask, NULL, &TempMeasure_attr);

	sTemperatureSetExtTimer = xTimerCreate("TemperatureSetExtTimer", // Just a text name, not used by the RTOS kernel
		pdMS_TO_TICKS(TEMP_SET_EXT_PERIOD),          // == default timer period (mS)
		false,               //
		(void*) this,       // init timer id
		TimerSetExtEventHandler // timer callback handler
		);

	return CHIP_NO_ERROR;
}

void TempSensorManager::StartInternalMeasurement() {
	isInternalActive = true;
}

void TempSensorManager::StartExternalMeasurement() {
	isExternalActive = true;
}

void TempSensorManager::StopInternalMeasurement() {
	isInternalActive = false;
}

void TempSensorManager::StopExternalMeasurement() {
	isExternalActive = false;
}

bool TempSensorManager::IsInternalActive() {
	return isInternalActive;
}

bool TempSensorManager::IsExternalActive() {
	return isExternalActive;
}

void TempSensorManager::MeasureTemperature(bool internal_temp) {
	static uint8_t demo_idx = 0;
	if (internal_temp)
	{
		if (0) // if cannot read sensor
		{
			TempMgr().SetIntReadOk(false);
			PlatformMgr().LockChipStack();
			ThermAttr::LocalTemperature::SetNull(kThermostatEndpoint);
			PlatformMgr().UnlockChipStack();
		}
		else
		{
			sensor_temperature = sensor_demo_vals[demo_idx++];
			if (demo_idx == 6) demo_idx = 0;
			ChipLogProgress(NotSpecified, "Measured temperature %d", (int16_t) sensor_temperature);

			TempMgr().SetIntReadOk(true);
			PlatformMgr().LockChipStack();
			ThermAttr::LocalTemperature::Set(kThermostatEndpoint, sensor_temperature);
			PlatformMgr().UnlockChipStack();
		}
	}
	else
	{
		BindingCommandData *data = Platform::New<BindingCommandData>();
		data->clusterId = app::Clusters::TemperatureMeasurement::Id;
		data->localEndpointId = kThermostatEndpoint;
		data->InvokeCommandFunc = ExternalTemperatureMeasurementReadHandler;
		DeviceLayer::PlatformMgr().ScheduleWork(TempSensorWorkerFunction, reinterpret_cast<intptr_t>(data));
	}
}

void TempSensorManager::SetExternal()
{
	PlatformMgr().LockChipStack();
	if (external_read_ok) ThermAttr::OutdoorTemperature::Set(kThermostatEndpoint, external_temperature);
	else ThermAttr::OutdoorTemperature::SetNull(kThermostatEndpoint);
	PlatformMgr().UnlockChipStack();
}

void TempSensorManager::ExternalTemperatureMeasurementReadHandler(const EmberBindingTableEntry &binding,
								  OperationalDeviceProxy *deviceProxy, BindingCommandData &bindingData)
{
	auto onSuccess = [](const ConcreteDataAttributePath &attributePath, const auto &dataResponse) {
		ChipLogProgress(NotSpecified, "Read Temperature Sensor attribute succeeded");

		VerifyOrReturn(!(dataResponse.IsNull()), ChipLogError(NotSpecified, "dataResponse.IsNull"););

		external_temperature = dataResponse.Value();
		ChipLogProgress(NotSpecified, "Read Temperature Sensor value %d", external_temperature);

		TempMgr().SetExtReadOk(true);
		external_read_ok = true;
		xTimerStart(sTemperatureSetExtTimer, 0);
	};

	auto onFailure = [](const ConcreteDataAttributePath *attributePath, CHIP_ERROR error) {
		ChipLogError(NotSpecified, "Read Temperature Sensor attribute failed: %" CHIP_ERROR_FORMAT,error.Format());

		TempMgr().SetExtReadOk(false);
		external_read_ok = false;
		xTimerStart(sTemperatureSetExtTimer, 0);
	};

	ChipLogProgress(NotSpecified, "ExternalTemperatureMeasurementReadHandler");

	VerifyOrReturn(deviceProxy != nullptr && deviceProxy->ConnectionReady(), ChipLogError(NotSpecified, "Device invalid"));



	Controller::ReadAttribute<app::Clusters::TemperatureMeasurement::Attributes::MeasuredValue::TypeInfo>(
		deviceProxy->GetExchangeManager(), deviceProxy->GetSecureSession().Value(), binding.remote, onSuccess,
		onFailure);
}

void TempSensorManager::TimerSetExtEventHandler(TimerHandle_t xTimer) {
	TempSensMgr().SetExternal();
}


