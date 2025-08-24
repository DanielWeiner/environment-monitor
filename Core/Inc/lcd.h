#ifndef INC_LCD_H_
#define INC_LCD_H_

#include "driver_st7789.h"
#include "driver_st7789_basic.h"

/**
 * @brief	Clear the LCD
 * @return	Status code - 0 success, 1 failure
 */
static inline uint8_t lcd_clear(void) { return st7789_basic_clear(); }
/**
 * @brief	Initialize the LCD
 * @return	Status code - 0 success, 1 failure
 */
static inline uint8_t lcd_init(void) { return st7789_basic_init(); }
/**
 * @brief	Fill a rectangle on the LCD
 * @param	left   Left coordinate x
 * @param	top    Top coordinate y
 * @param	right  Right coordinate x
 * @param	bottom Bottom coordinate y
 * @param	color  Display color
 * @return	Status code - 0 success, 1 failure
 */
static inline uint8_t lcd_fill_rect(uint16_t x, uint16_t y, uint16_t w,
                                    uint16_t h, uint16_t color) {
  return st7789_basic_rect(x, y, w, h, color);
}
/**
 * @brief	Display a string on the LCD
 * @param	x      X coordinate
 * @param	y      Y coordinate
 * @param	str    Pointer to the string
 * @param	len    Length of the string
 * @param	color  Display color
 * @param	font   Font type
 * @return	Status code - 0 success, 1 failure
 */
static inline uint8_t lcd_string(uint16_t x, uint16_t y, char *str,
                                 uint16_t len, uint32_t color,
                                 st7789_font_t font) {
  return st7789_basic_string(x, y, str, len, color, font);
}

#endif /* INC_LCD_H_ */