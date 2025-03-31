/**
  ******************************************************************************
  * @file    MbedTLS_HW_ALT/Encrypted_ITS_KeyImport/Src/main.c
  * @author  MCD Application Team
  * @brief   This example provides a short description of how to use
  *          PSA Encryptes ITS alternative implementation to store AES-CBC key
  *          in encrypted storage and use the encrypted key to perform AES-CBC
  *          Encryption and dycreption operation.
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
#include <string.h>
#include "main.h"
#include "crypto.h"
#include "psa_its_alt.h"
#include "storage_interface.h"

/* Global variables ----------------------------------------------------------*/
/* Private typedef --------------------------------s---------------------------*/
/* Private defines -----------------------------------------------------------*/
/* Private macros ------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* kDevelopmentDAC_PrivateKey_FFF1_8004 as exemple*/

const uint8_t DA_Private_Key[] =
{
 0x82, 0x0a, 0x24, 0x2a,
 0x03, 0x0e, 0xbc, 0xe1,
 0x1f, 0x38, 0x73, 0x5a,
 0xcf, 0x1a, 0x6f, 0x37,
 0xc3, 0xad, 0xa6, 0xe4,
 0x32, 0xd2, 0x47, 0x0a,
 0x8a, 0x41, 0x37, 0x43,
 0xf8, 0x95, 0x63, 0xf3
};




/* Private function prototypes -----------------------------------------------*/
static void SystemClock_Config(void);
void Error_Handler(void);
/* Functions Definition ------------------------------------------------------*/

/**
  * @brief  Main program
  * @param  None
  * @retval None
  */
int main(void)
{
  psa_status_t retval = PSA_SUCCESS;

  /* HAL Init */
  HAL_Init();

  /* Configure the System clock */
  SystemClock_Config();

  /* Configure LEDs */
  BSP_LED_Init(LED_GREEN);
  BSP_LED_Init(LED_RED);

  /*  PSA Crypto library Initialization */
    retval = psa_crypto_init();
  if (retval != PSA_SUCCESS)
  {
    Error_Handler();
  }

  /* Encrypt and store in NVM with ITS the  DA privateKey */
  retval = storage_DA_private_Key(DA_Private_Key,sizeof(DA_Private_Key));
  if(retval != PSA_SUCCESS)
  {
	  Error_Handler();
  }

  /* Clear all data associated with the PSA layer */
  mbedtls_psa_crypto_free();

  /* Turn on LD2 in an infinite loop in case of Key Wrap operations are successful */
  BSP_LED_On(LED_GREEN);

  while (1)
  {}
}


/**
  * @brief  System Clock Configuration
  *         The system Clock is configured as follows :
  *            System Clock source            = PLL (HSE)
  *            SYSCLK(Hz)                     = 100000000
  *            HCLK(Hz)                       = 100000000
  *            AHB Prescaler                  = 1
  *            AHB5 Prescaler                 = 4
  *            APB1 Prescaler                 = 1
  *            APB2 Prescaler                 = 1
  *            APB7 Prescaler                 = 1
  *            HSE Frequency(Hz)              = 32000000
  *            PLL_M                          = 2
  *            PLL_N                          = 25
  *            PLL_P                          = 4
  *            PLL_Q                          = 4
  *            PLL_R                          = 4
  *            Flash Latency(WS)              = 3
  *            Voltage range                  = 1
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  __HAL_RCC_PWR_CLK_ENABLE();
  /** At reset, the regulator is the LDO, in range 2
     Need to move to range 1 to reach 100 MHz
  */
  if (HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1) != HAL_OK)
  {
    /* Initialization error */
    Error_Handler();
  }

  /** Activate PLL with HSE as source
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEDiv = RCC_HSE_DIV1;
  RCC_OscInitStruct.PLL1.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL1.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL1.PLLFractional = 0U;
  RCC_OscInitStruct.PLL1.PLLM = 2U;
  RCC_OscInitStruct.PLL1.PLLN = 25U;   /* VCO = HSE/M * N = 32 / 2 * 25 = 400 MHz */
  RCC_OscInitStruct.PLL1.PLLR = 4U;    /* PLLSYS = 400 MHz / 4 = 100 MHz */
  RCC_OscInitStruct.PLL1.PLLP = 4U;
  RCC_OscInitStruct.PLL1.PLLQ = 4U;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    /* Initialization error */
    Error_Handler();
  }

  /** Select PLL as system clock source and configure the HCLK, PCLK1, PCLK2 and PCLK7
     clocks dividers */
  RCC_ClkInitStruct.ClockType = (RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_PCLK1 \
                               | RCC_CLOCKTYPE_PCLK2 | RCC_CLOCKTYPE_PCLK7 | RCC_CLOCKTYPE_HCLK5);
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB7CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.AHB5_PLL1_CLKDivider = RCC_SYSCLK_PLL1_DIV4;
  RCC_ClkInitStruct.AHB5_HSEHSI_CLKDivider = RCC_SYSCLK_HSEHSI_DIV1;
  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_3) != HAL_OK)
  {
    /* Initialization error */
    Error_Handler();
  }

  /** Deactivate HSI
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_OFF;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    /* Initialization error */
    Error_Handler();
  }
}


/**
  * @brief  This function is executed in case of error occurrence
  * @param  None
  * @retval None
  */
void Error_Handler(void)
{
  /* User may add here some code to deal with this error */
  /* Toggle LD3 @2Hz to notify error condition */
  while (1)
  {
    BSP_LED_Toggle(LED_RED);
    HAL_Delay(250);
  }
}

#ifdef USE_FULL_ASSERT

/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */

  /* Infinite loop */
  while (1)
  {}
}
#endif /* USE_FULL_ASSERT */
