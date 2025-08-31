#include "log.h"

#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "cmsis_os.h"
#include "stm32l4xx_hal.h"

extern osMutexId_t		  logMutexHandle;
extern osThreadId_t		  logTaskHandle;
extern UART_HandleTypeDef huart2;
static Log_HandleTypeDef  logHandle = {0};
extern int				  __io_putchar(int ch) __attribute__((weak));

static int log_printchar(int c) {
	if (!c) return 0;

	logHandle.buffer[logHandle.writeIndex++] = (char)c;
	if (logHandle.writeIndex == LOG_BUFFER_SIZE) {
		HAL_UART_Transmit(&huart2, (uint8_t *)logHandle.buffer, LOG_BUFFER_SIZE, HAL_MAX_DELAY);
		logHandle.writeIndex = 0;
	}
	return c;
}

void output_log_buffer(uint32_t timeout) {
	// wait for LOG_OUTPUT_DELAY ms after last log write
	uint32_t justModified = ulTaskNotifyTake(pdTRUE, timeout);
	// If we were notified of a new log, return early to allow more logging without interruption
	if (justModified > 0) return;

	if (logMutexHandle) osMutexAcquire(logMutexHandle, osWaitForever);
	if (logHandle.writeIndex > 0) {
		HAL_UART_Transmit(&huart2, (uint8_t *)logHandle.buffer, logHandle.writeIndex, HAL_MAX_DELAY);
		logHandle.writeIndex = 0;
	}
	if (logMutexHandle) osMutexRelease(logMutexHandle);
}

int _write(int file, char *ptr, int len) {
	int	 DataIdx;
	bool writeToLogBuffer = logMutexHandle && (file == STDOUT_FILENO || file == STDERR_FILENO);
	int (*fn)(int) = writeToLogBuffer ? log_printchar : __io_putchar;

	if (writeToLogBuffer) osMutexAcquire(logMutexHandle, osWaitForever);
	for (DataIdx = 0; DataIdx < len; DataIdx++) {
		fn((int)*ptr++);
	}
	if (logTaskHandle) xTaskNotifyGive(logTaskHandle);
	if (writeToLogBuffer) osMutexRelease(logMutexHandle);

	return len;
}
