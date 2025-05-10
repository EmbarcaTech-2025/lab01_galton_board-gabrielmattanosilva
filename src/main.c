/**
 * @file main.c
 * @brief Ponto de entrada para a simulação da Galton Board.
 */

#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/i2c.h"

#include "display.h"
#include "galton_board.h"

#define BUTTON_A_PIN 5
#define I2C_SDA 14
#define I2C_SCL 15
#define TICK_RATE_MS 16

/**
 * @brief Controlador de ticks temporizados.
 */
typedef struct {
    absolute_time_t last_tick_time;
    uint32_t tick_count;
} tick_controller_t;

static tick_controller_t tick_ctrl;

/**
 * @brief Inicializa o controlador de ticks.
 * @param tc Ponteiro para a estrutura tick_controller_t.
 */
static void init_tick_controller(tick_controller_t *tc) {
    tc->last_tick_time = get_absolute_time();
    tc->tick_count = 0;
}

/**
 * @brief Verifica se um tick deve ser processado.
 * @param tc Ponteiro para a estrutura tick_controller_t.
 * @return true se o tick deve ser processado, false caso contrário.
 */
static bool should_process_tick(tick_controller_t *tc) {
    absolute_time_t now = get_absolute_time();
    int64_t diff = absolute_time_diff_us(tc->last_tick_time, now);

    if (diff >= (TICK_RATE_MS * 1000)) {
        tc->last_tick_time = now;
        tc->tick_count++;
        return true;
    }
    return false;
}

/**
 * @brief Função principal da aplicação.
 * @return int Código de retorno (sempre 0).
 */
int main() {
    stdio_init_all();

    gpio_init(BUTTON_A_PIN);
    gpio_set_dir(BUTTON_A_PIN, GPIO_IN);
    gpio_pull_up(BUTTON_A_PIN);

    i2c_init(i2c1, 400 * 1000);
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);

    display_init();
    galton_board_init();
    init_tick_controller(&tick_ctrl);

    uint8_t buffer[ssd1306_buffer_length];

    while (true) {
        if (should_process_tick(&tick_ctrl)) {
            current_tick++;

            if (!gpio_get(BUTTON_A_PIN)) {
                show_histogram = !show_histogram;
                sleep_ms(200);
            }

            memset(buffer, 0, sizeof(buffer));

            galton_board_update(buffer);
            galton_board_draw_pins(buffer);

            if (show_histogram) {
                galton_board_draw_histogram(buffer);
            }

            char text[20];
            sprintf(text, "T:%d B:%d", current_tick, total_balls);
            display_draw_text(buffer, 5, 5, text);

            display_render(buffer);
        }
        sleep_ms(1);
    }

    return 0;
}