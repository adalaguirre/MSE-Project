/**
 * @file    gpio_driver.c
 * @brief   GPIO Driver implementation – STM32F446RE register-level access.
 * @author  Carlos Cayetano
 * @date    2026-04-07
 */

#include "gpio_driver.h"

/* ─── Private helper ────────────────────────────────────────────────────── */

/**
 * @brief  Validate port pointer and pin number.
 * @return GPIO_OK, GPIO_ERR_NULL_PORT, or GPIO_ERR_INVALID_PIN.
 */
static GPIO_Status_t validate(GPIO_RegDef_t *port, uint8_t pin)
{
    if (port == NULL) {
        return GPIO_ERR_NULL_PORT;
    }
    if (pin > 15U) {
        return GPIO_ERR_INVALID_PIN;
    }
    return GPIO_OK;
}

/* ─── GPIO_EnableClock ──────────────────────────────────────────────────── */
GPIO_Status_t GPIO_EnableClock(GPIO_RegDef_t *port)
{
    if (port == NULL) {
        return GPIO_ERR_NULL_PORT;
    }

    /* Map port address to its RCC_AHB1ENR enable bit */
    if      (port == GPIOA) { RCC_AHB1ENR |= RCC_AHB1ENR_GPIOAEN; }
    else if (port == GPIOB) { RCC_AHB1ENR |= RCC_AHB1ENR_GPIOBEN; }
    else if (port == GPIOC) { RCC_AHB1ENR |= RCC_AHB1ENR_GPIOCEN; }
    else if (port == GPIOD) { RCC_AHB1ENR |= RCC_AHB1ENR_GPIODEN; }
    else if (port == GPIOE) { RCC_AHB1ENR |= RCC_AHB1ENR_GPIOEEN; }
    else if (port == GPIOF) { RCC_AHB1ENR |= RCC_AHB1ENR_GPIOFEN; }
    else if (port == GPIOG) { RCC_AHB1ENR |= RCC_AHB1ENR_GPIOGEN; }
    else if (port == GPIOH) { RCC_AHB1ENR |= RCC_AHB1ENR_GPIOHEN; }
    else {
        return GPIO_ERR_NULL_PORT; /* Unknown port address */
    }

    return GPIO_OK;
}

/* ─── GPIO_Init ─────────────────────────────────────────────────────────── */
GPIO_Status_t GPIO_Init(const GPIO_Config_t *config)
{
    GPIO_Status_t status;

    /* Validate pointer and pin */
    if (config == NULL) {
        return GPIO_ERR_NULL_PORT;
    }
    status = validate(config->port, config->pin);
    if (status != GPIO_OK) {
        return status;
    }

    /* 1. Enable peripheral clock */
    status = GPIO_EnableClock(config->port);
    if (status != GPIO_OK) {
        return status;
    }

    uint8_t pin = config->pin;

    /* 2. MODER – 2 bits per pin [2n+1 : 2n] */
    config->port->MODER &= ~(0x3UL << (pin * 2U));
    config->port->MODER |=  ((uint32_t)config->mode << (pin * 2U));

    /* 3. OTYPER – 1 bit per pin [n] */
    config->port->OTYPER &= ~(0x1UL << pin);
    config->port->OTYPER |=  ((uint32_t)config->otype << pin);

    /* 4. OSPEEDR – 2 bits per pin [2n+1 : 2n] */
    config->port->OSPEEDR &= ~(0x3UL << (pin * 2U));
    config->port->OSPEEDR |=  ((uint32_t)config->speed << (pin * 2U));

    /* 5. PUPDR – 2 bits per pin [2n+1 : 2n] */
    config->port->PUPDR &= ~(0x3UL << (pin * 2U));
    config->port->PUPDR |=  ((uint32_t)config->pupd << (pin * 2U));

    return GPIO_OK;
}

/* ─── GPIO_WritePin ─────────────────────────────────────────────────────── */
GPIO_Status_t GPIO_WritePin(GPIO_RegDef_t *port, uint8_t pin, GPIO_PinState_t state)
{
    GPIO_Status_t status = validate(port, pin);
    if (status != GPIO_OK) {
        return status;
    }

    /*
     * BSRR register layout:
     *   Bits [15:0]  – BSy : writing 1 sets   ODR bit y
     *   Bits [31:16] – BRy : writing 1 resets ODR bit y
     * This is an atomic operation – no read-modify-write needed.
     */
    if (state == GPIO_PIN_SET) {
        port->BSRR = (1UL << pin);          /* Set   */
    } else {
        port->BSRR = (1UL << (pin + 16U));  /* Reset */
    }

    return GPIO_OK;
}

/* ─── GPIO_ReadPin ──────────────────────────────────────────────────────── */
GPIO_Status_t GPIO_ReadPin(GPIO_RegDef_t *port, uint8_t pin, GPIO_PinState_t *state)
{
    GPIO_Status_t status = validate(port, pin);
    if (status != GPIO_OK) {
        return status;
    }
    if (state == NULL) {
        return GPIO_ERR_NULL_PORT;
    }

    /* IDR bit n reflects the logic level present at the pin */
    *state = (GPIO_PinState_t)((port->IDR >> pin) & 0x1UL);

    return GPIO_OK;
}

/* ─── GPIO_TogglePin ────────────────────────────────────────────────────── */
GPIO_Status_t GPIO_TogglePin(GPIO_RegDef_t *port, uint8_t pin)
{
    GPIO_Status_t status = validate(port, pin);
    if (status != GPIO_OK) {
        return status;
    }

    /* XOR the corresponding ODR bit */
    port->ODR ^= (1UL << pin);

    return GPIO_OK;
}
