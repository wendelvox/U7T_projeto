#include <stdio.h>
#include <stdlib.h>
#include "pico/stdlib.h"
#include "hardware/adc.h"
#include "hardware/pwm.h"
#include "hardware/i2c.h"
#include "hardware/clocks.h" // Inclui a biblioteca para clocks do sistema
#include "lib/ssd1306.h"

#define I2C_PORT       i2c1
#define I2C_SDA        14
#define I2C_SCL        15
#define DISP_ADDR      0x3C
#define BUZZER_PIN     21

// -------------------------
// DEFINES PARA O JOYSTICK
// -------------------------
#define JOYSTICK_X     27  // ADC0
#define JOYSTICK_Y     26  // ADC1
#define JOYSTICK_BTN   22
#define BTN_A           5
#define BTN_B           6  // Botão B adicionado
#define LED_R          13  // LED Vermelho (PWM)
#define LED_G          11  // LED Verde (GPIO)
#define LED_B          12  // LED Azul (GPIO)
#define DEADZONE       200

// Estrutura do display
ssd1306_t disp;

// Calibração do joystick (usada só nos LEDs)
uint16_t x_center, y_center;

// ------------------------------------------------------
// Inicializa pino em modo PWM e configura wrap
// ------------------------------------------------------
void pwm_init_gpio(uint gpio) {
    gpio_set_function(gpio, GPIO_FUNC_PWM);
    uint slice = pwm_gpio_to_slice_num(gpio);

    // Configuração para 50 Hz (20 ms por ciclo)
    uint32_t clock_freq = clock_get_hz(clk_sys); // Frequência do sistema
    uint32_t wrap_value = clock_freq / 50 - 1;  // Wrap para 50 Hz
    pwm_set_wrap(slice, wrap_value);
    pwm_set_clkdiv(slice, clock_freq / (wrap_value + 1));
    pwm_set_enabled(slice, true);
}

// ------------------------------------------------------
// Subtrai o centro e aplica zona morta
// (usado só para controlar intensidade dos LEDs)
// ------------------------------------------------------
int16_t adjust_value(int16_t raw, int16_t center) {
    int16_t diff = raw - center;
    if (abs(diff) < DEADZONE) {
        return 0;
    }
    return diff;
}

// ------------------------------------------------------
// Calibra o joystick (média de N leituras)
// - somente para controlar LEDs
// ------------------------------------------------------
void calibrate_joystick() {
    const int samples = 100;
    uint32_t sum_x = 0, sum_y = 0;
    for (int i = 0; i < samples; i++) {
        adc_select_input(0); // canal 0 => pino 26 => X
        sum_x += adc_read();
        adc_select_input(1); // canal 1 => pino 27 => Y
        sum_y += adc_read();
        sleep_ms(5);
    }
    x_center = sum_x / samples;
    y_center = sum_y / samples;
    printf("Calibração: x_center=%d, y_center=%d\n", x_center, y_center);
}

// ------------------------------------------------------
// Função principal
// ------------------------------------------------------
int main() {
    stdio_init_all();

    // ---------- ADC -----------
    adc_init();
    adc_gpio_init(JOYSTICK_X);
    adc_gpio_init(JOYSTICK_Y);

    // ---------- Botões -----------
    gpio_init(JOYSTICK_BTN);
    gpio_set_dir(JOYSTICK_BTN, GPIO_IN);
    gpio_pull_up(JOYSTICK_BTN);

    gpio_init(BTN_A);
    gpio_set_dir(BTN_A, GPIO_IN);
    gpio_pull_up(BTN_A);

    gpio_init(BTN_B);
    gpio_set_dir(BTN_B, GPIO_IN);
    gpio_pull_up(BTN_B);

    // ---------- LEDs -----------
    pwm_init_gpio(LED_R);
    gpio_init(LED_G);
    gpio_set_dir(LED_G, GPIO_OUT);
    gpio_put(LED_G, false); // Desliga o LED verde inicialmente

    gpio_init(LED_B);
    gpio_set_dir(LED_B, GPIO_OUT);
    gpio_put(LED_B, false); // Desliga o LED azul inicialmente

    // ---------- Buzzer como Servo -----------
    gpio_set_function(BUZZER_PIN, GPIO_FUNC_PWM);
    uint buzzer_slice = pwm_gpio_to_slice_num(BUZZER_PIN);

    // Configuração para 50 Hz (20 ms por ciclo)
    uint32_t clock_freq = clock_get_hz(clk_sys); // Frequência do sistema
    uint32_t wrap_value = clock_freq / 50 - 1;  // Wrap para 50 Hz
    pwm_set_wrap(buzzer_slice, wrap_value);
    pwm_set_clkdiv(buzzer_slice, clock_freq / (wrap_value + 1));
    pwm_set_enabled(buzzer_slice, true);

    // Larguras de pulso para simular posições do servo
    uint16_t pulse_min = wrap_value * 0.05;   // 1 ms (0°)
    uint16_t pulse_center = wrap_value * 0.075; // 1.5 ms (90°)
    uint16_t pulse_max = wrap_value * 0.1;    // 2 ms (180°)

    // ---------- Display I2C -----------
    i2c_init(I2C_PORT, 400 * 1000);
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);
    ssd1306_init(&disp, 128, 64, false, DISP_ADDR, I2C_PORT);
    ssd1306_config(&disp);
    ssd1306_send_data(&disp);

    // Limpa o display
    ssd1306_fill(&disp, false);
    ssd1306_send_data(&disp);

    // ---------- Calibra Joystick (para LEDs) -----------
    calibrate_joystick();

    bool green_mode = false; // Estado do modo "LED verde"
    bool red_mode = false;   // Estado do modo "LED vermelho"
    float display_temperature = 0.0f; // Temperatura exibida no display
    bool timer_active = false; // Estado do cronômetro
    uint8_t timer_seconds = 0; // Segundos do cronômetro
    bool buzzer_triggered = false; // Indica se o buzzer foi acionado

    while (true) {
        // 1) Ler canal 0 (X) e canal 1 (Y)
        adc_select_input(0); // canal 0 => pino 26 => X
        uint16_t raw_x = adc_read();
        adc_select_input(1); // canal 1 => pino 27 => Y
        uint16_t raw_y = adc_read();

        // 2) Mapear o valor do joystick para uma temperatura simulada
        if (red_mode) {
            // Permite temperatura de até 98°C
            display_temperature += ((float)adjust_value(raw_y, y_center) / 4095.0f) * 2.0f; // Ajuste suave
            if (display_temperature > 98.0f) display_temperature = 98.0f;
            if (display_temperature < 0.0f) display_temperature = 0.0f;
        } else {
            // Limita a temperatura a 68°C
            float temp = ((float)raw_y / 4095.0f) * 80.0f + 20.0f; // Mapeia para 20°C - 100°C
            display_temperature = (temp > 68.0f) ? 68.0f : temp;
        }

        // 3) Verificar se o botão B foi pressionado para habilitar alta temperatura
        if (!gpio_get(BTN_B)) { // Botão B pressionado (pull-up)
            red_mode = true; // Habilita o modo de alta temperatura
            pwm_set_gpio_level(LED_R, 4095); // Liga o LED vermelho
        }

        // 4) Verificar se o botão A foi pressionado para ativar o cronômetro
        if (!gpio_get(BTN_A)) { // Botão A pressionado (pull-up)
            if (display_temperature == 68.0f) { // Só ativa se a temperatura for 68°C
                green_mode = true; // Ativa o modo "LED verde"
                timer_active = true; // Ativa o cronômetro
                timer_seconds = 0; // Reinicia o cronômetro
                buzzer_triggered = false; // Reseta o estado do buzzer
            }
        }

        // 5) Controlar os LEDs com base no estado atual
        if (green_mode) {
            if (timer_active && timer_seconds < 15) { // Cronômetro limitado a 15 segundos
                if (display_temperature >= 62.0f) {
                    gpio_put(LED_G, true);  // Liga o LED verde
                    pwm_set_gpio_level(LED_R, 0); // Apaga o LED vermelho
                } else {
                    gpio_put(LED_G, false); // Desliga o LED verde
                    pwm_set_gpio_level(LED_R, 4095); // Liga o LED vermelho
                    green_mode = false; // Sai do modo "LED verde"
                    timer_active = false; // Desativa o cronômetro
                }
            } else {
                // Cronômetro atingiu 15 segundos
                green_mode = false;
                timer_active = false;

                // Pisca o LED azul 3 vezes
                for (int i = 0; i < 3; i++) {
                    gpio_put(LED_B, true);  // Liga o LED azul
                    sleep_ms(500); // Mantém ligado por 500 ms
                    gpio_put(LED_B, false); // Desliga o LED azul
                    sleep_ms(500); // Mantém apagado por 500 ms
                }

                // Simula movimento do servo com o buzzer
                if (!buzzer_triggered) {
                    uint32_t start_time = to_ms_since_boot(get_absolute_time());
                    while (to_ms_since_boot(get_absolute_time()) - start_time < 5000) {
                        // Move para posição mínima (0°)
                        pwm_set_gpio_level(BUZZER_PIN, pulse_min);
                        sleep_ms(500);

                        // Move para posição central (90°)
                        pwm_set_gpio_level(BUZZER_PIN, pulse_center);
                        sleep_ms(500);

                        // Move para posição máxima (180°)
                        pwm_set_gpio_level(BUZZER_PIN, pulse_max);
                        sleep_ms(500);
                    }
                    pwm_set_gpio_level(BUZZER_PIN, 0); // Garante que o buzzer seja desligado
                    buzzer_triggered = true; // Marca o buzzer como acionado
                }

                // Volta ao estado inicial
                pwm_set_gpio_level(LED_R, 0); // Apaga o LED vermelho
                gpio_put(LED_G, false); // Apaga o LED verde
                gpio_put(LED_B, false); // Apaga o LED azul
            }
        }

        // 6) Modo de Alta Temperatura
        if (red_mode) {
            if (display_temperature >= 98.0f) {
                timer_active = true; // Ativa o cronômetro de alta temperatura
                timer_seconds = 0; // Reinicia o cronômetro
            }

            if (timer_active && timer_seconds < 15) {
                pwm_set_gpio_level(LED_R, 4095); // Mantém o LED vermelho ligado
            } else {
                red_mode = false;
                timer_active = false;
                pwm_set_gpio_level(LED_R, 0); // Apaga o LED vermelho
            }
        }

        // 7) Exibir a temperatura no display
        char temp_str[32]; // Buffer maior para evitar truncamento
        snprintf(temp_str, sizeof(temp_str), "Temp: %.1f C", display_temperature);

        // 8) Desenhar no display
        ssd1306_fill(&disp, false); // Limpa o display

        // Exibe a temperatura
        ssd1306_draw_string(&disp, temp_str, 0, 0); // Posição (x=0, y=0)

        // Exibe "Modo Verde" se o modo "LED verde" estiver ativo
        if (green_mode) {
            ssd1306_draw_string(&disp, "Modo Verde", 0, 10); // Posição (x=0, y=10)
        }

        // Exibe o cronômetro no centro do display
        if (timer_active) {
            char timer_str[16];
            snprintf(timer_str, sizeof(timer_str), "Timer: %02d s", timer_seconds);

            // Calcula o comprimento da string manualmente
            uint8_t length = 0;
            while (timer_str[length] != '\0') {
                length++;
            }

            // Usa o comprimento calculado para centralizar o texto
            uint8_t x = (disp.width / 2) - (length * 4); // Centraliza horizontalmente
            uint8_t y = (disp.height / 2) - 4; // Centraliza verticalmente
            ssd1306_draw_string(&disp, timer_str, x, y);
        }

        // Atualiza o display
        ssd1306_send_data(&disp);

        // Incrementa o cronômetro
        if (timer_active && timer_seconds < 15) {
            sleep_ms(1000); // Aguarda 1 segundo
            timer_seconds++;
        } else {
            sleep_ms(50); // Aguarda menos tempo se o cronômetro não estiver ativo
        }
    }

    return 0;
}