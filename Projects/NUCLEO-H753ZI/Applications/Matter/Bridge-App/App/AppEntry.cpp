/**
  ******************************************************************************
  * @file           : AppEntry.c
  * @brief          : Bridge between C and C++
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
#ifdef __cplusplus
extern "C" {
#endif

#include "AppEntry.h"

#include <string.h>
#include <stdint.h>

#include "main.h"

#include "stm32_console.h"

#include "lwip/netif.h"
#include "lwip/tcpip.h"
#include "lwip/opt.h"
#include "lwipopts.h"
#include "ethernetif.h"
#include "app_ethernet.h"

#ifdef __cplusplus
}
#endif

#include "FreeRTOS.h"
#include "task.h"
#include "portable.h"

#include "CHIPDevicePlatformConfig.h"
#include "AppLog.h"
#include "AppTask.h"

#include "STM32FreeRtosHooks.h"

/* Private variables ---------------------------------------------------------*/

/* Callback function to handle Button action */
static ButtonCallback ButtonCb = NULL;
/* Callback function to handle Console input */
static UartRxCallback ConsoleRxCb = NULL;
static UartHelpCallback ConsoleHelpCb = NULL;

static __IO bool ButtonUserState;

/* Private typedef -----------------------------------------------------------*/

/* Private macros ------------------------------------------------------------*/

#ifdef __cplusplus
extern "C" {
#endif

/* Private defines -----------------------------------------------------------*/
#define USER_BUTTON_TIME_WAITING_FOR_INPUT (portMAX_DELAY)
#define BUTTON_THREAD_NAME           "ButtonTask"
#define BUTTON_THREAD_STACK_DEPTH    (configMINIMAL_STACK_SIZE * 2)
#define BUTTON_THREAD_PRIORITY       (5)

#if LWIP_NETIF_LINK_CALLBACK
#define ETHLINK_THREAD_NAME           "EthLinkTask"
#define ETHLINK_THREAD_STACK_DEPTH    (configMINIMAL_STACK_SIZE * 2)
#define ETHLINK_THREAD_PRIORITY       (2)
#endif /* LWIP_NETIF_LINK_CALLBACK */

/* Private variables ---------------------------------------------------------*/
struct netif gnetif; /* network interface structure */

TaskHandle_t sButtonTaskHandle;
#if LWIP_NETIF_LINK_CALLBACK
TaskHandle_t sEthLinkTaskHandle;
#endif /* LWIP_NETIF_LINK_CALLBACK */

/* Global variables ----------------------------------------------------------*/

/* Private function prototypes -----------------------------------------------*/
static bool Netif_Config(void);
static void ButtonProcessTask(void *p_argument);
void vApplicationDaemonTaskStartupHook(void);

#ifdef __cplusplus
}
#endif

static void prvInitializeHeap(void);

/* Private function Definition -----------------------------------------------*/
static void prvInitializeHeap(void)
{
  static uint8_t ucHeap1[configTOTAL_HEAP_SIZE];

  HeapRegion_t xHeapRegions[] =
  {
    {(unsigned char *)ucHeap1, sizeof(ucHeap1)},
    {NULL, 0}
  };

  vPortDefineHeapRegions(xHeapRegions);
}

/**
 * @brief  Configure netif and ethernet
 * @param  None
 * @retval None
 */
static bool Netif_Config(void)
{
  bool result = true;
  ip_addr_t ipaddr;
  ip_addr_t netmask;
  ip_addr_t gw;

  /* IPv6 only */
  ip_addr_set_zero_ip4(&ipaddr);
  ip_addr_set_zero_ip4(&netmask);
  ip_addr_set_zero_ip4(&gw);
  /* add the network interface */
  netif_add(&gnetif, (ip_2_ip4(&ipaddr)), (ip_2_ip4(&netmask)), (ip_2_ip4(&gw)), NULL, &ethernetif_init, &tcpip_input);

  /* Create IPv6 local address */
  gnetif.name[0] = 'A';
  gnetif.name[1] = 'R';
  gnetif.ip6_autoconfig_enabled = 1;
  netif_create_ip6_linklocal_address(&gnetif, 1);
  /* netif_ip6_addr_set_state(&gnetif, 0, IP6_ADDR_VALID); */
  STM32_LOG("IPv6 linklocal address: %s\n", ip6addr_ntoa(netif_ip6_addr(&gnetif, 0)));
  gnetif.flags |= NETIF_FLAG_MLD6;

  /*  Registers the default network interface. */
  netif_set_default(&gnetif);

  ethernet_link_status_updated(&gnetif);

#if LWIP_NETIF_LINK_CALLBACK
  netif_set_link_callback(&gnetif, ethernet_link_status_updated);

  /* Create EthernetLinkTask. */
  xTaskCreate(EthernetLinkTask, ETHLINK_THREAD_NAME, ETHLINK_THREAD_STACK_DEPTH, &gnetif, ETHLINK_THREAD_PRIORITY,
              &sEthLinkTaskHandle);
  if (sEthLinkTaskHandle == NULL)
  {
    STM32_LOG("!!! Failed to create EthLinkTask !!!");
    result = false;
  }
#endif /* LWIP_NETIF_LINK_CALLBACK */

  return result;
}

#ifdef __cplusplus
extern "C" {
#endif

static bool StartIP(void);
static bool StartIP(void)
{
  bool result;

  /* Initialize LwIP stack */
  tcpip_init(NULL, NULL);
  /* Configure netif and ethernet */
  result = Netif_Config();

  return result;
}

/* Functions Definition ------------------------------------------------------*/
bool AppEntryInit(void)
{
  bool result;

  /* Heap_5 is being used because the RAM is not contiguous in memory,
   * so the heap must be initialized. */
  prvInitializeHeap();

  ButtonUserState = (BSP_PB_GetState(BUTTON_USER) == 1) ? true : false;
  /* MbedTLS initialization. */
  freertos_mbedtls_init();

  /* Console initialization */
  result = CONSOLE_Init();

  if (result == true)
  {
    /* IP/Netif Start */
    result = StartIP();
  }
  if (result == true)
  {
    /* AppTask initialization */
    result = (GetAppTask().Init() == CHIP_NO_ERROR ? true : false);
  }

  return (result);
}

/**
 * @brief Should be called after complete netif initialization
 *
 * @param buf the buffer to provide MAC address
 * @return MAC address length if copy as been done
 *         0 on error
 */
uint8_t stm32_GetMACAddr(uint8_t *buf_data, size_t buf_size)
{
  uint8_t ret = 0U;

  if (buf_size >= NETIF_MAX_HWADDR_LEN)
  {
    /* set MAC hardware address */
    memcpy(buf_data, gnetif.hwaddr, NETIF_MAX_HWADDR_LEN);
    ret = NETIF_MAX_HWADDR_LEN;
  }

  return ret;
}

void ConsoleSetCallback(UartRxCallback aUartRxCb, UartHelpCallback aUartHelpCb)
{
  ConsoleRxCb = aUartRxCb;
  ConsoleHelpCb = aUartHelpCb;
}

bool AppTreatInput(uint8_t *ptUartString, uint8_t UartStringLen)
{
    bool ret = false;

    if (ConsoleRxCb != NULL)
    {
        ret = ConsoleRxCb(ptUartString, UartStringLen);
    }

    return ret;
}

void AppHelpInput(void)
{
    if (ConsoleHelpCb != NULL)
    {
        ConsoleHelpCb();
    }
}

bool AppEntryStart(void)
{
    bool result = true;

    /* AppTask start. */
    if (GetAppTask().Start() == CHIP_NO_ERROR)
    {
        xTaskCreate(ButtonProcessTask, BUTTON_THREAD_NAME, BUTTON_THREAD_STACK_DEPTH, NULL, BUTTON_THREAD_PRIORITY,
                    &sButtonTaskHandle);
        if (sButtonTaskHandle == NULL)
        {
            STM32_LOG("!!! Failed to create ButtonProcessTask !!!");
            result = false;
        }
    }
    else
    {
        result = false;
    }

    return result;
}

void ButtonSetCallback(ButtonCallback aCallback)
{
  ButtonCb = aCallback;
}

static void ButtonProcessTask(void *p_argument)
{
  (void)(p_argument);

  for (;;)
  {
    ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
    {
        ButtonUserState = (BSP_PB_GetState(BUTTON_USER) == 1) ? true : false;
        STM32_LOG("Button %s", (ButtonUserState == true) ? "PRESSED" : "RELEASED");
        if (ButtonCb != NULL)
        {
            ButtonState_t Message;
            Message.Button = BUTTON_USER;
            Message.State = (ButtonUserState == true) ? 1 : 0;
            ButtonCb(&Message); /* callback to handle button */
        }
    }
  }
}

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
  switch (GPIO_Pin)
  {
    case (USER_BUTTON_Pin):
    {
      portBASE_TYPE xHigherPriorityTaskWoken = pdFALSE;
      vTaskNotifyGiveFromISR(sButtonTaskHandle, &xHigherPriorityTaskWoken);
      portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
      break;
    }
    default:
      break;
  }
}

void vApplicationIdleHook(void)
{
  static TickType_t xLastPrint = 0;
  TickType_t xTimeNow;
  const TickType_t xPrintFrequency = pdMS_TO_TICKS(2000);

  xTimeNow = xTaskGetTickCount();

  if( (xTimeNow - xLastPrint) > xPrintFrequency)
  {
    // configPRINT( "." );
    xLastPrint = xTimeNow;
  }
}

void vApplicationDaemonTaskStartupHook(void)
{
}

#ifdef __cplusplus
}
#endif
