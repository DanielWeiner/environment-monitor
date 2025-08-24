#ifndef INC_LOG_H_
#define INC_LOG_H_

#include <stdarg.h>

#define LOG_MAX_SIZE 256

void _log_printf(const char *fmt, va_list args)
    __attribute__((format(printf, 1, 0)));
void log_printf(const char *fmt, ...) __attribute__((format(printf, 1, 2)));
void log_init(void);

#endif /* INC_LOG_H_ */