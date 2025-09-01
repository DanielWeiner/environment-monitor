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
#include "lwesp/lwesp_conn.h"
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
#define LOG_OUTPUT_DELAY 10

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
static char *response_str(lwespr_t res) {
	switch (res) {
		case lwespOK:
			return "OK";
		case lwespOKIGNOREMORE:
			return "OKIGNOREMORE";
		case lwespERR:
			return "ERR";
		case lwespERRPAR:
			return "ERRPAR";
		case lwespERRMEM:
			return "ERRMEM";
		case lwespTIMEOUT:
			return "TIMEOUT";
		case lwespCONT:
			return "CONT";
		case lwespCLOSED:
			return "CLOSED";
		case lwespINPROG:
			return "INPROG";
		case lwespERRNOIP:
			return "ERRNOIP";
		case lwespERRNOFREECONN:
			return "ERRNOFREECONN";
		case lwespERRCONNTIMEOUT:
			return "ERRCONNTIMEOUT";
		case lwespERRPASS:
			return "ERRPASS";
		case lwespERRNOAP:
			return "ERRNOAP";
		case lwespERRCONNFAIL:
			return "ERRCONNFAIL";
		case lwespERRWIFINOTCONNECTED:
			return "ERRWIFINOTCONNECTED";
		case lwespERRNODEVICE:
			return "ERRNODEVICE";
		case lwespERRBLOCKING:
			return "ERRBLOCKING";
		case lwespERRCMDNOTSUPPORTED:
			return "ERRCMDNOTSUPPORTED";
		default:
			return "UNKNOWN";
	}
}
typedef enum ConnectionStatus {
	CONNECTION_STATUS_UNINITIALIZED = 1,
	CONNECTION_STATUS_INITIALIZED,
	CONNECTION_STATUS_ERROR,
	CONNECTION_STATUS_SENDING,
	CONNECTION_STATUS_RECEIVING,
	CONNECTION_STATUS_CONNECTED,
	CONNECTION_STATUS_DISCONNECTED
} ConnectionStatus;

typedef struct Connection {
	ConnectionStatus status;
	lwesp_conn_p	 conn;
	lwesp_pbuf_p	 buf;
	TaskHandle_t	 task;
} Connection;

static lwespr_t lwesp_global_event_handler(struct lwesp_evt *evt) {
	LWESP_UNUSED(evt);
	return lwespOK;
}
static lwespr_t on_connection_event(struct lwesp_evt *evt) {
	Connection		*connection;
	ConnectionStatus status = 0;

	switch (evt->type) {
		case LWESP_EVT_CONN_ACTIVE:
			connection = (Connection *)lwesp_conn_get_arg(lwesp_evt_conn_active_get_conn(evt));
			status = CONNECTION_STATUS_CONNECTED;
			break;
		case LWESP_EVT_CONN_CLOSE:
			connection = (Connection *)lwesp_conn_get_arg(lwesp_evt_conn_close_get_conn(evt));
			if (lwesp_evt_conn_close_get_result(evt) != lwespOK) {
				status = CONNECTION_STATUS_ERROR;
				break;
			}
			status = CONNECTION_STATUS_DISCONNECTED;
			break;
		case LWESP_EVT_CONN_ERROR:
			connection = (Connection *)lwesp_evt_conn_error_get_arg(evt);
			status = CONNECTION_STATUS_ERROR;
			break;
		case LWESP_EVT_CONN_SEND:
			connection = (Connection *)lwesp_conn_get_arg(lwesp_evt_conn_send_get_conn(evt));
			status = CONNECTION_STATUS_CONNECTED;
			break;
		case LWESP_EVT_CONN_RECV:
			connection = (Connection *)lwesp_conn_get_arg(lwesp_evt_conn_recv_get_conn(evt));
			connection->buf = lwesp_evt_conn_recv_get_buff(evt);
			status = CONNECTION_STATUS_RECEIVING;
			break;
		default:
			return lwespOK;
	}

	connection->status = status;
	xTaskNotifyGive(connection->task);
	return lwespOK;
}

static const uint8_t req_data[] =
	""

	"GET /get HTTP/1.1" CRLF

	"Host: " CONN_HOST CRLF

	"Connection: close" DOUBLE_CRLF;

static void on_data_packet(lwesp_pbuf_p *buf, size_t len) {
	size_t pos = 0;
	void  *data;
	while ((data = lwesp_pbuf_get_linear_addr(*buf, pos, &len)) != NULL) {
		printf("%.*s", (int)len, (const char *)data);
		pos += len;
	}
}

static void start_connection(Connection *connection, lwesp_conn_type_t connType, const char *host, size_t port) {
	lwespr_t resp;
	if ((resp = lwesp_conn_start(&connection->conn, connType, host, port, connection, on_connection_event, 0)) !=
		lwespOK) {
		printf("Cannot start connection to %s:%d: %s" CRLF, host, port, response_str(resp));
		connection->status = CONNECTION_STATUS_UNINITIALIZED;
		return;
	}
	connection->status = CONNECTION_STATUS_INITIALIZED;
	ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
	return;
}

static void send_request(Connection *connection, const uint8_t *data, size_t len) {
	if (connection->status != CONNECTION_STATUS_CONNECTED) {
		printf("Not connected" CRLF);
		return;
	}

	lwespr_t resp;
	size_t	 bytesWritten = 0;
	// Send the data in chunks whose size is determined by the internal tx buffer
	while (len > 0 &&
		   (resp = lwesp_conn_send(connection->conn, (const void *)data, len, &bytesWritten, 0)) == lwespOK) {
		connection->status = CONNECTION_STATUS_SENDING;

		while (ulTaskNotifyTake(pdTRUE, portMAX_DELAY)) {
			if (connection->status == CONNECTION_STATUS_ERROR || connection->status == CONNECTION_STATUS_DISCONNECTED) {
				return;
			}

			if (connection->status == CONNECTION_STATUS_RECEIVING) {
				on_data_packet(&connection->buf, lwesp_pbuf_length(connection->buf, 1));
				connection->status = CONNECTION_STATUS_CONNECTED;
				continue;
			}

			if (connection->status == CONNECTION_STATUS_CONNECTED) {
				printf("%.*s", (int)bytesWritten, (const char *)data);
				len -= bytesWritten;
				data += bytesWritten;
				break;
			}
		}
	}

	if (resp != lwespOK) {
		printf("Error sending data: %s" CRLF, response_str(resp));
		connection->status = CONNECTION_STATUS_ERROR;
		return;
	}
}

static void wait_for_connection_closed(Connection *connection) {
	while (connection->status != CONNECTION_STATUS_DISCONNECTED) {
		ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
		if (connection->status == CONNECTION_STATUS_ERROR) {
			return;
		}

		if (connection->status == CONNECTION_STATUS_RECEIVING) {
			on_data_packet(&connection->buf, lwesp_pbuf_length(connection->buf, 1));
			connection->status = CONNECTION_STATUS_CONNECTED;
		}
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
		printf("Connection not closed properly" CRLF);
		return;
	}
	printf("Connection closed" CRLF);
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
	TickType_t now = xTaskGetTickCount();
	Connection connection = {.status = CONNECTION_STATUS_UNINITIALIZED, .task = xTaskGetCurrentTaskHandle()};

	for (;;) {
		make_request(&connection);
		vTaskDelayUntil(&now, REQUEST_FREQUENCY);
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
