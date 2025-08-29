#ifndef INC_LOG_H_
#define INC_LOG_H_

#include <stdarg.h>
#include <stdint.h>

#include "cmsis_os.h"

#define LOG_BUFFER_SIZE 512

typedef struct Log_HandleTypeDef {
	char		 buffer[LOG_BUFFER_SIZE];
	uint16_t	 writeIndex;
	uint16_t	 readIndex;
	osThreadId_t logTask;
} Log_HandleTypeDef;

void log_init(osThreadId_t taskHandle);
void log_printf_va_list(const char *fmt, va_list args) __attribute__((format(printf, 1, 0)));
void log_printf(const char *fmt, ...) __attribute__((format(printf, 1, 2)));
void output_log_buffer(void);

#endif /* INC_LOG_H_ */