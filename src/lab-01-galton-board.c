#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "hardware/gpio.h"
#include "hardware/timer.h"
#include "include/ssd1306.h"

#define BUTTON_A_PIN 5

const uint I2C_SDA = 14;
const uint I2C_SCL = 15;

#define BALL_SIZE 2
#define GRAVITY 0.07f
#define NUM_BINS 16
#define BIN_WIDTH (ssd1306_width / NUM_BINS)
#define NUM_PIN_ROWS 8
#define PIN_SPACING ((ssd1306_height - INITIAL_Y_POS - 6) / NUM_PIN_ROWS - 2)
#define MAX_BALLS 30
#define BALL_SPAWN_RATE 5
#define INITIAL_Y_POS 5
#define HISTOGRAM_HEIGHT 20
#define TICK_RATE_MS 16

typedef struct
{
    float x, y;
    float vx, vy;
    bool active;
    int spawn_tick;
} ball_t;

ball_t balls[MAX_BALLS];
int total_balls = 0;
int current_tick = 0;
int bins[NUM_BINS] = {0};
bool show_histogram = false;

typedef struct
{
    absolute_time_t last_tick_time;
    uint32_t tick_count;
} tick_controller_t;

tick_controller_t tick_ctrl;

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
    ball->spawn_tick = current_tick;
}

void update_ball(ball_t *ball)
{
    if (!ball->active)
        return;

    ball->vy += GRAVITY;
    ball->x += ball->vx;
    ball->y += ball->vy;

    for (int row = 1; row <= NUM_PIN_ROWS; row++)
    {
        int pin_y = INITIAL_Y_POS + 8 + (row * PIN_SPACING);
        if (ball->y >= pin_y - 2 && ball->y <= pin_y + 2 && ball->vy > 0)
        {
            ball->vx = random_direction() ? 2.5f : -2.5f;
            break;
        }
    }

    if (ball->x < 0)
        ball->x = 0;
    if (ball->x > ssd1306_width - BALL_SIZE)
        ball->x = ssd1306_width - BALL_SIZE;

    if (ball->y >= ssd1306_height - BALL_SIZE)
    {
        int bin = (int)(ball->x / BIN_WIDTH);
        if (bin >= 0 && bin < NUM_BINS)
        {
            bins[bin]++;
        }
        init_ball(ball);
    }
}

void spawn_balls()
{
    if (current_tick % BALL_SPAWN_RATE == 0)
    {
        total_balls++;
        for (int i = 0; i < MAX_BALLS; i++)
        {
            if (!balls[i].active)
            {
                init_ball(&balls[i]);
                break;
            }
        }
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
        int y = INITIAL_Y_POS + 8 + (row * PIN_SPACING);
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

void draw_histogram(uint8_t *buffer)
{
    int max_count = 1;
    for (int i = 0; i < NUM_BINS; i++)
    {
        if (bins[i] > max_count)
            max_count = bins[i];
    }

    for (int i = 0; i < NUM_BINS; i++)
    {
        int bar_height = (bins[i] * HISTOGRAM_HEIGHT) / max_count;
        if (bar_height > 0)
        {
            int x_start = i * BIN_WIDTH;
            int x_end = x_start + BIN_WIDTH - 1;
            int y_start = ssd1306_height - 1;
            int y_end = y_start - bar_height;

            for (int y = y_start; y >= y_end; y--)
            {
                for (int x = x_start; x <= x_end; x++)
                {
                    ssd1306_set_pixel(buffer, x, y, true);
                }
            }
        }
    }
}

void init_tick_controller(tick_controller_t *tc)
{
    tc->last_tick_time = get_absolute_time();
    tc->tick_count = 0;
}

bool should_process_tick(tick_controller_t *tc)
{
    absolute_time_t now = get_absolute_time();
    int64_t diff = absolute_time_diff_us(tc->last_tick_time, now);

    if (diff >= (TICK_RATE_MS * 1000))
    {
        tc->last_tick_time = now;
        tc->tick_count++;
        return true;
    }
    return false;
}

int main()
{
    stdio_init_all();

    gpio_init(BUTTON_A_PIN);
    gpio_set_dir(BUTTON_A_PIN, GPIO_IN);
    gpio_pull_up(BUTTON_A_PIN);

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
    memset(bins, 0, sizeof(bins));
    init_tick_controller(&tick_ctrl);

    while (true)
    {
        if (should_process_tick(&tick_ctrl))
        {
            current_tick++;

            if (!gpio_get(BUTTON_A_PIN))
            {
                show_histogram = !show_histogram;
                sleep_ms(200);
            }

            memset(buffer, 0, sizeof(buffer));

            spawn_balls();
            for (int i = 0; i < MAX_BALLS; i++)
            {
                update_ball(&balls[i]);
                draw_ball(buffer, &balls[i]);
            }

            draw_pins(buffer);

            if (show_histogram)
            {
                draw_histogram(buffer);
            }

            char text[20];
            sprintf(text, "T:%d B:%d", current_tick, total_balls);
            ssd1306_draw_string(buffer, 5, 5, text);

            render_on_display(buffer, &area);
        }

        sleep_ms(1);
    }

    return 0;
}