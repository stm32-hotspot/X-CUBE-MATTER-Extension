#include "acl_storage_delegate.h"

#include <app/server/Server.h>
#include <app/util/attribute-storage.h>
#include <lib/support/logging/CHIPLogging.h>
#include <access/AccessControl.h>



using namespace chip;
using namespace chip::app;
using namespace chip::Access;

using EncodableEntry   = AclStorage::EncodableEntry;
using Entry            = AccessControl::Entry;
using EntryListener    = AccessControl::EntryListener;
using StagingAuthMode  = Clusters::AccessControl::AccessControlEntryAuthModeEnum;
using StagingPrivilege = Clusters::AccessControl::AccessControlEntryPrivilegeEnum;
using StagingTarget    = Clusters::AccessControl::Structs::AccessControlTargetStruct::Type;
using Target           = AccessControl::Entry::Target;

namespace AccessControlCluster = chip::app::Clusters::AccessControl;

class : public EntryListener
{
public:
    void OnEntryChanged(const SubjectDescriptor * subjectDescriptor, FabricIndex fabric, size_t index, const Entry * entry,
                        ChangeType changeType) override
    {
    	ChipLogDetail(NotSpecified, "AclStorageDelegate: OnEntryChanged");
    	mOnEntryChangedHandle();
//        (void) chip::DeviceLayer::SystemLayer().StartTimer(chip::System::Clock::Milliseconds32(500), OnEntryChangedTimerCallback, nullptr);

    }

    // Must initialize before use.
    void Init(void (*OnEntryChangedHandle)()) {
    	mOnEntryChangedHandle = OnEntryChangedHandle;
    }

private:
    void (*mOnEntryChangedHandle)() = nullptr;

    static void OnEntryChangedTimerCallback(chip::System::Layer * systemLayer, void * appState) {
    	return;
//    	chip::DeviceLayer::PlatformMgr().ScheduleWork([](intptr_t) {CallSaveNvm(); });
    //  chip::DeviceLayer::PlatformMgr().ScheduleWork([](intptr_t) {chip::Server::GetInstance().ScheduleFactoryReset(); });
    }

} sStmEntryListener;




AclStorageDelegate AclStorageDelegate::sAclStorageDelegate;

CHIP_ERROR AclStorageDelegate::Init(void (*NvmSaveHandle)())
{
	CHIP_ERROR err = CHIP_NO_ERROR;
    ChipLogDetail(NotSpecified, "AclStorageDelegate: initializing");

    mNvmSaveHandle = NvmSaveHandle;

    sStmEntryListener.Init(NvmSaveDelegate);
    chip::Access::GetAccessControl().AddEntryListener(sStmEntryListener);

    return err;
}

void AclStorageDelegate::OnSaveDelegateTimerCallback(chip::System::Layer * systemLayer, void * appState)
{
//	CallSaveNvm();
//    chip::DeviceLayer::PlatformMgr().ScheduleWork([](intptr_t) {GetInstance().CallSaveNvm(); });
	GetInstance().mNvmSaveHandle();
}

void AclStorageDelegate::CallSaveNvm(void)
{
	mNvmSaveHandle();
}

void AclStorageDelegate::NvmSaveDelegate(void)
{
	(void) chip::DeviceLayer::SystemLayer().StartTimer(chip::System::Clock::Milliseconds32(500), OnSaveDelegateTimerCallback, nullptr);
}




