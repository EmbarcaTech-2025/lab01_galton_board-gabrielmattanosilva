#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "include/ssd1306.h"

const uint I2C_SDA = 14;
const uint I2C_SCL = 15;

// Configurações aprimoradas (com espaçamento reduzido entre linhas)
#define BALL_SIZE 2
#define GRAVITY 0.07f
#define NUM_BINS 16
#define BIN_WIDTH (ssd1306_width / NUM_BINS)
#define NUM_PIN_ROWS 6
#define PIN_SPACING ((ssd1306_height - INITIAL_Y_POS - 6) / NUM_PIN_ROWS - 2) // Reduz 2px do espaçamento
#define MAX_BALLS 10
#define BALL_SPAWN_DELAY 20
#define INITIAL_Y_POS 5

typedef struct
{
    float x, y;
    float vx, vy;
    bool active;
    int spawn_timer;
} ball_t;

ball_t balls[MAX_BALLS];
int ball_count = 0;
int total_balls = 0;
absolute_time_t last_spawn_time;

bool random_direction()
{
    return rand() % 2;
}

void init_ball(ball_t *ball)
{
    ball->x = ssd1306_width / 2 + (rand() % 5) - 2;
    ball->y = INITIAL_Y_POS;
    ball->vx = 0;
    ball->vy = 0.1f;
    ball->active = true;
    ball->spawn_timer = 0;
}

void update_ball(ball_t *ball)
{
    if (!ball->active)
        return;

    ball->vy += GRAVITY;
    ball->x += ball->vx;
    ball->y += ball->vy;

    // Colisão com pinos (com espaçamento reduzido)
    for (int row = 1; row <= NUM_PIN_ROWS; row++)
    {
        int pin_y = INITIAL_Y_POS + 8 + (row * (PIN_SPACING)); // Início ajustado + espaçamento reduzido
        if (ball->y >= pin_y - 2 && ball->y <= pin_y + 2 && ball->vy > 0)
        {
            ball->vx = random_direction() ? 0.4f : -0.4f;
            break;
        }
    }

    if (ball->x < 0)
        ball->x = 0;
    if (ball->x > ssd1306_width - BALL_SIZE)
        ball->x = ssd1306_width - BALL_SIZE;

    if (ball->y >= ssd1306_height - BALL_SIZE)
    {
        ball->active = false;
        ball_count--;
    }
}

void draw_ball(uint8_t *buffer, ball_t *ball)
{
    if (!ball->active)
        return;

    for (int i = 0; i < BALL_SIZE; i++)
    {
        for (int j = 0; j < BALL_SIZE; j++)
        {
            int px = (int)ball->x + i;
            int py = (int)ball->y + j;
            if (px >= 0 && px < ssd1306_width && py >= 0 && py < ssd1306_height)
            {
                ssd1306_set_pixel(buffer, px, py, true);
            }
        }
    }
}

void draw_pins(uint8_t *buffer)
{
    for (int row = 1; row <= NUM_PIN_ROWS; row++)
    {
        int y = INITIAL_Y_POS + 8 + (row * (PIN_SPACING)); // Linhas mais compactadas
        int start_x = (row % 2) ? BIN_WIDTH / 2 : 0;
        for (int x = start_x; x < ssd1306_width; x += BIN_WIDTH)
        {
            ssd1306_set_pixel(buffer, x, y, true);
            ssd1306_set_pixel(buffer, x, y + 1, true);
            ssd1306_set_pixel(buffer, x + 1, y, true);
            ssd1306_set_pixel(buffer, x + 1, y + 1, true);
        }
    }
}

void spawn_new_ball()
{
    if (ball_count >= MAX_BALLS)
        return;

    absolute_time_t now = get_absolute_time();
    int64_t diff = absolute_time_diff_us(last_spawn_time, now) / 1000;

    if (diff > BALL_SPAWN_DELAY)
    {
        for (int i = 0; i < MAX_BALLS; i++)
        {
            if (!balls[i].active)
            {
                init_ball(&balls[i]);
                ball_count++;
                total_balls++;
                last_spawn_time = now;
                break;
            }
        }
    }
}

int main()
{
    stdio_init_all();

    i2c_init(i2c1, 400 * 1000);
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);

    ssd1306_init();

    struct render_area area = {
        .start_column = 0,
        .end_column = ssd1306_width - 1,
        .start_page = 0,
        .end_page = ssd1306_n_pages - 1};
    calculate_render_area_buffer_length(&area);

    uint8_t buffer[ssd1306_buffer_length];

    for (int i = 0; i < MAX_BALLS; i++)
    {
        balls[i].active = false;
    }
    last_spawn_time = get_absolute_time();

    while (true)
    {
        memset(buffer, 0, sizeof(buffer));

        spawn_new_ball();

        for (int i = 0; i < MAX_BALLS; i++)
        {
            update_ball(&balls[i]);
            draw_ball(buffer, &balls[i]);
        }

        draw_pins(buffer);

        char text[20];
        sprintf(text, "Total: %d", total_balls);
        ssd1306_draw_string(buffer, 5, 5, text);

        render_on_display(buffer, &area);

        sleep_ms(16);
    }

    return 0;
}