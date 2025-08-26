#include <stdarg.h>

#include "cmsis_os.h"
#include "driver_st7789_interface.h"
#include "log.h"
#include "spi.h"

extern SPI_HandleTypeDef hspi1;

/**
 * @brief  interface spi bus init
 * @return status code
 *         - 0 success
 *         - 1 spi init failed
 * @note   none
 */
uint8_t st7789_interface_spi_init(void) {
	return 0;
}

/**
 * @brief  interface spi bus deinit
 * @return status code
 *         - 0 success
 *         - 1 spi deinit failed
 * @note   none
 */
uint8_t st7789_interface_spi_deinit(void) {
	return 0;
}

/**
 * @brief     interface spi bus write
 * @param[in] *buf pointer to a data buffer
 * @param[in] len length of data buffer
 * @return    status code
 *            - 0 success
 *            - 1 write failed
 * @note      none
 */
uint8_t st7789_interface_spi_write_cmd(uint8_t *buf, uint16_t len) {
	HAL_GPIO_WritePin(LCD_CSX_GPIO_Port, LCD_CSX_Pin, GPIO_PIN_RESET);

	if (HAL_SPI_Transmit(&hspi1, buf, len, HAL_MAX_DELAY) != HAL_OK) {
		HAL_GPIO_WritePin(LCD_CSX_GPIO_Port, LCD_CSX_Pin, GPIO_PIN_SET);
		return 1;
	}

	HAL_GPIO_WritePin(LCD_CSX_GPIO_Port, LCD_CSX_Pin, GPIO_PIN_SET);
	return 0;
}

/**
 * @brief     interface delay ms
 * @param[in] ms time
 * @note      none
 */
void st7789_interface_delay_ms(uint32_t ms) {
	osDelay(ms);
}

/**
 * @brief     interface print format data
 * @param[in] fmt format data
 * @note      none
 */
void st7789_interface_debug_print(const char *const fmt, ...) {
	va_list args;
	va_start(args, fmt);
	log_printf_va_list(fmt, args);
	va_end(args);
}

/**
 * @brief  interface command && data gpio init
 * @return status code
 *         - 0 success
 *         - 1 gpio init failed
 * @note   none
 */
uint8_t st7789_interface_cmd_data_gpio_init(void) {
	return 0;
}

/**
 * @brief  interface command && data gpio deinit
 * @return status code
 *         - 0 success
 *         - 1 gpio deinit failed
 * @note   none
 */
uint8_t st7789_interface_cmd_data_gpio_deinit(void) {
	return 0;
}

/**
 * @brief     interface command && data gpio write
 * @param[in] value written value
 * @return    status code
 *            - 0 success
 *            - 1 gpio write failed
 * @note      none
 */
uint8_t st7789_interface_cmd_data_gpio_write(uint8_t value) {
	HAL_GPIO_WritePin(LCD_DCX_GPIO_Port, LCD_DCX_Pin, (GPIO_PinState)(value));
	return 0;
}

/**
 * @brief  interface reset gpio init
 * @return status code
 *         - 0 success
 *         - 1 gpio init failed
 * @note   none
 */
uint8_t st7789_interface_reset_gpio_init(void) {
	return 0;
}

/**
 * @brief  interface reset gpio deinit
 * @return status code
 *         - 0 success
 *         - 1 gpio deinit failed
 * @note   none
 */
uint8_t st7789_interface_reset_gpio_deinit(void) {
	return 0;
}

/**
 * @brief     interface reset gpio write
 * @param[in] value written value
 * @return    status code
 *            - 0 success
 *            - 1 gpio write failed
 * @note      none
 */
uint8_t st7789_interface_reset_gpio_write(uint8_t value) {
	HAL_GPIO_WritePin(LCD_RST_GPIO_Port, LCD_RST_Pin, (GPIO_PinState)(value));
	return 0;
}
