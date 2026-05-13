/**
 * @file uart.h
 * @brief USART2 Driver — Debug Serial Output
 * @author Daniel
 */
#ifndef UART_H
#define UART_H

#include <stdint.h>

#define USART2_BASE 0x40004400UL

typedef struct {
    volatile uint32_t SR;
    volatile uint32_t DR;
    volatile uint32_t BRR;
    volatile uint32_t CR1;
    volatile uint32_t CR2;
    volatile uint32_t CR3;
    volatile uint32_t GTPR;
} Usart_Registers_t;

#define USART2 ((Usart_Registers_t *)USART2_BASE)

#define USART_SR_TXE  (1U << 7U)
#define USART_SR_TC   (1U << 6U)
#define USART_SR_RXNE (1U << 5U)

typedef enum {
    UART_OK            = 0U,
    UART_ERROR_INVALID = 1U,
    UART_ERROR_TIMEOUT = 2U
} Uart_Status_t;

Uart_Status_t uart_init(void);
Uart_Status_t uart_sendByte(uint8_t byte);
Uart_Status_t uart_sendString(const char *str);

#endif /* UART_H */