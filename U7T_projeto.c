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
#define DEBOUNCE_TIME  50000 // 50 ms em microssegundos
#define SERVO_PIN 15  // Pino para o servo motor

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
    uint32_t duration;
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
static uint32_t timer_start = 0; // Para o temporizador de estágio
static uint32_t total_time_start = 0; // Para o temporizador total
static bool timer_active = false;
static bool timer_finished = false;
static float temperature = 0.0f;
static bool display_needs_update = false;

// Variáveis para debounce
static uint64_t last_debounce_time_a = 0;
static uint64_t last_debounce_time_b = 0;
static uint64_t last_debounce_time_joystick = 0; // Para o botão do joystick
static bool last_state_a = true;
static bool last_state_b = true;
static bool last_state_joystick = true; // Pull-up, HIGH (1) é o estado inicial
static float last_temperature = 0.0f; // Temperatura do ciclo anterior

static const uint8_t flame_frames[4][5][5] = {
    {{1, 2, 3, 2, 1}, {0, 1, 2, 1, 0}, {0, 0, 1, 0, 0}, {0, 0, 0, 0, 0}, {0, 0, 0, 0, 0}},
    {{1, 2, 3, 2, 1}, {1, 2, 3, 2, 1}, {0, 1, 2, 1, 0}, {0, 0, 1, 0, 0}, {0, 0, 0, 0, 0}},
    {{1, 2, 3, 2, 1}, {1, 2, 3, 2, 1}, {1, 2, 3, 2, 1}, {0, 1, 2, 1, 0}, {0, 0, 1, 0, 0}},
    {{1, 2, 3, 2, 1}, {1, 2, 3, 2, 1}, {1, 2, 3, 2, 1}, {1, 2, 3, 2, 0}, {0, 1, 0, 1, 0}}
};

// Função de debounce
bool debounce_button(uint gpio, uint64_t now, bool* last_state, uint64_t* last_debounce_time) {
    bool current_state = gpio_get(gpio);
    if (current_state != *last_state) {
        *last_debounce_time = now;
    }
    if ((now - *last_debounce_time) > DEBOUNCE_TIME) {
        if (!current_state) { // Botão pressionado (LOW)
            *last_state = current_state;
            return true;
        }
    }
    *last_state = current_state;
    return false;
}

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
    ssd1306_draw_string(16, 48, "A:Prox  B:Sel");
    ssd1306_update();
}

void update_display(float temperature, const brassagem_stage_t* stage, bool flame_active, uint32_t stage_time, uint32_t total_time) {
    ssd1306_clear();
    draw_double_border();
    char temp_str[16];
    snprintf(temp_str, sizeof(temp_str), "T: %.1f°C", temperature);
    ssd1306_draw_string(5, 6, stage->nome);
    ssd1306_draw_string(22, 20, temp_str);
    char stage_timer_str[16];
    snprintf(stage_timer_str, sizeof(stage_timer_str), "Ti: %lus", stage_time);
    ssd1306_draw_string(5, 38, stage_timer_str);
    char total_timer_str[16];
    snprintf(total_timer_str, sizeof(total_timer_str), "Tt: %lus", total_time);
    ssd1306_draw_string(64, 38, total_timer_str);
    char flame_str[16];
    snprintf(flame_str, sizeof(flame_str), "Chama: %s", flame_active ? "ON" : "OFF");
    ssd1306_draw_string(5, 48, flame_str);
    ssd1306_update();
}

// Funções do servo motor
void servo_init(uint gpio) {
    gpio_set_function(gpio, GPIO_FUNC_PWM);
    uint slice = pwm_gpio_to_slice_num(gpio);
    uint32_t clock_freq = clock_get_hz(clk_sys);
    uint32_t wrap_value = clock_freq / 50 - 1;
    pwm_set_wrap(slice, wrap_value);
    pwm_set_clkdiv(slice, 1.0f);
    pwm_set_enabled(slice, true);
}

void set_servo_angle(uint gpio, uint8_t angle) {
    if (angle > 180) angle = 180;
    uint slice = pwm_gpio_to_slice_num(gpio);
    uint32_t wrap_value = clock_get_hz(clk_sys) / 50 - 1;
    uint16_t level = (wrap_value * (1000 + (angle * 1000 / 180))) / 20000;
    pwm_set_gpio_level(gpio, level);
}

// Controle de temperatura
void control_stage(float* temperature, const brassagem_stage_t* stage, uint8_t* servo_angle) {
    adc_select_input(1); // JOYSTICK_Y
    uint16_t raw_y = adc_read();
    int16_t y_adjust = adjust_value(raw_y, y_center);

    // Ajusta a temperatura com base no movimento do joystick
    *temperature += ((float)y_adjust / 4095.0f) * 0.5f;

    if (*temperature < 0.0f) *temperature = 0.0f;
    if (*temperature > stage->temp_max) *temperature = stage->temp_max;

    // Verifica se a temperatura caiu em relação ao ciclo anterior
    if (*temperature < last_temperature) {
        flame_active = true;
    } else if (*temperature >= stage->temp_max) {
        flame_active = false;
    }

    // Controle proporcional da chama
    float temp_range = stage->temp_max - stage->temp_min;
    float temp_progress = (*temperature - stage->temp_min) / temp_range;
    if (temp_progress < 0) temp_progress = 0;
    if (temp_progress > 1) temp_progress = 1;

    if (flame_active) {
        *servo_angle = (uint8_t)(90 * (1 - temp_progress));
        set_servo_angle(SERVO_PIN, *servo_angle);
        uint16_t led_intensity = (uint16_t)(4095 * (1 - temp_progress));
        pwm_set_gpio_level(LED_R, led_intensity);
        *temperature += 0.1f; // Aumenta a temperatura enquanto a chama está ativa
    } else {
        *servo_angle = 0;
        set_servo_angle(SERVO_PIN, *servo_angle);
        pwm_set_gpio_level(LED_R, 0);
    }

    // Atualiza a temperatura anterior para o próximo ciclo
    last_temperature = *temperature;
}

// Animação da chama
void update_flame_animation(PIO pio, uint sm, uint8_t frame, uint8_t servo_angle) {
    static const uint8_t flame_frames[4][5][5] = {
        {{1, 2, 3, 2, 1}, {0, 1, 2, 1, 0}, {0, 0, 1, 0, 0}, {0, 0, 0, 0, 0}, {0, 0, 0, 0, 0}},
        {{1, 2, 3, 2, 1}, {1, 2, 3, 2, 1}, {0, 1, 2, 1, 0}, {0, 0, 1, 0, 0}, {0, 0, 0, 0, 0}},
        {{1, 2, 3, 2, 1}, {1, 2, 3, 2, 1}, {1, 2, 3, 2, 1}, {0, 1, 2, 1, 0}, {0, 0, 1, 0, 0}},
        {{1, 2, 3, 2, 1}, {1, 2, 3, 2, 1}, {1, 2, 3, 2, 1}, {1, 2, 3, 2, 0}, {0, 1, 0, 1, 0}}
    };

    float brightness = (float)servo_angle / 90.0f;
    if (brightness < 0.0f) brightness = 0.0f;
    if (brightness > 1.0f) brightness = 1.0f;

    for (int y = 0; y < 5; y++) {
        for (int x = 0; x < 5; x++) {
            uint8_t intensity = flame_frames[frame][y][x];
            uint8_t r = (intensity == 1) ? 64 : (intensity >= 2) ? 255 : 0;
            uint8_t g = (intensity == 2) ? 64 : (intensity == 3) ? 128 : 0;
            uint8_t b = 0;

            r = (uint8_t)(r * brightness);
            g = (uint8_t)(g * brightness);
            b = (uint8_t)(b * brightness);

            uint32_t grb = ((uint32_t)g << 16) | ((uint32_t)r << 8) | b;
            pio_sm_put_blocking(pio, sm, grb << 8u);
        }
    }
}
void show_tela_inicial() {
    ssd1306_clear();
    draw_double_border(); // Reutiliza a borda dupla que você já tem

    // "EMBARCATECH" tem 10 caracteres. Cada caractere é 5 pixels de largura + 1 de espaço = 6 pixels por caractere
    // Total: 10 * 6 = 60 pixels de largura
    // Centralizar horizontalmente: (128 - 60) / 2 = 34
    // Centralizar verticalmente: (64 - 8) / 2 = 28 (assumindo altura de fonte de 8 pixels)
    ssd1306_draw_string(20, 28, "EMBARCATECH");

    ssd1306_update();
    sleep_ms(3000); // Mostra a tela por 3 segundos
}
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
    uint16_t pulse_max = wrap_value * 0.1;
    stop_buzzer();

    servo_init(SERVO_PIN);
    set_servo_angle(SERVO_PIN, 0);

    ssd1306_init();
    show_tela_inicial();

    PIO pio = pio0;
    uint sm = 0;
    uint offset = pio_add_program(pio, &U7T_projeto_program);
    U7T_projeto_program_init(pio, sm, offset, WS2812_PIN, 800000, false);

    calibrate_joystick();

    uint32_t last_blink_time = 0;
    bool led_state = false;
    bool first_max_reached = false;
    uint8_t servo_angle = 0;

    while (true) {
        uint32_t current_time = time_us_32() / 1000000;
        uint64_t now = time_us_64();

        uint32_t stage_time = timer_active ? (current_time - timer_start) : 0;
        uint32_t total_time = (current_state != MENU_INICIAL && total_time_start != 0) ? (current_time - total_time_start) : 0;

        bool btn_a_pressed = debounce_button(BTN_A, now, &last_state_a, &last_debounce_time_a);
        bool btn_b_pressed = debounce_button(BTN_B, now, &last_state_b, &last_debounce_time_b);
        bool btn_joystick_pressed = debounce_button(JOYSTICK_BTN, now, &last_state_joystick, &last_debounce_time_joystick);

        if (btn_joystick_pressed) {
            current_state = MENU_INICIAL;
            menu_selection = 0;
            flame_active = false;
            timer_active = false;
            timer_finished = false;
            first_max_reached = false;
            temperature = 0.0f;
            last_temperature = 0.0f; // Reseta a temperatura anterior
            timer_start = 0;
            total_time_start = 0;
            pwm_set_gpio_level(LED_R, 0);
            gpio_put(LED_G, false);
            gpio_put(LED_B, false);
            set_servo_angle(SERVO_PIN, 0);
            servo_angle = 0;
            stop_buzzer();
        }

        if (btn_a_pressed) {
            if (current_state == MENU_INICIAL) {
                menu_selection = (menu_selection + 1) % NUM_STAGES;
            } else if (timer_finished) {
                if (current_state == MASH_OUT) {
                    current_state = MENU_INICIAL;
                    menu_selection = 0;
                    total_time_start = 0;
                } else {
                    current_state = (system_state_t)(current_state + 1);
                    temperature = STAGES[current_state - PARADA_PROTEICA].temp_min;
                    last_temperature = temperature; // Inicializa a temperatura anterior
                    timer_active = false;
                    timer_finished = false;
                    flame_active = true;
                    first_max_reached = false;
                }
                gpio_put(LED_G, false);
                gpio_put(LED_B, false);
                set_servo_angle(SERVO_PIN, 90);
                servo_angle = 90;
                stop_buzzer();
            }
        }

        if (btn_b_pressed) {
            if (current_state == MENU_INICIAL) {
                current_state = (system_state_t)(PARADA_PROTEICA + menu_selection);
                temperature = STAGES[menu_selection].temp_min;
                last_temperature = temperature; // Inicializa a temperatura anterior
                timer_active = false;
                timer_finished = false;
                flame_active = true;
                first_max_reached = false;
                set_servo_angle(SERVO_PIN, 90);
                servo_angle = 90;
                if (total_time_start == 0) total_time_start = current_time;
            } else {
                current_state = MENU_INICIAL;
                flame_active = false;
                timer_active = false;
                timer_finished = false;
                pwm_set_gpio_level(LED_R, 0);
                gpio_put(LED_G, false);
                gpio_put(LED_B, false);
                set_servo_angle(SERVO_PIN, 0);
                servo_angle = 0;
                stop_buzzer();
            }
        }

        switch (current_state) {
            case MENU_INICIAL:
                show_menu(menu_selection);
                timer_active = false;
                timer_finished = false;
                flame_active = false;
                first_max_reached = false;
                gpio_put(LED_G, false);
                gpio_put(LED_B, false);
                set_servo_angle(SERVO_PIN, 0);
                servo_angle = 0;
                stop_buzzer();
                break;

            case PARADA_PROTEICA:
            case BETA_AMILASE:
            case ALFA_AMILASE:
            case MASH_OUT: {
                const brassagem_stage_t* stage = &STAGES[current_state - PARADA_PROTEICA];

                control_stage(&temperature, stage, &servo_angle);

                if (!first_max_reached && temperature >= stage->temp_max) {
                    timer_start = current_time;
                    timer_active = true;
                    first_max_reached = true;
                    gpio_put(LED_B, false);
                }

                if (timer_active && stage_time >= stage->duration) {
                    timer_finished = true;
                    gpio_put(LED_B, false);
                }

                update_display(temperature, stage, flame_active, stage_time, total_time);

                if (timer_finished) {
                    if ((current_time - last_blink_time) >= 1) {
                        led_state = !led_state;
                        gpio_put(LED_G, led_state);
                        if (led_state) {
                            set_buzzer_position(pulse_max);
                            sleep_ms(250);
                            stop_buzzer();
                        }
                        last_blink_time = current_time;
                    }
                }
                break;
            }
        }

        if (flame_active) {
            update_flame_animation(pio, sm, flame_frame, servo_angle);
            flame_frame = (flame_frame + 1) % 4;
        } else {
            for (int i = 0; i < 25; i++) pio_sm_put_blocking(pio, sm, 0 << 8u);
        }

        sleep_ms(50);
    }

    return 0;
}