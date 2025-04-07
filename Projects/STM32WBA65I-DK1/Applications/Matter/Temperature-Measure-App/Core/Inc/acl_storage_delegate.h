/*
 *
 *    Copyright (c) 2022 Project CHIP Authors
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

#pragma once

#include <app/server/Server.h>
#include <app/util/attribute-storage.h>
#include <lib/support/logging/CHIPLogging.h>
#include <access/AccessControl.h>
#include <lib/core/CHIPError.h>

using namespace chip;
using namespace chip::app;
using namespace chip::Access;

class AclStorageDelegate
{
public:
	static AclStorageDelegate & GetInstance() { return sAclStorageDelegate; }
    CHIP_ERROR Init(void (*NvmSaveHandle)());
    static void NvmSaveDelegate(void);

private:
    static void OnSaveDelegateTimerCallback(chip::System::Layer * systemLayer, void * appState);
    void CallSaveNvm(void);
    void (*mNvmSaveHandle)() = nullptr;
    static AclStorageDelegate sAclStorageDelegate;

};


