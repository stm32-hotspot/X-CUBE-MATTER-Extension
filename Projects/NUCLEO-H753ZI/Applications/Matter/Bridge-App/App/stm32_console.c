/**
  ******************************************************************************
  * @file    stm32_console.c
  * @author  MCD Application Team
  * @brief   Manage console input / output
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2024 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include <stdint.h>
#include <string.h>
#include <stdio.h>

#include "main.h"
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

#include "AppLog.h"
#include "stm32_console.h"
#include "stm32_nvm_implement.h"

/* Private defines -----------------------------------------------------------*/

#define CONSOLE_BUFFER_SIZE           (uint32_t)35
#define CONSOLE_THREAD_NAME           "ConsoleTask"
#define CONSOLE_THREAD_STACK_DEPTH    (configMINIMAL_STACK_SIZE * 3)
#define CONSOLE_THREAD_PRIORITY       (5)
#define CONSOLE_UART_HANDLE           huart3
#define UART3_BUFFER_SIZE             (CONSOLE_BUFFER_SIZE)
#define UART3_INPUT_ENTER             ((uint8_t)'\n')

/* The time to block waiting for input. */
#define UART_TIME_WAITING_FOR_INPUT (portMAX_DELAY)

/* Private typedef -----------------------------------------------------------*/
typedef struct
{
  uint8_t buffer[UART3_BUFFER_SIZE];
  uint8_t len;
} Uart3_Buffer_t;

/* Private macros ------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/
static Uart3_Buffer_t Uart3_Buffer[2];
static __IO uint8_t Uart3_BufferWriteIdx   = 0U;
static __IO uint8_t Uart3_BufferWritingIdx = 0U;
static __IO uint8_t Uart3_BufferCompleted  = 0U;
SemaphoreHandle_t Uart3RxSemaphore;

TaskHandle_t  sConsoleTaskHandle;
SemaphoreHandle_t *ptConsoleRxSemaphore;  /* Semaphore to signal incoming console input */

/* Global variables ----------------------------------------------------------*/
extern bool AppTreatInput(uint8_t *p_UartBuffer, uint8_t UartBufferLen);
extern void AppHelpInput(void);

/* Private function prototypes -----------------------------------------------*/
static void CONSOLE_TaskMain(void *p_argument);
static void CONSOLE_TreatInput(uint8_t *ptBuffer, uint8_t BufferLen);
static void CONSOLE_InputHelp(void);

/* Private function Definition -----------------------------------------------*/
static void CONSOLE_InputHelp(void)
{
  /* Generic Help */
  STM32_LOG("Console commands help:");
  STM32_LOG("nvm save    : save whole NVM to Flash");
  STM32_LOG("nvm reset   : reset whole NVM / Flash and reboot");
  STM32_LOG("nvm status  : status NVM usage");
  STM32_LOG("config reset: reset the NVM CONFIG part");
  STM32_LOG("reboot      : reboot");
  /* Application Help */
  AppHelpInput();
}

static void CONSOLE_TreatInput(uint8_t *ptBuffer, uint8_t BufferLen)
{
  bool treated = true;
  NVM_StatusTypeDef nvmStatus;

  /* Generic Treatment */
  if ((BufferLen >= 8U) && (memcmp(ptBuffer, "nvm save", 8) == 0))
  {
    nvmStatus = NVM_Save();
    STM32_LOG("NVM %s", (nvmStatus == NVM_OK) ? "saved" : "NOT saved !!!");
  }
  else if ((BufferLen >= 9U) && (memcmp(ptBuffer, "nvm reset", 9) == 0))
  {
    NVM_FactoryReset();
  }
  else if ((BufferLen >= 12U) && (memcmp(ptBuffer, "config reset", 12) == 0))
  {
    nvmStatus = NVM_Reset(NVM_SECTOR_CONFIG);
    STM32_LOG("Config in NVM %s", (nvmStatus == NVM_OK) ? "reset" : "NOT reset !!!");
  }
  else if ((BufferLen >= 10U) && (memcmp(ptBuffer, "nvm status", 10) == 0))
  {
    uint32_t memoryUsed = 0U;
    uint32_t memoryFree = 0U;

    nvmStatus = NVM_MemoryStatus(&memoryUsed, &memoryFree, NVM_SECTOR_CONFIG);
    if (nvmStatus == NVM_OK)
    {
      STM32_LOG("NVM status CONFIG: used:%u - free:%u", memoryUsed, memoryFree);
    }
    else
    {
      STM32_LOG("NVM status CONFIG: ERROR around %u!", memoryUsed);
    }
    nvmStatus = NVM_MemoryStatus(&memoryUsed, &memoryFree, NVM_SECTOR_APPLICATION);
    if (nvmStatus == NVM_OK)
    {
      STM32_LOG("NVM status APPLICATION: used:%u - free:%u", memoryUsed, memoryFree);
    }
    else
    {
      STM32_LOG("NVM status APPLICATION: ERROR around %u!", memoryUsed);
    }
  }
  else if ((BufferLen >= 6U) && (memcmp(ptBuffer, "reboot", 6) == 0))
  {
    HAL_NVIC_SystemReset();
  }
  else
  {
    /* Application Treatment */
    treated = AppTreatInput(ptBuffer, BufferLen);
  }

  /* Display supported commands ? */
  if (treated == false)
  {
    CONSOLE_InputHelp();
  }
}

/* Functions Definition ------------------------------------------------------*/
bool CONSOLE_Init(void)
{
  bool ret = false;

  uint8_t welcome[] = "Welcome on Bridge Matter Application\n";

  /* Force no buffer for the printf() usage. */
  setbuf(stdout, NULL);
  /* Force no buffer for the getc() usage. */
  setbuf(stdin, NULL);

  /* Display welcome */
  HAL_UART_Transmit(&CONSOLE_UART_HANDLE, welcome, (uint8_t)strlen((char *)welcome), 0xFFFF);

  /* Create Semaphore */
  Uart3RxSemaphore = xSemaphoreCreateBinary();
  if (Uart3RxSemaphore != NULL)
  {
    ptConsoleRxSemaphore = &Uart3RxSemaphore;
    /* Create task handling Console UART input */
    xTaskCreate(CONSOLE_TaskMain, CONSOLE_THREAD_NAME, CONSOLE_THREAD_STACK_DEPTH, NULL, CONSOLE_THREAD_PRIORITY,
                &sConsoleTaskHandle);
    if (sConsoleTaskHandle == NULL)
    {
      STM32_LOG("!!! Failed to create ConsoleTask !!!");
    }
    else
    {
      HAL_UART_Receive_IT(&CONSOLE_UART_HANDLE,
                          (uint8_t *) & (Uart3_Buffer[Uart3_BufferWritingIdx].buffer[Uart3_BufferWriteIdx]), 1);
      ret = true;
    }
  }

  return (ret);
}

static void CONSOLE_TaskMain(void *p_argument)
{
  (void)(p_argument);

  for (;;)
  {
    if (xSemaphoreTake(*ptConsoleRxSemaphore, UART_TIME_WAITING_FOR_INPUT) == pdTRUE)
    {
      CONSOLE_TreatInput(&Uart3_Buffer[Uart3_BufferCompleted].buffer[0], Uart3_Buffer[Uart3_BufferCompleted].len);
    }
  }
}

#if (defined(__GNUC__) && !defined(__ARMCC_VERSION))
int __io_putchar(int ch);
#define PUTCHAR_PROTOTYPE int __io_putchar(int ch)

#elif defined(__ARMCC_VERSION)
#ifdef __MICROLIB
int fputc(int ch, FILE *f);
#define PUTCHAR_PROTOTYPE int fputc(int ch, FILE *f)
#else
int stdout_putchar(int ch);
#define PUTCHAR_PROTOTYPE int stdout_putchar(int ch)
#endif /* __MICROLIB */

#elif defined(__ICCARM__)
int iar_fputc(int ch);
#define PUTCHAR_PROTOTYPE int iar_fputc(int ch)
#endif /* defined(__GNUC__) && !defined(__ARMCC_VERSION) */

/**
  * @brief  Retargets the C library printf function to the USART.
  * @param  None
  * @retval None
  */
PUTCHAR_PROTOTYPE
{
  /* Write a character to the USART. */
  HAL_UART_Transmit(&CONSOLE_UART_HANDLE, (uint8_t *)&ch, 1, 0xFFFF);

  return ch;
}

#if defined(__ICCARM__)
size_t __write(int file, unsigned char const *ptr, size_t len)
{
  size_t idx;
  unsigned char const *pdata = ptr;

  for (idx = 0; idx < len; idx++)
  {
    iar_fputc((int)*pdata);
    pdata++;
  }
  return len;
}
#endif /* __ICCARM__ */

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
  if (huart->Instance == USART3)
  {
    if ((Uart3_Buffer[Uart3_BufferWritingIdx].buffer[Uart3_BufferWriteIdx] == UART3_INPUT_ENTER)
        || (Uart3_BufferWriteIdx == UART3_BUFFER_SIZE - 1U))
    {
      portBASE_TYPE taskWoken = pdFALSE;
      Uart3_BufferCompleted = Uart3_BufferWritingIdx;
      Uart3_Buffer[Uart3_BufferWritingIdx].buffer[Uart3_BufferWriteIdx] = (uint8_t)'\0';
      Uart3_Buffer[Uart3_BufferWritingIdx].len = Uart3_BufferWriteIdx;
      if (Uart3_BufferWritingIdx == 1U)
      {
        Uart3_BufferWritingIdx = 0U;
      }
      else
      {
        Uart3_BufferWritingIdx = 1U;
      }
      Uart3_BufferWriteIdx = 0;
      if (xSemaphoreGiveFromISR(Uart3RxSemaphore, &taskWoken) == pdTRUE)
      {
        portEND_SWITCHING_ISR(taskWoken);
      }
    }
    else
    {
      Uart3_BufferWriteIdx++;
    }
    HAL_UART_Receive_IT(&CONSOLE_UART_HANDLE,
                        (uint8_t *)&Uart3_Buffer[Uart3_BufferWritingIdx].buffer[Uart3_BufferWriteIdx], 1);
  }
}

void vMainUARTPrintString(const char * pcString)
{
  const uint32_t ulTimeout = 3000UL;
  HAL_UART_Transmit(&CONSOLE_UART_HANDLE, (uint8_t *)pcString, strlen(pcString), ulTimeout);
}

extern __WEAK bool AppTreatInput(uint8_t *p_UartBuffer, uint8_t UartBufferLen)
{
  (void)(p_UartBuffer);
  (void)(UartBufferLen);
  return false;
}

extern __WEAK void AppHelpInput() {}
