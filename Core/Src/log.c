#include "log.h"

#include <stdio.h>
#include <string.h>

#include "cmsis_os.h"
#include "stm32l4xx_hal.h"

extern osMutexId_t		  logMutexHandle;
extern UART_HandleTypeDef huart2;
static Log_HandleTypeDef  logHandle = {0};

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
	if (logMutexHandle) osMutexRelease(logMutexHandle);
}

static void log_printchar(char c) {
	if (logMutexHandle) osMutexAcquire(logMutexHandle, osWaitForever);

	logHandle.buffer[logHandle.writeIndex++] = c;
	if (logHandle.writeIndex >= LOG_BUFFER_SIZE) {
		HAL_UART_Transmit(&huart2, (uint8_t *)logHandle.buffer, LOG_BUFFER_SIZE, HAL_MAX_DELAY);
		logHandle.writeIndex = 0;
	}

	if (logMutexHandle) osMutexRelease(logMutexHandle);
}

void output_log_buffer() {
	// get a snapshot of the current log buffer index
	if (logMutexHandle) osMutexAcquire(logMutexHandle, osWaitForever);
	uint16_t lastIndex = logHandle.writeIndex;
	if (logMutexHandle) osMutexRelease(logMutexHandle);

	// wait for 10 ms and see if the log buffer has changed
	osDelay(10);

	// if the write index is 0, that means the buffer is empty
	// if the last index is not the same as the current, the buffer may be busy
	// if the buffer is not busy and not empty, we can transmit the log
	if (logMutexHandle) osMutexAcquire(logMutexHandle, osWaitForever);
	if (logHandle.writeIndex > 0 && lastIndex == logHandle.writeIndex) {
		HAL_UART_Transmit(&huart2, (uint8_t *)logHandle.buffer, logHandle.writeIndex, HAL_MAX_DELAY);
		logHandle.writeIndex = 0;
	}
	if (logMutexHandle) osMutexRelease(logMutexHandle);
}

int __io_putchar(int ch) {
	log_printchar(ch);
	return ch;
}