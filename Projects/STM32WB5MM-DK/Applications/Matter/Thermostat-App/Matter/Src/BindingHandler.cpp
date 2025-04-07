/*
 *
 *    Copyright (c) 2020 Project CHIP Authors
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

#include "BindingHandler.h"

#include "app/CommandSender.h"
#include "app/clusters/bindings/BindingManager.h"
#include "app/server/Server.h"
#include "controller/InvokeInteraction.h"
#include "platform/CHIPDeviceLayer.h"
#include <app/clusters/bindings/bindings.h>
#include <lib/support/CodeUtils.h>

using namespace chip;
using namespace chip::app;

BindingAddedCallback mBindingAddedCallback;

namespace {
void TempSensorChangedHandler(const EmberBindingTableEntry & binding, OperationalDeviceProxy * peer_device, void * context)
{
    VerifyOrReturn(context != nullptr, ChipLogError(NotSpecified, "OnDeviceConnectedFn: context is null"));
    BindingCommandData * data = static_cast<BindingCommandData *>(context);

    // only run bound action for TempSensor cluster since no other cluster valid for this device
    if (data->clusterId != Clusters::TemperatureMeasurement::Id)
    {
    	ChipLogError(NotSpecified, "TempSensorChangedHandler: invalid cluster ID");
    	return;
    }

    data->InvokeCommandFunc(binding, peer_device, *data);
}

void TempSensorContextReleaseHandler(void * context)
{
    VerifyOrReturn(context != nullptr, ChipLogError(NotSpecified, "TempSensorContextReleaseHandler: context is null"));
    Platform::Delete(static_cast<BindingCommandData *>(context));
}

void InitBindingHandlerInternal(intptr_t arg)
{
    auto & server = chip::Server::GetInstance();
    chip::BindingManager::GetInstance().Init(
        { &server.GetFabricTable(), server.GetCASESessionManager(), &server.GetPersistentStorage() });
    chip::BindingManager::GetInstance().RegisterBoundDeviceChangedHandler(TempSensorChangedHandler);
    chip::BindingManager::GetInstance().RegisterBoundDeviceContextReleaseHandler(TempSensorContextReleaseHandler);
}

} // namespace

/********************************************************
 * Switch functions
 *********************************************************/

void TempSensorWorkerFunction(intptr_t context)
{
    VerifyOrReturn(context != 0, ChipLogError(NotSpecified, "TempSensorWorkerFunction - Invalid work data"));

    if (BindingTable::GetInstance().Size() != 0)
    {
    	BindingCommandData * data = reinterpret_cast<BindingCommandData *>(context);
    	BindingManager::GetInstance().NotifyBoundClusterChanged(data->localEndpointId, data->clusterId, static_cast<void *>(data));
    }
    else
    {
    	ChipLogError(NotSpecified, "TempSensorWorkerFunction - No device bound")
    }

}

void BindingWorkerFunction(intptr_t context)
{
    VerifyOrReturn(context != 0, ChipLogError(NotSpecified, "BindingWorkerFunction - Invalid work data"));

    EmberBindingTableEntry * entry = reinterpret_cast<EmberBindingTableEntry *>(context);
    AddBindingEntry(*entry);

    Platform::Delete(entry);

    mBindingAddedCallback(false);
}

CHIP_ERROR InitBindingHandler()
{
    // The initialization of binding manager will try establishing connection with unicast peers
    // so it requires the Server instance to be correctly initialized. Post the init function to
    // the event queue so that everything is ready when initialization is conducted.
    chip::DeviceLayer::PlatformMgr().ScheduleWork(InitBindingHandlerInternal);

    return CHIP_NO_ERROR;
}
