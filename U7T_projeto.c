#include <stdio.h>
#include <stdlib.h>
#include "pico/stdlib.h"
#include "hardware/adc.h"
#include "hardware/pwm.h"
#include "hardware/i2c.h"
#include "hardware/clocks.h"
#include "lib/ssd1306.h"
#include "U7T_projeto.pio.h"

// Definições de pinos
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

// Definições de estados
typedef enum {
    MENU_INICIAL,
    PARADA_PROTEICA,
    BETA_AMILASE,
    ALFA_AMILASE,
    MASH_OUT
} system_state_t;

// Estrutura para estágios de brassagem
typedef struct {
    float temp_min;
    float temp_max;
    const char* nome;
    uint32_t duration; // Tempo em segundos
} brassagem_stage_t;

static const brassagem_stage_t STAGES[] = {
    {50.0f, 55.0f, "Parada Proteica", 15},
    {55.0f, 65.0f, "Beta Amilase", 60},
    {68.0f, 73.0f, "Alfa Amilase", 20},
    {75.0f, 79.0f, "Mash Out", 5}
};
#define NUM_STAGES (sizeof(STAGES) / sizeof(STAGES[0]))

// Variáveis globais
static uint16_t x_center, y_center;
static system_state_t current_state = MENU_INICIAL;
static int menu_selection = 0;
static uint8_t flame_frame = 0;
static bool flame_active = false;
static uint32_t timer_start = 0; // Tempo de início do temporizador
static bool timer_active = false;
static bool timer_finished = false;

static const uint8_t flame_frames[4][5][5] = {
    {{1, 2, 3, 2, 1}, {0, 1, 2, 1, 0}, {0, 0, 1, 0, 0}, {0, 0, 0, 0, 0}, {0, 0, 0, 0, 0}},
    {{1, 2, 3, 2, 1}, {1, 2, 3, 2, 1}, {0, 1, 2, 1, 0}, {0, 0, 1, 0, 0}, {0, 0, 0, 0, 0}},
    {{1, 2, 3, 2, 1}, {1, 2, 3, 2, 1}, {1, 2, 3, 2, 1}, {0, 1, 2, 1, 0}, {0, 0, 1, 0, 0}},
    {{1, 2, 3, 2, 1}, {1, 2, 3, 2, 1}, {1, 2, 3, 2, 1}, {1, 2, 3, 2, 0}, {0, 1, 0, 1, 0}}
};

// Funções de inicialização
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
        adc_select_input(0); sum_x += adc_read();
        adc_select_input(1); sum_y += adc_read();
        sleep_ms(5);
    }
    x_center = sum_x / samples;
    y_center = sum_y / samples;
    printf("Calibração: x_center=%d, y_center=%d\n", x_center, y_center);
}

// Controle de dispositivos
void set_buzzer_position(uint16_t pulse) {
    pwm_set_gpio_level(BUZZER_PIN, pulse);
}

void stop_buzzer() {
    pwm_set_gpio_level(BUZZER_PIN, 0);
}

// Funções de display
void draw_hline(int x0, int x1, int y, bool color) {
    for (int x = x0; x <= x1; x++) ssd1306_draw_pixel(x, y, color);
}

void draw_vline(int x, int y0, int y1, bool color) {
    for (int y = y0; y <= y1; y++) ssd1306_draw_pixel(x, y, color);
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

void show_menu(int selection) {
    ssd1306_clear();
    draw_double_border();
    ssd1306_draw_string(5, 6, "Selecione:");
    ssd1306_draw_string(5, 16, STAGES[selection].nome);
    ssd1306_draw_string(5, 48, "A:Prox  B:Sel");
    ssd1306_update();
}

void update_display(float temperature, const brassagem_stage_t* stage, bool flame_active, uint32_t elapsed_time) {
    ssd1306_clear();
    draw_double_border();
    char temp_str[16];
    snprintf(temp_str, sizeof(temp_str), "T: %.1f°C", temperature);
    ssd1306_draw_string(5, 6, temp_str);
    ssd1306_draw_string(5, 20, stage->nome);
    char timer_str[16];
    snprintf(timer_str, sizeof(timer_str), "Time: %lus", elapsed_time);
    ssd1306_draw_string(5, 38, timer_str);
    char flame_str[16];
    snprintf(flame_str, sizeof(flame_str), "Chama: %s", flame_active ? "ON" : "OFF");
    ssd1306_draw_string(5, 48, flame_str);
    ssd1306_update();
}

// Funções de controle de temperatura por estágio
void control_stage(float* temperature, const brassagem_stage_t* stage) {
    adc_select_input(1);
    uint16_t raw_y = adc_read();
    *temperature += ((float)adjust_value(raw_y, y_center) / 4095.0f) * 0.5f;
    if (*temperature < stage->temp_min) *temperature = stage->temp_min;
    if (*temperature > stage->temp_max) *temperature = stage->temp_max;

    // Tolerância de 5% abaixo do temp_max
    float tolerance = stage->temp_max * 0.05f;
    float threshold = stage->temp_max - tolerance;

    // Chama apaga apenas ao atingir temp_max, reacende se cair mais de 5%
    if (*temperature >= stage->temp_max) {
        flame_active = false;
    } else if (*temperature < threshold) {
        flame_active = true;
    }
    pwm_set_gpio_level(LED_R, flame_active ? 4095 : 0);
}

// Animação da chama
void update_flame_animation(PIO pio, uint sm, uint8_t frame) {
    for (int y = 0; y < 5; y++) {
        for (int x = 0; x < 5; x++) {
            uint8_t intensity = flame_frames[frame][y][x];
            uint8_t r = (intensity == 1) ? 64 : (intensity >= 2) ? 255 : 0;
            uint8_t g = (intensity == 2) ? 64 : (intensity == 3) ? 128 : 0;
            uint8_t b = 0;
            uint32_t grb = ((uint32_t)g << 16) | ((uint32_t)r << 8) | b;
            pio_sm_put_blocking(pio, sm, grb << 8u);
        }
    }
}

// Função principal
int main() {
    stdio_init_all();
    sleep_ms(2000);

    adc_init();
    adc_gpio_init(JOYSTICK_X);
    adc_gpio_init(JOYSTICK_Y);

    gpio_init(JOYSTICK_BTN); gpio_set_dir(JOYSTICK_BTN, GPIO_IN); gpio_pull_up(JOYSTICK_BTN);
    gpio_init(BTN_A);        gpio_set_dir(BTN_A, GPIO_IN);        gpio_pull_up(BTN_A);
    gpio_init(BTN_B);        gpio_set_dir(BTN_B, GPIO_IN);        gpio_pull_up(BTN_B);

    pwm_init_gpio(LED_R, 50);
    gpio_init(LED_G); gpio_set_dir(LED_G, GPIO_OUT); gpio_put(LED_G, false);
    gpio_init(LED_B); gpio_set_dir(LED_B, GPIO_OUT); gpio_put(LED_B, false);

    pwm_init_gpio(BUZZER_PIN, 500);
    uint32_t wrap_value = clock_get_hz(clk_sys) / 500 - 1;
    uint16_t pulse_min = wrap_value * 0.05;
    uint16_t pulse_center = wrap_value * 0.075;
    uint16_t pulse_max = wrap_value * 0.1;
    stop_buzzer();

    ssd1306_init();
    PIO pio = pio0;
    uint sm = 0;
    uint offset = pio_add_program(pio, &U7T_projeto_program);
    U7T_projeto_program_init(pio, sm, offset, WS2812_PIN, 800000, false);

    calibrate_joystick();

    float temperature = 0.0f;
    uint32_t last_blink_time = 0;
    bool led_state = false;
    bool first_max_reached = false;

    while (true) {
        uint32_t current_time = time_us_32() / 1000000; // Tempo em segundos

        switch (current_state) {
            case MENU_INICIAL:
                show_menu(menu_selection);
                timer_active = false;
                timer_finished = false;
                flame_active = false;
                first_max_reached = false;
                gpio_put(LED_G, false);
                stop_buzzer();
                if (!gpio_get(BTN_A)) { // Botão A avança no menu
                    menu_selection = (menu_selection + 1) % NUM_STAGES;
                    sleep_ms(200);
                }
                if (!gpio_get(BTN_B)) { // Botão B seleciona o estágio
                    current_state = (system_state_t)(PARADA_PROTEICA + menu_selection);
                    temperature = STAGES[menu_selection].temp_min;
                    timer_active = false;
                    timer_finished = false;
                    flame_active = true;
                    first_max_reached = false;
                    sleep_ms(200);
                }
                break;

            case PARADA_PROTEICA:
            case BETA_AMILASE:
            case ALFA_AMILASE:
            case MASH_OUT: {
                const brassagem_stage_t* stage = &STAGES[current_state - PARADA_PROTEICA];
                uint32_t elapsed_time = timer_active ? (current_time - timer_start) : 0;

                // Controle da temperatura
                control_stage(&temperature, stage);

                // Inicia o temporizador na primeira vez que atinge o máximo
                if (!first_max_reached && temperature >= stage->temp_max) {
                    timer_start = current_time;
                    timer_active = true;
                    first_max_reached = true;
                }

                // Verifica se o temporizador terminou
                if (timer_active && elapsed_time >= stage->duration) {
                    timer_finished = true;
                }

                update_display(temperature, stage, flame_active, elapsed_time);

                // Piscar LED verde e acionar buzzer quando o temporizador terminar
                if (timer_finished) {
                    if ((current_time - last_blink_time) >= 1) { // Piscar a cada 1 segundo
                        led_state = !led_state;
                        gpio_put(LED_G, led_state);
                        if (led_state) {
                            set_buzzer_position(pulse_max); // Som mais alto
                            sleep_ms(250); // Duração maior do som
                            stop_buzzer();
                        }
                        last_blink_time = current_time;
                    }
                    if (!gpio_get(BTN_A)) { // Botão A avança para próxima fase
                        if (current_state == MASH_OUT) {
                            current_state = MENU_INICIAL;
                            menu_selection = 0;
                        } else {
                            current_state = (system_state_t)(current_state + 1);
                            temperature = STAGES[current_state - PARADA_PROTEICA].temp_min;
                            timer_active = false;
                            timer_finished = false;
                            flame_active = true;
                            first_max_reached = false;
                        }
                        gpio_put(LED_G, false);
                        stop_buzzer();
                        sleep_ms(200);
                    }
                }

                if (!gpio_get(BTN_B)) { // Botão B volta ao menu
                    current_state = MENU_INICIAL;
                    flame_active = false;
                    timer_active = false;
                    timer_finished = false;
                    first_max_reached = false;
                    pwm_set_gpio_level(LED_R, 0);
                    gpio_put(LED_G, false);
                    stop_buzzer();
                    sleep_ms(200);
                }
                break;
            }
        }

        if (flame_active) {
            update_flame_animation(pio, sm, flame_frame);
            flame_frame = (flame_frame + 1) % 4;
        } else {
            for (int i = 0; i < 25; i++) pio_sm_put_blocking(pio, sm, 0 << 8u);
        }

        sleep_ms(50);
    }

    return 0;
}