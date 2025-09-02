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

#include "connection.h"
#include "constants.h"
#include "lcd.h"
#include "log.h"
#include "lwesp/lwesp.h"
#include "lwesp/lwesp_conn.h"
#include "lwesp_util.h"
#include "usart.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
typedef StaticTask_t	  osStaticThreadDef_t;
typedef StaticSemaphore_t osStaticMutexDef_t;
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define CONN_HOST "httpbin.org"
#define CONN_PORT 443
#define REQUEST_FREQUENCY 5000
#define LOG_OUTPUT_DELAY 1
#define REQUEST_TIMEOUT pdMS_TO_TICKS(10000)
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
uint32_t			 defaultTaskBuffer[128];
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
	.priority = (osPriority_t)osPriorityBelowNormal,
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
	TickType_t		 now = xTaskGetTickCount();
	const TickType_t delay = 1000;
	for (;;) {
		lcd_fill_rect(80, 160, 220, 184, 0);
		int len = snprintf(tickStr, sizeof(tickStr), "%lu", now / 1000);
		lcd_string(80, 160, tickStr, len, 0xFFFF, ST7789_FONT_24);
		vTaskDelayUntil(&now, delay);
	}
	/* USER CODE END StartDefaultTask */
}

/* USER CODE BEGIN Header_StartESP8266ATTask */
static lwespr_t lwesp_global_event_handler(struct lwesp_evt *evt) {
	LWESP_UNUSED(evt);
	return lwespOK;
}

static const uint8_t req_data[] =
	""

	"GET /get HTTP/1.1" CRLF

	"Host: " CONN_HOST CRLF
	"X-Long-Header: Lorem ipsum dolor sit amet, consectetur adipiscing elit. Sed euismod, urna eu tincidunt "
	"consectetur, nisi nisl aliquam enim, nec dictum nisi urna at velit. Pellentesque habitant morbi tristique "
	"senectus et netus et malesuada fames ac turpis egestas. Suspendisse potenti. Mauris non tempor quam, et lacinia "
	"sapien. Mauris accumsan eros eget libero posuere vulputate. Etiam elit elit, elementum sed varius at, adipiscing "
	"vitae est. Sed nec felis pellentesque, lacinia dui sed, ultricies sapien." CRLF

	"Connection: close" DOUBLE_CRLF;

static void on_close_connection(Connection *connection) {
	size_t pos = 0;
	void  *data;
	if (connection->buf == NULL) return;

	size_t len = lwesp_pbuf_length(connection->buf, 1);
	while ((data = lwesp_pbuf_get_linear_addr(connection->buf, pos, &len)) != NULL) {
		printf("%.*s", (int)len, (const char *)data);
		pos += len;
	}
}

static void make_request(Connection *connection) {
	start_connection(connection, LWESP_CONN_TYPE_SSL, CONN_HOST, CONN_PORT);
	if (connection->status != CONNECTION_STATUS_CONNECTED) {
		printf("Failed to start connection" CRLF);
		return;
	}
	send_request(connection, req_data, sizeof(req_data) - 1);
	wait_for_connection_closed(connection);
	if (connection->status != CONNECTION_STATUS_DISCONNECTED) {
		if (connection->timeout) {
			printf("Connection timed out" CRLF);
		} else {
			printf("Connection not closed properly" CRLF);
		}
		return;
	}
	printf("Connection closed" DOUBLE_CRLF);
}
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
		printf("initialization failed: %s" CRLF, response_str(resp));
		osDelay(500);
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
	do {
		printf("Connecting to %s..." CRLF, MY_SSID);

		resp = lwesp_sta_join(MY_SSID, MY_PASSWORD, &myMacAddress, NULL, NULL, 1);

		if (resp != lwespOK) {
			printf("Failed to connect to network: %s" CRLF, response_str(resp));
			continue;
		}
		printf("Connected to %s" CRLF, MY_SSID);
	} while (resp != lwespOK);

	/* Infinite loop */
	Connection connection = {.onClose = on_close_connection, .task = xTaskGetCurrentTaskHandle()};

	for (;;) {
		make_request(&connection);
		osDelay(REQUEST_FREQUENCY);
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
	for (;;) {
		output_log_buffer(LOG_OUTPUT_DELAY);
	}
	/* USER CODE END StartLogTask */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

/* USER CODE END Application */
