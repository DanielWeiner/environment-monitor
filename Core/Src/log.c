#include "log.h"
#include "cmsis_os.h"
#include "stm32l4xx_hal.h"
#include <stdio.h>

static osMutexId_t logMutex;
extern UART_HandleTypeDef huart2;

void log_init(void) {
  logMutex = osMutexNew(
      &(osMutexAttr_t){.attr_bits = osMutexPrioInherit | osMutexRecursive});
}

void log_printf(const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);
  _log_printf(fmt, args);
  va_end(args);
}

void _log_printf(const char *fmt, va_list args) {
  char buf[LOG_MAX_SIZE];
  int length = vsnprintf(buf, sizeof(buf), fmt, args);
  if (length < 0)
    return;

  if (logMutex)
    osMutexAcquire(logMutex, osWaitForever);
  HAL_UART_Transmit(&huart2, (uint8_t *)buf,
                    (length < LOG_MAX_SIZE) ? length : LOG_MAX_SIZE - 1,
                    HAL_MAX_DELAY);
  if (logMutex)
    osMutexRelease(logMutex);
}