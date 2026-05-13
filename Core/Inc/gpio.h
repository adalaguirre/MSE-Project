/**
 * @file    gpio.h
 * @brief   GPIO Pin Configuration Driver
 * @author  Adal
 */

#ifndef GPIO_H
#define GPIO_H

#include <stdint.h>
#include "clock.h"


typedef struct {
    volatile uint32_t MODER;
    volatile uint32_t OTYPER;
    volatile uint32_t OSPEEDR;
    volatile uint32_t PUPDR;
    volatile uint32_t IDR;
    volatile uint32_t ODR;
    volatile uint32_t BSRR;
    volatile uint32_t LCKR;
    volatile uint32_t AFRL;
    volatile uint32_t AFRH;
} Gpio_RegDef_t;

#define GPIOA ((Gpio_RegDef_t *)0x40020000UL)
#define GPIOB ((Gpio_RegDef_t *)0x40020400UL)



#define GPIO_MODE_INPUT   0U
#define GPIO_MODE_OUTPUT  1U
#define GPIO_MODE_AF      2U
#define GPIO_MODE_ANALOG  3U

typedef enum {
    GPIO_OK    = 0U,
    GPIO_ERROR = 1U
} Gpio_Status_t;

Gpio_Status_t gpio_init(void);

#endif /* GPIO_H */