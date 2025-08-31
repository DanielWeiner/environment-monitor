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

#include "constants.h"
#include "lcd.h"
#include "log.h"
#include "lwesp/lwesp.h"
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
#define READY_TOKEN (CRLF "ready" CRLF)
#define CWLAP_PREFIX_TOKEN (CRLF "+CWLAP:(")
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN Variables */
// static bool				  esp8266Ready = false;
extern UART_HandleTypeDef huart1;

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
/* Definitions for logMutex */
osMutexId_t			logMutexHandle;
osStaticMutexDef_t	logMutexControlBlock;
const osMutexAttr_t logMutex_attributes = {
	.name = "logMutex",
	.attr_bits = osMutexRecursive,
	.cb_mem = &logMutexControlBlock,
	.cb_size = sizeof(logMutexControlBlock),
};

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */
/* USER CODE END FunctionPrototypes */

void StartDefaultTask(void *argument);
void StartESP8266ATTask(void *argument);
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

	/* Create the recursive mutex(es) */
	/* creation of logMutex */
	logMutexHandle = osMutexNew(&logMutex_attributes);

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
lwespr_t lwesp_global_event_handler(struct lwesp_evt *evt) {
	LWESP_UNUSED(evt);
	return lwespOK;
}
/**
 * @brief  Function implementing the defaultTask thread.
 * @param  argument: Not used
 * @retval None
 */
/* USER CODE END Header_StartDefaultTask */
void StartDefaultTask(void *argument) {
	/* USER CODE BEGIN StartDefaultTask */
	static char tickStr[10] = {0};

	lcd_clear();
	lcd_string(80, 160, "0", 1, 0xFFFF, ST7789_FONT_24);
	uint32_t now = osKernelGetTickCount();
	for (;;) {
		lcd_fill_rect(80, 160, 220, 184, 0);
		int len = snprintf(tickStr, sizeof(tickStr), "%lu", now / 1000);
		lcd_string(80, 160, tickStr, len, 0xFFFF, ST7789_FONT_24);
		vTaskDelayUntil(&now, portTICK_PERIOD_MS * 1000);
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
	printf("initializing ESP8266" CRLF);

	lwespr_t resp;
	while (resp = lwesp_init(lwesp_global_event_handler, 1), resp != lwespOK) {
		printf("initialization failed: ");
		switch (resp) {
			case lwespERRMEM:
				printf("out of memory" CRLF);
				break;
			case lwespTIMEOUT:
				printf("timeout" CRLF);
				break;
			case lwespERRPAR:
				printf("wrong parameters" CRLF);
				break;
			case lwespERR:
			default:
				printf("unknown error" CRLF);
				break;
		}
	}
	printf("initialized ESP8266" CRLF);
	static lwesp_ap_t  accessPoints[40] = {0};
	static lwesp_mac_t myMacAddress = {0};
	bool			   foundMySsid = false;

	do {
		memset(accessPoints, 0, sizeof(accessPoints));
		size_t found = 0;
		printf("Scanning for networks..." CRLF);
		lwesp_sta_list_ap(NULL, accessPoints, sizeof(accessPoints) / sizeof(lwesp_ap_t), &found, NULL, NULL, 1);
		for (int i = 0; i < found; i++) {
			if (accessPoints[i].ssid[0] == 0) {
				continue;
			}
			if (strlen(accessPoints[i].ssid) == strlen(MY_SSID) && strcmp(accessPoints[i].ssid, MY_SSID) == 0) {
				memcpy(myMacAddress.mac, accessPoints[i].mac.mac, sizeof(myMacAddress.mac));
				foundMySsid = true;
				break;
			}
		}
	} while (!foundMySsid);

	printf("Found network: %s (%02x:%02x:%02x:%02x:%02x:%02x)" CRLF, MY_SSID, myMacAddress.mac[0], myMacAddress.mac[1],
		   myMacAddress.mac[2], myMacAddress.mac[3], myMacAddress.mac[4], myMacAddress.mac[5]);
	resp = lwesp_sta_join(MY_SSID, MY_PASSWORD, &myMacAddress, NULL, NULL, 1);
	if (resp != lwespOK) {
		printf("Failed to connect to network: %d" CRLF, resp);
	} else {
		printf("Connected to %s" CRLF, MY_SSID);
	}

	/* Infinite loop */
	for (;;) {
		osDelay(1);
	}
	/* USER CODE END StartESP8266ATTask */
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
	log_init(logTaskHandle);
	for (;;) {
		ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(10));
		output_log_buffer();
	}
	/* USER CODE END StartLogTask */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

/* USER CODE END Application */
