/**
 * @file display.h
 * @brief Interface para funções de controle do display OLED SSD1306.
 */

#ifndef DISPLAY_H
#define DISPLAY_H

#include <stdint.h>
#include <stdbool.h>
#include "ssd1306.h"

/**
 * @brief Inicializa o display SSD1306 via I2C.
 */
void display_init(void);

/**
 * @brief Renderiza a área do display a partir de um buffer.
 * @param buffer Buffer com os dados de pixels
 */
void display_render(uint8_t *buffer);

/**
 * @brief Desenha uma string no display OLED.
 * @param buffer Buffer do display
 * @param x Posição X
 * @param y Posição Y
 * @param text Texto a ser exibido
 */
void display_draw_text(uint8_t *buffer, int x, int y, const char *text);

#endif // DISPLAY_H