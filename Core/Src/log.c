#include "log.h"

#include <stdio.h>
#include <string.h>

#include "cmsis_os.h"
#include "stm32l4xx_hal.h"

extern osMutexId_t		  logMutexHandle;
extern UART_HandleTypeDef huart2;
static Log_HandleTypeDef  logHandle = {
	 .writeIndex = 0,
	 .readIndex = 0,
};

void log_printf(const char *fmt, ...) {
	va_list args;
	va_start(args, fmt);
	log_printf_va_list(fmt, args);
	va_end(args);
}

void log_printf_va_list(const char *fmt, va_list args) {
	if (logMutexHandle) osMutexAcquire(logMutexHandle, osWaitForever);
	static char formattedString[LOG_MAX_SIZE];
	int			length = vsnprintf(formattedString, sizeof(formattedString), fmt, args);
	if (length < 0 || length > LOG_MAX_SIZE) return;

	uint16_t nextIndex = (logHandle.writeIndex + length) % LOG_BUFFER_SIZE;
	if (nextIndex < logHandle.writeIndex) {
		// wrap around
		uint16_t firstPartLen = LOG_BUFFER_SIZE - logHandle.writeIndex;
		memcpy(&logHandle.buffer[logHandle.writeIndex], formattedString, firstPartLen);
		memcpy(&logHandle.buffer[0], &formattedString[firstPartLen], length - firstPartLen);
	} else {
		memcpy(&logHandle.buffer[logHandle.writeIndex], formattedString, length);
	}
	logHandle.writeIndex = nextIndex;
	if (logMutexHandle) osMutexRelease(logMutexHandle);
}

void output_log_buffer() {
	if (logMutexHandle) osMutexAcquire(logMutexHandle, osWaitForever);
	if (logHandle.readIndex == logHandle.writeIndex) {
		if (logMutexHandle) osMutexRelease(logMutexHandle);
		return;
	}
	if (logHandle.readIndex < logHandle.writeIndex) {
		// contiguous
		HAL_UART_Transmit(&huart2, (uint8_t *)&logHandle.buffer[logHandle.readIndex],
						  logHandle.writeIndex - logHandle.readIndex, HAL_MAX_DELAY);
	} else {
		// wrap around
		HAL_UART_Transmit(&huart2, (uint8_t *)&logHandle.buffer[logHandle.readIndex],
						  LOG_BUFFER_SIZE - logHandle.readIndex, HAL_MAX_DELAY);
		HAL_UART_Transmit(&huart2, (uint8_t *)&logHandle.buffer[0], logHandle.writeIndex, HAL_MAX_DELAY);
	}
	logHandle.readIndex = logHandle.writeIndex;
	if (logMutexHandle) osMutexRelease(logMutexHandle);
}

int __io_putchar(int ch) {
	log_printf("%c", ch);
	return ch;
}