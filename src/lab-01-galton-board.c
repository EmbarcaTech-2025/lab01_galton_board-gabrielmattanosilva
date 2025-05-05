#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "include/ssd1306.h"

const uint I2C_SDA = 14;
const uint I2C_SCL = 15;

// Configurações aprimoradas
#define BALL_SIZE 2
#define GRAVITY 0.07f // Gravidade mais suave
#define NUM_BINS 8
#define BIN_WIDTH (ssd1306_width / NUM_BINS)
#define NUM_PIN_ROWS 6                                    // 6 linhas de obstáculos
#define PIN_SPACING (ssd1306_height / (NUM_PIN_ROWS + 2)) // Ajuste para espaço antes da 1ª linha
#define MAX_BALLS 10                                      // Número máximo de bolas simultâneas
#define BALL_SPAWN_DELAY 20                               // Delay entre spawn de novas bolas
#define INITIAL_Y_POS 5                                   // Posição Y inicial (acima da primeira linha de obstáculos)

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

// Função de decisão aleatória
bool random_direction()
{
    return rand() % 2;
}

void init_ball(ball_t *ball)
{
    ball->x = ssd1306_width / 2 + (rand() % 5) - 2; // Pequena variação inicial
    ball->y = INITIAL_Y_POS;                        // Inicia acima da primeira linha de obstáculos
    ball->vx = 0;
    ball->vy = 0.1f; // Pequena velocidade vertical inicial
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

    // Colisão com pinos (6 linhas)
    for (int row = 1; row <= NUM_PIN_ROWS; row++)
    {
        int pin_y = row * PIN_SPACING + INITIAL_Y_POS; // Ajuste para posicionar corretamente
        if (ball->y >= pin_y - 2 && ball->y <= pin_y + 2 && ball->vy > 0)
        {
            ball->vx = random_direction() ? 0.4f : -0.4f; // Velocidade ajustada
            break;
        }
    }

    // Limites horizontais
    if (ball->x < 0)
        ball->x = 0;
    if (ball->x > ssd1306_width - BALL_SIZE)
        ball->x = ssd1306_width - BALL_SIZE;

    // Fundo do display
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
        int y = row * PIN_SPACING + INITIAL_Y_POS; // Ajuste para posicionar corretamente
        // Padrão triangular (linhas alternadas)
        int start_x = (row % 2) ? BIN_WIDTH / 2 : 0;
        for (int x = start_x; x < ssd1306_width; x += BIN_WIDTH)
        {
            ssd1306_set_pixel(buffer, x, y, true);
            // Desenha pequenos obstáculos (2x2 pixels)
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

    // Inicialização do I2C
    i2c_init(i2c1, 400 * 1000);
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);

    // Inicialização do display
    ssd1306_init();

    // Área de renderização
    struct render_area area = {
        .start_column = 0,
        .end_column = ssd1306_width - 1,
        .start_page = 0,
        .end_page = ssd1306_n_pages - 1};
    calculate_render_area_buffer_length(&area);

    uint8_t buffer[ssd1306_buffer_length];

    // Inicialização das bolas
    for (int i = 0; i < MAX_BALLS; i++)
    {
        balls[i].active = false;
    }
    last_spawn_time = get_absolute_time();

    while (true)
    {
        // Limpa o buffer
        memset(buffer, 0, sizeof(buffer));

        // Gera novas bolas
        spawn_new_ball();

        // Atualiza e desenha todas as bolas
        for (int i = 0; i < MAX_BALLS; i++)
        {
            update_ball(&balls[i]);
            draw_ball(buffer, &balls[i]);
        }

        // Desenha os pinos (6 linhas)
        draw_pins(buffer);

        // Mostra o contador
        char text[20];
        sprintf(text, "Total: %d", total_balls);
        ssd1306_draw_string(buffer, 5, 5, text);

        // Atualiza o display
        render_on_display(buffer, &area);

        sleep_ms(16); // ~60 FPS
    }

    return 0;
}