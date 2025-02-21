#include "matriz_led.h"
#include "U7T_projeto.pio.h" // Inclua o cabeçalho gerado específico

// Declaração do array de LEDs
pixel_t leds[LED_COUNT];

// Configuração do PIO para WS2812
static PIO pio = pio0;
static uint sm = 0;

// Frames da animação da chama (4 frames para 5x5)
static uint8_t frames_RG[4][5][5] = {
    {{0, 0, 0, 0, 0}, {0, 0, 0, 0, 0}, {0, 0, 1, 0, 0}, {0, 1, 1, 1, 0}, {1, 1, 1, 1, 1}},
    {{0, 0, 0, 0, 0}, {0, 0, 1, 0, 0}, {0, 1, 1, 1, 0}, {1, 1, 1, 1, 1}, {1, 1, 1, 1, 1}},
    {{0, 0, 1, 0, 0}, {0, 1, 1, 1, 0}, {1, 1, 1, 1, 1}, {1, 1, 1, 1, 1}, {1, 1, 1, 1, 1}},
    {{0, 1, 0, 1, 0}, {1, 1, 1, 1, 0}, {1, 1, 1, 1, 1}, {1, 1, 1, 1, 1}, {1, 1, 1, 1, 1}}
};

static uint8_t frames_B[4][5][5] = {
    {{0, 0, 0, 0, 0}, {0, 0, 0, 0, 0}, {0, 0, 0, 0, 0}, {0, 0, 0, 0, 0}, {0, 1, 1, 1, 0}},
    {{0, 0, 0, 0, 0}, {0, 0, 0, 0, 0}, {0, 0, 0, 0, 0}, {0, 1, 0, 1, 0}, {0, 1, 1, 1, 0}},
    {{0, 0, 0, 0, 0}, {0, 0, 0, 0, 0}, {0, 1, 0, 1, 0}, {0, 1, 1, 1, 0}, {0, 1, 1, 1, 0}},
    {{0, 0, 0, 0, 0}, {0, 0, 0, 0, 0}, {0, 1, 0, 1, 0}, {0, 1, 1, 1, 0}, {0, 1, 1, 1, 0}}
};

// Função para inicializar os LEDs WS2812
void matriz_led_init(void) {
    uint offset = pio_add_program(pio, &U7T_projeto_program);
    U7T_projeto_program_init(pio, sm, offset, LED_PIN, 800000); // 800kHz
    for (int i = 0; i < LED_COUNT; i++) {
        leds[i].R = 0;
        leds[i].G = 0;
        leds[i].B = 0;
    }
    matriz_led_update();
}

// Função para enviar os dados para os LEDs
void matriz_led_update(void) {
    for (int i = 0; i < LED_COUNT; i++) {
        uint32_t grb = ((uint32_t)leds[i].G << 16) | ((uint32_t)leds[i].R << 8) | (uint32_t)leds[i].B;
        pio_sm_put_blocking(pio, sm, grb << 8u); // Formato GRB
    }
}

// Função para atualizar a animação da chama
void update_flame_animation(uint8_t frame) {
    for (int y = 0; y < 5; y++) {
        for (int x = 0; x < 5; x++) {
            int led_index = y * 5 + x;
            leds[led_index].R = frames_RG[frame][y][x] * 255;
            leds[led_index].G = frames_RG[frame][y][x] * 128;
            leds[led_index].B = frames_B[frame][y][x] * 64;
        }
    }
    matriz_led_update();
}