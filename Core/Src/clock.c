/**
 * @file clock.c
 * @brief Configuracion del clock del sistema
 * @author Adrian
 */

#include "clock.h"

/* PLL: HSI 16 MHz -> SYSCLK 180 MHz
 * (16 MHz / 8) * 180 / 2 = 180 MHz
 */
#define PLL_M   8U
#define PLL_N   180U
#define PLL_P   0U

Clock_Status_t clock_init(void) {
    /* Encender HSI */
    RCC_CR |= (1U << 0U);
    while (!(RCC_CR & (1U << 1U)));

    /* Flash para trabajar a 180 MHz */
    FLASH_ACR &= ~(0xFU);
    FLASH_ACR |= (5U << 0U);
    FLASH_ACR |= (1U << 8U);
    FLASH_ACR |= (1U << 9U);
    FLASH_ACR |= (1U << 10U);

    /* Prescalers: AHB /1, APB1 /4, APB2 /2 */
    RCC_CFGR &= ~(0xFU << 4U);

    RCC_CFGR &= ~(0x7U << 10U);
    RCC_CFGR |= (0x5U << 10U);

    RCC_CFGR &= ~(0x7U << 13U);
    RCC_CFGR |= (0x4U << 13U);

    /* Configuracion del PLL con HSI */
    RCC_PLLCFGR = 0U;
    RCC_PLLCFGR |= (PLL_M << 0U);
    RCC_PLLCFGR |= (PLL_N << 6U);
    RCC_PLLCFGR |= (PLL_P << 16U);

    /* Encender PLL */
    RCC_CR |= (1U << 24U);
    while (!(RCC_CR & (1U << 25U)));

    /* Usar PLL como clock principal */
    RCC_CFGR &= ~(0x3U << 0U);
    RCC_CFGR |= (0x2U << 0U);

    while (((RCC_CFGR >> 2U) & 0x3U) != 0x2U);

    return CLOCK_OK;
}
