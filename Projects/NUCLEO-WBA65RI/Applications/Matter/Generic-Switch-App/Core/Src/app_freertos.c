/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * File Name          : app_freertos.c
  * Description        : Code for freertos applications
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

/* Includes ------------------------------------------------------------------*/
#include "stm32_rtos.h"
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "cmsis_os2.h"
/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "ll_sys_if.h"
#include "timer_if.h"

/* USER CODE END Includes */
#include "app_ble.h"
/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN Variables */

/* USER CODE END Variables */
/* Definitions for advertisingTask */
osThreadId_t advertisingTaskHandle;
const osThreadAttr_t advertisingTask_attributes = {
  .name = "advertisingTask",
  .priority = CFG_TASK_PRIO_ADVERTISING,
  .stack_size = TASK_ADVERTISING_STACK_SIZE
};


/* Definitions for advLowPowerTimer */
osTimerId_t advLowPowerTimerHandle;
const osTimerAttr_t advLowPowerTimer_attributes = {
  .name = "advLowPowerTimer"
};






/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */

/* USER CODE END FunctionPrototypes */

void advertisingTask_Entry(void *argument);

void advLowPowerTimer_cb(void *argument);


void MX_FREERTOS_Init(void); /* (MISRA C 2004 rule 8.1) */

/* Hook prototypes */
void configureTimerForRunTimeStats(void);
unsigned long getRunTimeCounterValue(void);

/* USER CODE BEGIN 1 */
/* Functions needed when configGENERATE_RUN_TIME_STATS is on */
__weak void configureTimerForRunTimeStats(void)
{

}

__weak unsigned long getRunTimeCounterValue(void)
{
  return TIMER_IF_GetTimerValue();
}
/* USER CODE END 1 */

/* USER CODE BEGIN VPORT_SUPPORT_TICKS_AND_SLEEP */
__weak void vPortSuppressTicksAndSleep( TickType_t xExpectedIdleTime )
{
  // Generated when configUSE_TICKLESS_IDLE == 2.
  // Function called in tasks.c (in portTASK_FUNCTION).
  // TO BE COMPLETED or TO BE REPLACED by a user one, overriding that weak one.
}
/* USER CODE END VPORT_SUPPORT_TICKS_AND_SLEEP */

/**
  * @brief  FreeRTOS initialization
  * @param  None
  * @retval None
  */
void MX_FREERTOS_Init(void) {
  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */



  /* USER CODE BEGIN RTOS_SEMAPHORES */

  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */
  /* creation of advLowPowerTimer */
  //advLowPowerTimerHandle = osTimerNew(advLowPowerTimer_cb, osTimerOnce, NULL, &advLowPowerTimer_attributes);



  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */
  /* creation of advertisingCmdQueue */
  advertisingCmdQueueHandle = osMessageQueueNew (16, sizeof(uint16_t), &advertisingCmdQueue_attributes);

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */
  /* creation of advertisingTask */
  advertisingTaskHandle = osThreadNew(advertisingTask_Entry, NULL, &advertisingTask_attributes);


  /* USER CODE BEGIN RTOS_THREADS */

  /* USER CODE END RTOS_THREADS */

  /* USER CODE BEGIN RTOS_EVENTS */
  /* add events, ... */
  /* USER CODE END RTOS_EVENTS */

}



/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

/* USER CODE END Application */

