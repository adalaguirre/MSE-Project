/**
 * @file    button.h
 * @brief   Button module – abstracts B1 (PC13) on the NUCLEO-F446RE.
 * @author  Carlos Cayetano
 * @date    2026-04-07
 *
 * The user button B1 is connected to PC13 and is active-LOW (pressing the
 * button pulls the line to GND). A pull-up resistor is already present on the
 * NUCLEO board, but the driver also enables the internal pull-up for safety.
 */

#ifndef BUTTON_H
#define BUTTON_H

#include "gpio_driver.h"

/* ── Hardware mapping (NUCLEO-F446RE User Button B1) ───────────────────── */
#define BUTTON_PORT  GPIOC
#define BUTTON_PIN   13U

/** Logical button states (active-LOW hardware → inverted logic here) */
typedef enum {
    BUTTON_RELEASED = 0U,
    BUTTON_PRESSED  = 1U
} ButtonState_t;

/* ── Public API ─────────────────────────────────────────────────────────── */

/**
 * @brief  Initialise the button pin as input with pull-up.
 */
void button_init(void);

/**
 * @brief  Read the current button state.
 * @return BUTTON_PRESSED if the button is held down, BUTTON_RELEASED otherwise.
 */
ButtonState_t button_get_state(void);

#endif /* BUTTON_H */
