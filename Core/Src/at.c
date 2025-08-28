#include "at.h"

#include <stdio.h>

#include "cmsis_os.h"

#define ALL_FLAGS 0xFFFFFFFF

extern osMutexId_t esp8266ATMutexHandle;
extern osMutexId_t esp8266TxChunkMutexHandle;

/**
 * @brief Recover from UART errors if present
 * @param handle AT handle
 */
bool at_recover_from_errors(AT_Handle *handle) {
	if (!handle) return true;

	if (HAL_UART_GetError(handle->uart) != HAL_UART_ERROR_NONE) {
		__HAL_UART_CLEAR_FLAG(handle->uart, ALL_FLAGS);
		HAL_UART_AbortReceive(handle->uart);
		HAL_UARTEx_ReceiveToIdle_DMA(handle->uart, (uint8_t *)handle->rxDmaBuffer, handle->rxDmaBufferSize);
		__HAL_DMA_DISABLE_IT(handle->uart->hdmarx, DMA_IT_HT);
		return true;
	}

	if (!((HAL_UART_GetState(handle->uart) == HAL_UART_STATE_BUSY_RX) ||
		  (HAL_UART_GetState(handle->uart) == HAL_UART_STATE_BUSY_TX_RX))) {
		__HAL_UART_CLEAR_FLAG(handle->uart, ALL_FLAGS);
		HAL_UART_AbortReceive(handle->uart);
		HAL_UARTEx_ReceiveToIdle_DMA(handle->uart, (uint8_t *)handle->rxDmaBuffer, handle->rxDmaBufferSize);
		__HAL_DMA_DISABLE_IT(handle->uart->hdmarx, DMA_IT_HT);
		return true;
	}
	return false;
}

/**
 * @brief Get the current RX buffer information, resetting the buffer status
 * @param handle AT handle
 * @param rxIndex Pointer to store the RX index
 * @param fullBuffer Pointer to store the full buffer status
 */
static void get_rx_buffer_info(AT_Handle *handle, uint16_t *rxIndex, bool *fullBuffer) {
	if (!handle || !rxIndex || !fullBuffer) return;
	__disable_irq();
	*rxIndex = handle->rxIndex;
	*fullBuffer = handle->fullBuffer;
	handle->fullBuffer = false;
	__enable_irq();
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
void at_init(AT_Handle *handle, UART_HandleTypeDef *uart) {
	if (!handle || !uart) return;
	handle->uart = uart;
	handle->onRxByte = NULL;
	handle->tokenHandlers = NULL;
	handle->numTokenHandlers = 0;
	handle->rxIndex = 0;
	handle->readIndex = 0;
	memset(handle->rxDmaBuffer, 0, handle->rxDmaBufferSize);
	memset((void *)handle->rxBuffer, 0, handle->rxBufferSize);
	memset((void *)handle->txBuffer, 0, handle->txBufferSize);

	__HAL_UART_CLEAR_FLAG(handle->uart, ALL_FLAGS);
	if (HAL_UARTEx_ReceiveToIdle_DMA(handle->uart, (uint8_t *)handle->rxDmaBuffer, handle->rxDmaBufferSize) != HAL_OK) {
		return;
	}
	__HAL_DMA_DISABLE_IT(handle->uart->hdmarx, DMA_IT_HT);
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

/**
 * @brief Advance the rx buffer index and copy data from the DMA buffer
 * @param handle AT handle
 * @param huart UART handle
 * @param len Length of the data
 * @return void
 */
void at_buffer_rx(AT_Handle *handle, UART_HandleTypeDef *huart, uint16_t len) {
	if (!handle || len == 0 || huart != handle->uart) return;

	uint16_t rxIndex = handle->rxIndex;
	uint16_t nextIndex = (rxIndex + len) % handle->rxBufferSize;

	if (rxIndex < nextIndex) {
		memcpy(&handle->rxBuffer[rxIndex], handle->rxDmaBuffer, len);
	} else {
		uint16_t firstPartLen = handle->rxBufferSize - rxIndex;
		if (firstPartLen > 0) {
			memcpy(&handle->rxBuffer[rxIndex], handle->rxDmaBuffer, firstPartLen);
		}
		memcpy(handle->rxBuffer, &handle->rxDmaBuffer[firstPartLen], len - firstPartLen);
	}

	handle->rxIndex = nextIndex;

	if (handle->rxIndex == handle->readIndex && len > 0) {
		handle->fullBuffer = true;
	}

	if (HAL_UARTEx_ReceiveToIdle_DMA(handle->uart, (uint8_t *)handle->rxDmaBuffer, handle->rxDmaBufferSize) == HAL_OK) {
		__HAL_DMA_DISABLE_IT(handle->uart->hdmarx, DMA_IT_HT);
	} else {
		__HAL_UART_CLEAR_FLAG(handle->uart, ALL_FLAGS);
		HAL_UART_AbortReceive(handle->uart);
		HAL_UARTEx_ReceiveToIdle_DMA(handle->uart, (uint8_t *)handle->rxDmaBuffer, handle->rxDmaBufferSize);
		__HAL_DMA_DISABLE_IT(handle->uart->hdmarx, DMA_IT_HT);
	}
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
 * @brief ISR routine for handling transmission complete interrupt
 * @param huart UART handle
 * @param handle AT handle
 * @return void
 */
void at_tx_complete(AT_Handle *handle, UART_HandleTypeDef *huart) {
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
	at_recover_from_errors(handle);

	uint16_t rxIndex = 0;
	bool	 fullBuffer = false;
	get_rx_buffer_info(handle, &rxIndex, &fullBuffer);

	while (handle->readIndex != rxIndex || fullBuffer) {
		fullBuffer = false;
		char ch = handle->rxBuffer[handle->readIndex++];
		handle->readIndex %= handle->rxBufferSize;

		process_rx_byte_handlers(handle, ch);

		if (handle->onRxByte) {
			handle->onRxByte(ch, handle->callbackArg);
		}
	}
}