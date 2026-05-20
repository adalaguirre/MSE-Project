/**
 * @file    main.c
 * @brief   Osciloscopio Digital Embebido - STM32F446RE
 * @details Integracion Semana 2: clock + gpio + uart + adc
 *          Entregable: valor ADC impreso por UART cada 100ms
 *
 * @authors Adrian, Daniel, Carlos, Adal
 * @board   STM32F446RE Nucleo-64
 */

#include <stdint.h>
#include "config.h"
#include "clock.h"
#include "gpio.h"
#include "uart.h"
#include "adc.h"

static void delay_ms(uint32_t ms) {
    volatile uint32_t count;
    while (ms--) {
        for (count = 0U; count < 180000U; count++) {
            __asm__("nop");
        }
    }
}

int main(void) {

    /* 1. Clock a 180 MHz */
    clock_init();

    /* 2. Todos los pines del proyecto */
    gpio_init();

    /* 3. UART debug */
    uart_init();
    uart_sendString("SEMANA 2: ADC POLLING TEST\r\n");

    /* 4. ADC1 en PA0 */
    adc_init();
    uart_sendString("ADC INIT OK\r\n");

    uint16_t adc_value = 0U;

    while (1) {
        if (adc_read(&adc_value) == ADC_OK) {
            uart_sendString("ADC = ");
            uart_sendUInt16(adc_value);
            uart_sendString("\r\n");
        }

        GPIOA->ODR ^= (1U << LED_PIN);
        delay_ms(100U);
    }

    return 0;
}