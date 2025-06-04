/**
  ******************************************************************************
  * @file    ethernetif.c
  * @author  MCD Application Team
  * @brief   Configuration of the ethernetif middleware
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
#include "ethernetif.h"

#include <string.h>

#include "FreeRTOS.h"
#include "semphr.h"
#include "lan8742.h"
#include "lwip/opt.h"
#include "lwip/ethip6.h"
#include "lwip/snmp.h"
#include "lwip/tcpip.h"
#include "netif/etharp.h"
#include "netif/ethernet.h"

#include "main.h"
#include "AppLog.h"

/* Private define ------------------------------------------------------------*/
/* The time to block waiting for input. */
#define TIME_WAITING_FOR_INPUT ( portMAX_DELAY )
/* Stack size of the interface thread */
#define ETHERNETIF_INPUT_THREAD_NAME          "EthIfInputTask"
#define ETHERNETIF_INPUT_THREAD_STACK_DEPTH   (configMINIMAL_STACK_SIZE * 1)
#define ETHERNETIF_INPUT_THREAD_PRIORITY      (0)
/* Network interface name */
#define IFNAME0 's'
#define IFNAME1 't'

/* ETH Setting  */
#define ETH_RX_BUFFER_SIZE                     ( 1536UL )
#define ETH_DMA_TRANSMIT_TIMEOUT               ( 20U )

/* USER CODE BEGIN 1 */

/* USER CODE END 1 */

/* Private variables ---------------------------------------------------------*/
/*
@Note: This interface is implemented to operate in zero-copy mode only:
        - Rx buffers will be allocated from LwIP stack memory heap,
          then passed to ETH HAL driver.
        - Tx buffers will be allocated from LwIP stack memory heap,
          then passed to ETH HAL driver.

@Notes:
  1.a. ETH DMA Rx descriptors must be contiguous, the default count is 4,
       to customize it please redefine ETH_RX_DESC_CNT in ETH GUI (Rx Descriptor Length)
       so that updated value will be generated in stm32xxxx_hal_conf.h
  1.b. ETH DMA Tx descriptors must be contiguous, the default count is 4,
       to customize it please redefine ETH_TX_DESC_CNT in ETH GUI (Tx Descriptor Length)
       so that updated value will be generated in stm32xxxx_hal_conf.h

  2.a. Rx Buffers number must be between ETH_RX_DESC_CNT and 2*ETH_RX_DESC_CNT
  2.b. Rx Buffers must have the same size: ETH_RX_BUFFER_SIZE, this value must
       passed to ETH DMA in the init field (heth.Init.RxBuffLen)
  2.c  The RX Ruffers addresses and sizes must be properly defined to be aligned
       to L1-CACHE line size (32 bytes).
*/

/* Data Type Definitions */
typedef enum
{
  RX_ALLOC_OK       = 0x00,
  RX_ALLOC_ERROR    = 0x01
} RxAllocStatusTypeDef;

typedef struct
{
  struct pbuf_custom pbuf_custom;
  uint8_t buff[(ETH_RX_BUFFER_SIZE + 31) & ~31] __ALIGNED(32);
} RxBuff_t;

/* Variable Definitions */
static uint8_t RxAllocStatus;

#if defined ( __ICCARM__ )

#pragma location=0x30040000
ETH_DMADescTypeDef  DMARxDscrTab[ETH_RX_DESC_CNT]; /* Ethernet Rx DMA Descriptors */
#pragma location=0x30040060
ETH_DMADescTypeDef  DMATxDscrTab[ETH_TX_DESC_CNT]; /* Ethernet Tx DMA Descriptors */
#pragma location=0x30040200
uint8_t memp_memory_RX_POOL_base[] __attribute__((section(".RxArraySection"))); /* Ethernet Receive Buffers */

#elif defined ( __CC_ARM )

__attribute__((at(0x30040000))) ETH_DMADescTypeDef  DMARxDscrTab[ETH_RX_DESC_CNT]; /* Ethernet Rx DMA Descriptors */
__attribute__((at(0x30040060))) ETH_DMADescTypeDef  DMATxDscrTab[ETH_TX_DESC_CNT]; /* Ethernet Tx DMA Descriptors */
extern __attribute__((at(0x30040200))) uint8_t memp_memory_RX_POOL_base; /* Ethernet Receive Buffer */

#elif defined ( __GNUC__ )

/* Ethernet Rx DMA Descriptors */
ETH_DMADescTypeDef DMARxDscrTab[ETH_RX_DESC_CNT] __attribute__((section(".RxDescripSection")));
/* Ethernet Tx DMA Descriptors */
ETH_DMADescTypeDef DMATxDscrTab[ETH_TX_DESC_CNT] __attribute__((section(".TxDescripSection")));
/* Ethernet Receive Buffers */
uint8_t memp_memory_RX_POOL_base[] __attribute__((section(".RxArraySection")));

#endif /* ( __ICCARM__ ) */

/* USER CODE BEGIN 2 */

/* USER CODE END 2 */

SemaphoreHandle_t RxPktSemaphore = NULL; /* Semaphore to signal incoming packets */
SemaphoreHandle_t TxPktSemaphore = NULL; /* Semaphore to signal transmit packets complete */

TaskHandle_t sEthernetIfInputHandle;

/* Global Ethernet handle */
ETH_HandleTypeDef heth;
ETH_TxPacketConfigTypeDef TxConfig;

/* Memory Pool Declaration */
#define ETH_RX_BUFFER_CNT             10U

LWIP_MEMPOOL_DECLARE(RX_POOL, ETH_RX_BUFFER_CNT, sizeof(RxBuff_t), "Zero-copy RX PBUF pool");

/* Private function prototypes -----------------------------------------------*/
static void ethernetif_input(void *p_argument);
void pbuf_free_custom(struct pbuf *p);

int32_t ETH_PHY_IO_Init(void);
int32_t ETH_PHY_IO_DeInit(void);
int32_t ETH_PHY_IO_ReadReg(uint32_t DevAddr, uint32_t RegAddr, uint32_t *pRegVal);
int32_t ETH_PHY_IO_WriteReg(uint32_t DevAddr, uint32_t RegAddr, uint32_t RegVal);
int32_t ETH_PHY_IO_GetTick(void);

lan8742_Object_t LAN8742;
lan8742_IOCtx_t  LAN8742_IOCtx =
{
  ETH_PHY_IO_Init,
  ETH_PHY_IO_DeInit,
  ETH_PHY_IO_WriteReg,
  ETH_PHY_IO_ReadReg,
  ETH_PHY_IO_GetTick
};

/* USER CODE BEGIN 3 */

/* USER CODE END 3 */

/* Private functions ---------------------------------------------------------*/

/**
  * @brief  Ethernet Rx Transfer completed callback
  * @param  heth: ETH handle
  * @retval None
  */
void HAL_ETH_RxCpltCallback(ETH_HandleTypeDef *heth)
{
  (void)(heth);
  portBASE_TYPE taskWoken = pdFALSE;

  if (xSemaphoreGiveFromISR(RxPktSemaphore, &taskWoken) == pdTRUE)
  {
    portEND_SWITCHING_ISR(taskWoken);
  }
}

/**
  * @brief  Ethernet Tx Transfer completed callback
  * @param  heth: ETH handler
  * @retval None
  */
void HAL_ETH_TxCpltCallback(ETH_HandleTypeDef *heth)
{
  (void)(heth);
  portBASE_TYPE taskWoken = pdFALSE;

  if (xSemaphoreGiveFromISR(TxPktSemaphore, &taskWoken) == pdTRUE)
  {
    portEND_SWITCHING_ISR(taskWoken);
  }
}

/* USER CODE BEGIN 4 */

/**
  * @brief  Ethernet DMA transfer error callback
  * @param  heth: ETH handler
  * @retval None
  */
void HAL_ETH_ErrorCallback(ETH_HandleTypeDef *heth)
{
  if((HAL_ETH_GetDMAError(heth) & ETH_DMACSR_RBU) == ETH_DMACSR_RBU)
  {
    /* The Receive Buffer Unavailable interrupt is cleared by the Ethernet HAL handler before the
       Ethernet task is scheduled.
       Temporarily mask it so that the Ethernet task is scheduled before a new IT is triggered.
     */
    portBASE_TYPE taskWoken = pdFALSE;

    if (xSemaphoreGiveFromISR(RxPktSemaphore, &taskWoken) == pdTRUE)
    {
      portEND_SWITCHING_ISR(taskWoken);
    }
  }
}

/* USER CODE END 4 */

/*******************************************************************************
                       LL Driver Interface ( LwIP stack --> ETH)
*******************************************************************************/
/**
  * @brief In this function, the hardware should be initialized.
  * Called from ethernetif_init().
  *
  * @param netif the already initialized lwip network interface structure
  *        for this ethernetif
  */
static void low_level_init(struct netif *netif)
{
  HAL_StatusTypeDef hal_eth_init_status;
  uint32_t duplex;
  uint32_t speed;
  int32_t PHYLinkState = 0;
  uint32_t uidW0 = HAL_GetUIDw0();
  uint32_t uidW1 = HAL_GetUIDw1();
  ETH_MACConfigTypeDef MACConf = {0};

  /* Start ETH HAL Init */
  uint8_t MACAddr[6];
  heth.Instance = ETH;

  MACAddr[0] = ETH_MACADDR_0;
  MACAddr[1] = ETH_MACADDR_1;
  /* Byte2 Byte3 Byte4 and Byte5 Ethernet MAC address are configured
     according to the hardware used */
  /*
    MACAddr[2] = ETH_MACADDR_2;
    MACAddr[3] = ETH_MACADDR_3;
    MACAddr[4] = ETH_MACADDR_4;
    MACAddr[5] = ETH_MACADDR_5;
  */
  MACAddr[2] = (uint8_t)((uidW1 & 0xFF000000) >> 24);
  MACAddr[3] = (uint8_t)((uidW1 & 0x000000FF) >> 0);
  MACAddr[4] = (uint8_t)((uidW0 & 0x00FF0000) >> 16);
  MACAddr[5] = (uint8_t)((uidW0 & 0x000000FF) >> 0);

  heth.Init.MACAddr = &MACAddr[0];
  STM32_LOG("MACAddr: %X:%X:%X:%X:%X:%X", MACAddr[0], MACAddr[1], MACAddr[2], MACAddr[3], MACAddr[4], MACAddr[5]);
  heth.Init.MediaInterface = HAL_ETH_RMII_MODE;
  heth.Init.TxDesc = DMATxDscrTab;
  heth.Init.RxDesc = DMARxDscrTab;
  /* heth.Init.RxBuffLen = 1536; */
  heth.Init.RxBuffLen = ETH_RX_BUFFER_SIZE;

  /* USER CODE BEGIN MACADDRESS */

  /* USER CODE END MACADDRESS */

  hal_eth_init_status = HAL_ETH_Init(&heth);

  /* memset(&TxConfig, 0 , sizeof(ETH_TxPacketConfigTypeDef)); */
  memset(&TxConfig, 0, sizeof(TxConfig));
  TxConfig.Attributes = ETH_TX_PACKETS_FEATURES_CSUM | ETH_TX_PACKETS_FEATURES_CRCPAD;
  TxConfig.ChecksumCtrl = ETH_CHECKSUM_IPHDR_PAYLOAD_INSERT_PHDR_CALC;
  TxConfig.CRCPadCtrl = ETH_CRC_PAD_INSERT;
  /* End ETH HAL Init */

  /* Initialize the RX POOL */
  LWIP_MEMPOOL_INIT(RX_POOL);

  /* Pass all multicast frames: needed for IPv6 protocol*/
  heth.Instance->MACPFR |= ETH_MACPFR_PM;

#if LWIP_ARP || LWIP_ETHERNET

  /* set MAC hardware address length */
  netif->hwaddr_len = ETH_HWADDR_LEN;
  /* set MAC hardware address */
  netif->hwaddr[0] =  heth.Init.MACAddr[0];
  netif->hwaddr[1] =  heth.Init.MACAddr[1];
  netif->hwaddr[2] =  heth.Init.MACAddr[2];
  netif->hwaddr[3] =  heth.Init.MACAddr[3];
  netif->hwaddr[4] =  heth.Init.MACAddr[4];
  netif->hwaddr[5] =  heth.Init.MACAddr[5];

  /* maximum transfer unit */
  netif->mtu = ETH_MAX_PAYLOAD;

  /* Accept broadcast address and ARP traffic */
  /* don't set NETIF_FLAG_ETHARP if this device is not an ethernet one */
#if LWIP_ARP
  netif->flags |= NETIF_FLAG_BROADCAST | NETIF_FLAG_ETHARP;
#else
  netif->flags |= NETIF_FLAG_BROADCAST;
#endif /* LWIP_ARP */

#if LWIP_IGMP
  netif->flags |= NETIF_FLAG_IGMP;
#endif /* LWIP_IGMP */

  /* create a binary semaphore used for informing ethernetif of frame reception */
  RxPktSemaphore = xSemaphoreCreateBinary();
  /* create a binary semaphore used for informing ethernetif of frame transmission */
  TxPktSemaphore = xSemaphoreCreateBinary();

  /* create the task that handles the ETH_MAC */
  xTaskCreate(ethernetif_input, ETHERNETIF_INPUT_THREAD_NAME, ETHERNETIF_INPUT_THREAD_STACK_DEPTH, netif,
              ETHERNETIF_INPUT_THREAD_PRIORITY, &sEthernetIfInputHandle);
  if (sEthernetIfInputHandle == NULL)
  {
    STM32_LOG("!!! Failed to create EthernetIfInputTask !!!");
  }

  /* USER CODE BEGIN PHY_PRE_CONFIG */
  {
    ETH_MACFilterConfigTypeDef FilterConfig;
    FilterConfig.PromiscuousMode = ENABLE;
    FilterConfig.PassAllMulticast = ENABLE;
    HAL_ETH_SetMACFilterConfig(&heth, &FilterConfig);
  }

  /* USER CODE END PHY_PRE_CONFIG */
  /* Set PHY IO functions */
  LAN8742_RegisterBusIO(&LAN8742, &LAN8742_IOCtx);

  /* Initialize the LAN8742 ETH PHY */
  LAN8742_Init(&LAN8742);

  if (hal_eth_init_status == HAL_OK)
  {
    PHYLinkState = LAN8742_GetLinkState(&LAN8742);

    /* Get link state */
    if(PHYLinkState <= LAN8742_STATUS_LINK_DOWN)
    {
      netif_set_link_down(netif);
      netif_set_down(netif);
    }
    else
    {
      switch (PHYLinkState)
      {
        case LAN8742_STATUS_100MBITS_FULLDUPLEX:
          duplex = ETH_FULLDUPLEX_MODE;
          speed = ETH_SPEED_100M;
          break;
        case LAN8742_STATUS_100MBITS_HALFDUPLEX:
          duplex = ETH_HALFDUPLEX_MODE;
          speed = ETH_SPEED_100M;
          break;
        case LAN8742_STATUS_10MBITS_FULLDUPLEX:
          duplex = ETH_FULLDUPLEX_MODE;
          speed = ETH_SPEED_10M;
          break;
        case LAN8742_STATUS_10MBITS_HALFDUPLEX:
          duplex = ETH_HALFDUPLEX_MODE;
          speed = ETH_SPEED_10M;
          break;
        default:
          duplex = ETH_FULLDUPLEX_MODE;
          speed = ETH_SPEED_100M;
          break;
      }

      /* Get MAC Config MAC */
      HAL_ETH_GetMACConfig(&heth, &MACConf);
      MACConf.DuplexMode = duplex;
      MACConf.Speed = speed;
      HAL_ETH_SetMACConfig(&heth, &MACConf);

      HAL_ETH_Start_IT(&heth);
      netif_set_up(netif);
      netif_set_link_up(netif);

      /* USER CODE BEGIN PHY_POST_CONFIG */

      /* USER CODE END PHY_POST_CONFIG */
    }

  }
  else
  {
    Error_Handler();
  }
#endif /* LWIP_ARP || LWIP_ETHERNET */

  /* USER CODE BEGIN LOW_LEVEL_INIT */

  /* USER CODE END LOW_LEVEL_INIT */
}

/**
  * @brief This function should do the actual transmission of the packet. The packet is
  * contained in the pbuf that is passed to the function. This pbuf
  * might be chained.
  *
  * @param netif the lwip network interface structure for this ethernetif
  * @param p the MAC packet to send (e.g. IP packet including MAC addresses and type)
  * @return ERR_OK if the packet could be sent
  *         an err_t value if the packet couldn't be sent
  *
  * @note Returning ERR_MEM here if a DMA queue of your MAC is full can lead to
  *       strange results. You might consider waiting for space in the DMA queue
  *       to become available since the stack doesn't retry to send a packet
  *       dropped because of memory failure (except for the TCP timers).
  */

static err_t low_level_output(struct netif *netif, struct pbuf *p)
{
  uint32_t i = 0U;
  struct pbuf *q;
  err_t errval = ERR_OK;
  ETH_BufferTypeDef Txbuffer[ETH_TX_DESC_CNT] = {0};

  (void)(netif);
  /* memset(Txbuffer, 0 , ETH_TX_DESC_CNT*sizeof(ETH_BufferTypeDef)); */
  memset(Txbuffer, 0, sizeof(Txbuffer));

  for(q = p; q != NULL; q = q->next)
  {
    if(i >= ETH_TX_DESC_CNT)
    {
      return ERR_IF;
    }

    Txbuffer[i].buffer = (uint8_t *) q->payload;
    Txbuffer[i].len = q->len;

    if(i > 0U)
    {
      Txbuffer[i-1].next = &Txbuffer[i];
    }

    if(q->next == NULL)
    {
      Txbuffer[i].next = NULL;
    }

    i++;
  }

  TxConfig.Length = p->tot_len;
  TxConfig.TxBuffer = Txbuffer;
  TxConfig.pData = p;

  pbuf_ref(p);

  HAL_ETH_Transmit_IT(&heth, &TxConfig);
  while(xSemaphoreTake(TxPktSemaphore, TIME_WAITING_FOR_INPUT) != pdTRUE)
  {
  }

  HAL_ETH_ReleaseTxPacket(&heth);

  return errval;
}

/**
  * @brief Should allocate a pbuf and transfer the bytes of the incoming
  * packet from the interface into the pbuf.
  *
  * @param netif the lwip network interface structure for this ethernetif
  * @return a pbuf filled with the received packet (including MAC header)
  *         NULL on memory error
  */
static struct pbuf * low_level_input(struct netif *netif)
{
  struct pbuf *p = NULL;
  (void)(netif);

  if(RxAllocStatus == RX_ALLOC_OK)
  {
    HAL_ETH_ReadData(&heth, (void **)&p);
  }

  return p;
}

/**
  * @brief This function should be called when a packet is ready to be read
  * from the interface. It uses the function low_level_input() that
  * should handle the actual reception of bytes from the network
  * interface. Then the type of the received packet is determined and
  * the appropriate input function is called.
  *
  * @param netif the lwip network interface structure for this ethernetif
  */
static void ethernetif_input(void *p_argument)
{
  struct pbuf *p;
  struct netif *netif = (struct netif *)p_argument;

  for( ;; )
  {
    if (xSemaphoreTake(RxPktSemaphore, TIME_WAITING_FOR_INPUT) == pdTRUE)
    {
      do
      {
        LOCK_TCPIP_CORE();
        p = low_level_input( netif );
        if (p != NULL)
        {
          if (netif->input( p, netif) != ERR_OK )
          {
            pbuf_free(p);
          }
        }
        UNLOCK_TCPIP_CORE();
      } while(p!=NULL);
    }
  }
}

#if !LWIP_ARP
/**
  * This function has to be completed by user in case of ARP OFF.
  *
  * @param netif the lwip network interface structure for this ethernetif
  * @return ERR_OK if ...
  */
static err_t low_level_output_arp_off(struct netif *netif, struct pbuf *q, const ip4_addr_t *ipaddr)
{
  err_t errval;
  errval = ERR_OK;

  /* USER CODE BEGIN 5 */

  /* USER CODE END 5 */

  return errval;
}
#endif /* LWIP_ARP */

/**
  * @brief Should be called at the beginning of the program to set up the
  * network interface. It calls the function low_level_init() to do the
  * actual setup of the hardware.
  *
  * This function should be passed as a parameter to netif_add().
  *
  * @param netif the lwip network interface structure for this ethernetif
  * @return ERR_OK if the loopif is initialized
  *         ERR_MEM if private data couldn't be allocated
  *         any other err_t on error
  */
err_t ethernetif_init(struct netif *netif)
{
  /* LWIP_ASSERT("netif != NULL", (netif != NULL)); */

#if LWIP_NETIF_HOSTNAME
  /* Initialize interface hostname */
  netif->hostname = "LwIP-MATTER";
#endif /* LWIP_NETIF_HOSTNAME */

  /*
   * Initialize the snmp variables and counters inside the struct netif.
   * The last argument should be replaced with your link speed, in units
   * of bits per second.
   */
  MIB2_INIT_NETIF(netif, snmp_ifType_ethernet_csmacd, LINK_SPEED_OF_YOUR_NETIF_IN_BPS);

  netif->name[0] = IFNAME0;
  netif->name[1] = IFNAME1;
  /* We directly use etharp_output() here to save a function call.
   * You can instead declare your own function an call etharp_output()
   * from it if you have to do some checks before sending (e.g. if link
   * is available...) */

#if LWIP_IPV4
#if LWIP_ARP || LWIP_ETHERNET
  netif->output = etharp_output;
#endif /* LWIP_ARP || LWIP_ETHERNET */
#endif /* LWIP_IPV4 */

#if LWIP_IPV6
  netif->output_ip6 = ethip6_output;
#endif /* LWIP_IPV6 */

  netif->linkoutput = low_level_output;

  /* initialize the hardware */
  low_level_init(netif);

  return ERR_OK;
}

/**
  * @brief  Custom Rx pbuf free callback
  * @param  pbuf: pbuf to be freed
  * @retval None
  */
void pbuf_free_custom(struct pbuf *p)
{
  if (p != NULL)
  {
    struct pbuf_custom* custom_pbuf = (struct pbuf_custom*)p;
    LWIP_MEMPOOL_FREE(RX_POOL, custom_pbuf);

    /* If the Rx Buffer Pool was exhausted, signal the ethernetif_input task to
     * call HAL_ETH_GetRxDataBuffer to rebuild the Rx descriptors. */
    if (RxAllocStatus == RX_ALLOC_ERROR)
    {
      portBASE_TYPE taskWoken = pdFALSE;
      RxAllocStatus = RX_ALLOC_OK;

      if (xSemaphoreGiveFromISR(RxPktSemaphore, &taskWoken) == pdTRUE)
      {
        portEND_SWITCHING_ISR(taskWoken);
      }
    }
  }
}

/**
  * @brief  Initializes the ETH MSP.
  * @param  ethHandle: ETH handle
  * @retval None
  */
void HAL_ETH_MspInit(ETH_HandleTypeDef* ethHandle)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  if(ethHandle->Instance==ETH)
  {
    /* USER CODE BEGIN ETH_MspInit 0 */

    /* USER CODE END ETH_MspInit 0 */
    /* Enable Peripheral clock */
    __HAL_RCC_ETH1MAC_CLK_ENABLE();
    __HAL_RCC_ETH1TX_CLK_ENABLE();
    __HAL_RCC_ETH1RX_CLK_ENABLE();

    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOG_CLK_ENABLE();
    /**ETH GPIO Configuration
    PC1     ------> ETH_MDC
    PA1     ------> ETH_REF_CLK
    PA2     ------> ETH_MDIO
    PA7     ------> ETH_CRS_DV
    PC4     ------> ETH_RXD0
    PC5     ------> ETH_RXD1
    PB13     ------> ETH_TXD1
    PG11     ------> ETH_TX_EN
    PG13     ------> ETH_TXD0
    */
    GPIO_InitStruct.Pin = RMII_MDC_Pin|RMII_RXD0_Pin|RMII_RXD1_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF11_ETH;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = RMII_REF_CLK_Pin|RMII_MDIO_Pin|RMII_CRS_DV_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF11_ETH;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = RMII_TXD1_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF11_ETH;
    HAL_GPIO_Init(RMII_TXD1_GPIO_Port, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = RMII_TX_EN_Pin|RMII_TXD0_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF11_ETH;
    HAL_GPIO_Init(GPIOG, &GPIO_InitStruct);

    /* Peripheral interrupt init */
    HAL_NVIC_SetPriority(ETH_IRQn, 7, 0);
    HAL_NVIC_EnableIRQ(ETH_IRQn);
    /* USER CODE BEGIN ETH_MspInit 1 */

    /* USER CODE END ETH_MspInit 1 */
  }
}

/**
  * @brief ETH MSP De-Initialization
  * This function freeze the hardware resources used in this example
  * @param ethHandle: ETH handle pointer
  * @retval None
  */
void HAL_ETH_MspDeInit(ETH_HandleTypeDef* ethHandle)
{
  if(ethHandle->Instance==ETH)
  {
    /* USER CODE BEGIN ETH_MspDeInit 0 */

    /* USER CODE END ETH_MspDeInit 0 */
    /* Disable Peripheral clock */
    __HAL_RCC_ETH1MAC_CLK_DISABLE();
    __HAL_RCC_ETH1TX_CLK_DISABLE();
    __HAL_RCC_ETH1RX_CLK_DISABLE();

    /**ETH GPIO Configuration
    PC1     ------> ETH_MDC
    PA1     ------> ETH_REF_CLK
    PA2     ------> ETH_MDIO
    PA7     ------> ETH_CRS_DV
    PC4     ------> ETH_RXD0
    PC5     ------> ETH_RXD1
    PB13     ------> ETH_TXD1
    PG11     ------> ETH_TX_EN
    PG13     ------> ETH_TXD0
    */
    HAL_GPIO_DeInit(GPIOC, RMII_MDC_Pin|RMII_RXD0_Pin|RMII_RXD1_Pin);

    HAL_GPIO_DeInit(GPIOA, RMII_REF_CLK_Pin|RMII_MDIO_Pin|RMII_CRS_DV_Pin);

    HAL_GPIO_DeInit(RMII_TXD1_GPIO_Port, RMII_TXD1_Pin);

    HAL_GPIO_DeInit(GPIOG, RMII_TX_EN_Pin|RMII_TXD0_Pin);

    /* Peripheral interrupt Deinit*/
    HAL_NVIC_DisableIRQ(ETH_IRQn);

    /* USER CODE BEGIN ETH_MspDeInit 1 */

    /* USER CODE END ETH_MspDeInit 1 */
  }
}

/* USER CODE BEGIN 6 */

/* USER CODE END 6 */

/*******************************************************************************
                       PHI IO Functions
*******************************************************************************/
/**
  * @brief  Initializes the MDIO interface GPIO and clocks.
  * @param  None
  * @retval 0 if OK, -1 if ERROR
  */
int32_t ETH_PHY_IO_Init(void)
{
  /** We assume that MDIO GPIO configuration is already done
    * in the ETH_MspInit() else it should be done here
    */

  /* Configure the MDIO Clock */
  HAL_ETH_SetMDIOClockRange(&heth);

  return 0;
}

/**
  * @brief  De-Initializes the MDIO interface .
  * @param  None
  * @retval 0 if OK, -1 if ERROR
  */
int32_t ETH_PHY_IO_DeInit (void)
{
  return 0;
}

/**
  * @brief  Read a PHY register through the MDIO interface.
  * @param  DevAddr: PHY port address
  * @param  RegAddr: PHY register address
  * @param  pRegVal: pointer to hold the register value
  * @retval 0 if OK -1 if Error
  */
int32_t ETH_PHY_IO_ReadReg(uint32_t DevAddr, uint32_t RegAddr, uint32_t *pRegVal)
{
  if(HAL_ETH_ReadPHYRegister(&heth, DevAddr, RegAddr, pRegVal) != HAL_OK)
  {
    return -1;
  }

  return 0;
}

/**
  * @brief  Write a value to a PHY register through the MDIO interface.
  * @param  DevAddr: PHY port address
  * @param  RegAddr: PHY register address
  * @param  RegVal: Value to be written
  * @retval 0 if OK -1 if Error
  */
int32_t ETH_PHY_IO_WriteReg(uint32_t DevAddr, uint32_t RegAddr, uint32_t RegVal)
{
  if(HAL_ETH_WritePHYRegister(&heth, DevAddr, RegAddr, RegVal) != HAL_OK)
  {
    return -1;
  }

  return 0;
}

/**
  * @brief  Get the time in millisecons used for internal PHY driver process.
  * @retval Time value
  */
int32_t ETH_PHY_IO_GetTick(void)
{
  return HAL_GetTick();
}

/**
  * @brief  Check the ETH link state then update ETH driver and netif link accordingly.
  * @param  argument: the network interface
  * @retval None
  */
void EthernetLinkTask(void *p_argument)
{
  ETH_MACConfigTypeDef MACConf = {0};
  /* int32_t PHYLinkState = 0; */
  volatile int32_t PHYLinkState = 0;
  uint32_t linkchanged = 0U;
  uint32_t speed = 0U;
  uint32_t duplex = 0U;

  struct netif *netif = (struct netif *)p_argument;

  /* USER CODE BEGIN ETH link init */
  /* USER CODE END ETH link init */

  for(;;)
  {
    PHYLinkState = LAN8742_GetLinkState(&LAN8742);

    if(netif_is_link_up(netif) && (PHYLinkState <= LAN8742_STATUS_LINK_DOWN))
    {
      HAL_ETH_Stop_IT(&heth);
      netif_set_down(netif);
      netif_set_link_down(netif);
      /* USER CODE BEGIN */
      STM32_LOG("Ethernet link down");
      /* USER CODE END */
    }
    else if(!netif_is_link_up(netif) && (PHYLinkState > LAN8742_STATUS_LINK_DOWN))
    {
      switch (PHYLinkState)
      {
        case LAN8742_STATUS_100MBITS_FULLDUPLEX:
          duplex = ETH_FULLDUPLEX_MODE;
          speed = ETH_SPEED_100M;
          linkchanged = 1;
          break;
        case LAN8742_STATUS_100MBITS_HALFDUPLEX:
          duplex = ETH_HALFDUPLEX_MODE;
          speed = ETH_SPEED_100M;
          linkchanged = 1;
          break;
        case LAN8742_STATUS_10MBITS_FULLDUPLEX:
          duplex = ETH_FULLDUPLEX_MODE;
          speed = ETH_SPEED_10M;
          linkchanged = 1;
          break;
        case LAN8742_STATUS_10MBITS_HALFDUPLEX:
          duplex = ETH_HALFDUPLEX_MODE;
          speed = ETH_SPEED_10M;
          linkchanged = 1;
          break;
        default:
          break;
      }

      if(linkchanged)
      {
        /* Get MAC Config MAC */
        HAL_ETH_GetMACConfig(&heth, &MACConf);
        MACConf.DuplexMode = duplex;
        MACConf.Speed = speed;
        HAL_ETH_SetMACConfig(&heth, &MACConf);
        HAL_ETH_Start_IT(&heth);
        netif_set_up(netif);
        netif_set_link_up(netif);
        /* USER CODE BEGIN */
        STM32_LOG("Ethernet link up");
        /* USER CODE END */
      }
    }

    /* USER CODE BEGIN ETH link Thread core code for User BSP */

    /* USER CODE END ETH link Thread core code for User BSP */

    vTaskDelay(100);
  }
}

/* USER CODE BEGIN 8 */
void HAL_ETH_RxAllocateCallback(uint8_t **buff)
{
  /* USER CODE BEGIN HAL ETH RxAllocateCallback */
  struct pbuf_custom *p = LWIP_MEMPOOL_ALLOC(RX_POOL);
  if (p)
  {
    /* Get the buff from the struct pbuf address. */
    *buff = (uint8_t *)p + offsetof(RxBuff_t, buff);
    p->custom_free_function = pbuf_free_custom;
    /* Initialize the struct pbuf.
    * This must be performed whenever a buffer's allocated because it may be
    * changed by lwIP or the app, e.g., pbuf_free decrements ref. */
    pbuf_alloced_custom(PBUF_RAW, 0, PBUF_REF, p, *buff, ETH_RX_BUFFER_SIZE);
  }
  else
  {
    RxAllocStatus = RX_ALLOC_ERROR;
    *buff = NULL;
  }
  /* USER CODE END HAL ETH RxAllocateCallback */
}

void HAL_ETH_RxLinkCallback(void **pStart, void **pEnd, uint8_t *buff, uint16_t Length)
{
  /* USER CODE BEGIN HAL ETH RxLinkCallback */
  struct pbuf **ppStart = (struct pbuf **)pStart;
  struct pbuf **ppEnd = (struct pbuf **)pEnd;
  struct pbuf *p = NULL;

  /* Get the struct pbuf from the buff address. */
  p = (struct pbuf *)(buff - offsetof(RxBuff_t, buff));
  p->next = NULL;
  p->tot_len = 0;
  p->len = Length;

  /* Chain the buffer. */
  if (!*ppStart)
  {
    /* The first buffer of the packet. */
    *ppStart = p;
  }
  else
  {
    /* Chain the buffer to the end of the packet. */
    (*ppEnd)->next = p;
  }
  *ppEnd  = p;

  /* Update the total length of all the buffers of the chain. Each pbuf in the chain should have its tot_len
   * set to its own length, plus the length of all the following pbufs in the chain. */
  for (p = *ppStart; p != NULL; p = p->next)
  {
    p->tot_len += Length;
  }

  /* Invalidate data cache because Rx DMA's writing to physical memory makes it stale. */
  /* SCB_InvalidateDCache_by_Addr((uint32_t *)buff, Length); */

  /* USER CODE END HAL ETH RxLinkCallback */
}

void HAL_ETH_TxFreeCallback(uint32_t * buff)
{
  /* USER CODE BEGIN HAL ETH TxFreeCallback */
  pbuf_free((struct pbuf *)buff);
  /* USER CODE END HAL ETH TxFreeCallback */
}

/*
int8_t stm32_get_mac_addr(uint8_t * buf)
{
  (void)memcpy(buf, &(netif->hwaddr), ETHARP_HWADDR_LEN);
}
*/
/* USER CODE END 8 */
