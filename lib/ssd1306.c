#include "ssd1306.h"
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include <string.h>
#include <stdlib.h>

// Definições do I2C e endereço do display
#define I2C_PORT i2c1
#define I2C_SDA  14
#define I2C_SCL  15
#define endereco 0x3C  // Endereço típico do SSD1306, ajuste se necessário

// Display buffer reorganizado para páginas
uint8_t buffer[DISPLAY_HEIGHT/8][DISPLAY_WIDTH];

// Matriz de fontes (baseada no seu exemplo anterior, expandida para ' ' a 'Z')
static const uint8_t font[] = {
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // ' ' (32)
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // ! (33)
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // " (34)
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // # (35)
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // $ (36)
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // % (37)
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // & (38)
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // ' (39)
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // ( (40)
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // ) (41)
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // * (42)
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // + (43)
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // , (44)
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // - (45)
  0x00, 0x00, 0x00, 0x60, 0x60, 0x00, 0x00, 0x00, // . (46)
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // / (47)
  0x3e, 0x41, 0x41, 0x49, 0x41, 0x41, 0x3e, 0x00, // 0 (48)
  0x00, 0x00, 0x42, 0x7f, 0x40, 0x00, 0x00, 0x00, // 1
  0x30, 0x49, 0x49, 0x49, 0x49, 0x46, 0x00, 0x00, // 2
  0x49, 0x49, 0x49, 0x49, 0x49, 0x49, 0x36, 0x00, // 3
  0x3f, 0x20, 0x20, 0x78, 0x20, 0x20, 0x00, 0x00, // 4
  0x4f, 0x49, 0x49, 0x49, 0x49, 0x30, 0x00, 0x00, // 5
  0x3f, 0x48, 0x48, 0x48, 0x48, 0x48, 0x30, 0x00, // 6
  0x01, 0x01, 0x01, 0x61, 0x31, 0x0d, 0x03, 0x00, // 7
  0x36, 0x49, 0x49, 0x49, 0x49, 0x49, 0x36, 0x00, // 8
  0x06, 0x09, 0x09, 0x09, 0x09, 0x09, 0x7f, 0x00, // 9
  0x00, 0x00, 0x36, 0x36, 0x00, 0x00, 0x00, 0x00, // : (58)
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // ; (59)
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // < (60)
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // = (61)
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // > (62)
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // ? (63)
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // @ (64)
  0x78, 0x14, 0x12, 0x11, 0x12, 0x14, 0x78, 0x00, // A (65)
  0x7f, 0x49, 0x49, 0x49, 0x49, 0x49, 0x7f, 0x00, // B
  0x7e, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x00, // C
  0x7f, 0x41, 0x41, 0x41, 0x41, 0x41, 0x7e, 0x00, // D
  0x7f, 0x49, 0x49, 0x49, 0x49, 0x49, 0x49, 0x00, // E
  0x7f, 0x09, 0x09, 0x09, 0x09, 0x01, 0x01, 0x00, // F
  0x7f, 0x41, 0x41, 0x41, 0x51, 0x51, 0x73, 0x00, // G
  0x7f, 0x08, 0x08, 0x08, 0x08, 0x08, 0x7f, 0x00, // H
  0x00, 0x00, 0x00, 0x7f, 0x00, 0x00, 0x00, 0x00, // I
  0x21, 0x41, 0x41, 0x3f, 0x01, 0x01, 0x01, 0x00, // J
  0x00, 0x7f, 0x08, 0x08, 0x14, 0x22, 0x41, 0x00, // K
  0x7f, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x00, // L
  0x7f, 0x02, 0x04, 0x08, 0x04, 0x02, 0x7f, 0x00, // M
  0x7f, 0x02, 0x04, 0x08, 0x10, 0x20, 0x7f, 0x00, // N
  0x3e, 0x41, 0x41, 0x41, 0x41, 0x41, 0x3e, 0x00, // O
  0x7f, 0x11, 0x11, 0x11, 0x11, 0x11, 0x0e, 0x00, // P
  0x3e, 0x41, 0x41, 0x49, 0x51, 0x61, 0x7e, 0x00, // Q
  0x7f, 0x11, 0x11, 0x11, 0x31, 0x51, 0x0e, 0x00, // R
  0x46, 0x49, 0x49, 0x49, 0x49, 0x30, 0x00, 0x00, // S
  0x01, 0x01, 0x01, 0x7f, 0x01, 0x01, 0x01, 0x00, // T
  0x3f, 0x40, 0x40, 0x40, 0x40, 0x40, 0x3f, 0x00, // U
  0x0f, 0x10, 0x20, 0x40, 0x20, 0x10, 0x0f, 0x00, // V
  0x7f, 0x20, 0x10, 0x08, 0x10, 0x20, 0x7f, 0x00, // W
  0x00, 0x41, 0x22, 0x14, 0x14, 0x22, 0x41, 0x00, // X
  0x01, 0x02, 0x04, 0x78, 0x04, 0x02, 0x01, 0x00, // Y
  0x41, 0x61, 0x59, 0x45, 0x43, 0x41, 0x00, 0x00, // Z (90)
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // [ (91)
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // \ (92)
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // ] (93)
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // ^ (94)
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // _ (95)
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // ` (96)
  0x20, 0x54, 0x54, 0x54, 0x38, 0x00, 0x00, 0x00, // a (97)
  0x7c, 0x44, 0x44, 0x44, 0x38, 0x00, 0x00, 0x00, // b
  0x38, 0x44, 0x44, 0x44, 0x00, 0x00, 0x00, 0x00, // c
  0x38, 0x44, 0x44, 0x44, 0x7c, 0x00, 0x00, 0x00, // d
  0x38, 0x54, 0x54, 0x54, 0x18, 0x00, 0x00, 0x00, // e
  0x08, 0x7e, 0x09, 0x01, 0x02, 0x00, 0x00, 0x00, // f
  0x18, 0x24, 0x24, 0x24, 0x1c, 0x00, 0x00, 0x00, // g
  0x7c, 0x08, 0x04, 0x04, 0x78, 0x00, 0x00, 0x00, // h
  0x00, 0x00, 0x48, 0x7c, 0x40, 0x00, 0x00, 0x00, // i
  0x40, 0x40, 0x40, 0x3c, 0x00, 0x00, 0x00, 0x00, // j
  0x7c, 0x10, 0x28, 0x44, 0x00, 0x00, 0x00, 0x00, // k
  0x00, 0x00, 0x7c, 0x40, 0x00, 0x00, 0x00, 0x00, // l
  0x7c, 0x04, 0x38, 0x04, 0x78, 0x00, 0x00, 0x00, // m
  0x7c, 0x04, 0x04, 0x78, 0x00, 0x00, 0x00, 0x00, // n
  0x38, 0x44, 0x44, 0x38, 0x00, 0x00, 0x00, 0x00, // o
  0x7c, 0x24, 0x24, 0x24, 0x18, 0x00, 0x00, 0x00, // p
  0x18, 0x24, 0x24, 0x24, 0x7c, 0x00, 0x00, 0x00, // q
  0x7c, 0x08, 0x04, 0x04, 0x00, 0x00, 0x00, 0x00, // r
  0x48, 0x54, 0x54, 0x24, 0x00, 0x00, 0x00, 0x00, // s
  0x04, 0x3e, 0x44, 0x20, 0x00, 0x00, 0x00, 0x00, // t
  0x3c, 0x40, 0x40, 0x7c, 0x00, 0x00, 0x00, 0x00, // u
  0x1c, 0x20, 0x40, 0x20, 0x1c, 0x00, 0x00, 0x00, // v
  0x3c, 0x40, 0x38, 0x40, 0x3c, 0x00, 0x00, 0x00, // w
  0x44, 0x28, 0x10, 0x28, 0x44, 0x00, 0x00, 0x00, // x
  0x1c, 0x20, 0x20, 0x20, 0x1c, 0x00, 0x00, 0x00, // y
  0x44, 0x64, 0x54, 0x4c, 0x44, 0x00, 0x00, 0x00  // z (122)
};

void ssd1306_send_command(uint8_t cmd) {
    uint8_t buf[2] = {0x00, cmd};  // 0x00 indica comando
    i2c_write_blocking(I2C_PORT, endereco, buf, 2, false);
}

void ssd1306_send_data(uint8_t *data, size_t len) {
    uint8_t *temp_buffer = malloc(len + 1);
    temp_buffer[0] = 0x40;
    memcpy(temp_buffer + 1, data, len);
    i2c_write_blocking(I2C_PORT, endereco, temp_buffer, len + 1, false);
    free(temp_buffer);
}

void ssd1306_init() {
    sleep_ms(100);

    i2c_init(I2C_PORT, 400 * 1000);
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);

    ssd1306_send_command(0xAE);
    ssd1306_send_command(0xD5);
    ssd1306_send_command(0x80);
    ssd1306_send_command(0xA8);
    ssd1306_send_command(0x3F);
    ssd1306_send_command(0xD3);
    ssd1306_send_command(0x00);
    ssd1306_send_command(0x40);
    ssd1306_send_command(0x8D);
    ssd1306_send_command(0x14);
    ssd1306_send_command(0x20);
    ssd1306_send_command(0x00);
    ssd1306_send_command(0xA1);
    ssd1306_send_command(0xC8);
    ssd1306_send_command(0xDA);
    ssd1306_send_command(0x12);
    ssd1306_send_command(0x81);
    ssd1306_send_command(0xCF);
    ssd1306_send_command(0xD9);
    ssd1306_send_command(0xF1);
    ssd1306_send_command(0xDB);
    ssd1306_send_command(0x30);
    ssd1306_send_command(0xA4);
    ssd1306_send_command(0xA6);
    ssd1306_send_command(0xAF);

    ssd1306_clear();
    ssd1306_update();
}

void ssd1306_clear() {
    memset(buffer, 0, sizeof(buffer));
}

void ssd1306_draw_pixel(int x, int y, bool color) {
    if (x < 0 || x >= DISPLAY_WIDTH || y < 0 || y >= DISPLAY_HEIGHT) 
        return;

    uint8_t page = y / 8;
    uint8_t bit = y % 8;

    if (color) {
        buffer[page][x] |= (1 << bit);
    } else {
        buffer[page][x] &= ~(1 << bit);
    }
}

void ssd1306_set_page_address(uint8_t start, uint8_t end) {
    ssd1306_send_command(0x22);
    ssd1306_send_command(start & 0x07);
    ssd1306_send_command(end & 0x07);
}

void ssd1306_set_column_address(uint8_t start, uint8_t end) {
    ssd1306_send_command(0x21);
    ssd1306_send_command(start & 0x7F);
    ssd1306_send_command(end & 0x7F);
}

void ssd1306_update() {
    ssd1306_set_page_address(0, 7);
    ssd1306_set_column_address(0, DISPLAY_WIDTH-1);
    
    for (int page = 0; page < DISPLAY_HEIGHT/8; page++) {
        ssd1306_send_data(buffer[page], DISPLAY_WIDTH);
    }
}

// Função para desenhar um caractere
void ssd1306_draw_char(int x, int y, char c) {
  if (c < ' ' || c > 'z') c = ' '; // Limita ao intervalo da fonte (' ' a 'z')
  uint16_t index = (c - ' ') * 8;

  for (uint8_t i = 0; i < 8; i++) {
      uint8_t line = font[index + i];
      for (uint8_t j = 0; j < 8; j++) {
          ssd1306_draw_pixel(x + i, y + j, line & (1 << j));
      }
  }
}

// Função para desenhar uma string
void ssd1306_draw_string(int x, int y, const char *str) {
    int current_x = x;
    while (*str) {
        ssd1306_draw_char(current_x, y, *str);
        current_x += 8; // Avança 8 pixels para o próximo caractere
        str++;
        if (current_x + 8 >= DISPLAY_WIDTH) {
            current_x = x;
            y += 8; // Pula para a próxima linha
            if (y + 8 >= DISPLAY_HEIGHT) break; // Sai se ultrapassar a altura
        }
    }
}
void ssd1306_draw_hline(int x0, int x1, int y, bool color) {
    // Garante que x0 seja menor que x1
    if (x0 > x1) {
        int temp = x0;
        x0 = x1;
        x1 = temp;
    }

    // Limita as coordenadas ao tamanho do display
    if (y < 0 || y >= DISPLAY_HEIGHT) return;
    if (x0 < 0) x0 = 0;
    if (x1 >= DISPLAY_WIDTH) x1 = DISPLAY_WIDTH - 1;

    // Desenha a linha horizontal pixel por pixel
    for (int x = x0; x <= x1; x++) {
        ssd1306_draw_pixel(x, y, color);
    }
}

void ssd1306_draw_vline(int x, int y0, int y1, bool color) {
    // Garante que y0 seja menor que y1
    if (y0 > y1) {
        int temp = y0;
        y0 = y1;
        y1 = temp;
    }

    // Limita as coordenadas ao tamanho do display
    if (x < 0 || x >= DISPLAY_WIDTH) return;
    if (y0 < 0) y0 = 0;
    if (y1 >= DISPLAY_HEIGHT) y1 = DISPLAY_HEIGHT - 1;

    // Desenha a linha vertical pixel por pixel
    for (int y = y0; y <= y1; y++) {
        ssd1306_draw_pixel(x, y, color);
    }
}