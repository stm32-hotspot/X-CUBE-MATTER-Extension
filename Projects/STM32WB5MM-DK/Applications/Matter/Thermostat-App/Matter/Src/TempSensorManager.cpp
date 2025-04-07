/**********************************************************
 * Includes
 *********************************************************/

#include "app_common.h"
#include "stm32wb5mm_dk_env_sensors.h"

#include "TempSensorManager.h"
#include "TemperatureManager.h"
#include <lib/support/logging/CHIPLogging.h>
#include <platform/CHIPDeviceLayer.h>

#include <app/ConcreteAttributePath.h>
#include "controller/ReadInteraction.h"

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

float sensor_temperature_float;
int16_t  sensor_temperature;
int16_t external_temperature;
bool external_read_ok = false;

CHIP_ERROR TempSensorManager::Init() {
	ChipLogProgress(NotSpecified, "TempSensorManager::Init");

	sTemperatureMeasureIntTimer = xTimerCreate("TemperatureMeasureIntTimer", // Just a text name, not used by the RTOS kernel
		pdMS_TO_TICKS(TEMP_MEAS_INT_PERIOD),          // == default timer period (mS)
		true,               //
		(void*) this,       // init timer id
		TimerTemperatureIntEventHandler // timer callback handler
		);

	sTemperatureMeasureExtTimer = xTimerCreate("TemperatureMeasureExtTimer", // Just a text name, not used by the RTOS kernel
		pdMS_TO_TICKS(TEMP_MEAS_EXT_PERIOD),          // == default timer period (mS)
		true,               //
		(void*) this,       // init timer id
		TimerTemperatureExtEventHandler // timer callback handler
		);

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
	xTimerStart(sTemperatureMeasureIntTimer, 0);
}

void TempSensorManager::StartExternalMeasurement() {
	isExternalActive = true;
	xTimerStart(sTemperatureMeasureExtTimer, 0);
}

void TempSensorManager::StopInternalMeasurement() {
	xTimerStop(sTemperatureMeasureIntTimer, 0);
	isInternalActive = false;
}

void TempSensorManager::StopExternalMeasurement() {
	xTimerStop(sTemperatureMeasureExtTimer, 0);
	isExternalActive = false;
}

bool TempSensorManager::IsInternalActive() {
	return isInternalActive;
}

bool TempSensorManager::IsExternalActive() {
	return isExternalActive;
}

void TempSensorManager::MeasureTemperature(bool internal_temp) {
	if (internal_temp)
	{
		if (BSP_ENV_SENSOR_GetValue(ENV_SENSOR_STTS22H_0, ENV_TEMPERATURE, &sensor_temperature_float) != BSP_ERROR_NONE)
		{
			TempMgr().SetIntReadOk(false);
			PlatformMgr().LockChipStack();
			ThermAttr::LocalTemperature::SetNull(kThermostatEndpoint);
			PlatformMgr().UnlockChipStack();
		}
		else
		{
			sensor_temperature = (int16_t) (sensor_temperature_float*100);
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


void TempSensorManager::TimerTemperatureIntEventHandler(TimerHandle_t xTimer) {
	TempSensMgr().MeasureTemperature(true);
}

void TempSensorManager::TimerTemperatureExtEventHandler(TimerHandle_t xTimer) {
	TempSensMgr().MeasureTemperature(false);
}

void TempSensorManager::TimerSetExtEventHandler(TimerHandle_t xTimer) {
	TempSensMgr().SetExternal();
}


