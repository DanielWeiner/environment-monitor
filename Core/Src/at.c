#include "at.h"

#include <stdio.h>

#include "cmsis_os.h"
#include "log.h"

static volatile char rxByte;

extern osMutexId_t esp8266ATMutexHandle;
extern osMutexId_t esp8266TxChunkMutexHandle;

void at_init(AT_Handle *handle, UART_HandleTypeDef *uart) {
	if (!handle || !uart) return;
	handle->uart = uart;
	handle->onRxByte = NULL;
	handle->tokenHandlers = NULL;
	handle->numTokenHandlers = 0;
	handle->rxIndex = 0;
	handle->readIndex = 0;
	handle->ready = false;
	HAL_UART_Receive_IT(handle->uart, (uint8_t *)&rxByte, 1);
}

void at_on_rx_byte(AT_Handle *handle, void (*callback)(char)) {
	handle->onRxByte = callback;
}

void at_set_tokens_n(AT_Handle *handle, uint16_t numTokens, const AT_TokenHandler *tokens) {
	handle->tokenHandlers = tokens;
	handle->numTokenHandlers = numTokens;
}

void at_buffer_rx_byte(UART_HandleTypeDef *huart, AT_Handle *handle) {
	if (huart->Instance != handle->uart->Instance) return;
	handle->rxBuffer[handle->rxIndex++] = rxByte;
	handle->rxIndex %= handle->rxBufferSize;

	HAL_UART_Receive_IT(handle->uart, (uint8_t *)&rxByte, 1);
}

void at_send(AT_Handle *handle, const char *str, uint16_t len) {
	if (!handle || !str) return;
	if (esp8266ATMutexHandle) {
		osMutexAcquire(esp8266ATMutexHandle, osWaitForever);
	}

	while (len > 0) {
		uint16_t batchLen = len < handle->txBufferSize ? len : handle->txBufferSize - 1;
		memcpy(handle->txBuffer, str, batchLen);
		handle->txBuffer[batchLen] = '\0';

		HAL_UART_Transmit_IT(handle->uart, (uint8_t *)handle->txBuffer, batchLen);

		if (esp8266TxChunkMutexHandle) {
			osMutexAcquire(esp8266TxChunkMutexHandle, osWaitForever);
		}
		str += batchLen;
		len -= batchLen;
	}
	if (esp8266ATMutexHandle) {
		osMutexRelease(esp8266ATMutexHandle);
	}
}

void at_tx_complete(UART_HandleTypeDef *huart, AT_Handle *handle) {
	if (huart->Instance != handle->uart->Instance) return;
	if (esp8266TxChunkMutexHandle) {
		osMutexRelease(esp8266TxChunkMutexHandle);
	}
}

void at_consume_rx(AT_Handle *handle) {
	uint16_t rxIndex = handle->rxIndex;

	while (handle->readIndex != rxIndex) {
		char ch = handle->rxBuffer[handle->readIndex++];
		handle->readIndex %= handle->rxBufferSize;

		if (handle->onRxByte) {
			handle->onRxByte(ch);
		}

		if (handle->numTokenHandlers == 0 || handle->tokenHandlers == NULL) continue;

		for (uint16_t i = 0; i < handle->numTokenHandlers; i++) {
			if (token_match(handle->tokenHandlers[i].token, ch) == TOKEN_MATCH_SUCCESS) {
				if (handle->tokenHandlers[i].onToken) {
					handle->tokenHandlers[i].onToken(handle->tokenHandlers[i].onTokenArg);
				}
			}
		}
	}
}