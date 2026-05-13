/**
 * @file uart.c
 * @brief USART2 bare metal driver — TX polling
 * @author Daniel
 */
#include "uart.h"
#include "clock.h"

#define GPIOA_BASE  0x40020000UL
#define GPIOA_MODER (*(volatile uint32_t *)(GPIOA_BASE + 0x00UL))
#define GPIOA_AFRL  (*(volatile uint32_t *)(GPIOA_BASE + 0x20UL))

#define AF7             7U
#define UART_BRR_115200 391U

Uart_Status_t uart_init(void) {
    RCC_AHB1ENR |= (1U << 0U);
    RCC_APB1ENR |= (1U << 17U);

    GPIOA_MODER &= ~(3U << 4U);
    GPIOA_MODER |=  (2U << 4U);
    GPIOA_MODER &= ~(3U << 6U);
    GPIOA_MODER |=  (2U << 6U);

    GPIOA_AFRL &= ~(0xFU << 8U);
    GPIOA_AFRL |=  (AF7  << 8U);
    GPIOA_AFRL &= ~(0xFU << 12U);
    GPIOA_AFRL |=  (AF7  << 12U);

    USART2->BRR  = UART_BRR_115200;
    USART2->CR1 |= (1U << 3U);
    USART2->CR1 |= (1U << 2U);
    USART2->CR1 |= (1U << 13U);

    return UART_OK;
}

Uart_Status_t uart_sendByte(uint8_t byte) {
    while (!(USART2->SR & USART_SR_TXE));
    USART2->DR = (uint32_t)byte;
    return UART_OK;
}

Uart_Status_t uart_sendString(const char *str) {
    if (str == (void *)0) { return UART_ERROR_INVALID; }
    while (*str != '\0') {
        uart_sendByte((uint8_t)*str);
        str++;
    }
    return UART_OK;
}