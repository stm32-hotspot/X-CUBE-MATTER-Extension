/*
 *
 *    Copyright (c) 2020 Project CHIP Authors
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

/**********************************************************
 * Includes
 *********************************************************/
#include "AppEvent.h"
#include "AppTask.h"
#include <app/util/config.h>

#ifdef MATTER_DM_PLUGIN_IDENTIFY_SERVER
#include <app/clusters/identify-server/identify-server.h>
#endif /* MATTER_DM_PLUGIN_IDENTIFY_SERVER */

#ifdef MATTER_DM_PLUGIN_IDENTIFY_SERVER
chip::app::Clusters::Identify::EffectIdentifierEnum sIdentifyEffect = chip::app::Clusters::Identify::EffectIdentifierEnum::kStopEffect;

Identify gIdentify = {
    chip::EndpointId{ 1 },
    AppTask::OnIdentifyStart,
    AppTask::OnIdentifyStop,
    chip::app::Clusters::Identify::IdentifyTypeEnum::kVisibleIndicator,
    AppTask::OnTriggerIdentifyEffect,
};

/**********************************************************
 * Identify Callbacks
 *********************************************************/
void AppTask::OnIdentifyStart(Identify * identify)
{
    ChipLogProgress(Zcl, "onIdentifyStart");

#if (CHIP_DEVICE_CONFIG_ENABLE_LED == 1)
    // Start the LED
#endif /* CHIP_DEVICE_CONFIG_ENABLE_LED == 1 */
}

void AppTask::OnIdentifyStop(Identify * identify)
{
    ChipLogProgress(Zcl, "onIdentifyStop");
#if (CHIP_DEVICE_CONFIG_ENABLE_LED == 1)
    // Stop the LED
#endif /* CHIP_DEVICE_CONFIG_ENABLE_LED == 1 */
}

void AppTask::OnTriggerIdentifyEffectCompleted(chip::System::Layer * systemLayer, void * appState)
{
    ChipLogProgress(Zcl, "Trigger Identify Complete");
    sIdentifyEffect = chip::app::Clusters::Identify::EffectIdentifierEnum::kStopEffect;
#if (CHIP_DEVICE_CONFIG_ENABLE_LED == 1)
    // Stop the LED
#endif /* CHIP_DEVICE_CONFIG_ENABLE_LED == 1 */
}

 void AppTask::OnTriggerIdentifyEffect(Identify * identify)
{
    sIdentifyEffect = identify->mCurrentEffectIdentifier;

    if (identify->mCurrentEffectIdentifier == chip::app::Clusters::Identify::EffectIdentifierEnum::kChannelChange)
    {
        ChipLogProgress(Zcl, "IDENTIFY_EFFECT_IDENTIFIER_CHANNEL_CHANGE - Not supported, use effect variant %d",
                        static_cast<uint8_t>(identify->mEffectVariant));
        sIdentifyEffect = static_cast<chip::app::Clusters::Identify::EffectIdentifierEnum>(identify->mEffectVariant);
    }

    switch (sIdentifyEffect)
    {
    case chip::app::Clusters::Identify::EffectIdentifierEnum::kBlink:
    case chip::app::Clusters::Identify::EffectIdentifierEnum::kBreathe:
    case chip::app::Clusters::Identify::EffectIdentifierEnum::kOkay:
        (void) chip::DeviceLayer::SystemLayer().StartTimer(chip::System::Clock::Seconds16(5), OnTriggerIdentifyEffectCompleted,
                                                           identify);
        break;

    case chip::app::Clusters::Identify::EffectIdentifierEnum::kFinishEffect:
        (void) chip::DeviceLayer::SystemLayer().CancelTimer(OnTriggerIdentifyEffectCompleted, identify);
        (void) chip::DeviceLayer::SystemLayer().StartTimer(chip::System::Clock::Seconds16(1), OnTriggerIdentifyEffectCompleted,
                                                           identify);
        break;

    case chip::app::Clusters::Identify::EffectIdentifierEnum::kStopEffect:
        (void) chip::DeviceLayer::SystemLayer().CancelTimer(OnTriggerIdentifyEffectCompleted, identify);
        sIdentifyEffect = chip::app::Clusters::Identify::EffectIdentifierEnum::kStopEffect;
        break;

    default:
        ChipLogProgress(Zcl, "No identifier effect");
        sIdentifyEffect = chip::app::Clusters::Identify::EffectIdentifierEnum::kStopEffect;
        break;
    }
}

#endif /* MATTER_DM_PLUGIN_IDENTIFY_SERVER */


