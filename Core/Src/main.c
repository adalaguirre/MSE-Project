/**
 * @file    main.c
 * @brief   Osciloscopio Digital Embebido - STM32F446RE
 * @details Integracion Semana 1: clock + gpio + uart
 *          Entregable: "OSCILOSCOPE INIT OK" en terminal serie
 *
 * Pin Mapping
 * -----------
 *  PA0  -> ADC1_IN0  (entrada analogica)
 *  PA2  -> USART2_TX (debug)
 *  PA3  -> USART2_RX (debug)
 *  PA5  -> LED LD2   (debug visual)
 *  PB8  -> I2C1_SCL  (OLED)
 *  PB9  -> I2C1_SDA  (OLED)
 *
 * @authors Adrian, Daniel, Carlos, Adal
 * @board   STM32F446RE Nucleo-64
 */

#include <stdint.h>
#include "config.h"
#include "clock.h"
#include "gpio.h"
#include "uart.h"

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
    uart_sendString("OSCILOSCOPE INIT OK\r\n");
    uart_sendString("SYSCLK: 180 MHz\r\n");
    uart_sendString("UART:   115200 baud\r\n");

    /* 4. Blink LED como indicador visual */
    uint32_t count = 0U;
    while (1) {
        GPIOA->ODR ^= (1U << LED_PIN);
        delay_ms(500U);
        count++;
        if (count % 10U == 0U) {
            uart_sendString("ALIVE\r\n");
        }
    }

    return 0;
}