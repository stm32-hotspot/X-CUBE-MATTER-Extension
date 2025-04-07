/*
 *
 *    Copyright (c) 2020 Project CHIP Authors
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

#include "AppTask.h"
#include "BoltLockManager.h"
#include "AppEvent.h"

#include <app-common/zap-generated/attributes/Accessors.h>
#include <app-common/zap-generated/ids/Clusters.h>
#include <app/ConcreteAttributePath.h>
#include <app/clusters/door-lock-server/door-lock-server.h>
#include <lib/support/CodeUtils.h>
#include <lib/support/logging/CHIPLogging.h>
using namespace ::chip;
using namespace ::chip::app::Clusters;
using namespace ::chip::app::Clusters::DoorLock;

void MatterPostAttributeChangeCallback(const chip::app::ConcreteAttributePath & path, uint8_t type, uint16_t size, uint8_t * value)
{
    VerifyOrReturn(path.mClusterId == DoorLock::Id && path.mAttributeId == DoorLock::Attributes::LockState::Id);

    switch (*value)
    {
    case to_underlying(DlLockState::kLocked):
        BoltLockMgr().InitiateAction(AppEvent::kEventType_Lock, BoltLockManager::LOCK_ACTION);
        break;
    case to_underlying(DlLockState::kUnlocked):
        BoltLockMgr().InitiateAction(AppEvent::kEventType_Lock, BoltLockManager::UNLOCK_ACTION);
        break;
    default:
        break;
    }
}

bool emberAfPluginDoorLockGetUser(EndpointId endpointId, uint16_t userIndex, EmberAfPluginDoorLockUserInfo & user)
{
    return BoltLockMgr().GetUser(userIndex, user);
}

bool emberAfPluginDoorLockSetUser(EndpointId endpointId, uint16_t userIndex, FabricIndex creator, FabricIndex modifier,
                                  const CharSpan & userName, uint32_t uniqueId, UserStatusEnum userStatus, UserTypeEnum userType,
                                  CredentialRuleEnum credentialRule, const CredentialStruct * credentials, size_t totalCredentials)
{
    return BoltLockMgr().SetUser(userIndex, creator, modifier, userName, uniqueId, userStatus, userType, credentialRule,
                                 credentials, totalCredentials);
}

bool emberAfPluginDoorLockGetCredential(EndpointId endpointId, uint16_t credentialIndex, CredentialTypeEnum credentialType,
                                        EmberAfPluginDoorLockCredentialInfo & credential)
{
    return BoltLockMgr().GetCredential(credentialIndex, credentialType, credential);
}

bool emberAfPluginDoorLockSetCredential(EndpointId endpointId, uint16_t credentialIndex, FabricIndex creator, FabricIndex modifier,
                                        DlCredentialStatus credentialStatus, CredentialTypeEnum credentialType,
                                        const ByteSpan & credentialData)
{
    return BoltLockMgr().SetCredential(credentialIndex, creator, modifier, credentialStatus, credentialType, credentialData);
}

bool emberAfPluginDoorLockOnDoorLockCommand(EndpointId endpointId, const Nullable<FabricIndex> & fabricIdx,
                                            const Nullable<NodeId> & nodeId, const Optional<ByteSpan> & pinCode,
                                            OperationErrorEnum & err)
{
    bool returnValue = false;

    if (BoltLockMgr().ValidatePIN(pinCode, err))
    {
        returnValue = BoltLockMgr().InitiateAction(AppEvent::kEventType_Lock, BoltLockManager::LOCK_ACTION);
    }

    return returnValue;
}

bool emberAfPluginDoorLockOnDoorUnlockCommand(EndpointId endpointId, const Nullable<FabricIndex> & fabricIdx,
                                              const Nullable<NodeId> & nodeId, const Optional<ByteSpan> & pinCode,
                                              OperationErrorEnum & err)
{
    bool returnValue = false;

    if (BoltLockMgr().ValidatePIN(pinCode, err))
    {
        returnValue = BoltLockMgr().InitiateAction(AppEvent::kEventType_Lock, BoltLockManager::UNLOCK_ACTION);
    }

    return returnValue;
}

void emberAfDoorLockClusterInitCallback(EndpointId endpoint)
{
    DoorLockServer::Instance().InitServer(endpoint);

//   const auto logOnFailure = [](Protocols::InteractionModel::Status status, const char * attributeName) {
//       if (status != Protocols::InteractionModel::Status::Success)
//       {
//           ChipLogError(Zcl, "Failed to set DoorLock %s: %x", attributeName, to_underlying(status));
//       }
//   };
//   logOnFailure(DoorLock::Attributes::LockState::Set(endpoint, DlLockState::kLocked), "lock state");
}
