/**
 * @file timer.c
 * @brief TIM2 configurado para Fs = 10 kHz
 * @author Adrian
 */

#include "timer.h"
#include "clock.h"

/* TIM2EN: bit 0 de RCC_APB1ENR */
#define TIM2_EN_BIT 0U

Timer_Status_t timer_init(void) {
    /* Habilitar clock de TIM2 */
    RCC_APB1ENR |= (1U << TIM2_EN_BIT);

    /* Apagar TIM2 mientras se configura */
    TIM2->CR1 &= ~(1U << 0U);

    /* APB1 = 45 MHz
     * PSC = 44  -> tick = 1 us
     * ARR = 99  -> periodo = 100 us = 10 kHz
     */
    TIM2->PSC = 44U;
    TIM2->ARR = 99U;

    /* TRGO en Update Event: MMS = 010 en CR2 bits 6:4 */
    TIM2->CR2 &= ~(0x7U << 4U);
    TIM2->CR2 |=  (0x2U << 4U);

    /* Cargar PSC y ARR */
    TIM2->EGR |= (1U << 0U);

    /* Encender TIM2 */
    TIM2->CR1 |= (1U << 0U);

    return TIMER_OK;
}
