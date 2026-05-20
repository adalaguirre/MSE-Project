/**
 * @file adc.c
 * @brief ADC1 polling driver para PA0 / ADC1_IN0
 * @author Adrian
 */

#include "adc.h"
#include "clock.h"

/* Bits usados */
#define ADC1_EN_BIT     8U
#define ADC_ADON_BIT    0U
#define ADC_SWSTART_BIT 30U
#define ADC_EOC_BIT     1U

Adc_Status_t adc_init(void) {
    /* Habilitar clock de ADC1 en APB2 */
    RCC_APB2ENR |= (1U << ADC1_EN_BIT);

    /* Prescaler ADC /4: APB2 = 90 MHz, ADC_CLK = 22.5 MHz */
    ADC_CCR &= ~(0x3U << 16U);
    ADC_CCR |= (0x1U << 16U);

    /* Resolucion de 12 bits */
    ADC1->CR1 &= ~(0x3U << 24U);

    /* Una conversion, alineacion derecha */
    ADC1->CR2 = 0U;

    /* Secuencia regular de 1 conversion */
    ADC1->SQR1 &= ~(0xFU << 20U);

    /* Canal 0 en la primera conversion */
    ADC1->SQR3 &= ~(0x1FU << 0U);

    /* Tiempo de muestreo para canal 0 */
    ADC1->SMPR2 &= ~(0x7U << 0U);
    ADC1->SMPR2 |= (0x4U << 0U);

    /* Encender ADC */
    ADC1->CR2 |= (1U << ADC_ADON_BIT);

    return ADC_OK;
}

Adc_Status_t adc_read(uint16_t *value) {
    uint32_t timeout = 100000U;

    if (value == (void *)0) {
        return ADC_ERROR_INVALID;
    }

    /* Iniciar conversion por software */
    ADC1->CR2 |= (1U << ADC_SWSTART_BIT);

    /* Esperar fin de conversion */
    while (!(ADC1->SR & (1U << ADC_EOC_BIT))) {
        if (timeout == 0U) {
            return ADC_ERROR_TIMEOUT;
        }

        timeout--;
    }

    /* Leer resultado de 12 bits */
    *value = (uint16_t)(ADC1->DR & 0x0FFFU);

    return ADC_OK;
}

Adc_Status_t adc_init_triggered(void) {
    /* Habilitar clock ADC1 */
    RCC_APB2ENR |= (1U << 8U);

    /* Prescaler ADC /4: 90 MHz / 4 = 22.5 MHz */
    ADC_CCR &= ~(0x3U << 16U);
    ADC_CCR |=  (0x1U << 16U);

    /* Resolucion 12 bits */
    ADC1->CR1 &= ~(0x3U << 24U);

    /* Habilitar interrupcion EOC */
    ADC1->CR1 |= (1U << 5U);

    /* Trigger externo: TIM2 TRGO */
    ADC1->CR2 &= ~(0x3U << 28U);
    ADC1->CR2 |=  (0x1U << 28U);

    ADC1->CR2 &= ~(0xFU << 24U);
    ADC1->CR2 |=  (0x6U << 24U);

    /* 1 conversion en la secuencia */
    ADC1->SQR1 &= ~(0xFU << 20U);

    /* Canal 0 PA0 */
    ADC1->SQR3 &= ~(0x1FU << 0U);

    /* Tiempo de muestreo canal 0 */
    ADC1->SMPR2 &= ~(0x7U << 0U);
    ADC1->SMPR2 |=  (0x4U << 0U);

    /* Habilitar interrupcion ADC en NVIC: IRQ18 */
    (*(volatile uint32_t *)0xE000E100UL) |= (1U << 18U);

    /* Encender ADC */
    ADC1->CR2 |= (1U << 0U);

    return ADC_OK;
}
