#include <stdio.h>
#include <stdlib.h>
#include "pico/stdlib.h"
#include "hardware/adc.h"
#include "hardware/pwm.h"
#include "hardware/i2c.h"
#include "hardware/clocks.h"
#include "lib/ssd1306.h"    // Biblioteca para o display OLED
#include "U7T_projeto.pio.h"

// ============================
// Definições de pinos
// ============================
#define BUZZER_PIN     21
#define JOYSTICK_X     27
#define JOYSTICK_Y     26
#define JOYSTICK_BTN   22
#define BTN_A          5
#define BTN_B          6
#define LED_R          13
#define LED_G          11
#define LED_B          12
#define DEADZONE       200
#define WS2812_PIN     7

// ============================
// Variáveis globais
// ============================
uint16_t x_center, y_center; // Valores centrais do joystick após calibração
typedef enum {
    RAMPA_INICIAL,
    MOSTURACAO,
    RAMPA_FINAL
} system_state_t;
system_state_t current_state = RAMPA_INICIAL;

static uint8_t flame_frame = 0;         // Frame atual da animação da chama
static uint32_t last_flame_time = 0;    // Último momento em que a chama foi atualizada
static bool flame_active = false;       // Indica se a chama está ativa

// Frames da animação da chama (4 frames para 5x5)
const uint8_t flame_frames[4][5][5] = {
    {
        {0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0},
        {0, 0, 1, 0, 0},
        {0, 1, 2, 1, 0},
        {1, 2, 3, 2, 1}
    },
    {
        {0, 0, 0, 0, 0},
        {0, 0, 1, 0, 0},
        {0, 1, 2, 1, 0},
        {1, 2, 3, 2, 1},
        {1, 2, 3, 2, 1}
    },
    {
        {0, 0, 1, 0, 0},
        {0, 1, 2, 1, 0},
        {1, 2, 3, 2, 1},
        {1, 2, 3, 2, 1},
        {1, 2, 3, 2, 1}
    },
    {
        {0, 1, 0, 1, 0},
        {1, 2, 3, 2, 0},
        {1, 2, 3, 2, 1},
        {1, 2, 3, 2, 1},
        {1, 2, 3, 2, 1}
    }
};

// ============================
// Funções de inicialização
// ============================
void pwm_init_gpio(uint gpio, uint32_t freq) {
    gpio_set_function(gpio, GPIO_FUNC_PWM);
    uint slice = pwm_gpio_to_slice_num(gpio);
    uint32_t clock_freq = clock_get_hz(clk_sys);
    uint32_t wrap_value = clock_freq / freq - 1;
    pwm_set_wrap(slice, wrap_value);
    pwm_set_clkdiv(slice, 1.0f);
    pwm_set_enabled(slice, true);
}

int16_t adjust_value(int16_t raw, int16_t center) {
    int16_t diff = raw - center;
    if (abs(diff) < DEADZONE) return 0;
    return diff;
}

void calibrate_joystick() {
    const int samples = 100;
    uint32_t sum_x = 0, sum_y = 0;
    for (int i = 0; i < samples; i++) {
        adc_select_input(0);
        sum_x += adc_read();
        adc_select_input(1);
        sum_y += adc_read();
        sleep_ms(5);
    }
    x_center = sum_x / samples;
    y_center = sum_y / samples;
    printf("Calibração: x_center=%d, y_center=%d\n", x_center, y_center);
}

// ============================
// Controle de dispositivos
// ============================
void set_buzzer_position(uint16_t pulse) {
    pwm_set_gpio_level(BUZZER_PIN, pulse);
}

void stop_buzzer() {
    pwm_set_gpio_level(BUZZER_PIN, 0);
}

void draw_hline(int x0, int x1, int y, bool color) {
    for (int x = x0; x <= x1; x++) {
        ssd1306_draw_pixel(x, y, color);
    }
}

void draw_vline(int x, int y0, int y1, bool color) {
    for (int y = y0; y <= y1; y++) {
        ssd1306_draw_pixel(x, y, color);
    }
}

void draw_double_border() {
    draw_hline(0, DISPLAY_WIDTH - 1, 0, true);
    draw_hline(0, DISPLAY_WIDTH - 1, DISPLAY_HEIGHT - 1, true);
    draw_vline(0, 0, DISPLAY_HEIGHT - 1, true);
    draw_vline(DISPLAY_WIDTH - 1, 0, DISPLAY_HEIGHT - 1, true);
    draw_hline(2, DISPLAY_WIDTH - 3, 2, true);
    draw_hline(2, DISPLAY_WIDTH - 3, DISPLAY_HEIGHT - 3, true);
    draw_vline(2, 2, DISPLAY_HEIGHT - 3, true);
    draw_vline(DISPLAY_WIDTH - 3, 2, DISPLAY_HEIGHT - 3, true);
}

void update_display(float temperature, system_state_t state, bool timer_active, uint8_t timer_seconds, bool flame_active) {
    ssd1306_clear();
    draw_double_border();
    char temp_str[16];
    snprintf(temp_str, sizeof(temp_str), "T: %.1f°C", temperature);
    ssd1306_draw_string(4, 4, temp_str);

    const char* state_str;
    switch (state) {
        case RAMPA_INICIAL: state_str = "Rampa Inicial"; break;
        case MOSTURACAO: state_str = "Mosturacao"; break;
        case RAMPA_FINAL: state_str = "Rampa Final"; break;
        default: state_str = "ERRO"; break;
    }
    ssd1306_draw_string(4, 12, state_str);

    if (timer_active) {
        char timer_str[16];
        snprintf(timer_str, sizeof(timer_str), "Timer: %02ds", timer_seconds);
        ssd1306_draw_string(4, 20, timer_str);
    }

    char flame_str[16];
    snprintf(flame_str, sizeof(flame_str), "Chama: %s", flame_active ? "ON" : "OFF");
    ssd1306_draw_string(4, 28, flame_str);

    ssd1306_update();
}

// ============================
// Animação da Chama
// ============================
void update_flame_animation(PIO pio, uint sm, uint8_t frame) {
    for (int y = 0; y < 5; y++) {
        for (int x = 0; x < 5; x++) {
            int led_index = y * 5 + x;
            uint8_t intensity = flame_frames[frame][y][x];

            uint8_t r = 0, g = 0, b = 0;
            switch (intensity) {
                case 0: // Desligado
                    r = 0;
                    g = 0;
                    b = 0;
                    break;
                case 1: // Baixa intensidade (vermelho escuro)
                    r = 64;
                    g = 0;
                    b = 0;
                    break;
                case 2: // Média intensidade (laranja)
                    r = 255;
                    g = 64;
                    b = 0;
                    break;
                case 3: // Alta intensidade (amarelo)
                    r = 255;
                    g = 128;
                    b = 0;
                    break;
            }

            // Formata a cor no formato GRB (Green-Red-Blue)
            uint32_t grb = ((uint32_t)g << 16) | ((uint32_t)r << 8) | (uint32_t)b;
            pio_sm_put_blocking(pio, sm, grb << 8u);
        }
    }
}

// ============================
// Função principal
// ============================
int main() {
    stdio_init_all();
    sleep_ms(2000); // Aguarda inicialização do USB para depuração

    // Inicialização ADC
    adc_init();
    adc_gpio_init(JOYSTICK_X);
    adc_gpio_init(JOYSTICK_Y);

    // Inicialização Botões
    gpio_init(JOYSTICK_BTN);
    gpio_set_dir(JOYSTICK_BTN, GPIO_IN);
    gpio_pull_up(JOYSTICK_BTN);
    gpio_init(BTN_A);
    gpio_set_dir(BTN_A, GPIO_IN);
    gpio_pull_up(BTN_A);
    gpio_init(BTN_B);
    gpio_set_dir(BTN_B, GPIO_IN);
    gpio_pull_up(BTN_B);

    // Inicialização LEDs
    pwm_init_gpio(LED_R, 50);
    gpio_init(LED_G);
    gpio_set_dir(LED_G, GPIO_OUT);
    gpio_put(LED_G, false);
    gpio_init(LED_B);
    gpio_set_dir(LED_B, GPIO_OUT);
    gpio_put(LED_B, false);

    // Inicialização Buzzer
    pwm_init_gpio(BUZZER_PIN, 50);
    uint32_t wrap_value = clock_get_hz(clk_sys) / 50 - 1;
    uint16_t pulse_min = wrap_value * 0.05;
    uint16_t pulse_center = wrap_value * 0.075;
    uint16_t pulse_max = wrap_value * 0.1;
    stop_buzzer();

    // Inicialização do Display e LEDs WS2812
    ssd1306_init();

    // Inicializa o PIO para controle do WS2812
    PIO pio = pio0;
    uint sm = 0;
    uint offset = pio_add_program(pio, &U7T_projeto_program);
    U7T_projeto_program_init(pio, sm, offset, WS2812_PIN, 800000, false);

    printf("Inicialização concluída\n"); // Depuração via USB
    calibrate_joystick();

    float temperature = 0.0f;
    bool timer_active = false;
    uint8_t timer_seconds = 0;
    bool buzzer_triggered = false;

    while (true) {
        adc_select_input(1);
        uint16_t raw_y = adc_read();

        switch (current_state) {
            case RAMPA_INICIAL:
                temperature += ((float)adjust_value(raw_y, y_center) / 4095.0f) * 0.5f;
                if (temperature < 0.0f) temperature = 0.0f;
                if (temperature < 68.0f) {
                    pwm_set_gpio_level(LED_R, 4095);
                    flame_active = true;
                } else {
                    temperature = 68.0f;
                    pwm_set_gpio_level(LED_R, 0);
                    flame_active = false;
                }
                if (!gpio_get(BTN_A) && temperature >= 68.0f) {
                    current_state = MOSTURACAO;
                    timer_active = true;
                    timer_seconds = 0;
                    gpio_put(LED_G, true);
                    sleep_ms(50);
                }
                break;

            case MOSTURACAO:
                flame_active = false;
                if (timer_active && timer_seconds < 15) {
                    // Mantém LED verde aceso
                } else {
                    timer_active = false;
                    gpio_put(LED_G, false);
                    for (int i = 0; i < 3; i++) {
                        gpio_put(LED_B, true);
                        sleep_ms(500);
                        gpio_put(LED_B, false);
                        sleep_ms(500);
                    }
                    if (!buzzer_triggered) {
                        set_buzzer_position(pulse_min);
                        sleep_ms(500);
                        set_buzzer_position(pulse_center);
                        sleep_ms(500);
                        set_buzzer_position(pulse_max);
                        sleep_ms(500);
                        stop_buzzer();
                        buzzer_triggered = true;
                    }
                    if (!gpio_get(BTN_B)) {
                        current_state = RAMPA_FINAL;
                        buzzer_triggered = false;
                        sleep_ms(50);
                    }
                }
                break;

            case RAMPA_FINAL:
                temperature += ((float)adjust_value(raw_y, y_center) / 4095.0f) * 0.5f;
                if (temperature < 68.0f) temperature = 68.0f;
                if (temperature < 100.0f) {
                    pwm_set_gpio_level(LED_R, 4095);
                    flame_active = true;
                } else {
                    temperature = 100.0f;
                    pwm_set_gpio_level(LED_R, 4095);
                    flame_active = true;
                    if (!timer_active) {
                        timer_active = true;
                        timer_seconds = 0;
                    }
                }
                if (timer_active && timer_seconds >= 15) {
                    timer_active = false;
                    pwm_set_gpio_level(LED_R, 0);
                    flame_active = false;
                    current_state = RAMPA_INICIAL;
                    temperature = 0.0f;
                }
                break;
        }

        // Atualiza a animação da chama se o LED vermelho estiver ligado
        if (flame_active) {
            update_flame_animation(pio, sm, flame_frame);
            flame_frame = (flame_frame + 1) % 4; // Alterna para o próximo frame
        } else {
            // Desliga todos os LEDs da matriz quando a chama estiver desativada
            for (int i = 0; i < 25; i++) {
                uint32_t grb = 0; // Cor preta (desligado)
                pio_sm_put_blocking(pio, sm, grb << 8u);
            }
        }

        update_display(temperature, current_state, timer_active, timer_seconds, flame_active);

        if (timer_active && timer_seconds < 15) {
            sleep_ms(1000);
            timer_seconds++;
        } else {
            sleep_ms(50);
        }
    } 

    return 0;
}