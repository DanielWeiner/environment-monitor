/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * File Name          : freertos.c
 * Description        : Code for freertos applications
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2025 STMicroelectronics.
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
#include "FreeRTOS.h"

#include "cmsis_os.h"
#include "main.h"
#include "task.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdbool.h>

#include "at.h"
#include "constants.h"
#include "lcd.h"
#include "log.h"
#include "usart.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
typedef StaticTask_t	  osStaticThreadDef_t;
typedef StaticSemaphore_t osStaticMutexDef_t;
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define OK_TOKEN (CRLF "OK" CRLF)
#define ERROR_TOKEN (CRLF "ERROR" CRLF)
#define AT_GMR_TOKEN ("AT+GMR" CRLF)
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN Variables */
static volatile bool esp8266Ready = false;
extern AT_Handle	 esp8266ATHandle;
/* USER CODE END Variables */
/* Definitions for defaultTask */
osThreadId_t		 defaultTaskHandle;
uint32_t			 defaultTaskBuffer[256];
osStaticThreadDef_t	 defaultTaskControlBlock;
const osThreadAttr_t defaultTask_attributes = {
	.name = "defaultTask",
	.cb_mem = &defaultTaskControlBlock,
	.cb_size = sizeof(defaultTaskControlBlock),
	.stack_mem = &defaultTaskBuffer[0],
	.stack_size = sizeof(defaultTaskBuffer),
	.priority = (osPriority_t)osPriorityNormal,
};
/* Definitions for esp8266ATTask */
osThreadId_t		 esp8266ATTaskHandle;
uint32_t			 esp8266ATTaskBuffer[256];
osStaticThreadDef_t	 esp8266ATTaskControlBlock;
const osThreadAttr_t esp8266ATTask_attributes = {
	.name = "esp8266ATTask",
	.cb_mem = &esp8266ATTaskControlBlock,
	.cb_size = sizeof(esp8266ATTaskControlBlock),
	.stack_mem = &esp8266ATTaskBuffer[0],
	.stack_size = sizeof(esp8266ATTaskBuffer),
	.priority = (osPriority_t)osPriorityLow,
};
/* Definitions for esp8266Dispatch */
osThreadId_t		 esp8266DispatchHandle;
uint32_t			 esp8266DispatchBuffer[256];
osStaticThreadDef_t	 esp8266DispatchControlBlock;
const osThreadAttr_t esp8266Dispatch_attributes = {
	.name = "esp8266Dispatch",
	.cb_mem = &esp8266DispatchControlBlock,
	.cb_size = sizeof(esp8266DispatchControlBlock),
	.stack_mem = &esp8266DispatchBuffer[0],
	.stack_size = sizeof(esp8266DispatchBuffer),
	.priority = (osPriority_t)osPriorityNormal,
};
/* Definitions for logTask */
osThreadId_t		 logTaskHandle;
uint32_t			 logTaskBuffer[128];
osStaticThreadDef_t	 logTaskControlBlock;
const osThreadAttr_t logTask_attributes = {
	.name = "logTask",
	.cb_mem = &logTaskControlBlock,
	.cb_size = sizeof(logTaskControlBlock),
	.stack_mem = &logTaskBuffer[0],
	.stack_size = sizeof(logTaskBuffer),
	.priority = (osPriority_t)osPriorityLow,
};
/* Definitions for esp8266TxChunkMutex */
osMutexId_t			esp8266TxChunkMutexHandle;
osStaticMutexDef_t	esp8266TxChunkMutexControlBlock;
const osMutexAttr_t esp8266TxChunkMutex_attributes = {
	.name = "esp8266TxChunkMutex",
	.cb_mem = &esp8266TxChunkMutexControlBlock,
	.cb_size = sizeof(esp8266TxChunkMutexControlBlock),
};
/* Definitions for logMutex */
osMutexId_t			logMutexHandle;
osStaticMutexDef_t	logMutexControlBlock;
const osMutexAttr_t logMutex_attributes = {
	.name = "logMutex",
	.attr_bits = osMutexRecursive,
	.cb_mem = &logMutexControlBlock,
	.cb_size = sizeof(logMutexControlBlock),
};
/* Definitions for esp8266ATMutex */
osMutexId_t			esp8266ATMutexHandle;
osStaticMutexDef_t	esp8266ATMutexControlBlock;
const osMutexAttr_t esp8266ATMutex_attributes = {
	.name = "esp8266ATMutex",
	.attr_bits = osMutexRecursive,
	.cb_mem = &esp8266ATMutexControlBlock,
	.cb_size = sizeof(esp8266ATMutexControlBlock),
};

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */

/* USER CODE END FunctionPrototypes */

void StartDefaultTask(void *argument);
void StartESP8266ATTask(void *argument);
void StartESP8266Dispatch(void *argument);
void StartLogTask(void *argument);

void MX_FREERTOS_Init(void); /* (MISRA C 2004 rule 8.1) */

/**
 * @brief  FreeRTOS initialization
 * @param  None
 * @retval None
 */
void MX_FREERTOS_Init(void) {
	/* USER CODE BEGIN Init */
	lcd_init();
	/* USER CODE END Init */
	/* Create the mutex(es) */
	/* creation of esp8266TxChunkMutex */
	esp8266TxChunkMutexHandle = osMutexNew(&esp8266TxChunkMutex_attributes);

	/* Create the recursive mutex(es) */
	/* creation of logMutex */
	logMutexHandle = osMutexNew(&logMutex_attributes);

	/* creation of esp8266ATMutex */
	esp8266ATMutexHandle = osMutexNew(&esp8266ATMutex_attributes);

	/* USER CODE BEGIN RTOS_MUTEX */
	/* add mutexes, ... */
	/* USER CODE END RTOS_MUTEX */

	/* USER CODE BEGIN RTOS_SEMAPHORES */
	/* add semaphores, ... */
	/* USER CODE END RTOS_SEMAPHORES */

	/* USER CODE BEGIN RTOS_TIMERS */
	/* start timers, add new ones, ... */
	/* USER CODE END RTOS_TIMERS */

	/* USER CODE BEGIN RTOS_QUEUES */
	/* add queues, ... */
	/* USER CODE END RTOS_QUEUES */

	/* Create the thread(s) */
	/* creation of defaultTask */
	defaultTaskHandle = osThreadNew(StartDefaultTask, NULL, &defaultTask_attributes);

	/* creation of esp8266ATTask */
	esp8266ATTaskHandle = osThreadNew(StartESP8266ATTask, NULL, &esp8266ATTask_attributes);

	/* creation of esp8266Dispatch */
	esp8266DispatchHandle = osThreadNew(StartESP8266Dispatch, NULL, &esp8266Dispatch_attributes);

	/* creation of logTask */
	logTaskHandle = osThreadNew(StartLogTask, NULL, &logTask_attributes);

	/* USER CODE BEGIN RTOS_THREADS */
	/* add threads, ... */
	/* USER CODE END RTOS_THREADS */

	/* USER CODE BEGIN RTOS_EVENTS */
	/* add events, ... */
	/* USER CODE END RTOS_EVENTS */
}

/* USER CODE BEGIN Header_StartDefaultTask */

/**
 * @brief  Function implementing the defaultTask thread.
 * @param  argument: Not used
 * @retval None
 */
/* USER CODE END Header_StartDefaultTask */
void StartDefaultTask(void *argument) {
	/* USER CODE BEGIN StartDefaultTask */
	static char		tickStr[10] = {0};
	static uint32_t lastSecond = 0;
	lcd_clear();
	lcd_string(80, 160, "0", 1, 0xFFFF, ST7789_FONT_24);
	for (;;) {
		uint32_t now = osKernelGetTickCount() / 1000;
		if (now - lastSecond >= 1) {
			lcd_fill_rect(80, 160, 220, 184, 0);
			int len = snprintf(tickStr, sizeof(tickStr), "%lu", now);
			lcd_string(80, 160, tickStr, len, 0xFFFF, ST7789_FONT_24);
		}
		lastSecond = now;
		osDelay(1);
	}
	/* USER CODE END StartDefaultTask */
}

/* USER CODE BEGIN Header_StartESP8266ATTask */
/**
 * @brief Function implementing the esp8266ATTask thread.
 * @param argument: Not used
 * @retval None
 */
/* USER CODE END Header_StartESP8266ATTask */
void StartESP8266ATTask(void *argument) {
	/* USER CODE BEGIN StartESP8266ATTask */

	// trigger a reset on the ESP8266
	HAL_GPIO_WritePin(ESP_RST_GPIO_Port, ESP_RST_Pin, GPIO_PIN_SET);

	/* Infinite loop */
	for (;;) {
		at_consume_rx(&esp8266ATHandle);
		vTaskDelay(0);
	}
	/* USER CODE END StartESP8266ATTask */
}

/* USER CODE BEGIN Header_StartESP8266Dispatch */
static bool gmrDone = false;
static bool gmrError = false;
static bool outputGmr = false;

static void on_gmr_char(char ch) {
	if (outputGmr) {
		printf("%c", ch);
	}
}

static void on_gmr_token(void *) {
	outputGmr = true;
}

static void on_gmr_output_done(void *) {
	outputGmr = false;
}

static void on_gmr_ok(void *) {
	gmrDone = true;
}

static void on_gmr_error(void *) {
	gmrError = true;
	gmrDone = true;
}

static TokenMatcher okTokenMatcher = TOKEN_MATCHER(OK_TOKEN);
static TokenMatcher errorTokenMatcher = TOKEN_MATCHER(ERROR_TOKEN);
static TokenMatcher gmrTokenMatcher = TOKEN_MATCHER(AT_GMR_TOKEN);
static TokenMatcher doublCrlfTokenMatcher = TOKEN_MATCHER(DOUBLE_CRLF);

static const AT_TokenHandler gmrHandlers[] = {{&okTokenMatcher, on_gmr_ok},
											  {&errorTokenMatcher, on_gmr_error},
											  {&gmrTokenMatcher, on_gmr_token},
											  {&doublCrlfTokenMatcher, on_gmr_output_done}};
/**
 * @brief Function implementing the esp8266Dispatch thread.
 * @param argument: Not used
 * @retval None
 */
/* USER CODE END Header_StartESP8266Dispatch */
void StartESP8266Dispatch(void *argument) {
	/* USER CODE BEGIN StartESP8266Dispatch */
	/* Infinite loop */

	while (!esp8266ATHandle.ready) {
		osDelay(1);
	}

	at_on_rx_byte(&esp8266ATHandle, on_gmr_char);
	at_set_tokens(&esp8266ATHandle, gmrHandlers);

	osDelay(1);
	at_send(&esp8266ATHandle, "AT+GMR\r\n", 8);
	while (!gmrDone) {
		osDelay(1);
	}

	for (;;) {
		osDelay(1);
	}
	/* USER CODE END StartESP8266Dispatch */
}

/* USER CODE BEGIN Header_StartLogTask */
/**
 * @brief Function implementing the logTask thread.
 * @param argument: Not used
 * @retval None
 */
/* USER CODE END Header_StartLogTask */
void StartLogTask(void *argument) {
	/* USER CODE BEGIN StartLogTask */
	/* Infinite loop */
	for (;;) {
		output_log_buffer();
		vTaskDelay(0);
	}
	/* USER CODE END StartLogTask */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */
/* USER CODE END Application */
