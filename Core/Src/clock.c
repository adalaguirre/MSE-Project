/**
 * @file clock.c
 * @brief Configuracion del clock del sistema
 * @author Adrian
 */

#include "clock.h"

/* PLL: HSI 16 MHz -> SYSCLK 100 MHz  (STM32F411RE, max = 100 MHz)
 * SYSCLK = (16 MHz / PLL_M) * PLL_N / PLL_P
 *        = (16 / 8) * 100 / 2 = 100 MHz
 * APB1   = 100 / 2 = 50 MHz  (max 50 MHz)
 * APB2   = 100 / 1 = 100 MHz (max 100 MHz)
 */
#define PLL_M   8U     /* VCO input  = 16/8 = 2 MHz  */
#define PLL_N   100U   /* VCO output = 2*100 = 200 MHz */
#define PLL_P   0U     /* PLLP[1:0] = 00 -> /2 -> 100 MHz */

Clock_Status_t clock_init(void) {
    /* Encender HSI */
    RCC_CR |= (1U << 0U);
    while (!(RCC_CR & (1U << 1U)));

    /* Flash: 3 wait states para 100 MHz @ 3.3V (F411 RM tabla 6) */
    FLASH_ACR &= ~(0xFU);
    FLASH_ACR |= (3U << 0U);   /* LATENCY = 3 */
    FLASH_ACR |= (1U << 8U);   /* PRFTEN  */
    FLASH_ACR |= (1U << 9U);   /* ICEN    */
    FLASH_ACR |= (1U << 10U);  /* DCEN    */

    /* Prescalers: AHB /1, APB1 /2, APB2 /1 */
    RCC_CFGR &= ~(0xFU << 4U);   /* AHB /1  */

    RCC_CFGR &= ~(0x7U << 10U);
    RCC_CFGR |=  (0x4U << 10U);  /* APB1 /2 -> 50 MHz */

    RCC_CFGR &= ~(0x7U << 13U);  /* APB2 /1 -> 100 MHz */

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
