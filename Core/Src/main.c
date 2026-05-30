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
* @file main.c
* @brief Osciloscopio Digital Embebido - STM32F446RE
*
* Integracion Semana 3: clock + gpio + uart + adc + i2c + oled
* Entregable: forma de onda del ADC desplegada en pantalla OLED en tiempo real.
*
* Flujo principal:
*   1. Inicializar todos los perifericos.
*   2. Recolectar SAMPLE_COUNT muestras del ADC (CH1 en PA0).
*   3. Limpiar frame buffer de la OLED.
*   4. Dibujar la gratica (grid) de osciloscopio.
*   5. Dibujar la forma de onda sobre la gratica.
*   6. Enviar el frame buffer a la pantalla (flush).
*   7. Repetir indefinidamente.
*
* Pines:
*   PA0         - ADC1_IN0  (entrada analogica CH1)
*   PA2 / PA3   - USART2 TX/RX (debug)
*   PA5         - LED LD2 (heartbeat)
*   PB8 / PB9   - I2C1 SCL/SDA (OLED)
*
* @authors Adrian, Daniel, Carlos, Adal
* @board   STM32F446RE Nucleo-64
* @date    2026-05-28
*/

/*** Includes ***/
#include <stdint.h>
#include "config.h"
#include "clock.h"
#include "gpio.h"
#include "uart.h"
#include "adc.h"
#include "i2c.h"
#include "oled.h"

/*** Preprocessor Definitions ***/
#define SAMPLE_COUNT    128U    /* Numero de muestras por frame de display */
#define LED_TOGGLE_PIN  5U      /* PA5 = LED LD2 */

/*** Local Variables ***/
static uint16_t adc_ch1[SAMPLE_COUNT];   /* PA0 - ADC1_IN0 */
static uint16_t adc_ch2[SAMPLE_COUNT];   /* PA1 - ADC1_IN1 */

/*** Function Prototypes ***/
static void delay_ms(uint32_t ms);
static void collect_samples(void);

/*** Function Definitions ***/

/**
 * @brief Retardo por software (aproximado).
 *        A 100 MHz (STM32F411), ~100000 ciclos por milisegundo.
 */
static void delay_ms(uint32_t ms) {
    volatile uint32_t count;
    while (ms-- > 0U) {
        for (count = 0U; count < 100000U; count++) {
            __asm__("nop");
        }
    }
}

/**
 * @brief Recolecta SAMPLE_COUNT muestras de CH1 (PA0) y CH2 (PA1)
 *        de forma intercalada para que ambas tengan el mismo tiempo base.
 */
static void collect_samples(void) {
    uint16_t i;
    uint16_t value;

    for (i = 0U; i < SAMPLE_COUNT; i++) {
        /* CH1: PA0 = ADC1_IN0 */
        if (adc_read_channel(0U, &value) == ADC_OK) {
            adc_ch1[i] = value;
        } else {
            adc_ch1[i] = 0U;
        }

        /* CH2: PA1 = ADC1_IN1 */
        if (adc_read_channel(1U, &value) == ADC_OK) {
            adc_ch2[i] = value;
        } else {
            adc_ch2[i] = 0U;
        }
    }
}

int main(void) {
    Oled_Status_t oled_status;

    /* --- 1. Inicializacion de perifericos --- */
    clock_init();
    gpio_init();

    uart_init();
    uart_sendString("=== OSCILOSCOPIO DIGITAL STM32F446RE ===\r\n");

    adc_init();
    uart_sendString("ADC OK\r\n");

    i2c_init();
    uart_sendString("I2C OK\r\n");

    oled_status = oled_init();
    if (oled_status != OLED_OK) {
        uart_sendString("ERROR: OLED no responde. Verificar conexiones PB8/PB9.\r\n");
    } else {
        uart_sendString("OLED OK\r\n");
    }

    uart_sendString("Iniciando captura...\r\n");
    delay_ms(100U);

    /* --- 2. Bucle principal --- */
    while (1) {
        /* Recolectar muestras del ADC */
        collect_samples();

        /* Limpiar el frame buffer */
        oled_clear();

        /* Dibujar la gratica de osciloscopio */
        oled_draw_grid();

        /* Dibujar CH1 (linea solida) y CH2 (linea punteada) */
        oled_draw_waveform(adc_ch1, SAMPLE_COUNT);
        oled_draw_waveform_ch2(adc_ch2, SAMPLE_COUNT);

        /* Enviar el frame buffer a la pantalla.
         * Si falla (I2C atascado), reinicializar la pantalla automaticamente. */
        if (oled_flush() != OLED_OK) {
            delay_ms(10U);
            i2c_init();
            oled_init();
        }

        /* Heartbeat: parpadeo del LED para indicar que el sistema corre */
        GPIOA->ODR ^= (1U << LED_TOGGLE_PIN);
    }

    return 0;
}
