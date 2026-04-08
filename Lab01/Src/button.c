/**
 * @file    button.c
 * @brief   Button module implementation – B1 (PC13) on NUCLEO-F446RE.
 * @author  Carlos Cayetano
 * @date    2026-04-07
 */

#include "button.h"

/* ─── button_init ───────────────────────────────────────────────────────── */
void button_init(void)
{
    GPIO_Config_t cfg = {
        .port  = BUTTON_PORT,
        .pin   = BUTTON_PIN,
        .mode  = GPIO_MODE_INPUT,
        .otype = GPIO_OTYPE_PP,     /* Irrelevant for inputs */
        .speed = GPIO_SPEED_LOW,    /* Irrelevant for inputs */
        .pupd  = GPIO_PUPD_PULLUP   /* Internal pull-up – button is active-LOW */
    };

    GPIO_Init(&cfg);
}

/* ─── button_get_state ──────────────────────────────────────────────────── */
ButtonState_t button_get_state(void)
{
    GPIO_PinState_t raw = GPIO_PIN_RESET;
    GPIO_ReadPin(BUTTON_PORT, BUTTON_PIN, &raw);

    /*
     * B1 is active-LOW:
     *   IDR = 0  →  button pressed  →  return BUTTON_PRESSED
     *   IDR = 1  →  button released →  return BUTTON_RELEASED
     */
    return (raw == GPIO_PIN_RESET) ? BUTTON_PRESSED : BUTTON_RELEASED;
}
