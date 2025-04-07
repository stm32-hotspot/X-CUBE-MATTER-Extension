/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * File Name          : ZclCallback.c
 * Description        : Cluster output source file for Matter.
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2019-2021 STMicroelectronics.
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 *
 ******************************************************************************
 */
/* USER CODE END Header */

#include "AppTask.h"
#include "TemperatureManager.h"

#include <app-common/zap-generated/ids/Attributes.h>
#include <app-common/zap-generated/ids/Clusters.h>
#include <app/ConcreteAttributePath.h>
#include <lib/support/logging/CHIPLogging.h>

using namespace chip;
using namespace chip::app::Clusters;

void MatterPostAttributeChangeCallback(const chip::app::ConcreteAttributePath &attributePath,
    uint8_t type, uint16_t size, uint8_t *value) {
ClusterId clusterId = attributePath.mClusterId;
AttributeId attributeId = attributePath.mAttributeId;

if (clusterId == Thermostat::Id)
{
    ChipLogProgress(Zcl, "MatterPostAttributeChangeCallback Cluster Thermostat: attributeId=%d, value=%u",attributeId, *value);
}
else
{
    ChipLogProgress(Zcl, "MatterPostAttributeChangeCallback Cluster=%d: attributeId=%d, value=%u",clusterId,attributeId, *value);
}

if (clusterId == Thermostat::Id && attributeId == Thermostat::Attributes::LocalTemperature::Id)
{
    TempMgr().AttributeChangeHandler(TemperatureManager::LOCAL_T_ACTION, value, size);
} else if (clusterId == Thermostat::Id && attributeId == Thermostat::Attributes::OutdoorTemperature::Id)
{
    TempMgr().AttributeChangeHandler(TemperatureManager::OUTDOOR_T_ACTION, value, size);
} else if (clusterId == Thermostat::Id && attributeId == Thermostat::Attributes::OccupiedCoolingSetpoint::Id)
{
    TempMgr().AttributeChangeHandler(TemperatureManager::COOL_SET_ACTION, value, size);
} else if (clusterId == Thermostat::Id && attributeId == Thermostat::Attributes::OccupiedHeatingSetpoint::Id)
{
    TempMgr().AttributeChangeHandler(TemperatureManager::HEAT_SET_ACTION, value, size);
} else if (clusterId == Thermostat::Id && attributeId == Thermostat::Attributes::SystemMode::Id)
{
    TempMgr().AttributeChangeHandler(TemperatureManager::MODE_SET_ACTION, value, size);
} else if (clusterId == Identify::Id)
{
    ChipLogProgress(Zcl, "Cluster Identify: value set to %u", *value);
//		LightingMgr().StartIdentification(*value);
} else
{
    ChipLogProgress(NotSpecified, "Unhandled ZCL callback, clusterId %d, attribute %x", clusterId,attributeId);
}
}

void emberAfOnOffClusterInitCallback(EndpointId endpoint) {
}

