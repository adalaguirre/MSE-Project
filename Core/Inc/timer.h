/**
 * @file timer.h
 * @brief TIM2 Driver para frecuencia de muestreo del ADC
 * @author Adrian
 */

#ifndef TIMER_H
#define TIMER_H

#include <stdint.h>

/* TIM2 Register Map */
typedef struct {
    volatile uint32_t CR1;
    volatile uint32_t CR2;
    volatile uint32_t SMCR;
    volatile uint32_t DIER;
    volatile uint32_t SR;
    volatile uint32_t EGR;
    volatile uint32_t CCMR1;
    volatile uint32_t CCMR2;
    volatile uint32_t CCER;
    volatile uint32_t CNT;
    volatile uint32_t PSC;
    volatile uint32_t ARR;
} Tim_RegDef_t;

#define TIM2 ((Tim_RegDef_t *)0x40000000UL)

/* Status codes */
typedef enum {
    TIMER_OK = 0U,
    TIMER_ERROR = 1U
} Timer_Status_t;

/* API */
Timer_Status_t timer_init(void);

#endif /* TIMER_H */
