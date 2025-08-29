#include "at.h"

#include <stdio.h>

#include "cmsis_os.h"

#define ALL_FLAGS 0xFFFFFFFF
#define IDLE_TIMEOUT_TICKS 1

extern osMutexId_t esp8266ATMutexHandle;
extern osMutexId_t esp8266TxChunkMutexHandle;

/**
 * @brief Recover from UART errors if present
 * @param handle AT handle
 */
void at_recover_from_errors(AT_Handle *handle) {
	if (HAL_UART_GetError(handle->uart) != HAL_UART_ERROR_NONE) {
		__HAL_UART_CLEAR_FLAG(handle->uart, ALL_FLAGS);
		HAL_UART_AbortReceive(handle->uart);
		handle->readIndex = handle->rxBufferSize - __HAL_DMA_GET_COUNTER(handle->uart->hdmarx);
		HAL_UART_Receive_DMA(handle->uart, (uint8_t *)handle->rxBuffer, handle->rxBufferSize);
		__HAL_DMA_DISABLE_IT(handle->uart->hdmarx, DMA_IT_HT | DMA_IT_TC);
	}

	if (!((HAL_UART_GetState(handle->uart) == HAL_UART_STATE_BUSY_RX) ||
		  (HAL_UART_GetState(handle->uart) == HAL_UART_STATE_BUSY_TX_RX))) {
		__HAL_UART_CLEAR_FLAG(handle->uart, ALL_FLAGS);
		HAL_UART_AbortReceive(handle->uart);
		handle->readIndex = handle->rxBufferSize - __HAL_DMA_GET_COUNTER(handle->uart->hdmarx);
		HAL_UART_Receive_DMA(handle->uart, (uint8_t *)handle->rxBuffer, handle->rxBufferSize);
		__HAL_DMA_DISABLE_IT(handle->uart->hdmarx, DMA_IT_HT | DMA_IT_TC);
	}
}

/**
 * @brief Feed a received byte to the registered token handlers
 * @param handle AT handle
 * @param byte Received byte
 */
static void process_rx_byte_handlers(AT_Handle *handle, char byte) {
	if (!handle) return;
	if (handle->numTokenHandlers == 0 || handle->tokenHandlers == NULL) return;
	uint16_t			   numHandlers = handle->numTokenHandlers;
	const AT_TokenHandler *tokenHandlers = handle->tokenHandlers;

	for (uint16_t i = 0; i < numHandlers; i++) {
		if (token_match(tokenHandlers[i].token, byte) == TOKEN_MATCH_SUCCESS) {
			if (tokenHandlers[i].onToken) {
				tokenHandlers[i].onToken(handle->callbackArg);
			}
		}
	}
}

/**
 * @brief Initialize the AT command handler
 * @param handle AT handle
 * @param uart UART handle
 * @return void
 */
bool at_init(AT_Handle *handle, UART_HandleTypeDef *uart) {
	if (!handle || !uart) return false;
	handle->uart = uart;
	handle->onRxByte = NULL;
	handle->tokenHandlers = NULL;
	handle->numTokenHandlers = 0;
	handle->readIndex = 0;
	memset((void *)handle->rxBuffer, 0, handle->rxBufferSize);
	memset((void *)handle->txBuffer, 0, handle->txBufferSize);

	__HAL_UART_CLEAR_FLAG(handle->uart, ALL_FLAGS);
	if (HAL_UART_Receive_DMA(handle->uart, (uint8_t *)handle->rxBuffer, handle->rxBufferSize) != HAL_OK) {
		return false;
	}
	__HAL_DMA_DISABLE_IT(handle->uart->hdmarx, DMA_IT_HT | DMA_IT_TC);
	return true;
}

/**
 * @brief Set the callback function for received bytes
 * @param handle AT handle
 * @param callback Callback function
 * @return void
 */
void at_on_rx_byte(AT_Handle *handle, void (*callback)(char, void *)) {
	handle->onRxByte = callback;
}

void at_set_callback_data(AT_Handle *handle, void *arg) {
	handle->callbackArg = arg;
}

/**
 * @brief Set the token handlers
 * @param handle AT handle
 * @param numTokens Number of token handlers
 * @param tokens Array of token handlers
 * @return void
 */
void at_set_tokens_n(AT_Handle *handle, uint16_t numTokens, const AT_TokenHandler *tokens) {
	if (!tokens || numTokens == 0) {
		handle->tokenHandlers = NULL;
		handle->numTokenHandlers = 0;
		return;
	}
	handle->tokenHandlers = tokens;
	handle->numTokenHandlers = numTokens;
}

void at_uart_error(AT_Handle *handle, UART_HandleTypeDef *huart) {
	if (!handle || !huart || huart != handle->uart) return;
	at_recover_from_errors(handle);
}

/**
 * @brief Send a string via AT command interface
 * @param handle AT handle
 * @param str String to send
 * @param len Length of the string
 * @return void
 */
void at_send_n(AT_Handle *handle, const char *str, uint16_t len) {
	if (!handle || !str) return;

	if (esp8266ATMutexHandle) osMutexAcquire(esp8266ATMutexHandle, osWaitForever);

	while (len > 0) {
		uint16_t batchLen = len < handle->txBufferSize ? len : handle->txBufferSize - 1;
		memcpy(handle->txBuffer, str, batchLen);
		handle->txBuffer[batchLen] = '\0';

		HAL_UART_Transmit(handle->uart, (uint8_t *)handle->txBuffer, batchLen, HAL_MAX_DELAY);

		// wait until TX is done
		str += batchLen;
		len -= batchLen;
	}

	if (esp8266ATMutexHandle) osMutexRelease(esp8266ATMutexHandle);
}

/**
 * @brief Consume received bytes from the circular buffer
 * @param handle AT handle
 * @return void
 */
void at_consume_rx(AT_Handle *handle) {
	static uint32_t lastData = 0;
	while (handle->readIndex != handle->rxBufferSize - __HAL_DMA_GET_COUNTER(handle->uart->hdmarx)) {
		lastData = osKernelGetTickCount();
		char ch = handle->rxBuffer[handle->readIndex++];
		handle->readIndex %= handle->rxBufferSize;
		process_rx_byte_handlers(handle, ch);

		if (handle->onRxByte) {
			handle->onRxByte(ch, handle->callbackArg);
		}
	}
	if (osKernelGetTickCount() - lastData < IDLE_TIMEOUT_TICKS) return;
	osDelay(1);
	at_recover_from_errors(handle);
}