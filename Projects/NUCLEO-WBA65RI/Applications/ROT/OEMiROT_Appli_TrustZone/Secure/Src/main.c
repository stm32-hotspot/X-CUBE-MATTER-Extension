/**
  ******************************************************************************
  * @file    main.c
  * @author  MCD Application Team
  * @brief   Main application file.
  *          This application demonstrates Secure Services
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2022 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */


/* Includes ------------------------------------------------------------------*/
#include "appli_region_defs.h"
#include "main.h"

/* Private includes ----------------------------------------------------------*/


/* Private typedef -----------------------------------------------------------*/


/* Private define ------------------------------------------------------------*/


/* Non-secure Vector table to jump to (internal Flash Bank2 here)             */
/* Caution: address must correspond to non-secure internal Flash where is     */
/*          mapped in the non-secure vector table
                             */
#define VTOR_TABLE_NS_START_ADDR  NS_CODE_START


/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/
static uint32_t SecureInitIODone = 0;

/* Private function prototypes -----------------------------------------------*/
static void NonSecure_Init(void);
static void MX_GPIO_Init(void);
static void MX_GTZC_Init(void);
static void  configure_sram1_sram2_sram6();

/* Private user code ---------------------------------------------------------*/


/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* SAU/IDAU, FPU and interrupts secure/non-secure allocation setup done */
  /* in SystemInit() based on partition_stm32xxx.h file's definitions. */


  /* Enable SecureFault handler (HardFault is default) */
  SCB->SHCSR |= SCB_SHCSR_SECUREFAULTENA_Msk;

  /* STM32xxx **SECURE** HAL library initialization:
       - Secure Systick timer is configured by default as source of time base,
         but user can eventually implement his proper time base source (a general
         purpose timer for example or other time source), keeping in mind that
         Time base duration should be kept 1ms since PPP_TIMEOUT_VALUEs are defined
         and handled in milliseconds basis.
       - Low Level Initialization
     */


  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* !!! To boot in a secure way, the RoT has configured and activated the Memory Protection Unit
     In order to keep a secure environment execution, you should reconfigure the
     MPU to make it compatible with your application
     In this example, MPU is disabled */
  HAL_MPU_Disable();

  /* GTZC initialisation */
  MX_GTZC_Init();

  /* Initialize all configured peripherals */
  MX_GPIO_Init();


  SecureInitIODone = 1;

  /* All IOs are by default allocated to secure */
  /* Release them all to non-secure */
#if defined(GPIOA)
  __HAL_RCC_GPIOA_CLK_ENABLE();
#endif /* GPIOA */
#if defined(GPIOB)
  __HAL_RCC_GPIOB_CLK_ENABLE();
#endif /* GPIOB */
#if defined(GPIOC)
  __HAL_RCC_GPIOC_CLK_ENABLE();
#endif /* GPIOC */
#if defined(GPIOD)
  __HAL_RCC_GPIOD_CLK_ENABLE();
#endif /* GPIOD */
#if defined(GPIOE)
  __HAL_RCC_GPIOE_CLK_ENABLE();
#endif /* GPIOE */
#if defined(GPIOG)
  __HAL_RCC_GPIOG_CLK_ENABLE();
#endif /* GPIOG */
#if defined(GPIOH)
  __HAL_RCC_GPIOH_CLK_ENABLE();
#endif /* GPIOH */

#if defined(GPIOA)
  HAL_GPIO_ConfigPinAttributes(GPIOA, GPIO_PIN_All, GPIO_PIN_NSEC);
#endif /* GPIOA */
#if defined(GPIOB)
  HAL_GPIO_ConfigPinAttributes(GPIOB, GPIO_PIN_All, GPIO_PIN_NSEC);
#endif /* GPIOB */
#if defined(GPIOC)
  HAL_GPIO_ConfigPinAttributes(GPIOC, GPIO_PIN_All, GPIO_PIN_NSEC);
#endif /* GPIOC */
#if defined(GPIOD)
  HAL_GPIO_ConfigPinAttributes(GPIOD, GPIO_PIN_All, GPIO_PIN_NSEC);
#endif /* GPIOD */
#if defined(GPIOE)
  HAL_GPIO_ConfigPinAttributes(GPIOE, GPIO_PIN_All, GPIO_PIN_NSEC);
#endif /* GPIOE */
#if defined(GPIOG)
  HAL_GPIO_ConfigPinAttributes(GPIOG, GPIO_PIN_All, GPIO_PIN_NSEC);
#endif /* GPIOG */
#if defined(GPIOH)
  HAL_GPIO_ConfigPinAttributes(GPIOH, GPIO_PIN_All, GPIO_PIN_NSEC);
#endif /* GPIOH */

  /* Leave the GPIO clocks enabled to let non-secure having I/Os control */

  /* Secure SysTick should rather be suspended before calling non-secure  */
  /* in order to avoid wake-up from sleep mode entered by non-secure      */
  /* The Secure SysTick shall be resumed on non-secure callable functions */
  /* For the purpose of this example, however the Secure SysTick is kept  */
  /* running to toggle the secure IO and the following is commented:      */
  /* HAL_SuspendTick(); */


  /*************** Setup and jump to non-secure *******************************/

  NonSecure_Init();

  /* Non-secure software does not return, this code is not executed */

  /* Infinite loop */

  while (1)
  {

  }
}

/**
  * @brief  Non-secure call function
  *         This function is responsible for Non-secure initialization and switch
  *         to non-secure state
  * @retval None
  */
static void NonSecure_Init(void)
{
  funcptr_NS NonSecure_ResetHandler;

  SCB_NS->VTOR = VTOR_TABLE_NS_START_ADDR;

  /* Set non-secure main stack (MSP_NS) */
  __TZ_set_MSP_NS((*(uint32_t *)VTOR_TABLE_NS_START_ADDR));

  /* Get non-secure reset handler */
  NonSecure_ResetHandler = (funcptr_NS)(*((uint32_t *)((VTOR_TABLE_NS_START_ADDR) + 4U)));

  /* Start non-secure state software application */
  NonSecure_ResetHandler();
}

/**
  * @brief  Configure of MPCBB of SRAM1, SRAM2 and SRAM6
  * @retval None
  */
static void  configure_sram1_sram2_sram6()
{
  uint32_t index;

  MPCBB_ConfigTypeDef MPCBB_Area_Desc = {0};
  MPCBB_Area_Desc.SecureRWIllegalMode = GTZC_MPCBB_SRWILADIS_ENABLE;
  MPCBB_Area_Desc.InvertSecureState = GTZC_MPCBB_INVSECSTATE_NOT_INVERTED;

  /* SRAM6 MPCBB configuration
   *   Security Config = Nonsecure (=0)
   *   Privileged config = Privileged and unprivileged (=0)
   */
  MPCBB_Area_Desc.AttributeConfig.MPCBB_SecConfig_array[0] =   0x00000000;
  MPCBB_Area_Desc.AttributeConfig.MPCBB_PrivConfig_array[0] =   0x00000000;
  MPCBB_Area_Desc.AttributeConfig.MPCBB_LockConfig_array[0] =   0x00000000;
  if (HAL_GTZC_MPCBB_ConfigMem(SRAM6_BASE, &MPCBB_Area_Desc) != HAL_OK)
  {
    Error_Handler();
  }  

  /* SRAM2 MPCBB configuration
   *   Security Config = Secure only (=1)
   *   Privileged config = Only Priviledged (=1)
   */
  for (index = 0; index <= 3; index++)
  {
    MPCBB_Area_Desc.AttributeConfig.MPCBB_SecConfig_array[index] = 0xFFFFFFFF;
    MPCBB_Area_Desc.AttributeConfig.MPCBB_PrivConfig_array[index] = 0xFFFFFFFF;
  }
  MPCBB_Area_Desc.AttributeConfig.MPCBB_LockConfig_array[0] =   0x00000000;
  if (HAL_GTZC_MPCBB_ConfigMem(SRAM2_BASE, &MPCBB_Area_Desc) != HAL_OK)
  {
    Error_Handler();
  }

  /* SRAM1 MPCBB configuration
   *   Security Config = Nonsecure (=0)
   *   Privileged config = Privileged and unprivileged (=0)
   */
  for (index = 0; index <= 27; index++)
  {
    MPCBB_Area_Desc.AttributeConfig.MPCBB_SecConfig_array[index] = 0x00000000;
    MPCBB_Area_Desc.AttributeConfig.MPCBB_PrivConfig_array[index] = 0x00000000;
  }
  MPCBB_Area_Desc.AttributeConfig.MPCBB_LockConfig_array[0] =   0x00000000;  
  if (HAL_GTZC_MPCBB_ConfigMem(SRAM1_BASE, &MPCBB_Area_Desc) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief GTZC Initialization Function
  * @param None
  * @retval None
  */
static void MX_GTZC_Init(void)
{

  if (HAL_GTZC_TZSC_ConfigPeriphAttributes(GTZC_PERIPH_ICACHE_REG,
                                           GTZC_TZSC_PERIPH_SEC | GTZC_TZSC_PERIPH_NPRIV) != HAL_OK)
  {
    Error_Handler();
  }

  /* GTZC TZSC config for RNG */
  if (HAL_GTZC_TZSC_ConfigPeriphAttributes(GTZC_PERIPH_RNG,
                                           GTZC_TZSC_PERIPH_NSEC | GTZC_TZSC_PERIPH_NPRIV) != HAL_OK)
  {
    Error_Handler();
  }

  /* GTZC TZSC config for PKA */
  if (HAL_GTZC_TZSC_ConfigPeriphAttributes(GTZC_PERIPH_PKA,
                                           GTZC_TZSC_PERIPH_NSEC | GTZC_TZSC_PERIPH_NPRIV) != HAL_OK)
  {
    Error_Handler();
  }  

  /* GTZC TZSC config for SPI3 (external flash) */
  if (HAL_GTZC_TZSC_ConfigPeriphAttributes(GTZC_PERIPH_SPI3,
                                           GTZC_TZSC_PERIPH_NSEC | GTZC_TZSC_PERIPH_NPRIV) != HAL_OK)
  {
    Error_Handler();
  }  

  /* SRAM1, SRAM2 and SRMA6 confiuration */
  configure_sram1_sram2_sram6();
}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{

}

/**
  * @brief  SYSTICK callback.
  * @retval None
  */
void HAL_SYSTICK_Callback(void)
{
}

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{

  /* Infinite loop */
  while (1)
  {

  }

}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
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
  {
  }

}
#endif /* USE_FULL_ASSERT */
