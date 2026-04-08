/**
 * @file    led.c
 * @brief   LED module implementation – LD2 (PA5) on NUCLEO-F446RE.
 * @author  Carlos Cayetano
 * @date    2026-04-07
 */

#include "led.h"

/* ─── led_init ──────────────────────────────────────────────────────────── */
void led_init(void)
{
    GPIO_Config_t cfg = {
        .port  = LED_PORT,
        .pin   = LED_PIN,
        .mode  = GPIO_MODE_OUTPUT,
        .otype = GPIO_OTYPE_PP,       /* Push-pull output */
        .speed = GPIO_SPEED_LOW,      /* Low speed is enough for an LED */
        .pupd  = GPIO_PUPD_NONE       /* No pull resistor needed */
    };

    GPIO_Init(&cfg);

    /* Ensure LED is off at startup */
    led_off();
}

/* ─── led_on ────────────────────────────────────────────────────────────── */
void led_on(void)
{
    GPIO_WritePin(LED_PORT, LED_PIN, GPIO_PIN_SET);
}

/* ─── led_off ───────────────────────────────────────────────────────────── */
void led_off(void)
{
    GPIO_WritePin(LED_PORT, LED_PIN, GPIO_PIN_RESET);
}

/* ─── led_toggle ────────────────────────────────────────────────────────── */
void led_toggle(void)
{
    GPIO_TogglePin(LED_PORT, LED_PIN);
}
