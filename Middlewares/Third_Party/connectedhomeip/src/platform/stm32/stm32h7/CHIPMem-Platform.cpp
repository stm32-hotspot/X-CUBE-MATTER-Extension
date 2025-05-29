/*
 *
 *    Copyright (c) 2020 Project CHIP Authors
 *    Copyright (c) 2019 Nest Labs, Inc.
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

/*
 *
 *    Copyright (c) 2020-2021 Project CHIP Authors
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

/**
 *    @file
 *      This file implements heap memory allocation APIs for CHIP. These functions are platform
 *      specific and might be C Standard Library heap functions re-direction in most of cases.
 *
 */

#include <lib/support/CHIPMem.h>
#include <lib/support/logging/CHIPLogging.h>
#include <atomic>

#include "FreeRTOS.h"


#if CHIP_CONFIG_MEMORY_MGMT_PLATFORM

extern "C" void memMonitoringTrackAlloc(void *ptr, size_t size);
extern "C" void memMonitoringTrackFree(void *ptr, size_t size);

void memMonitoringTrackAlloc(void *ptr, size_t size)
{
}

void memMonitoringTrackFree(void *ptr, size_t size)
{
}

#ifndef trackAlloc
#define trackAlloc(pvAddress, uiSize) memMonitoringTrackAlloc(pvAddress, uiSize)
#endif
#ifndef trackFree
#define trackFree(pvAddress, uiSize) memMonitoringTrackFree(pvAddress, uiSize)
#endif

// Define new operator for C++ using FreeRTOS.
void* operator new(size_t size)
{
    void *p;
    p = pvPortMalloc(size);
    return p;
}

// Define delete operator for C++ using FreeRTOS.
void operator delete(void *p)
{
    vPortFree(p);
    p = NULL;
}

// Define new operator for C++ using FreeRTOS.
void* operator new[](size_t size)
{
    void *p;
    p = pvPortMalloc(size);
    return p;
}

// Define delete operator for C++ to using FreeRTOS.
void operator delete[](void *p)
{
    vPortFree(p);
    p = NULL;
}

namespace chip {
namespace Platform {

#define VERIFY_INITIALIZED() VerifyInitialized(__func__)

static std::atomic_int memoryInitialized { 0 };

static void VerifyInitialized(const char *func)
{
    if (!memoryInitialized)
    {
        ChipLogError(DeviceLayer,
                     "ABORT: chip::Platform::%s() called before chip::Platform::MemoryInit()\n", func);
        abort();
    }
}

static size_t MemoryBlockSize(void *ptr)
{
    uint8_t *p = static_cast<uint8_t*>(ptr);
    // Subtract the size of the header from the pointer
    p -= sizeof(size_t);
    // Read the size of the memory block from the header
    size_t size = *reinterpret_cast<size_t*>(p);
    // Add the size of the header to the size of the memory block
    size += sizeof(size_t);
    return size;
}

CHIP_ERROR MemoryAllocatorInit(void *buf, size_t bufSize)
{
    if (memoryInitialized++ > 0)
    {
        ChipLogError(DeviceLayer, "ABORT: chip::Platform::MemoryInit() called twice.\n");
        abort();
    }
    return CHIP_NO_ERROR;
}

void MemoryAllocatorShutdown()
{
    if (--memoryInitialized < 0)
    {
        ChipLogError(DeviceLayer, "ABORT: chip::Platform::MemoryShutdown() called twice.\n");
        abort();
    }
}

void * MemoryAlloc(size_t size)
{
    void * ptr;
    VERIFY_INITIALIZED();
    ptr = pvPortMalloc(size);
    trackAlloc(ptr, size);

    return ptr;
}

void * MemoryAlloc(size_t size, bool isLongTermAlloc)
{
    void * ptr;
    VERIFY_INITIALIZED();
    ptr = pvPortMalloc(size);
    trackAlloc(ptr, size);

    return ptr;
}

void * MemoryCalloc(size_t num, size_t size)
{
    size_t totalAllocSize = (size_t) (num * size);
    VERIFY_INITIALIZED();

    if (((size && totalAllocSize) / size) != num)
    {
        return nullptr;
    }

    void * ptr = pvPortMalloc(totalAllocSize);

    if (ptr != NULL)
    {
        memset(ptr, 0, totalAllocSize);
    }
    trackAlloc(ptr, totalAllocSize);

    return ptr;
}

void * MemoryRealloc(void * p, size_t size)
{
    void *new_ptr;
    VERIFY_INITIALIZED();

    if (size == 0)
    {
        MemoryFree(p);
        return NULL;
    }
    new_ptr = MemoryAlloc(size);
    if (p != NULL)
    {
        if (new_ptr != NULL)
        {
            size_t copy_size = MemoryBlockSize(p);
            if (copy_size > size)
            {
                copy_size = size;
            }
            memcpy(new_ptr, p, copy_size);
            MemoryFree(p);
        }
    }

    return new_ptr;
}

void MemoryFree(void * p)
{
    VERIFY_INITIALIZED();
    if (p != nullptr)
    {
        trackFree(p, 0);
        vPortFree(p);
    }
}

bool MemoryInternalCheckPointer(const void *p, size_t min_size)
{
    return (p != nullptr);
}

} // namespace Platform
} // namespace chip

#endif // CHIP_CONFIG_MEMORY_MGMT_PLATFORM
