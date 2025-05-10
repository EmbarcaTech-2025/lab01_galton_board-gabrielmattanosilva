/**
 * @file display.c
 * @brief Implementação das funções de controle do display OLED SSD1306.
 */

#include "display.h"
#include "ssd1306.h"
#include <string.h>

static struct render_area area;

/**
 * @brief Inicializa o display SSD1306.
 */
void display_init(void) {
    ssd1306_init();
    area.start_column = 0;
    area.end_column = ssd1306_width - 1;
    area.start_page = 0;
    area.end_page = ssd1306_n_pages - 1;
    calculate_render_area_buffer_length(&area);
}

/**
 * @brief Renderiza o conteúdo do buffer no display.
 * @param buffer Ponteiro para o buffer de pixels a ser renderizado.
 */
void display_render(uint8_t *buffer) {
    render_on_display(buffer, &area);
}

/**
 * @brief Desenha texto no display.
 * @param buffer Ponteiro para o buffer do display.
 * @param x Posição horizontal inicial do texto.
 * @param y Posição vertical inicial do texto.
 * @param text Texto a ser desenhado.
 */
void display_draw_text(uint8_t *buffer, int x, int y, const char *text) {
    ssd1306_draw_string(buffer, x, y, (char *)text);
}