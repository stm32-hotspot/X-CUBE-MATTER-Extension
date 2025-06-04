/**
  ******************************************************************************
  * @file    app_ethernet.c
  * @author  MCD Application Team
  * @brief   Ethernet specific module
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
#include "app_ethernet.h"

#include <stdbool.h>

#include "FreeRTOS.h"
#include "task.h"
#include "lwip/opt.h"
#include "lwip/dhcp.h"

#include "main.h"
#include "AppLog.h"
#include "ethernetif.h"

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
#define MAX_DHCP_TRIES  4
__IO uint8_t DHCP_state = DHCP_OFF;
SemaphoreHandle_t IPCfgSemaphore = NULL;

/* Private function prototypes -----------------------------------------------*/
/* Private functions ---------------------------------------------------------*/
/**
  * @brief  Notify the User about the network interface config status
  * @param  netif: the network interface
  * @retval None
  */
void ethernet_link_status_updated(struct netif *netif)
{
  if (netif_is_up(netif))
  {
    /* Update DHCP state machine */
    DHCP_state = DHCP_START;
  }
  else
  {
    /* Update DHCP state machine */
    DHCP_state = DHCP_LINK_DOWN;
  }
}

void ethernet_status_updated(struct netif *netif)
{
  static bool done_once = false;

  if (!done_once)
  {
    if (dhcp_supplied_address(netif) == 1)
    {
      done_once = true;
      xSemaphoreGive(IPCfgSemaphore);
    }
  }
}

/**
  * @brief  DHCP Process
  * @param  argument: network interface
  * @retval None
  */
void DHCP_Thread(void * argument)
{
  struct netif *netif = (struct netif *) argument;
  struct dhcp *dhcp;

  uint8_t iptxt[20];

  for (;;)
  {
    switch (DHCP_state)
    {
    case DHCP_START:
    {
      STM32_LOG("DHCP State: Looking for DHCP server ...");
      ip_addr_set_zero_ip4(&netif->ip_addr);
      ip_addr_set_zero_ip4(&netif->netmask);
      ip_addr_set_zero_ip4(&netif->gw);
      DHCP_state = DHCP_WAIT_ADDRESS;
      BSP_LED_On(LED3);
      dhcp_start(netif);
      break;
    }

    case DHCP_WAIT_ADDRESS:
    {
      STM32_LOG("DHCP WAIT ADDRESS");
      if (dhcp_supplied_address(netif))
      {
        DHCP_state = DHCP_ADDRESS_ASSIGNED;
        sprintf((char *)iptxt, "%s", ip4addr_ntoa((const ip4_addr_t *)&netif->ip_addr));
        STM32_LOG("IP address assigned by a DHCP server: %s", iptxt);
        BSP_LED_Off(LED3);
      }
      else
      {
        dhcp = (struct dhcp *)netif_get_client_data(netif, LWIP_NETIF_CLIENT_DATA_INDEX_DHCP);
        /* DHCP timeout */
        STM32_LOG("DHCP timeout !!");
        if (dhcp->tries > MAX_DHCP_TRIES)
        {
          DHCP_state = DHCP_TIMEOUT;
          /* Stop DHCP */
          dhcp_stop(netif);
          BSP_LED_On(LED3);
        }
      }
      break;
    }

    case DHCP_LINK_DOWN:
    {
      STM32_LOG("DHCP_LINK_DOWN");
      /* Stop DHCP */
      dhcp_stop(netif);
      DHCP_state = DHCP_OFF;
      BSP_LED_On(LED3);
      break;
    }

    default:
      break;
    }
    /* wait 500 ms */
    vTaskDelay(500);
  }
}
