/**
 * @file galton_board.h
 * @brief Funções de simulação da Galton Board.
 */

#ifndef GALTON_BOARD_H
#define GALTON_BOARD_H

#include <stdint.h>
#include <stdbool.h>

#define MAX_BALLS 30
#define NUM_BINS 16
#define NUM_PIN_ROWS 8

extern int total_balls;
extern int current_tick;
extern int bins[NUM_BINS];
extern bool show_histogram;

/**
 * @brief Inicializa a simulação da Galton Board.
 */
void galton_board_init(void);

/**
 * @brief Atualiza o estado da simulação (movimento das bolinhas).
 * @param buffer Buffer do display
 */
void galton_board_update(uint8_t *buffer);

/**
 * @brief Desenha os pinos da Galton Board.
 * @param buffer Buffer do display
 */
void galton_board_draw_pins(uint8_t *buffer);

/**
 * @brief Desenha o histograma no buffer do display.
 * @param buffer Buffer do display
 */
void galton_board_draw_histogram(uint8_t *buffer);

#endif // GALTON_BOARD_H