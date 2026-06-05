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
#define SAMPLE_COUNT    256U    /* Buffer total — doble para poder hacer trigger */
#define DISPLAY_COUNT   128U    /* Muestras que se dibujan en pantalla */
#define LED_TOGGLE_PIN  5U      /* PA5 = LED LD2 */

/* Canales ADC de los potenciometros */
#define POT_TIMEBASE_CH   4U    /* PA4 = ADC1_IN4 — base de tiempo */
#define POT_AMPLITUDE_CH  8U    /* PB0 = ADC1_IN8 — zoom amplitud  */

/*** Local Variables ***/
static uint16_t adc_ch1[SAMPLE_COUNT];   /* PA0 - ADC1_IN0 */
static uint16_t adc_ch2[SAMPLE_COUNT];   /* PA1 - ADC1_IN1 */

/*** Function Prototypes ***/
static void delay_ms(uint32_t ms);
static void delay_us(uint32_t us);
static void collect_samples(uint32_t inter_sample_us);
static uint32_t measure_freq_hz(const uint16_t *samples, uint16_t n,
                                 uint32_t inter_sample_us);
static uint16_t find_trigger_idx(const uint16_t *samples, uint16_t n);

/*** Function Definitions ***/

/**
 * @brief Retardo por software en milisegundos (aprox).
 *        A 100 MHz (STM32F411): ~100000 ciclos por ms.
 */
static void delay_ms(uint32_t ms) {
    volatile uint32_t count;
    while (ms-- > 0U) {
        for (count = 0U; count < 100000U; count++) {
            __asm__("nop");
        }
    }
}

/* Registros DWT del Cortex-M4 — permiten contar ciclos de CPU exactos.
 * Usamos estos en lugar de NOP loops para que delay_us sea preciso
 * independientemente del nivel de optimizacion del compilador. */
#define DWT_CTRL   (*(volatile uint32_t *)0xE0001000UL)
#define DWT_CYCCNT (*(volatile uint32_t *)0xE0001004UL)
#define DEMCR      (*(volatile uint32_t *)0xE000EDFCUL)

/**
 * @brief Inicializa el contador de ciclos DWT.
 *        Debe llamarse una vez al inicio, despues de clock_init().
 */
static void delay_us_init(void) {
    DEMCR      |= (1U << 24U);   /* Habilitar trace (bit TRCENA) */
    DWT_CYCCNT  = 0U;             /* Reiniciar contador */
    DWT_CTRL   |= (1U << 0U);    /* Habilitar CYCCNT */
}

/**
 * @brief Retardo preciso en microsegundos usando el contador DWT.
 *        A 100 MHz: 100 ciclos = 1 us exacto, sin importar el nivel -O.
 */
static void delay_us(uint32_t us) {
    uint32_t start = DWT_CYCCNT;
    uint32_t ticks = us * 100U;   /* 100 MHz -> 100 ciclos por us */
    while ((DWT_CYCCNT - start) < ticks) { /* esperar */ }
}

/**
 * @brief Recolecta SAMPLE_COUNT muestras de CH1 (PA0) y CH2 (PA1).
 *        El parametro inter_sample_us controla el tiempo entre muestras,
 *        lo que define cuanto tiempo real cabe en la pantalla (timebase).
 *
 *        inter_sample_us = 0  -> maximo refresco (~0.6 us entre muestras)
 *        inter_sample_us = 63 -> ~1 ms/div (senales hasta ~500 Hz visibles)
 *        inter_sample_us = 3125 -> ~50 ms/div (senales de ~10 Hz visibles)
 */
static void collect_samples(uint32_t inter_sample_us) {
    uint16_t i;
    uint16_t value;

    for (i = 0U; i < SAMPLE_COUNT; i++) {
        if (adc_read_channel(0U, &value) == ADC_OK) {
            adc_ch1[i] = value;
        } else {
            adc_ch1[i] = 0U;
        }
        if (adc_read_channel(1U, &value) == ADC_OK) {
            adc_ch2[i] = value;
        } else {
            adc_ch2[i] = 0U;
        }
        if (inter_sample_us > 0U) {
            delay_us(inter_sample_us);
        }
    }
}

/**
 * @brief Estima la frecuencia de la senal en CH1 contando cruces ascendentes
 *        por el valor medio del buffer.
 *
 *        Metodo: calcula el promedio de las muestras como umbral dinamico,
 *        cuenta cuantas veces la senal sube por encima de ese umbral,
 *        y divide entre el tiempo total capturado.
 *
 * @param samples          Buffer de muestras ADC (CH1).
 * @param n                Numero de muestras.
 * @param inter_sample_us  Tiempo entre muestras en us. 0 = modo Auto (~2 us).
 * @return Frecuencia estimada en Hz. 0 si no se detectan cruces.
 */
static uint32_t measure_freq_hz(const uint16_t *samples, uint16_t n,
                                 uint32_t inter_sample_us) {
    uint16_t i;
    uint32_t sum = 0U;
    uint16_t threshold;
    uint16_t crossings = 0U;
    uint8_t  was_above;
    uint32_t effective_us;
    uint32_t total_us;

    if ((samples == (void *)0) || (n < 4U)) { return 0U; }

    /* Umbral dinamico = promedio del buffer */
    for (i = 0U; i < n; i++) { sum += (uint32_t)samples[i]; }
    threshold = (uint16_t)(sum / (uint32_t)n);

    /* Contar cruces ascendentes (de abajo hacia arriba del umbral) */
    was_above = (samples[0U] > threshold) ? 1U : 0U;
    for (i = 1U; i < n; i++) {
        uint8_t now_above = (samples[i] > threshold) ? 1U : 0U;
        if ((was_above == 0U) && (now_above == 1U)) {
            crossings++;
        }
        was_above = now_above;
    }

    if (crossings == 0U) { return 0U; }

    /* Tiempo total capturado en la pantalla */
    effective_us = (inter_sample_us == 0U) ? 2U : inter_sample_us;
    total_us = (uint32_t)n * effective_us;
    if (total_us == 0U) { return 0U; }

    /* freq = cruces / tiempo_total  (cada cruce ascendente = 1 ciclo) */
    return (crossings * 1000000UL) / total_us;
}

/**
 * @brief Busca el primer cruce ascendente del umbral dinamico en la primera
 *        mitad del buffer. Esto es el "trigger de flanco ascendente".
 *
 *        Igual que en un osciloscopio real: el display siempre empieza en
 *        el mismo punto del ciclo, por eso la onda se ve estatica en vez
 *        de desplazarse.
 *
 * @param samples  Buffer completo (SAMPLE_COUNT muestras).
 * @param n        Numero de muestras en el buffer.
 * @return Indice del cruce. 0 si no se encontro (sin trigger).
 */
static uint16_t find_trigger_idx(const uint16_t *samples, uint16_t n) {
    uint32_t sum = 0U;
    uint16_t i;
    uint16_t thr;

    if ((samples == (void *)0) || (n < 4U)) { return 0U; }

    /* Umbral = promedio del buffer (trigger al 50% de la onda) */
    for (i = 0U; i < n; i++) { sum += (uint32_t)samples[i]; }
    thr = (uint16_t)(sum / (uint32_t)n);

    /* Buscar primer cruce ascendente en la primera mitad del buffer.
     * La segunda mitad queda disponible para el display (DISPLAY_COUNT). */
    for (i = 1U; i < (n / 2U); i++) {
        if ((samples[i - 1U] < thr) && (samples[i] >= thr)) {
            return i;
        }
    }
    return 0U;  /* Sin cruce encontrado — empezar desde el inicio */
}

int main(void) {
    Oled_Status_t oled_status;

    /* --- 1. Inicializacion de perifericos --- */
    clock_init();
    delay_us_init();   /* Habilitar DWT para delays precisos en us */
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
        uint16_t pot_timebase  = 0U;
        uint16_t pot_amplitude = 0U;
        uint16_t amp_scale;
        uint32_t inter_sample_us;
        uint32_t freq_hz;
        uint16_t trig_idx;     /* Indice del trigger en el buffer */

        /* Leer potenciometros */
        adc_read_channel(POT_TIMEBASE_CH,  &pot_timebase);
        adc_read_channel(POT_AMPLITUDE_CH, &pot_amplitude);

        /* Pot 1 -> timebase real: microsegundos entre muestras.
         * Rango: 0 us (Auto, ~10 us/div) a 5000 us/muestra (80 ms/div).
         * Esto hace que cada pixel horizontal represente un tiempo real,
         * igual que el time/div de un osciloscopio de laboratorio. */
        inter_sample_us = ((uint32_t)pot_timebase * 5000UL) / 4095UL;

        /* Pot 2 -> escala vertical: rango 512 (4x zoom) a 4095 (sin zoom) */
        amp_scale = (uint16_t)(512U + ((uint32_t)pot_amplitude * 3583UL / 4095UL));
        oled_set_amplitude_scale(amp_scale);

        /* Recolectar muestras con el timebase seleccionado por Pot 1 */
        collect_samples(inter_sample_us);

        /* Trigger: buscar flanco ascendente en CH1.
         * El display empieza siempre en el mismo punto del ciclo,
         * igual que en un osciloscopio real. */
        trig_idx = find_trigger_idx(adc_ch1, SAMPLE_COUNT);

        /* Frecuencia: usar todo el buffer (256 muestras) para mejor resolucion */
        freq_hz = measure_freq_hz(adc_ch1, SAMPLE_COUNT, inter_sample_us);

        /* Limpiar el frame buffer */
        oled_clear();

        /* Dibujar la gratica de osciloscopio */
        oled_draw_grid();

        /* Dibujar barras de estado: voltaje del punto medio de la ventana visible */
        oled_draw_status_bars(inter_sample_us, freq_hz, pot_amplitude,
                              adc_ch1[trig_idx + DISPLAY_COUNT / 2U],
                              adc_ch2[trig_idx + DISPLAY_COUNT / 2U]);

        /* Dibujar formas de onda desde el punto de trigger.
         * Pasamos &samples[trig_idx] para que ambas funciones dibujen
         * la ventana sincronizada. DISPLAY_COUNT = 128 muestras. */
        oled_draw_waveform(&adc_ch1[trig_idx], DISPLAY_COUNT);
        oled_draw_waveform_ch2(&adc_ch2[trig_idx], DISPLAY_COUNT);

        /* Enviar el frame buffer a la pantalla.
         * Si falla (I2C atascado), reinicializar la pantalla automaticamente. */
        if (oled_flush() != OLED_OK) {
            delay_ms(10U);
            i2c_init();
            oled_init();
        }

        /* Heartbeat: parpadeo del LED para indicar que el sistema corre */
        GPIOA->ODR ^= (1U << LED_TOGGLE_PIN);

        /* Streaming binario a PC via UART.
         * En Auto mode (inter_sample_us=0) se envia 1 de cada 4 frames
         * para no bloquear el MCU y mantener la OLED fluida.
         * En timebase lento se envia cada frame (Python tiene tiempo de sobra). */
        {
            static uint8_t uart_skip_ctr = 0U;
            uint8_t uart_divider = (inter_sample_us < 100U) ? 4U : 1U;
            uart_skip_ctr++;
            if (uart_skip_ctr < uart_divider) { goto skip_uart; }
            uint8_t  hdr[6];
            uint16_t i;
            uint8_t  pair[2];
            uint16_t us16 = (inter_sample_us > 0xFFFFU) ? 0xFFFFU
                            : (uint16_t)inter_sample_us;

            /* Header de 6 bytes: sync(2) + inter_sample_us(2) + amp_scale(2)
             * Python usa amp_scale para mostrar el V/div correcto segun POT2. */
            hdr[0] = 0xAAU;
            hdr[1] = 0x55U;
            hdr[2] = (uint8_t)(us16 & 0xFFU);
            hdr[3] = (uint8_t)((us16 >> 8U) & 0xFFU);
            hdr[4] = (uint8_t)(amp_scale & 0xFFU);
            hdr[5] = (uint8_t)((amp_scale >> 8U) & 0xFFU);
            uart_sendBytes(hdr, 6U);

            /* Enviar ventana de display: DISPLAY_COUNT muestras desde trigger */
            for (i = 0U; i < DISPLAY_COUNT; i++) {
                pair[0] = (uint8_t)(adc_ch1[trig_idx + i] & 0xFFU);
                pair[1] = (uint8_t)((adc_ch1[trig_idx + i] >> 8U) & 0xFFU);
                uart_sendBytes(pair, 2U);
            }
            for (i = 0U; i < DISPLAY_COUNT; i++) {
                pair[0] = (uint8_t)(adc_ch2[trig_idx + i] & 0xFFU);
                pair[1] = (uint8_t)((adc_ch2[trig_idx + i] >> 8U) & 0xFFU);
                uart_sendBytes(pair, 2U);
            }
            uart_skip_ctr = 0U;
            skip_uart:;
        }
    }

    return 0;
}
