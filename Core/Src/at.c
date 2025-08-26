#include "at.h"

#include <stdio.h>

#include "cmsis_os.h"
#include "log.h"

extern osMutexId_t esp8266ATMutexHandle;
extern osMutexId_t esp8266TxChunkMutexHandle;

/**
 * @brief Initialize the AT command handler
 * @param handle AT handle
 * @param uart UART handle
 * @return void
 */
void at_init(AT_Handle *handle, UART_HandleTypeDef *uart) {
	if (!handle || !uart) return;
	handle->uart = uart;
	handle->onRxByte = NULL;
	handle->tokenHandlers = NULL;
	handle->numTokenHandlers = 0;
	handle->rxIndex = 0;
	handle->readIndex = 0;
	handle->ready = false;

	// start receiving the data
	HAL_UART_Receive_IT(handle->uart, (uint8_t *)&handle->rxByte, 1);
}

/**
 * @brief Set the callback function for received bytes
 * @param handle AT handle
 * @param callback Callback function
 * @return void
 */
void at_on_rx_byte(AT_Handle *handle, void (*callback)(char)) {
	handle->onRxByte = callback;
}

/**
 * @brief Set the token handlers
 * @param handle AT handle
 * @param numTokens Number of token handlers
 * @param tokens Array of token handlers
 * @return void
 */
void at_set_tokens_n(AT_Handle *handle, uint16_t numTokens, const AT_TokenHandler *tokens) {
	handle->tokenHandlers = tokens;
	handle->numTokenHandlers = numTokens;
}

/**
 * @brief ISR routine for buffering received bytes into a circular buffer
 * @param huart UART handle
 * @param handle AT handle
 * @return void
 */
void at_buffer_rx_byte(UART_HandleTypeDef *huart, AT_Handle *handle) {
	if (huart->Instance != handle->uart->Instance) return;

	handle->rxBuffer[handle->rxIndex++] = handle->rxByte;
	handle->rxIndex %= handle->rxBufferSize;

	// continue receiving the data
	HAL_UART_Receive_IT(handle->uart, (uint8_t *)&handle->rxByte, 1);
}

/**
 * @brief Send a string via AT command interface
 * @param handle AT handle
 * @param str String to send
 * @param len Length of the string
 * @return void
 */
void at_send(AT_Handle *handle, const char *str, uint16_t len) {
	if (!handle || !str) return;

	if (esp8266ATMutexHandle) osMutexAcquire(esp8266ATMutexHandle, osWaitForever);

	while (len > 0) {
		uint16_t batchLen = len < handle->txBufferSize ? len : handle->txBufferSize - 1;
		memcpy(handle->txBuffer, str, batchLen);
		handle->txBuffer[batchLen] = '\0';

		HAL_UART_Transmit_IT(handle->uart, (uint8_t *)handle->txBuffer, batchLen);

		// wait until TX is done
		if (esp8266TxChunkMutexHandle) osMutexAcquire(esp8266TxChunkMutexHandle, osWaitForever);

		str += batchLen;
		len -= batchLen;
	}

	if (esp8266ATMutexHandle) osMutexRelease(esp8266ATMutexHandle);
}

/**
 * @brief ISR routine for handling transmission complete interrupt
 * @param huart UART handle
 * @param handle AT handle
 * @return void
 */
void at_tx_complete(UART_HandleTypeDef *huart, AT_Handle *handle) {
	if (huart->Instance != handle->uart->Instance) return;

	// release the TX chunk mutex so that more chunks can be sent
	if (esp8266TxChunkMutexHandle) osMutexRelease(esp8266TxChunkMutexHandle);
}

/**
 * @brief Consume received bytes from the circular buffer
 * @param handle AT handle
 * @return void
 */
void at_consume_rx(AT_Handle *handle) {
	while (handle->readIndex != handle->rxIndex) {
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