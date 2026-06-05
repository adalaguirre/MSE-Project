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
* @file oled.h
* @brief Driver para pantalla OLED GME12864-11 (controlador SSD1306).
*
* Display: 0.96", 128x64 pixeles, interfaz I2C, direccion 0x3C.
* Implementa un frame buffer de 1024 bytes (8 paginas x 128 columnas).
* Provee funciones para dibujar la gratica (grid), la forma de onda
* del ADC y enviar el buffer completo a la pantalla.
*
* Uso tipico:
*   oled_init();
*   oled_clear();
*   oled_draw_grid();
*   oled_draw_waveform(buffer, 128);
*   oled_flush();
*
* @author Team MSE - Adrian, Daniel, Carlos, Adal
* @date 2026-05-28
*
*/

#ifndef __OLED_H__
#define __OLED_H__

/*** Includes ***/
#include <stdint.h>

/*** Preprocessor Definitions ***/
#define OLED_WIDTH      128U   /* Ancho en pixeles */
#define OLED_HEIGHT     64U    /* Alto en pixeles */
#define OLED_PAGES      8U     /* Paginas = 64 / 8 */
#define OLED_ADDR       0x3CU  /* Direccion I2C de 7 bits (SA0 = GND) */

#define OLED_ADC_MAX    4095U  /* Valor maximo del ADC de 12 bits */

/*** Type Prototypes ***/
typedef enum {
    OLED_OK    = 0U,
    OLED_ERROR = 1U
} Oled_Status_t;

typedef enum {
    OLED_COLOR_BLACK = 0U,   /* Apagar pixel */
    OLED_COLOR_WHITE = 1U    /* Encender pixel */
} Oled_Color_t;

/*** Function Prototypes ***/

/**
 * @brief Inicializa el controlador SSD1306 con la secuencia de arranque.
 *        Debe llamarse despues de i2c_init().
 * @return OLED_OK si fue exitoso.
 */
Oled_Status_t oled_init(void);

/**
 * @brief Limpia el frame buffer interno (apaga todos los pixeles en RAM).
 *        No escribe a la pantalla; llamar oled_flush() despues.
 */
void oled_clear(void);

/**
 * @brief Enciende o apaga un pixel en el frame buffer.
 * @param x     Columna (0 a 127).
 * @param y     Fila (0 = arriba, 63 = abajo).
 * @param color OLED_COLOR_WHITE para encender, OLED_COLOR_BLACK para apagar.
 */
void oled_set_pixel(uint8_t x, uint8_t y, Oled_Color_t color);

/**
 * @brief Dibuja la gratica (grid) de osciloscopio en el frame buffer.
 *        Incluye borde exterior y puntos en las intersecciones de divisiones.
 */
void oled_draw_grid(void);

/**
 * @brief Dibuja la forma de onda del canal ADC en el frame buffer.
 *        Mapea n_samples al ancho de la pantalla y los valores ADC al alto.
 * @param samples    Arreglo de valores ADC (0 a 4095).
 * @param n_samples  Numero de muestras en el arreglo (idealmente 128).
 */
void oled_draw_waveform(const uint16_t *samples, uint16_t n_samples);

/**
 * @brief Dibuja la forma de onda del CH2 con linea punteada (pixels alternos)
 *        para distinguirla visualmente del CH1.
 * @param samples    Arreglo de valores ADC (0 a 4095).
 * @param n_samples  Numero de muestras en el arreglo (idealmente 128).
 */
void oled_draw_waveform_ch2(const uint16_t *samples, uint16_t n_samples);

/**
 * @brief Ajusta el zoom vertical de las formas de onda.
 *        Controlado por el potenciometro de amplitud (Pot 2).
 *
 * @param scale  Rango ADC visible centrado en 2048 (0V).
 *               4095 = escala completa (sin zoom).
 *               2048 = zoom 2x. 1024 = zoom 4x.
 *               Minimo permitido: 256.
 */
void oled_set_amplitude_scale(uint16_t scale);

/**
 * @brief Dibuja el contorno de un circulo usando el algoritmo de Bresenham.
 *
 * @param cx     Columna del centro (0 a 127).
 * @param cy     Fila del centro (0 a 63).
 * @param r      Radio en pixeles.
 * @param color  OLED_COLOR_WHITE para encender, OLED_COLOR_BLACK para borrar.
 */
void oled_draw_circle(uint8_t cx, uint8_t cy, uint8_t r, Oled_Color_t color);

/**
 * @brief Dibuja un caracter de la fuente 5x7 en el frame buffer.
 *        El caracter tiene 5px de ancho y 7px de alto.
 *        La posicion 'y' debe ser multiplo de 8 (inicio de pagina).
 *
 * @param x  Columna de inicio (0 a 122).
 * @param y  Fila de inicio, multiplo de 8 (0, 8, 16, 24, 32, 40, 48, 56).
 * @param c  Caracter ASCII (32 a 126).
 */
void oled_draw_char(uint8_t x, uint8_t y, char c);

/**
 * @brief Dibuja una cadena de caracteres en el frame buffer.
 *
 * @param x    Columna de inicio.
 * @param y    Fila de inicio, multiplo de 8.
 * @param str  Cadena terminada en null.
 */
void oled_draw_string(uint8_t x, uint8_t y, const char *str);

/**
 * @brief Dibuja las barras de estado superior e inferior.
 *        Superior: etiquetas CH1, CH2, zoom y timebase.
 *        Inferior: voltaje actual de cada canal.
 *
 * @param inter_sample_us  Tiempo entre muestras en microsegundos (Pot 1).
 *                         0 = maximo refresco (Auto). Max util: ~5000.
 * @param freq_hz          Frecuencia estimada de CH1 en Hz (0 = sin senal).
 * @param pot_amp          Lectura ADC del Pot 2 (0-4095) para indicar zoom.
 * @param ch1_adc          Muestra ADC de CH1 para calcular voltaje.
 * @param ch2_adc          Muestra ADC de CH2 para calcular voltaje.
 */
void oled_draw_status_bars(uint32_t inter_sample_us, uint32_t freq_hz,
                            uint16_t pot_amp,
                            uint16_t ch1_adc, uint16_t ch2_adc);

/**
 * @brief Envia el frame buffer completo a la pantalla via I2C.
 *        Actualiza las 8 paginas del SSD1306.
 * @return OLED_OK si fue exitoso.
 */
Oled_Status_t oled_flush(void);

#endif /* __OLED_H__ */
