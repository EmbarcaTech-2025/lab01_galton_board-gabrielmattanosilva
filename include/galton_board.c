/**
 * @file galton_board.c
 * @brief Implementação da simulação da Galton Board.
 */

#include "galton_board.h"
#include "ssd1306.h"
#include <stdlib.h>
#include <string.h>

#define BALL_SIZE 2
#define GRAVITY 0.07f
#define BIN_WIDTH (ssd1306_width / NUM_BINS)
#define INITIAL_Y_POS 5
#define HISTOGRAM_HEIGHT 20
#define PIN_SPACING ((ssd1306_height - INITIAL_Y_POS - 6) / NUM_PIN_ROWS - 2)

typedef struct {
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

/**
 * @brief Gera uma direção aleatória para as bolas.
 * @return true para direita, false para esquerda.
 */
static bool random_direction(void) {
    return rand() % 2;
}

/**
 * @brief Inicializa uma bola com posição e velocidade iniciais.
 * @param ball Ponteiro para a estrutura da bola a ser inicializada.
 */
static void init_ball(ball_t *ball) {
    ball->x = ssd1306_width / 2 + (rand() % 5) - 2;
    ball->y = INITIAL_Y_POS;
    ball->vx = 0;
    ball->vy = 0.1f;
    ball->active = true;
    ball->spawn_tick = current_tick;
}

/**
 * @brief Desenha uma bola no buffer do display.
 * @param buffer Ponteiro para o buffer do display.
 * @param ball Ponteiro para a bola a ser desenhada.
 */
static void draw_ball(uint8_t *buffer, ball_t *ball) {
    if (!ball->active) return;

    for (int i = 0; i < BALL_SIZE; i++) {
        for (int j = 0; j < BALL_SIZE; j++) {
            int px = (int)ball->x + i;
            int py = (int)ball->y + j;
            if (px >= 0 && px < ssd1306_width && py >= 0 && py < ssd1306_height) {
                ssd1306_set_pixel(buffer, px, py, true);
            }
        }
    }
}

/**
 * @brief Cria uma nova bola na simulação.
 */
static void spawn_ball(void) {
    total_balls++;
    for (int i = 0; i < MAX_BALLS; i++) {
        if (!balls[i].active) {
            init_ball(&balls[i]);
            break;
        }
    }
}

/**
 * @brief Atualiza o estado da simulação da Galton Board.
 * @param buffer Ponteiro para o buffer do display.
 */
void galton_board_update(uint8_t *buffer) {
    if (current_tick % 5 == 0) spawn_ball();

    for (int i = 0; i < MAX_BALLS; i++) {
        ball_t *ball = &balls[i];
        if (!ball->active) continue;

        ball->vy += GRAVITY;
        ball->x += ball->vx;
        ball->y += ball->vy;

        for (int row = 1; row <= NUM_PIN_ROWS; row++) {
            int pin_y = INITIAL_Y_POS + 8 + (row * PIN_SPACING);
            if (ball->y >= pin_y - 2 && ball->y <= pin_y + 2 && ball->vy > 0) {
                ball->vx = random_direction() ? 2.5f : -2.5f;
                break;
            }
        }

        if (ball->x < 0) ball->x = 0;
        if (ball->x > ssd1306_width - BALL_SIZE) ball->x = ssd1306_width - BALL_SIZE;

        if (ball->y >= ssd1306_height - BALL_SIZE) {
            int bin = (int)(ball->x / BIN_WIDTH);
            if (bin >= 0 && bin < NUM_BINS) bins[bin]++;
            init_ball(ball);
        }

        draw_ball(buffer, ball);
    }
}

/**
 * @brief Desenha os pinos da Galton Board no buffer.
 * @param buffer Ponteiro para o buffer do display.
 */
void galton_board_draw_pins(uint8_t *buffer) {
    for (int row = 1; row <= NUM_PIN_ROWS; row++) {
        int y = INITIAL_Y_POS + 8 + (row * PIN_SPACING);
        int start_x = (row % 2) ? BIN_WIDTH / 2 : 0;
        for (int x = start_x; x < ssd1306_width; x += BIN_WIDTH) {
            ssd1306_set_pixel(buffer, x, y, true);
            ssd1306_set_pixel(buffer, x + 1, y, true);
            ssd1306_set_pixel(buffer, x, y + 1, true);
            ssd1306_set_pixel(buffer, x + 1, y + 1, true);
        }
    }
}

/**
 * @brief Desenha o histograma de distribuição das bolas.
 * @param buffer Ponteiro para o buffer do display.
 */
void galton_board_draw_histogram(uint8_t *buffer) {
    int max_count = 1;
    for (int i = 0; i < NUM_BINS; i++) if (bins[i] > max_count) max_count = bins[i];

    for (int i = 0; i < NUM_BINS; i++) {
        int bar_height = (bins[i] * HISTOGRAM_HEIGHT) / max_count;
        int x_start = i * BIN_WIDTH;
        int x_end = x_start + BIN_WIDTH - 1;
        int y_start = ssd1306_height - 1;
        int y_end = y_start - bar_height;

        for (int y = y_start; y >= y_end; y--) {
            for (int x = x_start; x <= x_end; x++) {
                ssd1306_set_pixel(buffer, x, y, true);
            }
        }
    }
}

/**
 * @brief Inicializa a simulação da Galton Board.
 */
void galton_board_init(void) {
    for (int i = 0; i < MAX_BALLS; i++) balls[i].active = false;
    memset(bins, 0, sizeof(bins));
}