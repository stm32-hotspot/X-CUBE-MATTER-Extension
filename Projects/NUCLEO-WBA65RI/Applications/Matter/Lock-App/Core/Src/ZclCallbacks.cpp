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
#include "BoltLockManager.h"
#include "AppEvent.h"

#include <app-common/zap-generated/ids/Attributes.h>
#include <app-common/zap-generated/ids/Clusters.h>
#include <app/ConcreteAttributePath.h>
#include <app/clusters/door-lock-server/door-lock-server.h>
#include <lib/support/logging/CHIPLogging.h>

using namespace chip;
using namespace chip::app::Clusters;
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
}
