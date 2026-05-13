#ifndef CONFIG_H
#define CONFIG_H

#include <stdint.h>

/* Clock */
#define SYSCLK_HZ        180000000UL
#define APB1_CLK_HZ       45000000UL
#define APB2_CLK_HZ       90000000UL

/* ADC */
#define SAMPLE_RATE_HZ    10000UL
#define BUFFER_SIZE       128U
#define TRIGGER_LEVEL     2048U
#define ADC_MAX_VALUE     4095U

/* UART */
#define UART_BAUDRATE     115200UL

/* Pines */
#define ADC_PIN           0U
#define OLED_SCL_PIN      8U
#define OLED_SDA_PIN      9U
#define LED_PIN           5U

#endif /* CONFIG_H */
