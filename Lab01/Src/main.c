/**
 * @file    main.c
 * @brief   Application – Toggle LD2 each time B1 is pressed (NUCLEO-F446RE).
 * @author  Carlos Cayetano
 * @date    2026-04-07
 *
 * Behaviour
 * ─────────
 *  • LED (PA5) is off at startup.
 *  • Each press of the user button (PC13) toggles the LED.
 *  • Software debounce: the button state must be stable for
 *    DEBOUNCE_CYCLES consecutive polling cycles before it is
 *    accepted as a valid edge.
 */

#include "led.h"
#include "button.h"

/* ─── Debounce configuration ────────────────────────────────────────────── */
/**
 * Number of consecutive identical samples required to validate a button edge.
 * Adjust to taste – at ~168 MHz with a simple for-loop each iteration is
 * roughly 1 µs, so 5 000 ≈ 5 ms.
 */
#define DEBOUNCE_CYCLES  5000U

/* ─── Private helper ────────────────────────────────────────────────────── */

/**
 * @brief  Busy-wait for approximately @p cycles iterations.
 *         Not accurate – used only for simple debounce.
 */
static void delay_cycles(volatile uint32_t cycles)
{
    while (cycles--) {
        __asm volatile ("nop");
    }
}

/**
 * @brief  Return 1 if the button is stably pressed (debounced).
 *
 * Reads the button DEBOUNCE_CYCLES times. Accepts the press only if
 * every single sample reports BUTTON_PRESSED.
 */
static int debounced_press(void)
{
    for (uint32_t i = 0U; i < DEBOUNCE_CYCLES; i++) {
        if (button_get_state() != BUTTON_PRESSED) {
            return 0;
        }
    }
    return 1;
}

/* ─── main ──────────────────────────────────────────────────────────────── */
int main(void)
{
    /* Task 2 – initialise LED module */
    led_init();

    /* Task 3 – initialise Button module */
    button_init();

    /* Task 4 – application loop */
    while (1)
    {
        /* Wait until a debounced press is detected */
        if (debounced_press()) {

            /* Toggle the LED on each valid press */
            led_toggle();

            /* Wait for button release before accepting another press */
            while (button_get_state() == BUTTON_PRESSED) {
                delay_cycles(100U);
            }

            /* Small delay after release to avoid phantom edges */
            delay_cycles(DEBOUNCE_CYCLES);
        }
    }

    /* Never reached */
    return 0;
}
