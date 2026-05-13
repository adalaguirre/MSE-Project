/**
 * @file clock.c
 * @brief RCC Clock Configuration — PLL a 180 MHz
 * @details HSI (16 MHz) -> PLL -> SYSCLK 180 MHz
 * APB1 prescaler /4 -> 45 MHz
 * APB2 prescaler /2 -> 90 MHz
 * @author Adrian
 */

#include "clock.h"

/* PLL para 180 MHz desde HSI 16 MHz
 * SYSCLK = (HSI / PLLM) * PLLN / PLLP
 * 180 MHz = (16 / 8) * 180 / 2
 */
#define PLL_M   8U
#define PLL_N   180U
#define PLL_P   0U      /* 0b00 en PLLP = division entre 2 */

Clock_Status_t clock_init(void) {
    /* 1. Habilitar HSI y esperar a que esté listo */
    RCC_CR |= (1U << 0U);              /* HSION */
    while (!(RCC_CR & (1U << 1U)));    /* HSIRDY */

    /* 2. Configurar latencia de Flash para 180 MHz */
    FLASH_ACR &= ~(0xFU);
    FLASH_ACR |= (5U << 0U);           /* 5 wait states */
    FLASH_ACR |= (1U << 8U);           /* PRFTEN */
    FLASH_ACR |= (1U << 9U);           /* ICEN */
    FLASH_ACR |= (1U << 10U);          /* DCEN */

    /* 3. Configurar prescalers */
    RCC_CFGR &= ~(0xFU << 4U);         /* AHB /1 */

    RCC_CFGR &= ~(0x7U << 10U);
    RCC_CFGR |= (0x5U << 10U);         /* APB1 /4 */

    RCC_CFGR &= ~(0x7U << 13U);
    RCC_CFGR |= (0x4U << 13U);         /* APB2 /2 */

    /* 4. Configurar PLL: fuente HSI, M=8, N=180, P=2 */
    RCC_PLLCFGR = 0U;
    RCC_PLLCFGR |= (PLL_M << 0U);
    RCC_PLLCFGR |= (PLL_N << 6U);
    RCC_PLLCFGR |= (PLL_P << 16U);
    /* Bit 22 = 0 -> HSI como fuente del PLL */

    /* 5. Habilitar PLL y esperar */
    RCC_CR |= (1U << 24U);             /* PLLON */
    while (!(RCC_CR & (1U << 25U)));   /* PLLRDY */

    /* 6. Seleccionar PLL como SYSCLK */
    RCC_CFGR &= ~(0x3U << 0U);
    RCC_CFGR |= (0x2U << 0U);          /* SW = PLL */

    while (((RCC_CFGR >> 2U) & 0x3U) != 0x2U); /* SWS = PLL */

    return CLOCK_OK;
}
