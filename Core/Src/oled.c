/******************************************************************************
* Copyright (C) 2026 by Carlos Villarreal - CETYS Universidad
*
* Redistribution, modification or use of this software in source or binary
* forms is permitted as long as the files maintain this copyright. Users are
* permitted to modify this and use it to learn about the field of embedded
* software. Carlos Villarreal and CETYS Universidad are not liable for any
* misuse of this material.
*
*****************************************************************************/
/**
* @file oled.c
* @brief Implementacion del driver OLED SSD1306 para GME12864-11.
*
* El SSD1306 divide la pantalla en 8 "paginas" horizontales de 8 pixeles
* de alto cada una. El frame buffer tiene 8 * 128 = 1024 bytes.
* Cada byte representa 8 pixeles verticales: bit 0 = pixel superior,
* bit 7 = pixel inferior de la pagina.
*
* Para modificar el pixel (x, y):
*   pagina = y / 8
*   bit    = y % 8
*   frame_buf[pagina * 128 + x] |= (1 << bit)    <- encender
*   frame_buf[pagina * 128 + x] &= ~(1 << bit)   <- apagar
*
* La funcion oled_flush() usa "Page Addressing Mode" del SSD1306:
* para cada pagina envía el comando de posicion y luego los 128 bytes.
*
* Gratica de osciloscopio (128x64):
*   - Borde exterior solido
*   - Divisiones cada 16 px horizontal y vertical
*   - 8 divisiones horizontales x 4 divisiones verticales
*
* @author Team MSE - Adrian, Daniel, Carlos, Adal
* @date 2026-05-28
*
*/

/*** Includes ***/
#include "oled.h"
#include "i2c.h"

/*** Preprocessor Definitions ***/

/* Comandos SSD1306 */
#define SSD1306_CMD_CTRL        0x00U  /* Control byte: todos los bytes siguientes son comandos */
#define SSD1306_DATA_CTRL       0x40U  /* Control byte: todos los bytes siguientes son datos */

#define SSD1306_DISPLAYOFF      0xAEU
#define SSD1306_DISPLAYON       0xAFU
#define SSD1306_SETCONTRAST     0x81U
#define SSD1306_DISPLAYALLON    0xA4U  /* Sigue el contenido de RAM */
#define SSD1306_NORMALDISPLAY   0xA6U
#define SSD1306_SETDISPLAYOFF   0xD3U
#define SSD1306_SETDISPLAYCLK   0xD5U
#define SSD1306_SETMULTIPLEX    0xA8U
#define SSD1306_CHARGEPUMP      0x8DU
#define SSD1306_MEMADDRMODE     0x20U
#define SSD1306_SEGREMAP        0xA1U
#define SSD1306_COMSCANDEC      0xC8U
#define SSD1306_SETCOMPINS      0xDAU
#define SSD1306_SETPRECHARGE    0xD9U
#define SSD1306_SETVCOMDETECT   0xDBU
#define SSD1306_SETSTARTLINE    0x40U

/* Modo de direccionamiento: Page = 0x02 */
#define SSD1306_PAGEMODE        0x02U

/* Dentro del frame buffer: area "interior" (sin el borde) */
#define DISP_X_MIN  1U
#define DISP_X_MAX  ((uint8_t)(OLED_WIDTH - 2U))   /* 126 */
#define DISP_Y_MIN  1U
#define DISP_Y_MAX  ((uint8_t)(OLED_HEIGHT - 2U))  /* 62 */

/* Divisiones de la gratica (en pixeles) */
#define GRID_DIV_H  16U   /* Division horizontal cada 16 px */
#define GRID_DIV_V  16U   /* Division vertical cada 16 px */

/*** Local Variables ***/

/* Frame buffer: 8 paginas x 128 columnas = 1024 bytes */
static uint8_t frame_buf[OLED_PAGES * OLED_WIDTH];

/* Buffer de TX para I2C: 1 byte de control + 128 bytes de datos por pagina */
static uint8_t tx_page_buf[OLED_WIDTH + 1U];

/*** Function Prototypes ***/
static Oled_Status_t oled_send_cmd(const uint8_t *cmds, uint32_t len);

/*** Function Definitions ***/

/**
 * @brief Envia uno o varios comandos al SSD1306.
 *        El primer byte del buffer de TX es el control byte 0x00.
 */
static Oled_Status_t oled_send_cmd(const uint8_t *cmds, uint32_t len) {
    static uint8_t cmd_buf[32U];
    uint32_t i;

    if ((cmds == (void *)0) || (len == 0U) || (len > 31U)) {
        return OLED_ERROR;
    }

    cmd_buf[0] = SSD1306_CMD_CTRL;
    for (i = 0U; i < len; i++) {
        cmd_buf[i + 1U] = cmds[i];
    }

    if (i2c_write(OLED_ADDR, cmd_buf, len + 1U) != I2C_OK) {
        return OLED_ERROR;
    }
    return OLED_OK;
}

Oled_Status_t oled_init(void) {
    /* Secuencia de inicializacion del SSD1306 para pantalla 128x64 */
    static const uint8_t init_seq[] = {
        SSD1306_DISPLAYOFF,               /* 0xAE: Apagar display */
        SSD1306_SETDISPLAYCLK,  0x80U,   /* 0xD5: Clock divide ratio / oscilador */
        SSD1306_SETMULTIPLEX,   0x3FU,   /* 0xA8: Multiplex = 63 (1/64 duty) */
        0xD3U,                  0x00U,   /* Offset de display = 0 */
        SSD1306_SETSTARTLINE,            /* 0x40: Start line = 0 */
        SSD1306_CHARGEPUMP,     0x14U,   /* 0x8D: Charge pump ON */
        SSD1306_MEMADDRMODE,    SSD1306_PAGEMODE,  /* 0x20, 0x02: Page mode */
        SSD1306_SEGREMAP,                /* 0xA1: Columna 127 -> SEG0 */
        SSD1306_COMSCANDEC,              /* 0xC8: COM scan de COM63 a COM0 */
        SSD1306_SETCOMPINS,     0x12U,   /* 0xDA: COM pins config alternativa */
        SSD1306_SETCONTRAST,    0xCFU,   /* 0x81: Contraste alto */
        SSD1306_SETPRECHARGE,   0xF1U,   /* 0xD9: Fase1=1, Fase2=15 */
        SSD1306_SETVCOMDETECT,  0x40U,   /* 0xDB: VCOMH deselect ~0.89*Vcc */
        SSD1306_DISPLAYALLON,            /* 0xA4: Display sigue la RAM */
        SSD1306_NORMALDISPLAY,           /* 0xA6: No invertido */
        SSD1306_DISPLAYON                /* 0xAF: Display ON */
    };

    if (oled_send_cmd(init_seq, sizeof(init_seq)) != OLED_OK) {
        return OLED_ERROR;
    }

    oled_clear();
    return OLED_OK;
}

void oled_clear(void) {
    uint32_t i;
    for (i = 0U; i < (OLED_PAGES * OLED_WIDTH); i++) {
        frame_buf[i] = 0x00U;
    }
}

void oled_set_pixel(uint8_t x, uint8_t y, Oled_Color_t color) {
    uint8_t page, bit;

    if ((x >= OLED_WIDTH) || (y >= OLED_HEIGHT)) {
        return;  /* Fuera de rango */
    }

    page = y >> 3U;           /* y / 8  */
    bit  = y &  0x07U;        /* y % 8  */

    if (color == OLED_COLOR_WHITE) {
        frame_buf[page * OLED_WIDTH + x] |=  (uint8_t)(1U << bit);
    } else {
        frame_buf[page * OLED_WIDTH + x] &= ~(uint8_t)(1U << bit);
    }
}

void oled_draw_grid(void) {
    uint8_t x, y;

    /* --- Borde exterior solido --- */
    for (x = 0U; x < OLED_WIDTH; x++) {
        oled_set_pixel(x, 0U,                        OLED_COLOR_WHITE);
        oled_set_pixel(x, (uint8_t)(OLED_HEIGHT - 1U), OLED_COLOR_WHITE);
    }
    for (y = 0U; y < OLED_HEIGHT; y++) {
        oled_set_pixel(0U,                       y, OLED_COLOR_WHITE);
        oled_set_pixel((uint8_t)(OLED_WIDTH - 1U), y, OLED_COLOR_WHITE);
    }

    /* --- Marcas de division interior (un pixel en cada interseccion) ---
     * Horizontal: cada 16 px -> x = 16, 32, 48, 64, 80, 96, 112
     * Vertical:   cada 16 px -> y = 16, 32, 48
     */
    for (x = GRID_DIV_H; x < (OLED_WIDTH - 1U); x += GRID_DIV_H) {
        for (y = GRID_DIV_V; y < (OLED_HEIGHT - 1U); y += GRID_DIV_V) {
            oled_set_pixel(x, y, OLED_COLOR_WHITE);
        }
    }

    /* --- Marcas en los bordes (tick marks) para las divisiones --- */
    /* Ticks horizontales en y=0 y y=63 */
    for (x = GRID_DIV_H; x < (OLED_WIDTH - 1U); x += GRID_DIV_H) {
        oled_set_pixel(x, 0U,                           OLED_COLOR_WHITE);
        oled_set_pixel(x, (uint8_t)(OLED_HEIGHT - 1U),  OLED_COLOR_WHITE);
    }
    /* Ticks verticales en x=0 y x=127 */
    for (y = GRID_DIV_V; y < (OLED_HEIGHT - 1U); y += GRID_DIV_V) {
        oled_set_pixel(0U,                        y, OLED_COLOR_WHITE);
        oled_set_pixel((uint8_t)(OLED_WIDTH - 1U), y, OLED_COLOR_WHITE);
    }
}

void oled_draw_waveform(const uint16_t *samples, uint16_t n_samples) {
    uint8_t x;
    uint8_t curr_y, prev_y;
    uint8_t y_lo, y_hi;
    uint16_t sample_idx;
    uint16_t sample_val;

    if ((samples == (void *)0) || (n_samples == 0U)) {
        return;
    }

    prev_y = 0U;

    /*
     * Iterar cada columna dentro del area interior (x = 1 a 126).
     * Para cada columna, calcular el indice de muestra y mapear
     * el valor ADC (0-4095) al rango de pixeles (1-62), invertido
     * porque y=0 es la parte superior de la pantalla.
     */
    for (x = DISP_X_MIN; x <= DISP_X_MAX; x++) {
        /* Mapear columna a indice de muestra */
        sample_idx = (uint16_t)(
            ((uint32_t)(x - DISP_X_MIN) * (uint32_t)n_samples) /
            (uint32_t)(DISP_X_MAX - DISP_X_MIN + 1U)
        );
        if (sample_idx >= n_samples) {
            sample_idx = n_samples - 1U;
        }

        sample_val = samples[sample_idx];

        /* Mapear ADC (0-4095) a pixel Y (62 hasta 1), invertido */
        curr_y = (uint8_t)(DISP_Y_MAX -
                 (uint8_t)(((uint32_t)sample_val * (uint32_t)(DISP_Y_MAX - DISP_Y_MIN))
                            / (uint32_t)OLED_ADC_MAX));

        /* Clampear dentro del area interior */
        if (curr_y < DISP_Y_MIN) { curr_y = DISP_Y_MIN; }
        if (curr_y > DISP_Y_MAX) { curr_y = DISP_Y_MAX; }

        if (x == DISP_X_MIN) {
            /* Primera muestra: solo pintar el punto */
            oled_set_pixel(x, curr_y, OLED_COLOR_WHITE);
        } else {
            /* Conectar con el punto anterior usando una linea vertical */
            if (prev_y <= curr_y) {
                y_lo = prev_y;
                y_hi = curr_y;
            } else {
                y_lo = curr_y;
                y_hi = prev_y;
            }
            for (uint8_t y = y_lo; y <= y_hi; y++) {
                oled_set_pixel(x, y, OLED_COLOR_WHITE);
            }
        }

        prev_y = curr_y;
    }
}

void oled_draw_waveform_ch2(const uint16_t *samples, uint16_t n_samples) {
    uint8_t x;
    uint8_t curr_y;
    uint16_t sample_idx;
    uint16_t sample_val;

    if ((samples == (void *)0) || (n_samples == 0U)) {
        return;
    }

    /*
     * CH2 se dibuja con linea punteada: solo los pixeles en columnas pares.
     * Esto lo distingue visualmente del CH1 (linea solida).
     */
    for (x = DISP_X_MIN; x <= DISP_X_MAX; x++) {
        /* Solo dibujar en columnas impares -> efecto punteado */
        if ((x & 0x01U) == 0U) { continue; }

        sample_idx = (uint16_t)(
            ((uint32_t)(x - DISP_X_MIN) * (uint32_t)n_samples) /
            (uint32_t)(DISP_X_MAX - DISP_X_MIN + 1U)
        );
        if (sample_idx >= n_samples) {
            sample_idx = n_samples - 1U;
        }

        sample_val = samples[sample_idx];

        curr_y = (uint8_t)(DISP_Y_MAX -
                 (uint8_t)(((uint32_t)sample_val * (uint32_t)(DISP_Y_MAX - DISP_Y_MIN))
                            / (uint32_t)OLED_ADC_MAX));

        if (curr_y < DISP_Y_MIN) { curr_y = DISP_Y_MIN; }
        if (curr_y > DISP_Y_MAX) { curr_y = DISP_Y_MAX; }

        oled_set_pixel(x, curr_y, OLED_COLOR_WHITE);
    }
}

Oled_Status_t oled_flush(void) {
    uint8_t page;
    uint8_t col;
    uint8_t cmd_set_page[3U];

    tx_page_buf[0] = SSD1306_DATA_CTRL;  /* 0x40: lo que sigue son datos */

    for (page = 0U; page < OLED_PAGES; page++) {
        /* Comando: ir a la pagina 'page', columna 0 */
        cmd_set_page[0] = (uint8_t)(0xB0U | page);  /* Set Page Start Address */
        cmd_set_page[1] = 0x00U;                     /* Lower column address = 0 */
        cmd_set_page[2] = 0x10U;                     /* Higher column address = 0 */

        if (oled_send_cmd(cmd_set_page, 3U) != OLED_OK) {
            return OLED_ERROR;
        }

        /* Copiar los 128 bytes de la pagina al buffer de TX */
        for (col = 0U; col < OLED_WIDTH; col++) {
            tx_page_buf[1U + col] = frame_buf[page * OLED_WIDTH + col];
        }

        /* Enviar los datos de la pagina */
        if (i2c_write(OLED_ADDR, tx_page_buf, OLED_WIDTH + 1U) != I2C_OK) {
            return OLED_ERROR;
        }
    }

    return OLED_OK;
}
