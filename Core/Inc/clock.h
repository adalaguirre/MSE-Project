/**
 * @file clock.h
 * @brief Driver basico para configurar el clock del sistema
 * @author Adrian
 */

#ifndef CLOCK_H
#define CLOCK_H

#include <stdint.h>

/* RCC */
#define RCC_BASE        0x40023800UL
#define RCC_CR          (*(volatile uint32_t *)(RCC_BASE + 0x00UL))
#define RCC_PLLCFGR     (*(volatile uint32_t *)(RCC_BASE + 0x04UL))
#define RCC_CFGR        (*(volatile uint32_t *)(RCC_BASE + 0x08UL))
#define RCC_AHB1ENR     (*(volatile uint32_t *)(RCC_BASE + 0x30UL))
#define RCC_APB1ENR     (*(volatile uint32_t *)(RCC_BASE + 0x40UL))
#define RCC_APB2ENR     (*(volatile uint32_t *)(RCC_BASE + 0x44UL))

/* FLASH */
#define FLASH_BASE      0x40023C00UL
#define FLASH_ACR       (*(volatile uint32_t *)(FLASH_BASE + 0x00UL))

typedef enum {
    CLOCK_OK = 0U,
    CLOCK_ERROR = 1U
} Clock_Status_t;

Clock_Status_t clock_init(void);

#endif /* CLOCK_H */
