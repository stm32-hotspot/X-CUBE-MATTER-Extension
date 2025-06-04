/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    ethernetif.h
  * @author  MCD Application Team
  * @brief   Header for ethernetif.c module
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
/* USER CODE END Header */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __ETHERNETIF_H__
#define __ETHERNETIF_H__

#ifdef __cplusplus
 extern "C" {
#endif

#include "lwip/err.h"
#include "lwip/netif.h"

/* Within 'USER CODE' section, code will be kept by default at each generation */
/* USER CODE BEGIN 0 */
#define ETH_MACADDR_0 (0xBA) /* used for Ethernet MAC address Byte0 */
#define ETH_MACADDR_1 (0xD0) /* used for Ethernet MAC address Byte1 */
/* Byte2 Byte3 Byte4 and Byte5 Ethernet MAC address are configured according to the hardware used */
/* #define ETH_MACADDR_2 (0xDE) */ /* used for Ethernet MAC address Byte2 */
/* #define ETH_MACADDR_3 (0xAD) */ /* used for Ethernet MAC address Byte3 */
/* #define ETH_MACADDR_4 (0xBE) */ /* used for Ethernet MAC address Byte4 */
/* #define ETH_MACADDR_5 (0xEF) */ /* used for Ethernet MAC address Byte5 */
/* USER CODE END 0 */

/* Exported functions ------------------------------------------------------- */
err_t ethernetif_init(struct netif *netif);
void EthernetLinkTask(void *p_argument);

/* USER CODE BEGIN 1 */

/* USER CODE END 1 */

#ifdef __cplusplus
}
#endif

#endif /* __ETHERNETIF_H__ */

