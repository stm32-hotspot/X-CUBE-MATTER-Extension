/**
  ******************************************************************************
  * @file    stm32_nvm_implement.c
  * @author  MCD Application Team
  * @brief   Middleware between keymanager and flash_driver to manage keys
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2023 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "stm32_nvm_implement.h"
#include "stm32_flash_int.h"
#include "main.h"
#include "AppLog.h"

#include "FreeRTOS.h"
#include "semphr.h"
#include <stdio.h>

/* Private defines -----------------------------------------------------------*/
#define NVM_OFFSET_KEY     (512)
#define NVM_BLOCK_SIZE     (NVM_OFFSET_KEY)
#define NVM_DEFAULT_VALUE  (0xFF)
#define NVM_SECTOR_1_SIZE  (4096)
#define NVM_SECTOR_2_SIZE  (4096 * 3) /* a minimum of 5 fabrics must be supported */
#define NVM_SIZE_FLASH     (NVM_SECTOR_1_SIZE + NVM_SECTOR_2_SIZE)

#define NVM_MATTER_ADDR_INIT              (FLASH_ADDR_SECTOR_0_BANK2)
#define NVM_MATTER_ADDR_INIT_PTR          ((void * const) NVM_MATTER_ADDR_INIT)

typedef struct
{
  NVM_SectorTypeDef sector_id;
  size_t  sector_size;
  uint8_t keyname_size;
  uint8_t keyvaluelength_size;
  uint8_t *ram_ptr;
} NVM_Sector_Struct;

/* Private macros ------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
uint8_t nvm_ram[NVM_SIZE_FLASH] = { 0 };

static NVM_Sector_Struct nvm_sector2 =
{
  .sector_id = NVM_SECTOR_2, .sector_size = NVM_SECTOR_2_SIZE,
  .keyname_size = 0U, .keyvaluelength_size = 0U, .ram_ptr = nvm_ram
};
static NVM_Sector_Struct nvm_sector1 =
{
  .sector_id = NVM_SECTOR_1, .sector_size = NVM_SECTOR_1_SIZE,
  .keyname_size = 0U, .keyvaluelength_size = 0U, .ram_ptr = nvm_ram + NVM_SECTOR_2_SIZE
};

SemaphoreHandle_t NVMSemaphore = NULL;

/* NVM protection against concurrent access */
#define NVM_LOCK()                (xSemaphoreTake(NVMSemaphore, portMAX_DELAY))
#define NVM_UNLOCK()              (xSemaphoreGive(NVMSemaphore))

/* Global variables ----------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/
static size_t GetKeyValueSize(const NVM_Sector_Struct select_sector, uint8_t *ptKeyName);
static uint8_t *SearchKey(const NVM_Sector_Struct select_sector, uint8_t *ptKeyName);

static NVM_StatusTypeDef flash_get(const NVM_Sector_Struct select_sector, uint8_t *ptKeyAddr,
                                   uint8_t *ptKeyValue, size_t KeyValueSize, size_t *ptReadSize);
static NVM_StatusTypeDef flash_update(const NVM_Sector_Struct select_sector, uint8_t *ptKeyName,
                                      uint8_t *ptKeyValue, size_t KeyValueSize);
static NVM_StatusTypeDef flash_replace(const NVM_Sector_Struct select_sector, uint8_t *ptKeyFind,
                                       uint8_t *ptKeyName, uint8_t *ptKeyValue, size_t KeyValueSize);
static NVM_StatusTypeDef flash_write(const NVM_Sector_Struct select_sector, uint8_t *ptKeyFree,
                                     uint8_t *ptKeyName, uint8_t *ptKeyValue, size_t KeyValueSize);
static NVM_StatusTypeDef flash_delete(const NVM_Sector_Struct select_sector, uint8_t *ptKeyFind);

/* Public functions ----------------------------------------------------------*/

NVM_StatusTypeDef NVM_Init(uint8_t KeyNameMaxSize, uint8_t KeyValueLengthSize, NVM_SectorTypeDef Sector)
{
    NVM_StatusTypeDef err = NVM_OK;
    static bool NVMInitialized = false;

    /* STM32_LOG("NVM Init"); */
    if (NVMSemaphore == NULL)
    {
        NVMSemaphore = xSemaphoreCreateMutex();
        if (NVMSemaphore == NULL)
        {
            STM32_LOG("Failed to allocate NVM resources");
            err = NVM_LACK_SPACE;
        }
    }
    if (NVMInitialized == false)
    {
        memset(nvm_ram, NVM_DEFAULT_VALUE, sizeof(nvm_ram));
        memcpy(nvm_ram, NVM_MATTER_ADDR_INIT_PTR, sizeof(nvm_ram));
        NVMInitialized = true;
    }
    switch (Sector)
    {
    case NVM_SECTOR_1:
        nvm_sector1.keyname_size = KeyNameMaxSize;
        nvm_sector1.keyvaluelength_size = KeyValueLengthSize;
        break;
    case NVM_SECTOR_2:
        nvm_sector2.keyname_size = KeyNameMaxSize;
        nvm_sector2.keyvaluelength_size = KeyValueLengthSize;
        break;
    default:
        err = NVM_PARAMETER_ERROR;
        break;
    }

    /* STM32_LOG("NVM Init: result:%d", (err == NVM_OK) ? true : false); */
    return err;
}

NVM_StatusTypeDef NVM_Save(void)
{
    NVM_StatusTypeDef err = NVM_DELETE_FAILED;

    /* STM32_LOG("NVM Save"); */
    NVM_LOCK();
    if (FLASH_INT_Delete(NVM_MATTER_ADDR_INIT, sizeof(nvm_ram)) == FLASH_OK)
    {
        if (FLASH_INT_Write(NVM_MATTER_ADDR_INIT, (uint32_t *)nvm_ram, sizeof(nvm_ram)) == FLASH_OK)
        {
            err = NVM_OK;
        }
        else
        {
            err = NVM_WRITE_FAILED;
        }
    }
    NVM_UNLOCK();

    return err;
}

NVM_StatusTypeDef NVM_Reset(NVM_SectorTypeDef Sector)
{
    NVM_StatusTypeDef err = NVM_OK;
    NVM_Sector_Struct select_nvm;

    /* STM32_LOG("NVM Reset"); */
    switch (Sector)
    {
    case NVM_SECTOR_1:
        select_nvm = nvm_sector1;
        break;

    case NVM_SECTOR_2:
        select_nvm = nvm_sector2;
        break;

    default:
        err = NVM_PARAMETER_ERROR;
        break;
    }

    if (err == NVM_OK)
    {
        NVM_LOCK();
        memset(select_nvm.ram_ptr, NVM_DEFAULT_VALUE, select_nvm.sector_size);
        NVM_UNLOCK();
    }

    return err;
}

void NVM_FactoryReset(void)
{
    /* STM32_LOG("NVM FactoryReset"); */
    while (1)
    {
        FLASH_INT_Delete(NVM_MATTER_ADDR_INIT, sizeof(nvm_ram));
        HAL_NVIC_SystemReset();
    }
}

NVM_StatusTypeDef NVM_GetKeyExists(const char *ptKeyName, NVM_SectorTypeDef Sector)
{
    NVM_StatusTypeDef err = NVM_OK;
    NVM_Sector_Struct select_nvm;

    /* STM32_LOG("NVM GetKeyExists:%s", ptKeyName); */
    if (ptKeyName == NULL)
    {
        err = NVM_PARAMETER_ERROR;
    }
    switch (Sector)
    {
    case NVM_SECTOR_1:
        select_nvm = nvm_sector1;
        break;

    case NVM_SECTOR_2:
        select_nvm = nvm_sector2;
        break;

    default:
        err = NVM_PARAMETER_ERROR;
        break;
    }

    if (err == NVM_OK)
    {
        NVM_LOCK();
        uint8_t *ptKeyFound = SearchKey(select_nvm, (uint8_t *)ptKeyName);
        if (ptKeyFound != NULL)
        {
            err = NVM_OK;
        }
        else
        {
            err = NVM_KEY_NOT_FOUND;
        }
        NVM_UNLOCK();
    }

    /* STM32_LOG("NVM GetKeyExists:%s result:%d", ptKeyName, (err == NVM_OK) ? true : false); */
    return err;
}

NVM_StatusTypeDef NVM_GetKeyValue(const char *ptKeyName, void *ptKeyValue, uint32_t KeyValueSize,
                                  size_t *ptReadKeyValueSize, NVM_SectorTypeDef Sector)
{
    NVM_StatusTypeDef err = NVM_OK;
    NVM_Sector_Struct select_nvm;

    /* STM32_LOG("NVM GetKeyValue:%s", ptKeyName); */
    if ((ptKeyName == NULL) || (ptKeyValue == NULL) || (ptReadKeyValueSize == NULL))
    {
        err = NVM_PARAMETER_ERROR;
    }
    switch (Sector)
    {
    case NVM_SECTOR_1:
        select_nvm = nvm_sector1;
        break;

    case NVM_SECTOR_2:
        select_nvm = nvm_sector2;
        break;

    default:
        err = NVM_PARAMETER_ERROR;
        break;
    }

    if (err == NVM_OK)
    {
        NVM_LOCK();
        uint8_t *ptKeyFound = SearchKey(select_nvm, (uint8_t *)ptKeyName);
        if (ptKeyFound != NULL)
        {
            /* copy ptKeyName's value in ptKeyValue and copy the size of ptKeyValue in ptReadKeyValueSize */
            err = flash_get(select_nvm, ptKeyFound, ptKeyValue, KeyValueSize, ptReadKeyValueSize);
        }
        else
        {
            err = NVM_KEY_NOT_FOUND;
        }
        NVM_UNLOCK();
    }

    return err;
}

NVM_StatusTypeDef NVM_SetKeyValue(const char *ptKeyName, const void *ptKeyValue, uint32_t KeyValueSize,
                                  NVM_SectorTypeDef Sector)
{
    NVM_StatusTypeDef err = NVM_OK;
    NVM_Sector_Struct select_nvm;

    /* STM32_LOG("NVM SetKeyValue:%s size:%u", ptKeyName, KeyValueSize); */
    if ((ptKeyName == NULL) || (ptKeyValue == NULL))
    {
        err = NVM_PARAMETER_ERROR;
    }
    switch (Sector)
    {
    case NVM_SECTOR_1:
        select_nvm = nvm_sector1;
        break;

    case NVM_SECTOR_2:
        select_nvm = nvm_sector2;
        break;

    default:
        err = NVM_PARAMETER_ERROR;
        break;
    }
    if (KeyValueSize > NVM_BLOCK_SIZE)
    {
        err = NVM_BLOCK_SIZE_OVERFLOW;
    }

    if (err == NVM_OK)
    {
        NVM_LOCK();
        /* call function to search the pointer of key if it already exist => replace or not exist => create */
        uint8_t *ptKeyFound = SearchKey(select_nvm, (uint8_t *)ptKeyName);
        if (ptKeyFound == NULL)
        {
            err = flash_update(select_nvm, (uint8_t *)ptKeyName, (uint8_t *)ptKeyValue, KeyValueSize);
        }
        else
        {
            err = flash_replace(select_nvm, ptKeyFound, (uint8_t *)ptKeyName, (uint8_t *)ptKeyValue, KeyValueSize);
            if (err != NVM_OK)
            {
                err = NVM_WRITE_FAILED; /* need to force ? */
            }
        }
        NVM_UNLOCK();
    }

    return err;
}

NVM_StatusTypeDef NVM_DeleteKey(const char *ptKeyName, NVM_SectorTypeDef Sector)
{
    NVM_StatusTypeDef err = NVM_OK;
    NVM_Sector_Struct select_nvm;

    /* STM32_LOG("NVM DeleteKey:%s", ptKeyName); */
    if (ptKeyName == NULL)
    {
        err = NVM_PARAMETER_ERROR;
    }
    switch (Sector)
    {
    case NVM_SECTOR_1:
        select_nvm = nvm_sector1;
        break;

    case NVM_SECTOR_2:
        select_nvm = nvm_sector2;
        break;

    default:
        err = NVM_PARAMETER_ERROR;
        break;
    }

    if (err == NVM_OK)
    {
        NVM_LOCK();
        uint8_t *ptKeyFound = SearchKey(select_nvm, (uint8_t *)ptKeyName);
        if (ptKeyFound != NULL)
        {
            err = flash_delete(select_nvm, ptKeyFound);
        }
        else
        {
            err = NVM_KEY_NOT_FOUND;
        }
        NVM_UNLOCK();
    }

    /* STM32_LOG("NVM DeleteKey:%s result:%d", ptKeyName, (err == NVM_OK) ? true : false); */
    return err;
}

NVM_StatusTypeDef NVM_MemoryStatus(uint32_t *ptMemoryUsed, uint32_t *ptMemoryFree, NVM_SectorTypeDef Sector)
{
    NVM_StatusTypeDef err = NVM_OK;
    NVM_Sector_Struct select_nvm;

    /* STM32_LOG("NVM MemoryStatus"); */
    if ((ptMemoryUsed == NULL) || (ptMemoryFree == NULL))
    {
        err = NVM_PARAMETER_ERROR;
    }
    switch (Sector)
    {
    case NVM_SECTOR_1:
        select_nvm = nvm_sector1;
        break;

    case NVM_SECTOR_2:
        select_nvm = nvm_sector2;
        break;

    default:
        err = NVM_PARAMETER_ERROR;
        break;
    }

    if (err == NVM_OK)
    {
        NVM_LOCK();
        uint8_t *i = select_nvm.ram_ptr;
        size_t read_by_size = 0;
        bool exit = false;

        *ptMemoryUsed = 0U;
        while ((i < (select_nvm.ram_ptr + select_nvm.sector_size)) && (exit == false))
        {
            if (*i == NVM_DEFAULT_VALUE)
            {
                exit = true;
            }
            else
            {
                read_by_size = GetKeyValueSize(select_nvm, i);
                /* ensure size of the data being read does not exceed the remaining size of the buffer */
                if ((read_by_size + select_nvm.keyvaluelength_size + select_nvm.keyname_size) > 
                    (select_nvm.sector_size - (i - select_nvm.ram_ptr)))
                {
                    err = NVM_CORRUPTION;
                    exit = true;
                }
                else
                {
                    i += read_by_size + select_nvm.keyvaluelength_size + select_nvm.keyname_size;
                    *ptMemoryUsed += read_by_size + select_nvm.keyvaluelength_size + select_nvm.keyname_size;
                }
            }
        }
        *ptMemoryFree = select_nvm.sector_size - *ptMemoryUsed;
        NVM_UNLOCK();
    }

    return err;
}

/*************************************************************
  *
  * LOCAL FUNCTIONS
  *
  *************************************************************/
static size_t GetKeyValueSize(const NVM_Sector_Struct select_sector, uint8_t *ptKeyName)
{
    size_t keyvalue_size;

    if (select_sector.keyvaluelength_size == sizeof(uint8_t))
    {
        keyvalue_size = *(uint8_t *)(ptKeyName + select_sector.keyname_size);
    }
    else if (select_sector.keyvaluelength_size == sizeof(uint16_t))
    {
        keyvalue_size = *(uint16_t *)(ptKeyName + select_sector.keyname_size);
    }
    else
    {
        keyvalue_size = *(uint32_t *)(ptKeyName + select_sector.keyname_size);
    }

    return keyvalue_size;
}

static uint8_t *SearchKey(const NVM_Sector_Struct select_sector, uint8_t *ptKeyName)
{
    uint8_t *PtPage = select_sector.ram_ptr;
    uint8_t *i = PtPage;
    size_t read_by_size = 0;

    /* STM32_LOG("NVMSearchKey:%s", ptKeyName); */
    while ((i >= PtPage) || (i < (PtPage + select_sector.sector_size)))
    {
        if (*i != NVM_DEFAULT_VALUE)
        {
            if (strncmp((const char *)ptKeyName, (const char *)i, select_sector.keyname_size) == 0)
            {
                return i;
            }
            read_by_size = GetKeyValueSize(select_sector, i);
            /* ensure size of the data being read does not exceed the remaining size of the buffer */
            if ((read_by_size + select_sector.keyvaluelength_size + select_sector.keyname_size) >
                (select_sector.sector_size - (i - PtPage)))
            {
                return NULL;
            }
            i += read_by_size + select_sector.keyvaluelength_size + select_sector.keyname_size;
            /* Flash is corrupted */
            if ((i < PtPage) || (i > (PtPage + select_sector.sector_size)))
            {
                NVM_FactoryReset();
            }
        }
        else
        {
            return NULL;
        }
    }

    return NULL;
}

static NVM_StatusTypeDef flash_get(const NVM_Sector_Struct select_sector, uint8_t *ptKeyAddr,
                                   uint8_t *ptKeyValue, size_t KeyValueSize, size_t *ptReadSize)
{
    NVM_StatusTypeDef err;

    *ptReadSize = GetKeyValueSize(select_sector, ptKeyAddr);
    if (KeyValueSize >= *ptReadSize)
    {
        memcpy(ptKeyValue, ptKeyAddr + select_sector.keyname_size + select_sector.keyvaluelength_size, *ptReadSize);
        err = NVM_OK;
    }
    else
    {
        err = NVM_BUFFER_TOO_SMALL;
    }

    /* STM32_LOG("NVM flash_get:%.*s result:%d size:%u", select_sector.keyname_size, (char *)ptKeyAddr,
              (err == NVM_OK) ? true : false, *ptReadSize); */
    return err;
}

static NVM_StatusTypeDef flash_update(const NVM_Sector_Struct select_sector, uint8_t *ptKeyName,
                                      uint8_t *ptKeyValue, size_t KeyValueSize)
{
    uint8_t *i = select_sector.ram_ptr;
    size_t read_by_size = 0;

    /* STM32_LOG("NVM flash_update:%s", ptKeyName); */
    while (i < (select_sector.ram_ptr + select_sector.sector_size))
    {
        if (*i == NVM_DEFAULT_VALUE)
        {
            return flash_write(select_sector, i, ptKeyName, ptKeyValue, KeyValueSize);
        }
        read_by_size = GetKeyValueSize(select_sector, i);
        if (read_by_size > NVM_BLOCK_SIZE)
        {
            return NVM_ERROR_BLOCK_ALIGN;
        }
        i += read_by_size + select_sector.keyvaluelength_size + select_sector.keyname_size;
    }

    return NVM_LACK_SPACE;
}

static NVM_StatusTypeDef flash_replace(const NVM_Sector_Struct select_sector, uint8_t *ptKeyFind,
                                       uint8_t *ptKeyName, uint8_t *ptKeyValue, size_t KeyValueSize)
{
    NVM_StatusTypeDef err = NVM_OK;

    err = flash_delete(select_sector, ptKeyFind);
    if (err == NVM_OK)
    {
        err = flash_update(select_sector, ptKeyName, ptKeyValue, KeyValueSize);
    }

    return err;
}

static NVM_StatusTypeDef flash_write(const NVM_Sector_Struct select_sector, uint8_t *ptKeyFree,
                                     uint8_t *ptKeyName, uint8_t *ptKeyValue, size_t KeyValueSize)
{
    memset(ptKeyFree, 0x00, select_sector.keyname_size);
    memcpy(ptKeyFree, ptKeyName, strlen((char *)ptKeyName));
    memcpy(ptKeyFree + select_sector.keyname_size, &KeyValueSize, select_sector.keyvaluelength_size);
    memcpy(ptKeyFree + select_sector.keyname_size + select_sector.keyvaluelength_size, ptKeyValue, KeyValueSize);
    /* STM32_LOG("NVM flash_write:%.*s size:%u total-size:%u", select_sector.keyname_size, (char *)ptKeyName, 
              KeyValueSize, (select_sector.keyname_size + select_sector.keyvaluelength_size + KeyValueSize)); */
    return NVM_OK;
}

static NVM_StatusTypeDef flash_delete(const NVM_Sector_Struct select_sector, uint8_t *ptKeyFind)
{
    uint8_t *PtKeyNext = NULL;
    uint8_t *PtKeyCpy = NULL;
    size_t read_by_size;

    read_by_size = GetKeyValueSize(select_sector, ptKeyFind);
    /* STM32_LOG("NVM flash_delete:%.*s size:%u total-size:%u", select_sector.keyname_size, (char *)ptKeyFind,
              read_by_size, (select_sector.keyname_size + select_sector.keyvaluelength_size + read_by_size)); */
    PtKeyNext = ptKeyFind + read_by_size + select_sector.keyname_size + select_sector.keyvaluelength_size;
    PtKeyCpy = ptKeyFind;
    while ((*PtKeyNext != 0xFF)
            && (PtKeyNext < (select_sector.ram_ptr + select_sector.sector_size)))
    {
        read_by_size = GetKeyValueSize(select_sector, PtKeyNext);
        memcpy(PtKeyCpy, PtKeyNext, read_by_size + select_sector.keyvaluelength_size + select_sector.keyname_size);
        PtKeyCpy += read_by_size + select_sector.keyvaluelength_size + select_sector.keyname_size;
        PtKeyNext += read_by_size + select_sector.keyname_size + select_sector.keyvaluelength_size;
    }
    memset(PtKeyCpy, NVM_DEFAULT_VALUE,
           (select_sector.ram_ptr + select_sector.sector_size - PtKeyCpy));

    return NVM_OK;
}
