/**
 * @file    gpio.c
 * @brief   Configuracion de todos los pines del proyecto
 * @author  Adal
 */

#include "gpio.h"

Gpio_Status_t gpio_init(void) {

    /* Habilitar clocks GPIOA y GPIOB */
    RCC_AHB1ENR |= (1U << 0U);   /* GPIOAEN */
    RCC_AHB1ENR |= (1U << 1U);   /* GPIOBEN */

    /* PA0 - ADC1_IN0 - Analogico */
    GPIOA->MODER &= ~(3U << 0U);
    GPIOA->MODER |=  (3U << 0U);

    /* PA2 - USART2_TX - AF7 */
    GPIOA->MODER &= ~(3U << 4U);
    GPIOA->MODER |=  (2U << 4U);
    GPIOA->AFRL  &= ~(0xFU << 8U);
    GPIOA->AFRL  |=  (7U   << 8U);

    /* PA3 - USART2_RX - AF7 */
    GPIOA->MODER &= ~(3U << 6U);
    GPIOA->MODER |=  (2U << 6U);
    GPIOA->AFRL  &= ~(0xFU << 12U);
    GPIOA->AFRL  |=  (7U   << 12U);

    /* PA5 - LED LD2 - Output */
    GPIOA->MODER &= ~(3U << 10U);
    GPIOA->MODER |=  (1U << 10U);

    /* PB8 - I2C1_SCL - AF4, open-drain, pull-up */
    GPIOB->MODER  &= ~(3U << 16U);
    GPIOB->MODER  |=  (2U << 16U);
    GPIOB->AFRH   &= ~(0xFU << 0U);
    GPIOB->AFRH   |=  (4U   << 0U);
    GPIOB->OTYPER |=  (1U << 8U);
    GPIOB->PUPDR  &= ~(3U << 16U);
    GPIOB->PUPDR  |=  (1U << 16U);

    /* PB9 - I2C1_SDA - AF4, open-drain, pull-up */
    GPIOB->MODER  &= ~(3U << 18U);
    GPIOB->MODER  |=  (2U << 18U);
    GPIOB->AFRH   &= ~(0xFU << 4U);
    GPIOB->AFRH   |=  (4U   << 4U);
    GPIOB->OTYPER |=  (1U << 9U);
    GPIOB->PUPDR  &= ~(3U << 18U);
    GPIOB->PUPDR  |=  (1U << 18U);

    return GPIO_OK;
}