#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "hardware/gpio.h"
#include "include/ssd1306.h"

// Definição do pino do botão A (GPIO10)
#define BUTTON_A_PIN 5

const uint I2C_SDA = 14;
const uint I2C_SCL = 15;

// Configurações ajustadas
#define BALL_SIZE 2
#define GRAVITY 0.07f
#define NUM_BINS 16
#define BIN_WIDTH (ssd1306_width / NUM_BINS)
#define NUM_PIN_ROWS 8
#define PIN_SPACING ((ssd1306_height - INITIAL_Y_POS - 6) / NUM_PIN_ROWS - 2)
#define MAX_BALLS 30
#define BALL_SPAWN_RATE 1
#define INITIAL_Y_POS 5
#define HISTOGRAM_HEIGHT 20  // Altura máxima do histograma

typedef struct
{
    float x, y;
    float vx, vy;
    bool active;
} ball_t;

ball_t balls[MAX_BALLS];
int total_balls = 0;
int frame_count = 0;
int bins[NUM_BINS] = {0};  // Array para contar as bolas em cada canaleta
bool show_histogram = false;  // Flag para controlar a exibição do histograma

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
}

void update_ball(ball_t *ball)
{
    if (!ball->active)
        return;

    ball->vy += GRAVITY;
    ball->x += ball->vx;
    ball->y += ball->vy;

    // Colisão com pinos
    for (int row = 1; row <= NUM_PIN_ROWS; row++)
    {
        int pin_y = INITIAL_Y_POS + 8 + (row * PIN_SPACING);
        if (ball->y >= pin_y - 2 && ball->y <= pin_y + 2 && ball->vy > 0)
        {
            ball->vx = random_direction() ? 3.0f : -3.0f;
            break;
        }
    }

    if (ball->x < 0)
        ball->x = 0;
    if (ball->x > ssd1306_width - BALL_SIZE)
        ball->x = ssd1306_width - BALL_SIZE;

    // Chegada ao fundo - atualiza o histograma
    if (ball->y >= ssd1306_height - BALL_SIZE)
    {
        // Determina em qual bin a bola caiu
        int bin = (int)(ball->x / BIN_WIDTH);
        if (bin >= 0 && bin < NUM_BINS)
        {
            bins[bin]++;
        }
        init_ball(ball); // Reinicia a bola no topo
    }
}

void spawn_balls()
{
    if (frame_count % BALL_SPAWN_RATE == 0)
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
    frame_count++;
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
    // Encontra o valor máximo para normalização
    int max_count = 1; // Evita divisão por zero
    for (int i = 0; i < NUM_BINS; i++)
    {
        if (bins[i] > max_count)
            max_count = bins[i];
    }

    // Desenha as barras do histograma
    for (int i = 0; i < NUM_BINS; i++)
    {
        int bar_height = (bins[i] * HISTOGRAM_HEIGHT) / max_count;
        if (bar_height > 0)
        {
            int x_start = i * BIN_WIDTH;
            int x_end = x_start + BIN_WIDTH - 1;
            int y_start = ssd1306_height - 1;
            int y_end = y_start - bar_height;

            // Desenha a barra vertical
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

int main()
{
    stdio_init_all();

    // Configuração do botão A (GPIO5 com pull-up)
    gpio_init(BUTTON_A_PIN);
    gpio_set_dir(BUTTON_A_PIN, GPIO_IN);
    gpio_pull_up(BUTTON_A_PIN);

    // Configuração do display
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

    // Inicializa as bolas e o histograma
    for (int i = 0; i < MAX_BALLS; i++)
    {
        balls[i].active = false;
    }
    memset(bins, 0, sizeof(bins));

    while (true)
    {
        // Verifica se o botão A foi pressionado (LOW quando pressionado)
        if (!gpio_get(BUTTON_A_PIN))
        {
            show_histogram = !show_histogram; // Alterna a exibição do histograma
            sleep_ms(200); // Debounce
        }

        memset(buffer, 0, sizeof(buffer));

        spawn_balls();

        for (int i = 0; i < MAX_BALLS; i++)
        {
            update_ball(&balls[i]);
            draw_ball(buffer, &balls[i]);
        }

        draw_pins(buffer);

        // Mostra o histograma se o flag estiver ativo
        if (show_histogram)
        {
            draw_histogram(buffer);
        }

        char text[20];
        sprintf(text, "Total: %d", total_balls);
        ssd1306_draw_string(buffer, 5, 5, text);

        render_on_display(buffer, &area);
        sleep_ms(16);
    }

    return 0;
}