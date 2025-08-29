#ifndef INC_AT_H_
#define INC_AT_H_

#include "cmsis_os.h"
#include "stm32l4xx_hal.h"
#include "string.h"
#include "token.h"

#define at_set_tokens(handle, tks) at_set_tokens_n(handle, sizeof(tks) / sizeof(AT_TokenHandler), tks)
#define at_clear_tokens(handle) at_set_tokens_n(handle, 0, NULL)
#define at_send(handle, str) at_send_n(handle, (str), sizeof(str) - 1)

typedef struct AT_TokenHandler {
	TokenMatcher *token;
	void (*onToken)(void *);
} AT_TokenHandler;

typedef struct AT_Handle {
	char *const	   rxBuffer;
	char *const	   txBuffer;
	const uint16_t rxBufferSize;
	const uint16_t txBufferSize;
	uint16_t	   pendingBytes;
	uint16_t	   readIndex;
	void (*onRxByte)(char, void *);
	void				  *callbackArg;
	const AT_TokenHandler *tokenHandlers;
	uint16_t			   numTokenHandlers;
	UART_HandleTypeDef	  *uart;
	osThreadId_t		   taskHandle;
} AT_Handle;

void at_recover_from_errors(AT_Handle *handle);
bool at_init(AT_Handle *handle, UART_HandleTypeDef *uart);
void at_on_rx_byte(AT_Handle *handle, void (*callback)(char, void *));
void at_set_callback_data(AT_Handle *handle, void *arg);
void at_set_tokens_n(AT_Handle *handle, uint16_t numTokens, const AT_TokenHandler *tokens);
void at_uart_error(AT_Handle *handle, UART_HandleTypeDef *huart);
void at_send_n(AT_Handle *handle, const char *str, uint16_t len);
void at_consume_rx(AT_Handle *handle);
#endif /* INC_AT_H_ */