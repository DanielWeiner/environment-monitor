#ifndef INC_LOG_H_
#define INC_LOG_H_

#include <stdarg.h>
#include <stdint.h>

#include "cmsis_os.h"

#define LOG_BUFFER_SIZE 512

typedef struct Log_HandleTypeDef {
	char	 buffer[LOG_BUFFER_SIZE];
	uint16_t writeIndex;
} Log_HandleTypeDef;

void output_log_buffer(uint32_t timeout);

#endif /* INC_LOG_H_ */