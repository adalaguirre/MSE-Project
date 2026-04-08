/**
 * @file    led.h
 * @brief   LED module – abstracts LD2 (PA5) on the NUCLEO-F446RE.
 * @author  Carlos Cayetano
 * @date    2026-04-07
 */

#ifndef LED_H
#define LED_H

#include "gpio_driver.h"

/* ── Hardware mapping (NUCLEO-F446RE User LED LD2) ─────────────────────── */
#define LED_PORT   GPIOA
#define LED_PIN    5U

/* ── Public API ─────────────────────────────────────────────────────────── */

/**
 * @brief  Initialise the LED pin as push-pull output.
 *         The LED is turned off after initialisation.
 */
void led_init(void);

/** @brief  Turn the LED on. */
void led_on(void);

/** @brief  Turn the LED off. */
void led_off(void);

/** @brief  Toggle the LED state. */
void led_toggle(void);

#endif /* LED_H */
