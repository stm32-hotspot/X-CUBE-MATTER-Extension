/*
 *
 *    Copyright (c) 2021 Project CHIP Authors
 *    Copyright (c) 2019 Google LLC.
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

#include "SmartPlugManager.h"
#include <lib/support/logging/CHIPLogging.h>
#include "stm32wba65i_discovery.h"

#include <ElectricalPowerMeasurementDelegate.h>
#include <PowerTopologyDelegate.h>
#include <EnergyTimeUtils.h>

#include <platform/CHIPDeviceLayer.h>
#include <app/server/Server.h>
#include <app-common/zap-generated/ids/Attributes.h>
#include <app-common/zap-generated/ids/Clusters.h>
#include <app/ConcreteAttributePath.h>
#include <app/clusters/electrical-energy-measurement-server/electrical-energy-measurement-server.h>
#include <app/clusters/power-topology-server/power-topology-server.h>

#include "cmsis_os2.h"

SmartPlugManager SmartPlugManager::sPlug;

using namespace chip;
using namespace chip::app;
using namespace ::chip::Platform;
using namespace chip::app::DataModel;
using namespace chip::app::Clusters;
using namespace chip::app::Clusters::ElectricalPowerMeasurement;
using namespace chip::app::Clusters::ElectricalEnergyMeasurement;
using namespace chip::app::Clusters::PowerTopology;
using namespace chip::app::Clusters::DeviceEnergyManagement;

using namespace chip::app::Clusters::ElectricalEnergyMeasurement::Structs;

static std::unique_ptr<PowerTopologyDelegate> gPTDelegate;
static std::unique_ptr<PowerTopologyInstance> gPTInstance;
static std::unique_ptr<ElectricalPowerMeasurementDelegate> gEPMDelegate;
static std::unique_ptr<ElectricalPowerMeasurementInstance> gEPMInstance;
static std::unique_ptr<ElectricalEnergyMeasurementAttrAccess> gEEMAttrAccess;

#define ENERGY_MEAS_ENDPOINT 2

int64_t power_demo_vals[6] = {40'000, 50'000, 60'000, 64'000, 60'000, 50'000, };
int64_t current_demo_vals[6] = {20, 25, 30, 32, 30, 25};
int64_t voltage_demo_vals[6] = {2000, 2000, 2000, 2000, 2000, 2000};
int64_t power_demo_actual = 0;
int64_t current_demo_actual = 0;
int64_t voltage_demo_actual = 0;
int64_t energy_demo_actual = 0;
bool isMeasureActive = false;
bool sendLastZeroMeasure = false;
#define POWER_MEASURE_PERIOD 5000
#define ENERGY_USED_STEP 1000

/*
 *  @brief  Creates a Delegate and Instance for PowerTopology clusters
 *
 * The Instance is a container around the Delegate, so
 * create the Delegate first, then wrap it in the Instance
 * Then call the Instance->Init() to register the attribute and command handlers
 */
CHIP_ERROR SmartPlugManager::PowerTopologyInit()
{
    CHIP_ERROR err;

    if (gPTDelegate || gPTInstance)
    {
        ChipLogError(AppServer, "PowerTopology Instance or Delegate already exist.");
        return CHIP_ERROR_INCORRECT_STATE;
    }

    gPTDelegate = std::make_unique<PowerTopologyDelegate>();
    if (!gPTDelegate)
    {
        ChipLogError(AppServer, "Failed to allocate memory for PowerTopology Delegate");
        return CHIP_ERROR_NO_MEMORY;
    }

    gPTInstance =
        std::make_unique<PowerTopologyInstance>(EndpointId(ENERGY_MEAS_ENDPOINT), *gPTDelegate,
                                                BitMask<PowerTopology::Feature, uint32_t>(PowerTopology::Feature::kNodeTopology),
                                                BitMask<PowerTopology::OptionalAttributes, uint32_t>(0));

    if (!gPTInstance)
    {
        ChipLogError(AppServer, "Failed to allocate memory for PowerTopology Instance");
        gPTDelegate.reset();
        return CHIP_ERROR_NO_MEMORY;
    }

    err = gPTInstance->Init(); /* Register Attribute & Command handlers */
    if (err != CHIP_NO_ERROR)
    {
        ChipLogError(AppServer, "Init failed on gPTInstance");
        gPTInstance.reset();
        gPTDelegate.reset();
        return err;
    }

    return CHIP_NO_ERROR;
}

CHIP_ERROR SmartPlugManager::PowerTopologyShutdown()
{
    /* Do this in the order Instance first, then delegate
     * Ensure we call the Instance->Shutdown to free attribute & command handlers first
     */
    if (gPTInstance)
    {
        /* deregister attribute & command handlers */
        gPTInstance->Shutdown();
        gPTInstance.reset();
    }

    if (gPTDelegate)
    {
        gPTDelegate.reset();
    }

    return CHIP_NO_ERROR;
}

/*
 *  @brief  Creates a Delegate and Instance for Electrical Power/Energy Measurement clusters
 *
 * The Instance is a container around the Delegate, so
 * create the Delegate first, then wrap it in the Instance
 * Then call the Instance->Init() to register the attribute and command handlers
 */
CHIP_ERROR SmartPlugManager::EnergyMeterInit()
{
    CHIP_ERROR err;

    if (gEPMDelegate || gEPMInstance)
    {
        ChipLogError(AppServer, "EPM Instance or Delegate already exist.");
        return CHIP_ERROR_INCORRECT_STATE;
    }

    gEPMDelegate = std::make_unique<ElectricalPowerMeasurementDelegate>();
    if (!gEPMDelegate)
    {
        ChipLogError(AppServer, "Failed to allocate memory for EPM Delegate");
        return CHIP_ERROR_NO_MEMORY;
    }

    /* Manufacturer may optionally not support all features, commands & attributes */
    /* Turning on all optional features and attributes for test certification purposes */
    gEPMInstance = std::make_unique<ElectricalPowerMeasurementInstance>(
        EndpointId(ENERGY_MEAS_ENDPOINT), *gEPMDelegate,
        BitMask<ElectricalPowerMeasurement::Feature, uint32_t>(
            ElectricalPowerMeasurement::Feature::kDirectCurrent, ElectricalPowerMeasurement::Feature::kAlternatingCurrent,
            ElectricalPowerMeasurement::Feature::kPolyphasePower, ElectricalPowerMeasurement::Feature::kHarmonics,
            ElectricalPowerMeasurement::Feature::kPowerQuality),
        BitMask<ElectricalPowerMeasurement::OptionalAttributes, uint32_t>(
            ElectricalPowerMeasurement::OptionalAttributes::kOptionalAttributeRanges,
            ElectricalPowerMeasurement::OptionalAttributes::kOptionalAttributeVoltage,
            ElectricalPowerMeasurement::OptionalAttributes::kOptionalAttributeActiveCurrent,
            ElectricalPowerMeasurement::OptionalAttributes::kOptionalAttributeReactiveCurrent,
            ElectricalPowerMeasurement::OptionalAttributes::kOptionalAttributeApparentCurrent,
            ElectricalPowerMeasurement::OptionalAttributes::kOptionalAttributeReactivePower,
            ElectricalPowerMeasurement::OptionalAttributes::kOptionalAttributeApparentPower,
            ElectricalPowerMeasurement::OptionalAttributes::kOptionalAttributeRMSVoltage,
            ElectricalPowerMeasurement::OptionalAttributes::kOptionalAttributeRMSCurrent,
            ElectricalPowerMeasurement::OptionalAttributes::kOptionalAttributeRMSPower,
            ElectricalPowerMeasurement::OptionalAttributes::kOptionalAttributeFrequency,
            ElectricalPowerMeasurement::OptionalAttributes::kOptionalAttributePowerFactor,
            ElectricalPowerMeasurement::OptionalAttributes::kOptionalAttributeNeutralCurrent));

    if (!gEPMInstance)
    {
        ChipLogError(AppServer, "Failed to allocate memory for EPM Instance");
        gEPMDelegate.reset();
        return CHIP_ERROR_NO_MEMORY;
    }

    err = gEPMInstance->Init(); /* Register Attribute & Command handlers */
    if (err != CHIP_NO_ERROR)
    {
        ChipLogError(AppServer, "Init failed on gEPMInstance");
        gEPMInstance.reset();
        gEPMDelegate.reset();
        return err;
    }

    return CHIP_NO_ERROR;
}

CHIP_ERROR SmartPlugManager::EnergyMeterShutdown()
{
    /* Do this in the order Instance first, then delegate
     * Ensure we call the Instance->Shutdown to free attribute & command handlers first
     */
    if (gEPMInstance)
    {
        /* deregister attribute & command handlers */
        gEPMInstance->Shutdown();
        gEPMInstance.reset();
    }

    if (gEPMDelegate)
    {
        gEPMDelegate.reset();
    }

    return CHIP_NO_ERROR;
}

CHIP_ERROR SendPowerReading()
{
    ElectricalPowerMeasurementDelegate * dg = gEPMDelegate.get();
    VerifyOrReturnError(dg != nullptr, CHIP_ERROR_UNINITIALIZED);

    ChipLogProgress(NotSpecified, "SendPowerReading %d", (int) power_demo_actual);

    chip::DeviceLayer::PlatformMgr().LockChipStack();
    dg->SetActivePower(MakeNullable(power_demo_actual));
    dg->SetVoltage(MakeNullable(voltage_demo_actual));
    dg->SetActiveCurrent(MakeNullable(current_demo_actual));
    chip::DeviceLayer::PlatformMgr().UnlockChipStack();

    return CHIP_NO_ERROR;
}


CHIP_ERROR SendCumulativeEnergyReading()
{
    MeasurementData * data = MeasurementDataForEndpoint(EndpointId(ENERGY_MEAS_ENDPOINT));
    VerifyOrReturnError(data != nullptr, CHIP_ERROR_UNINITIALIZED);

    EnergyMeasurementStruct::Type energyImported;
    EnergyMeasurementStruct::Type energyExported;

    /** IMPORT */
    // Copy last endTimestamp into new startTimestamp if it exists
    energyImported.startTimestamp.ClearValue();
    energyImported.startSystime.ClearValue();
    if (data->cumulativeImported.HasValue())
    {
        energyImported.startTimestamp = data->cumulativeImported.Value().endTimestamp;
        energyImported.startSystime   = data->cumulativeImported.Value().endSystime;
    }

    energyImported.energy = energy_demo_actual;

    /** EXPORT */
    // Copy last endTimestamp into new startTimestamp if it exists
    energyExported.startTimestamp.ClearValue();
    energyExported.startSystime.ClearValue();
    if (data->cumulativeExported.HasValue())
    {
        energyExported.startTimestamp = data->cumulativeExported.Value().endTimestamp;
        energyExported.startSystime   = data->cumulativeExported.Value().endSystime;
    }

    energyExported.energy = 0;

    // Get current timestamp
    uint32_t currentTimestamp;
    CHIP_ERROR err = GetEpochTS(currentTimestamp);
    if (err == CHIP_NO_ERROR)
    {
        // use EpochTS
        energyImported.endTimestamp.SetValue(currentTimestamp);
        energyExported.endTimestamp.SetValue(currentTimestamp);
    }
    else
    {
        ChipLogError(AppServer, "GetEpochTS returned error getting timestamp %" CHIP_ERROR_FORMAT, err.Format());

        // use systemTime as a fallback
        System::Clock::Milliseconds64 system_time_ms =
            std::chrono::duration_cast<System::Clock::Milliseconds64>(chip::Server::GetInstance().TimeSinceInit());
        uint64_t nowMS = static_cast<uint64_t>(system_time_ms.count());

        energyImported.endSystime.SetValue(nowMS);
        energyExported.endSystime.SetValue(nowMS);
    }

    ChipLogProgress(NotSpecified, "SendCumulativeEnergyReading %d", (int) energy_demo_actual);
    chip::DeviceLayer::PlatformMgr().LockChipStack();
    // call the SDK to update attributes and generate an event
    if (!NotifyCumulativeEnergyMeasured(EndpointId(ENERGY_MEAS_ENDPOINT), MakeOptional(energyImported), MakeOptional(energyExported)))
    {
        ChipLogError(AppServer, "Failed to notify Cumulative Energy reading.");
        return CHIP_ERROR_INTERNAL;
    }
    chip::DeviceLayer::PlatformMgr().UnlockChipStack();

    return CHIP_NO_ERROR;
}

CHIP_ERROR SendPeriodicEnergyReading()
{
    MeasurementData * data = MeasurementDataForEndpoint(EndpointId(ENERGY_MEAS_ENDPOINT));
    VerifyOrReturnError(data != nullptr, CHIP_ERROR_UNINITIALIZED);

    EnergyMeasurementStruct::Type energyImported;
    EnergyMeasurementStruct::Type energyExported;

    /** IMPORT */
    // Copy last endTimestamp into new startTimestamp if it exists
    energyImported.startTimestamp.ClearValue();
    energyImported.startSystime.ClearValue();
    if (data->periodicImported.HasValue())
    {
        energyImported.startTimestamp = data->periodicImported.Value().endTimestamp;
        energyImported.startSystime   = data->periodicImported.Value().endSystime;
    }

    energyImported.energy = ENERGY_USED_STEP;

    /** EXPORT */
    // Copy last endTimestamp into new startTimestamp if it exists
    energyExported.startTimestamp.ClearValue();
    energyExported.startSystime.ClearValue();
    if (data->periodicExported.HasValue())
    {
        energyExported.startTimestamp = data->periodicExported.Value().endTimestamp;
        energyExported.startSystime   = data->periodicExported.Value().endSystime;
    }

    energyExported.energy = 0;

    // Get current timestamp
    uint32_t currentTimestamp;
    CHIP_ERROR err = GetEpochTS(currentTimestamp);
    if (err == CHIP_NO_ERROR)
    {
        // use EpochTS
        energyImported.endTimestamp.SetValue(currentTimestamp);
        energyExported.endTimestamp.SetValue(currentTimestamp);
    }
    else
    {
        ChipLogError(AppServer, "GetEpochTS returned error getting timestamp");

        // use systemTime as a fallback
        System::Clock::Milliseconds64 system_time_ms =
            std::chrono::duration_cast<System::Clock::Milliseconds64>(chip::Server::GetInstance().TimeSinceInit());
        uint64_t nowMS = static_cast<uint64_t>(system_time_ms.count());

        energyImported.endSystime.SetValue(nowMS);
        energyExported.endSystime.SetValue(nowMS);
    }

    ChipLogProgress(NotSpecified, "SendPeriodicEnergyReading %d", energyImported.energy);
	chip::DeviceLayer::PlatformMgr().LockChipStack();
    // call the SDK to update attributes and generate an event
    if (!NotifyPeriodicEnergyMeasured(EndpointId(ENERGY_MEAS_ENDPOINT), MakeOptional(energyImported), MakeOptional(energyExported)))
    {
        ChipLogError(AppServer, "Failed to notify Periodic Energy reading.");
        return CHIP_ERROR_INTERNAL;
    }
    chip::DeviceLayer::PlatformMgr().UnlockChipStack();

    return CHIP_NO_ERROR;
}


void SmartPlugManager::PowerMeasureStart()
{
    isMeasureActive = true;
}

void SmartPlugManager::PowerMeasureStop()
{
    isMeasureActive = false;
}

const osThreadAttr_t PowerMeasure_attr =
{
  .name = "Power_measure",
  .attr_bits = 0,
  .cb_mem = 0,
  .cb_size = 0,
  .stack_mem = 0,
  .stack_size = 1024*4,
  .priority =  osPriorityNormal,
};

void SmartPlugManager::PowerMeasureTask(void * pvParameter) {
    uint8_t demo_idx = 0;
    CHIP_ERROR pwr_ret, energy_ret, energy_ret2;
	while(1) {
		if (isMeasureActive) {
			voltage_demo_actual = voltage_demo_vals[demo_idx];
			current_demo_actual = current_demo_vals[demo_idx];
			power_demo_actual = voltage_demo_actual * current_demo_actual;
			energy_demo_actual += ENERGY_USED_STEP;

            pwr_ret = SendPowerReading();
            energy_ret = SendPeriodicEnergyReading();
            energy_ret2 = SendCumulativeEnergyReading();

            ChipLogProgress(NotSpecified, "PowerMeasureTask, %d %d", pwr_ret, energy_ret, energy_ret2);

            demo_idx++;
            if (demo_idx == 6) demo_idx = 0;
        } else if (sendLastZeroMeasure) {
        	sendLastZeroMeasure = false;
        	voltage_demo_actual = 0;
        	current_demo_actual = 0;
        	power_demo_actual = 0;
        	pwr_ret = SendPowerReading();
        	ChipLogProgress(NotSpecified, "PowerMeasureTask, %d", pwr_ret);
        }
		vTaskDelay(pdMS_TO_TICKS(POWER_MEASURE_PERIOD));
	}
}

CHIP_ERROR SmartPlugManager::Init() {
    mState = kState_Off;

    if (EnergyMeterInit() != CHIP_NO_ERROR)
    {
        ChipLogError(NotSpecified, "SmartPlugMgr:EnergyMeterInit fail");
        return CHIP_ERROR_BUSY;
    }
    if (PowerTopologyInit() != CHIP_NO_ERROR)
    {
        ChipLogError(NotSpecified, "SmartPlugMgr:PowerTopologyInit fail");
        return CHIP_ERROR_BUSY;
    }

    osThreadNew(PowerMeasureTask, NULL, &PowerMeasure_attr);

    return CHIP_NO_ERROR;
}

bool SmartPlugManager::IsTurnedOn() {
    return mState == kState_On;
}

void SmartPlugManager::SetCallbacks(SmartPlugCallback_fn aActionInitiated_CB,
		SmartPlugCallback_fn aActionCompleted_CB) {
    mActionInitiated_CB = aActionInitiated_CB;
    mActionCompleted_CB = aActionCompleted_CB;
}

bool SmartPlugManager::InitiateAction(Action_t aAction, int32_t aActor, uint16_t size,
        uint8_t *value) {
    bool action_initiated = false;
    State_t new_state = kState_Off;

    switch (aAction) {
    case ON_ACTION:
        ChipLogProgress(NotSpecified, "SmartPlugMgr:ON: %s->ON", mState == kState_On ? "ON" : "OFF")
        ;
        break;
    case OFF_ACTION:
        ChipLogProgress(NotSpecified, "SmartPlugMgr:OFF: %s->OFF", mState == kState_On ? "ON" : "OFF")
        ;
        break;
    default:
        ChipLogProgress(NotSpecified, "SmartPlugMgr:Unknown")
        ;
        break;
    }

    // Initiate On/Off Action only when the previous one is complete.
    if (mState == kState_Off && aAction == ON_ACTION) {
        action_initiated = true;
        new_state = kState_On;
    } else if (mState == kState_On && aAction == OFF_ACTION) {
        action_initiated = true;
        new_state = kState_Off;
    }

	Set(new_state == kState_On);

    if (action_initiated) {
        if (mActionInitiated_CB) {
            mActionInitiated_CB(aAction);
        }

        if (mActionCompleted_CB) {
            mActionCompleted_CB(aAction);
        }
    }

    return action_initiated;
}

void SmartPlugManager::Set(bool aOn) {
    if (aOn) {
        mState = kState_On;
    } else {
        mState = kState_Off;
    }
    UpdatePlug();
}

void SmartPlugManager::UpdatePlug() {
	if (mState == kState_On) {
		BSP_LED_On(LED_GREEN);
		isMeasureActive = true;
	}
	else {
		BSP_LED_Off(LED_GREEN);
		isMeasureActive = false;
		sendLastZeroMeasure = true;
	}
}

