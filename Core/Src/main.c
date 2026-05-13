/**
 * @file    main.c
 * @brief   Osciloscopio Digital Embebido - STM32F446RE
 * @details Bare metal, sin HAL.
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

int main(void) {

    /* Semana 1: clock + uart
     * Semana 2: gpio + adc
     * Semana 3: timer + muestreo
     * Semana 4: oled driver
     * Semana 5: visualizacion
     * Semana 6: integracion */

    while (1) {
    }

    return 0;
}
