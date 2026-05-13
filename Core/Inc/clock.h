/**
 * @file clock.h
 * @brief RCC Clock Configuration Driver
 * @details Configura el PLL para llevar el sistema a 180 MHz.
 * APB1 = 45 MHz, APB2 = 90 MHz.
 * @author Adrian
 */

#ifndef CLOCK_H
#define CLOCK_H

#include <stdint.h>

/* RCC Register Map */
#define RCC_BASE        0x40023800UL
#define RCC_CR          (*(volatile uint32_t *)(RCC_BASE + 0x00UL))
#define RCC_PLLCFGR     (*(volatile uint32_t *)(RCC_BASE + 0x04UL))
#define RCC_CFGR        (*(volatile uint32_t *)(RCC_BASE + 0x08UL))
#define RCC_AHB1ENR     (*(volatile uint32_t *)(RCC_BASE + 0x30UL))
#define RCC_APB1ENR     (*(volatile uint32_t *)(RCC_BASE + 0x40UL))
#define RCC_APB2ENR     (*(volatile uint32_t *)(RCC_BASE + 0x44UL))

/* Flash Access Control */
#define FLASH_BASE      0x40023C00UL
#define FLASH_ACR       (*(volatile uint32_t *)(FLASH_BASE + 0x00UL))

/* Status codes */
typedef enum {
    CLOCK_OK = 0U,
    CLOCK_ERROR = 1U
} Clock_Status_t;

/* API */
Clock_Status_t clock_init(void);

#endif /* CLOCK_H */
