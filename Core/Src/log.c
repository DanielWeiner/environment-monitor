#include "log.h"

#include <stdio.h>
#include <string.h>

#include "cmsis_os.h"
#include "stm32l4xx_hal.h"

extern osMutexId_t		  logMutexHandle;
extern UART_HandleTypeDef huart2;
static Log_HandleTypeDef  logHandle = {0};

void log_init(osThreadId_t taskHandle) {
	logHandle.logTask = taskHandle;
}

void log_printf(const char *fmt, ...) {
	va_list args;
	va_start(args, fmt);
	log_printf_va_list(fmt, args);
	va_end(args);
}

void log_printf_va_list(const char *fmt, va_list args) {
	if (logMutexHandle) osMutexAcquire(logMutexHandle, osWaitForever);

	static char intermediateBuf[LOG_BUFFER_SIZE] = {0};
	int			length = vsnprintf(intermediateBuf, sizeof(intermediateBuf), fmt, args);
	uint16_t	nextIndex = logHandle.writeIndex + length;
	if (length <= 0) return;

	if (length + logHandle.writeIndex >= LOG_BUFFER_SIZE) {
		HAL_UART_Transmit(&huart2, (uint8_t *)logHandle.buffer, logHandle.writeIndex, HAL_MAX_DELAY);
		logHandle.writeIndex = 0;
	}

	vsnprintf(logHandle.buffer + logHandle.writeIndex, sizeof(logHandle.buffer) - logHandle.writeIndex, fmt, args);
	logHandle.writeIndex = nextIndex;

	if (length >= LOG_BUFFER_SIZE) {
		HAL_UART_Transmit(&huart2, (uint8_t *)logHandle.buffer, logHandle.writeIndex, HAL_MAX_DELAY);
		logHandle.writeIndex = 0;
	}
	if (logHandle.logTask) xTaskNotifyGive(logHandle.logTask);
	if (logMutexHandle) osMutexRelease(logMutexHandle);
}

void output_log_buffer() {
	if (logMutexHandle) osMutexAcquire(logMutexHandle, osWaitForever);
	if (logHandle.writeIndex) {
		HAL_UART_Transmit(&huart2, (uint8_t *)logHandle.buffer, logHandle.writeIndex, HAL_MAX_DELAY);
		logHandle.writeIndex = 0;
	}
	if (logMutexHandle) osMutexRelease(logMutexHandle);
}

int __io_putchar(int ch) {
	log_printf("%c", ch);
	return ch;
}