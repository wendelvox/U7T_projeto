#ifndef MATRIZ_LED_H
#define MATRIZ_LED_H

#include "pico/stdlib.h"

// Definições da matriz
#define LED_COUNT 25
#define LED_PIN 7

// Definição de pixel GRB
typedef struct {
    uint8_t G, R, B;
} pixel_t;

extern pixel_t leds[LED_COUNT];

// Funções da biblioteca
void matriz_led_init(void); // Inicializa os LEDs WS2812
void update_flame_animation(uint8_t frame); // Atualiza a animação da chama
void matriz_led_update(void); // Envia os dados para os LEDs

#endif // MATRIZ_LED_H