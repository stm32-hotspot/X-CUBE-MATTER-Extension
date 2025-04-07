

#include "AppEvent.h"
#include "AppTask.h"
#include <app/clusters/identify-server/identify-server.h>

chip::app::Clusters::Identify::EffectIdentifierEnum sIdentifyEffect = chip::app::Clusters::Identify::EffectIdentifierEnum::kStopEffect;


/**********************************************************
 * Identify Callbacks
 *********************************************************/

#if (CFG_IDENTIFY_LED_DEMO == 1U)
bool IdentifyLedIsOn = false;

void OnIdentifyToggle(chip::System::Layer * systemLayer, void * appState)
{
	if (IdentifyLedIsOn)
	{
		GetAppTask().UpdateRgbLed(0, 0, 0);
		IdentifyLedIsOn = false;
	}
	else
	{
		GetAppTask().UpdateRgbLed(CFG_IDENTIFY_LED_R, CFG_IDENTIFY_LED_G, CFG_IDENTIFY_LED_B);
		IdentifyLedIsOn = true;
	}
	(void) chip::DeviceLayer::SystemLayer().StartTimer(chip::System::Clock::Milliseconds32(500), OnIdentifyToggle, appState);

}
#endif



void OnIdentifyStart(Identify * identify)
{
	ChipLogProgress(Zcl, "onIdentifyStart");
#if (CFG_IDENTIFY_LED_DEMO == 1U)
	GetAppTask().UpdateRgbLed(CFG_IDENTIFY_LED_R, CFG_IDENTIFY_LED_G, CFG_IDENTIFY_LED_B);
	IdentifyLedIsOn = true;
	(void) chip::DeviceLayer::SystemLayer().StartTimer(chip::System::Clock::Milliseconds32(500), OnIdentifyToggle, identify);
#endif
}

void OnIdentifyStop(Identify * identify)
{
	ChipLogProgress(Zcl, "onIdentifyStop");
#if (CFG_IDENTIFY_LED_DEMO == 1U)
	(void) chip::DeviceLayer::SystemLayer().CancelTimer(OnIdentifyToggle, identify);
	GetAppTask().UpdateRgbLed(0, 0, 0);
#endif
}


namespace {
void OnTriggerIdentifyEffectCompleted(chip::System::Layer * systemLayer, void * appState)
{
    sIdentifyEffect = chip::app::Clusters::Identify::EffectIdentifierEnum::kStopEffect;
}
} // namespace

 void OnTriggerIdentifyEffect(Identify * identify)
{
    sIdentifyEffect = identify->mCurrentEffectIdentifier;

    if (identify->mCurrentEffectIdentifier == chip::app::Clusters::Identify::EffectIdentifierEnum::kChannelChange)
    {
        ChipLogProgress(Zcl, "IDENTIFY_EFFECT_IDENTIFIER_CHANNEL_CHANGE - Not supported, use effect variant %d",
                        identify->mEffectVariant);
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
    }
}

 Identify gIdentify = {
     chip::EndpointId{ 1 },
	 OnIdentifyStart,
	 OnIdentifyStop,
     chip::app::Clusters::Identify::IdentifyTypeEnum::kVisibleIndicator,
     OnTriggerIdentifyEffect,
 };
