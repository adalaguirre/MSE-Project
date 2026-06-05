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
/* BRR = APB1 / baudrate
 * 115200 baud -> BRR = 434  (original, referencia)
 * 921600 baud -> BRR = 54   (50e6/921600 = 54.25 -> error 0.47%, dentro del 2%)
 * Usamos 921600 para el streaming binario: 516 bytes en ~5.6 ms vs ~45 ms.
 */
#define UART_BRR_115200 434U
#define UART_BRR_921600  54U

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

    USART2->BRR  = UART_BRR_921600;
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

Uart_Status_t uart_sendBytes(const uint8_t *buf, uint16_t len) {
    uint16_t i;
    if (buf == (void *)0) { return UART_ERROR_INVALID; }
    for (i = 0U; i < len; i++) {
        uart_sendByte(buf[i]);
    }
    return UART_OK;
}

Uart_Status_t uart_sendUInt16(uint16_t value) {
    char buf[6];
    uint8_t idx = 5U;
    buf[idx] = '\0';
    if (value == 0U) {
        buf[--idx] = '0';
    } else {
        while (value > 0U) {
            buf[--idx] = (char)('0' + (value % 10U));
            value /= 10U;
        }
    }
    return uart_sendString(&buf[idx]);
}
