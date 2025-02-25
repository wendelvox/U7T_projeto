#ifndef SSD1306_H
#define SSD1306_H

#include "pico/stdlib.h"

#define DISPLAY_WIDTH  128
#define DISPLAY_HEIGHT 64
#define I2C_PORT       i2c1
#define I2C_SDA        14
#define I2C_SCL        15

void ssd1306_init();
void ssd1306_clear();
void ssd1306_draw_pixel(int x, int y, bool color);
void ssd1306_set_page_address(uint8_t start, uint8_t end);
void ssd1306_set_column_address(uint8_t start, uint8_t end);
void ssd1306_update();
void ssd1306_draw_char(int x, int y, char c);
void ssd1306_draw_string(int x, int y, const char *str);
void ssd1306_draw_hline(int x0, int x1, int y, bool color);
void ssd1306_draw_vline(int x, int y0, int y1, bool color);

#endif