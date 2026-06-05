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

/* Area de dibujo de formas de onda (dentro de las barras de estado) */
#define DISP_X_MIN  1U
#define DISP_X_MAX  ((uint8_t)(OLED_WIDTH - 2U))  /* 126 */
#define DISP_Y_MIN  WAVE_Y_MIN                     /* 9  */
#define DISP_Y_MAX  WAVE_Y_MAX                     /* 54 */

/* Layout del display: barras de estado arriba y abajo, forma de onda en medio */
#define STATUS_BAR_H    8U    /* Alto de cada barra de estado en pixeles (1 pagina) */
#define WAVE_Y_MIN      9U    /* Primer pixel del area de forma de onda */
#define WAVE_Y_MAX      54U   /* Ultimo pixel del area de forma de onda */
#define WAVE_Y_CENTER   32U   /* Centro del area = linea de 0V */

/* Divisiones de la gratica (en pixeles) */
#define GRID_DIV_H  16U   /* Division horizontal cada 16 px */
#define GRID_DIV_V  16U   /* Division vertical cada 16 px */

/* Fuente 5x7 — columnas en orden izquierda a derecha, bit0 = pixel superior.
 * Cubre ASCII 32 (espacio) a 126 (~). 95 caracteres x 5 bytes = 475 bytes de flash.
 * Datos adaptados del font clasico de 5x7 usado en sistemas embebidos.
 */
static const uint8_t FONT5X7[95U][5U] = {
    {0x00,0x00,0x00,0x00,0x00}, /* 32 sp */
    {0x00,0x00,0x5F,0x00,0x00}, /* 33  ! */
    {0x00,0x07,0x00,0x07,0x00}, /* 34  " */
    {0x14,0x7F,0x14,0x7F,0x14}, /* 35  # */
    {0x24,0x2A,0x7F,0x2A,0x12}, /* 36  $ */
    {0x23,0x13,0x08,0x64,0x62}, /* 37  % */
    {0x36,0x49,0x55,0x22,0x50}, /* 38  & */
    {0x00,0x05,0x03,0x00,0x00}, /* 39  ' */
    {0x00,0x1C,0x22,0x41,0x00}, /* 40  ( */
    {0x00,0x41,0x22,0x1C,0x00}, /* 41  ) */
    {0x2A,0x1C,0x7F,0x1C,0x2A}, /* 42  * */
    {0x08,0x08,0x3E,0x08,0x08}, /* 43  + */
    {0x00,0x50,0x30,0x00,0x00}, /* 44  , */
    {0x08,0x08,0x08,0x08,0x08}, /* 45  - */
    {0x00,0x60,0x60,0x00,0x00}, /* 46  . */
    {0x20,0x10,0x08,0x04,0x02}, /* 47  / */
    {0x3E,0x51,0x49,0x45,0x3E}, /* 48  0 */
    {0x00,0x42,0x7F,0x40,0x00}, /* 49  1 */
    {0x42,0x61,0x51,0x49,0x46}, /* 50  2 */
    {0x21,0x41,0x45,0x4B,0x31}, /* 51  3 */
    {0x18,0x14,0x12,0x7F,0x10}, /* 52  4 */
    {0x27,0x45,0x45,0x45,0x39}, /* 53  5 */
    {0x3C,0x4A,0x49,0x49,0x30}, /* 54  6 */
    {0x01,0x71,0x09,0x05,0x03}, /* 55  7 */
    {0x36,0x49,0x49,0x49,0x36}, /* 56  8 */
    {0x06,0x49,0x49,0x29,0x1E}, /* 57  9 */
    {0x00,0x36,0x36,0x00,0x00}, /* 58  : */
    {0x00,0x56,0x36,0x00,0x00}, /* 59  ; */
    {0x08,0x14,0x22,0x41,0x00}, /* 60  < */
    {0x14,0x14,0x14,0x14,0x14}, /* 61  = */
    {0x00,0x41,0x22,0x14,0x08}, /* 62  > */
    {0x02,0x01,0x51,0x09,0x06}, /* 63  ? */
    {0x32,0x49,0x79,0x41,0x3E}, /* 64  @ */
    {0x7E,0x11,0x11,0x11,0x7E}, /* 65  A */
    {0x7F,0x49,0x49,0x49,0x36}, /* 66  B */
    {0x3E,0x41,0x41,0x41,0x22}, /* 67  C */
    {0x7F,0x41,0x41,0x22,0x1C}, /* 68  D */
    {0x7F,0x49,0x49,0x49,0x41}, /* 69  E */
    {0x7F,0x09,0x09,0x09,0x01}, /* 70  F */
    {0x3E,0x41,0x49,0x49,0x7A}, /* 71  G */
    {0x7F,0x08,0x08,0x08,0x7F}, /* 72  H */
    {0x00,0x41,0x7F,0x41,0x00}, /* 73  I */
    {0x20,0x40,0x41,0x3F,0x01}, /* 74  J */
    {0x7F,0x08,0x14,0x22,0x41}, /* 75  K */
    {0x7F,0x40,0x40,0x40,0x40}, /* 76  L */
    {0x7F,0x02,0x0C,0x02,0x7F}, /* 77  M */
    {0x7F,0x04,0x08,0x10,0x7F}, /* 78  N */
    {0x3E,0x41,0x41,0x41,0x3E}, /* 79  O */
    {0x7F,0x09,0x09,0x09,0x06}, /* 80  P */
    {0x3E,0x41,0x51,0x21,0x5E}, /* 81  Q */
    {0x7F,0x09,0x19,0x29,0x46}, /* 82  R */
    {0x46,0x49,0x49,0x49,0x31}, /* 83  S */
    {0x01,0x01,0x7F,0x01,0x01}, /* 84  T */
    {0x3F,0x40,0x40,0x40,0x3F}, /* 85  U */
    {0x1F,0x20,0x40,0x20,0x1F}, /* 86  V */
    {0x3F,0x40,0x38,0x40,0x3F}, /* 87  W */
    {0x63,0x14,0x08,0x14,0x63}, /* 88  X */
    {0x07,0x08,0x70,0x08,0x07}, /* 89  Y */
    {0x61,0x51,0x49,0x45,0x43}, /* 90  Z */
    {0x00,0x7F,0x41,0x41,0x00}, /* 91  [ */
    {0x02,0x04,0x08,0x10,0x20}, /* 92  \ */
    {0x00,0x41,0x41,0x7F,0x00}, /* 93  ] */
    {0x04,0x02,0x01,0x02,0x04}, /* 94  ^ */
    {0x40,0x40,0x40,0x40,0x40}, /* 95  _ */
    {0x00,0x01,0x02,0x04,0x00}, /* 96  ` */
    {0x20,0x54,0x54,0x54,0x78}, /* 97  a */
    {0x7F,0x48,0x44,0x44,0x38}, /* 98  b */
    {0x38,0x44,0x44,0x44,0x20}, /* 99  c */
    {0x38,0x44,0x44,0x48,0x7F}, /* 100 d */
    {0x38,0x54,0x54,0x54,0x18}, /* 101 e */
    {0x08,0x7E,0x09,0x01,0x02}, /* 102 f */
    {0x0C,0x52,0x52,0x52,0x3E}, /* 103 g */
    {0x7F,0x08,0x04,0x04,0x78}, /* 104 h */
    {0x00,0x44,0x7D,0x40,0x00}, /* 105 i */
    {0x20,0x40,0x44,0x3D,0x00}, /* 106 j */
    {0x7F,0x10,0x28,0x44,0x00}, /* 107 k */
    {0x00,0x41,0x7F,0x40,0x00}, /* 108 l */
    {0x7C,0x04,0x18,0x04,0x78}, /* 109 m */
    {0x7C,0x08,0x04,0x04,0x78}, /* 110 n */
    {0x38,0x44,0x44,0x44,0x38}, /* 111 o */
    {0x7C,0x14,0x14,0x14,0x08}, /* 112 p */
    {0x08,0x14,0x14,0x18,0x7C}, /* 113 q */
    {0x7C,0x08,0x04,0x04,0x08}, /* 114 r */
    {0x48,0x54,0x54,0x54,0x20}, /* 115 s */
    {0x04,0x3F,0x44,0x40,0x20}, /* 116 t */
    {0x3C,0x40,0x40,0x20,0x7C}, /* 117 u */
    {0x1C,0x20,0x40,0x20,0x1C}, /* 118 v */
    {0x3C,0x40,0x30,0x40,0x3C}, /* 119 w */
    {0x44,0x28,0x10,0x28,0x44}, /* 120 x */
    {0x0C,0x50,0x50,0x50,0x3C}, /* 121 y */
    {0x44,0x64,0x54,0x4C,0x44}, /* 122 z */
    {0x00,0x08,0x36,0x41,0x00}, /* 123 { */
    {0x00,0x00,0x7F,0x00,0x00}, /* 124 | */
    {0x00,0x41,0x36,0x08,0x00}, /* 125 } */
    {0x10,0x08,0x08,0x10,0x08}, /* 126 ~ */
};

/*** Local Variables ***/

/* Frame buffer: 8 paginas x 128 columnas = 1024 bytes */
static uint8_t frame_buf[OLED_PAGES * OLED_WIDTH];

/* Buffer de TX para I2C: 1 byte de control + 128 bytes de datos por pagina */
static uint8_t tx_page_buf[OLED_WIDTH + 1U];

/* Escala de amplitud: rango ADC visible centrado en 2048.
 * 4095 = sin zoom. 2048 = 2x. 1024 = 4x. Minimo: 256. */
static uint16_t s_amp_scale = OLED_ADC_MAX;

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

void oled_set_amplitude_scale(uint16_t scale) {
    if (scale < 256U)          { scale = 256U; }
    if (scale > OLED_ADC_MAX)  { scale = OLED_ADC_MAX; }
    s_amp_scale = scale;
}

/**
 * @brief Escribe un pixel manejando coordenadas con signo (para circulos
 *        cuyos puntos pueden caer fuera de los bordes).
 */
static void oled_set_pixel_safe(int16_t x, int16_t y, Oled_Color_t color) {
    if ((x >= 0) && (x < (int16_t)OLED_WIDTH) &&
        (y >= 0) && (y < (int16_t)OLED_HEIGHT)) {
        oled_set_pixel((uint8_t)x, (uint8_t)y, color);
    }
}

void oled_draw_circle(uint8_t cx, uint8_t cy, uint8_t r, Oled_Color_t color) {
    int16_t x  = 0;
    int16_t y  = (int16_t)r;
    int16_t d  = 1 - (int16_t)r;
    int16_t icx = (int16_t)cx;
    int16_t icy = (int16_t)cy;

    while (x <= y) {
        oled_set_pixel_safe(icx + x, icy + y, color);
        oled_set_pixel_safe(icx - x, icy + y, color);
        oled_set_pixel_safe(icx + x, icy - y, color);
        oled_set_pixel_safe(icx - x, icy - y, color);
        oled_set_pixel_safe(icx + y, icy + x, color);
        oled_set_pixel_safe(icx - y, icy + x, color);
        oled_set_pixel_safe(icx + y, icy - x, color);
        oled_set_pixel_safe(icx - y, icy - x, color);

        if (d < 0) {
            d += 2 * x + 3;
        } else {
            d += 2 * (x - y) + 5;
            y--;
        }
        x++;
    }
}

void oled_draw_char(uint8_t x, uint8_t y, char c) {
    uint8_t page = (uint8_t)(y >> 3U);
    uint8_t col;
    const uint8_t *glyph;

    if (page >= OLED_PAGES) { return; }
    if ((c < 32) || (c > 126)) { c = ' '; }
    glyph = FONT5X7[(uint8_t)((uint8_t)c - 32U)];

    for (col = 0U; col < 5U; col++) {
        if ((x + col) < OLED_WIDTH) {
            frame_buf[page * OLED_WIDTH + x + col] |= glyph[col];
        }
    }
}

void oled_draw_string(uint8_t x, uint8_t y, const char *str) {
    if (str == (void *)0) { return; }
    while ((*str != '\0') && ((x + 5U) < OLED_WIDTH)) {
        oled_draw_char(x, y, *str);
        x += 6U;  /* 5px caracter + 1px separacion */
        str++;
    }
}

/**
 * @brief Convierte un entero con signo a string (para voltage en 0.1V).
 *        Ejemplo: tenths = -15 → "-1.5V", tenths = 16 → "+1.6V"
 */
/**
 * @brief Escribe los digitos decimales de val en buf.
 *        No agrega terminador; devuelve el numero de chars escritos.
 *        Rango soportado: 0 a 9999.
 */
static uint8_t uint_to_buf(uint32_t val, char *buf) {
    char tmp[5];
    uint8_t len = 0U;
    uint8_t i;
    if (val == 0U) { buf[0] = '0'; return 1U; }
    while (val > 0U) {
        tmp[len++] = (char)('0' + (uint8_t)(val % 10U));
        val /= 10U;
    }
    for (i = 0U; i < len; i++) { buf[i] = tmp[len - 1U - i]; }
    return len;
}

static void tenths_to_str(int16_t tenths, char *buf) {
    uint8_t idx = 0U;
    int16_t abs_val;

    if (tenths < 0) {
        buf[idx++] = '-';
        abs_val = (int16_t)(-tenths);
    } else {
        buf[idx++] = '+';
        abs_val = tenths;
    }

    buf[idx++] = (char)('0' + (uint8_t)(abs_val / 10));
    buf[idx++] = '.';
    buf[idx++] = (char)('0' + (uint8_t)(abs_val % 10U));
    buf[idx++] = 'V';
    buf[idx]   = '\0';
}

void oled_draw_status_bars(uint32_t inter_sample_us, uint32_t freq_hz,
                            uint16_t pot_amp,
                            uint16_t ch1_adc, uint16_t ch2_adc) {
    char buf[12];
    int16_t ch1_tenths, ch2_tenths;
    uint8_t zoom_x;

    /* ---- BARRA SUPERIOR (y = 0) ---- */

    /* Etiqueta CH1: circulo con "1" adentro, centrado en x=4, y=3 */
    oled_draw_circle(4U, 3U, 4U, OLED_COLOR_WHITE);
    oled_draw_string(2U, 0U, "1");

    /* V/div: voltios por division segun zoom actual.
     * Area de onda = 45 px / 16 px por div = ~2.8 divs.
     * Rango CH1 = (s_amp_scale/4095)*3.3 V visible.
     * V/div = rango / 2.8 ~ (s_amp_scale * 12) / 4095 en decimas.
     * Tabla: 1x->1.2V  2x->0.6V  4x->0.3V  8x->0.1V */
    zoom_x = (uint8_t)(4096U / (uint16_t)s_amp_scale);
    if      (zoom_x >= 8U) { oled_draw_string(14U, 0U, "0.1V"); }
    else if (zoom_x >= 4U) { oled_draw_string(14U, 0U, "0.3V"); }
    else if (zoom_x >= 2U) { oled_draw_string(14U, 0U, "0.6V"); }
    else                   { oled_draw_string(14U, 0U, "1.2V"); }

    /* Timebase: mostrar tiempo por division horizontal.
     * Pantalla = 128 muestras / 8 divisiones = 16 muestras por division.
     * time_per_div = 16 * inter_sample_us.
     * Ejemplos:
     *   0 us/muestra -> "T:Auto"
     *   1 us/muestra -> 16 us/div  -> "T:16us"
     *   63 us/muestra-> 1008 us/div -> "T:1ms"
     *   312 us/muestra-> 4992 us/div -> "T:4ms"
     *   3125 us/muestra-> 50000 us/div -> "T:50ms"
     */
    {
        char tbuf[10];
        uint8_t idx = 0U;
        uint32_t tpd_us = 16UL * inter_sample_us;  /* tiempo/div en us */

        tbuf[idx++] = 'T';
        tbuf[idx++] = ':';

        if (inter_sample_us == 0U) {
            tbuf[idx++] = 'A'; tbuf[idx++] = 'u';
            tbuf[idx++] = 't'; tbuf[idx++] = 'o';
        } else if (tpd_us < 1000U) {
            idx += uint_to_buf(tpd_us, &tbuf[idx]);
            tbuf[idx++] = 'u'; tbuf[idx++] = 's';
        } else {
            idx += uint_to_buf(tpd_us / 1000U, &tbuf[idx]);
            tbuf[idx++] = 'm'; tbuf[idx++] = 's';
        }
        tbuf[idx] = '\0';
        oled_draw_string(42U, 0U, tbuf);
    }

    /* Frecuencia medida de CH1: mostrar entre el timebase y el circulo CH2.
     * 0 Hz     -> "--Hz" (sin senal detectada)
     * < 1000   -> "XXXHz"
     * >= 1000  -> "XXkHz"
     * Posicion x=80: deja espacio al timebase (hasta x~78) y al circulo (x=119). */
    {
        char fbuf[8];
        uint8_t fidx = 0U;
        if (freq_hz == 0U) {
            fbuf[fidx++] = '-'; fbuf[fidx++] = '-';
            fbuf[fidx++] = 'H'; fbuf[fidx++] = 'z';
        } else if (freq_hz < 1000U) {
            fidx += uint_to_buf(freq_hz, &fbuf[fidx]);
            fbuf[fidx++] = 'H'; fbuf[fidx++] = 'z';
        } else {
            fidx += uint_to_buf(freq_hz / 1000U, &fbuf[fidx]);
            fbuf[fidx++] = 'k'; fbuf[fidx++] = 'H'; fbuf[fidx++] = 'z';
        }
        fbuf[fidx] = '\0';
        oled_draw_string(80U, 0U, fbuf);
    }

    /* Etiqueta CH2: circulo con "2" adentro, centrado en x=123, y=3 */
    oled_draw_circle(123U, 3U, 4U, OLED_COLOR_WHITE);
    oled_draw_string(121U, 0U, "2");

    /* ---- BARRA INFERIOR (y = 56) ---- */

    /* CH1: prefijo "1:" + voltaje. Rango 0..3.3V. */
    buf[0] = '1';
    buf[1] = ':';
    ch1_tenths = (int16_t)(((uint32_t)ch1_adc * 33UL) / 4095UL);
    tenths_to_str(ch1_tenths, &buf[2]);
    oled_draw_string(2U, 56U, buf);

    /* CH2: prefijo "2:" + voltaje. LM358: V_real = 2*V_adc - 3.3V */
    buf[0] = '2';
    buf[1] = ':';
    ch2_tenths = (int16_t)(((uint32_t)ch2_adc * 66UL) / 4095UL) - 33;
    tenths_to_str(ch2_tenths, &buf[2]);
    oled_draw_string(68U, 56U, buf);
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

    /* --- Separadores de barra de estado (lineas solidas) --- */
    /* Linea inferior de la barra superior: y = STATUS_BAR_H - 1 = 7 */
    for (x = 0U; x < OLED_WIDTH; x++) {
        oled_set_pixel(x, (uint8_t)(STATUS_BAR_H - 1U), OLED_COLOR_WHITE);
    }
    /* Linea superior de la barra inferior: y = OLED_HEIGHT - STATUS_BAR_H = 56 */
    for (x = 0U; x < OLED_WIDTH; x++) {
        oled_set_pixel(x, (uint8_t)(OLED_HEIGHT - STATUS_BAR_H), OLED_COLOR_WHITE);
    }

    /* --- Bordes laterales del area de forma de onda --- */
    for (y = STATUS_BAR_H; y < (OLED_HEIGHT - STATUS_BAR_H); y++) {
        oled_set_pixel(0U,                        y, OLED_COLOR_WHITE);
        oled_set_pixel((uint8_t)(OLED_WIDTH - 1U), y, OLED_COLOR_WHITE);
    }

    /* --- Puntos de gratica dentro del area de forma de onda ---
     * Horizontal cada 16 px: x = 16, 32, 48, 64, 80, 96, 112
     * Vertical   cada 16 px: y = 16 y 48 (dentro del area 8-55)
     */
    for (x = GRID_DIV_H; x < (OLED_WIDTH - 1U); x += GRID_DIV_H) {
        for (y = (uint8_t)(STATUS_BAR_H + GRID_DIV_V);
             y < (OLED_HEIGHT - STATUS_BAR_H);
             y += GRID_DIV_V) {
            oled_set_pixel(x, y, OLED_COLOR_WHITE);
        }
        /* Tick marks en la linea separadora inferior */
        oled_set_pixel(x, (uint8_t)(OLED_HEIGHT - STATUS_BAR_H), OLED_COLOR_WHITE);
    }

    /* --- Linea punteada de referencia 0V en el centro (y = 32) --- */
    for (x = 2U; x < (OLED_WIDTH - 1U); x += 4U) {
        oled_set_pixel(x, WAVE_Y_CENTER, OLED_COLOR_WHITE);
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

        /* Mapear ADC al pixel Y para CH1 (0..3.3V, sin bias).
         * ADC=0    -> fondo de pantalla (0V real).
         * ADC=4095 -> tope de pantalla (3.3V real).
         * El zoom recorta simetricamente desde el fondo:
         *   s_amp_scale = 4095 -> rango completo 0..4095
         *   s_amp_scale = 2048 -> solo se ve 0..2048 (zoom 2x desde abajo)
         * Esto mantiene el 0V anclado al fondo aunque se haga zoom.
         */
        {
            uint32_t v_max = (uint32_t)s_amp_scale;   /* techo visible */
            uint32_t sv    = (uint32_t)sample_val;

            if (sv > v_max) { sv = v_max; }

            curr_y = (uint8_t)(DISP_Y_MAX -
                     (uint8_t)((sv * (uint32_t)(DISP_Y_MAX - DISP_Y_MIN))
                               / v_max));
        }

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

        /* Mismo mapeo que CH1: centra en 2048 (= 0V) con zoom s_amp_scale */
        {
            int32_t center = 2048;
            int32_t v_min  = center - (int32_t)(s_amp_scale / 2U);
            int32_t v_max  = center + (int32_t)(s_amp_scale / 2U);
            int32_t sv     = (int32_t)sample_val;
            uint32_t range = (uint32_t)s_amp_scale;

            if (v_min < 0)    { v_min = 0; }
            if (v_max > 4095) { v_max = 4095; }
            if (sv < v_min)   { sv = v_min; }
            if (sv > v_max)   { sv = v_max; }

            curr_y = (uint8_t)(DISP_Y_MAX -
                     (uint8_t)(((uint32_t)(sv - v_min) *
                     (uint32_t)(DISP_Y_MAX - DISP_Y_MIN)) / range));
        }

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
