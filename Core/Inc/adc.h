/**
 * @file adc.h
 * @brief ADC1 Driver para entrada analogica PA0
 * @author Adrian
 */

#ifndef ADC_H
#define ADC_H

#include <stdint.h>

/* ADC Register Map */
typedef struct {
    volatile uint32_t SR;
    volatile uint32_t CR1;
    volatile uint32_t CR2;
    volatile uint32_t SMPR1;
    volatile uint32_t SMPR2;
    volatile uint32_t JOFR1;
    volatile uint32_t JOFR2;
    volatile uint32_t JOFR3;
    volatile uint32_t JOFR4;
    volatile uint32_t HTR;
    volatile uint32_t LTR;
    volatile uint32_t SQR1;
    volatile uint32_t SQR2;
    volatile uint32_t SQR3;
    volatile uint32_t JSQR;
    volatile uint32_t JDR1;
    volatile uint32_t JDR2;
    volatile uint32_t JDR3;
    volatile uint32_t JDR4;
    volatile uint32_t DR;
} Adc_Registers_t;

#define ADC1 ((Adc_Registers_t *)0x40012000UL)

/* ADC Common Control Register */
#define ADC_CCR (*(volatile uint32_t *)0x40012304UL)

/* Status codes */
typedef enum {
    ADC_OK = 0U,
    ADC_ERROR_INVALID = 1U,
    ADC_ERROR_TIMEOUT = 2U
} Adc_Status_t;

/* API */
Adc_Status_t adc_init(void);
Adc_Status_t adc_read(uint16_t *value);

#endif /* ADC_H */
