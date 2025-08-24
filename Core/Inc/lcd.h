#ifndef INC_LCD_H_
#define INC_LCD_H_

#include "driver_st7789.h"
#include "driver_st7789_basic.h"

#define lcd_init() st7789_basic_init()
#define lcd_fill_rect(left, top, right, bottom, color)                         \
  st7789_basic_rect(left, top, right, bottom, color)

#endif /* INC_LCD_H_ */