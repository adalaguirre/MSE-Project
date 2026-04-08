/**
 * @file    gpio_driver.h
 * @brief   GPIO Driver – Register-level API for STM32F446RE (NUCLEO-F446RE)
 * @author  Carlos Cayetano
 * @date    2026-04-07
 *
 * Functional Requirements covered: FR-1 … FR-10
 *   FR-1  : Driver shall support ports A–H.
 *   FR-2  : Driver shall support pins 0–15.
 *   FR-3  : Driver shall configure pin mode (Input / Output / AF / Analog).
 *   FR-4  : Driver shall configure output type (Push-Pull / Open-Drain).
 *   FR-5  : Driver shall configure output speed (Low / Medium / High / Very-High).
 *   FR-6  : Driver shall configure pull-up / pull-down / no-pull resistors.
 *   FR-7  : Driver shall write a digital value to an output pin.
 *   FR-8  : Driver shall read a digital value from an input pin.
 *   FR-9  : Driver shall toggle an output pin.
 *   FR-10 : Driver shall return an error code for invalid port or pin arguments.
 */

#ifndef GPIO_DRIVER_H
#define GPIO_DRIVER_H

#include <stdint.h>

/* ─── Peripheral base addresses ──────────────────────────────────────────── */
#define GPIOA_BASE  0x40020000UL
#define GPIOB_BASE  0x40020400UL
#define GPIOC_BASE  0x40020800UL
#define GPIOD_BASE  0x40020C00UL
#define GPIOE_BASE  0x40021000UL
#define GPIOF_BASE  0x40021400UL
#define GPIOG_BASE  0x40021800UL
#define GPIOH_BASE  0x40021C00UL

#define RCC_BASE    0x40023800UL
/** RCC AHB1 peripheral clock enable register */
#define RCC_AHB1ENR (*(volatile uint32_t *)(RCC_BASE + 0x30UL))

/* RCC_AHB1ENR bit positions for GPIO ports */
#define RCC_AHB1ENR_GPIOAEN  (1UL << 0)
#define RCC_AHB1ENR_GPIOBEN  (1UL << 1)
#define RCC_AHB1ENR_GPIOCEN  (1UL << 2)
#define RCC_AHB1ENR_GPIODEN  (1UL << 3)
#define RCC_AHB1ENR_GPIOEEN  (1UL << 4)
#define RCC_AHB1ENR_GPIOFEN  (1UL << 5)
#define RCC_AHB1ENR_GPIOGEN  (1UL << 6)
#define RCC_AHB1ENR_GPIOHEN  (1UL << 7)

/* ─── GPIO register map ───────────────────────────────────────────────────── */
typedef struct {
    volatile uint32_t MODER;    /*!< Mode register             offset 0x00 */
    volatile uint32_t OTYPER;   /*!< Output type register      offset 0x04 */
    volatile uint32_t OSPEEDR;  /*!< Output speed register     offset 0x08 */
    volatile uint32_t PUPDR;    /*!< Pull-up/pull-down register offset 0x0C */
    volatile uint32_t IDR;      /*!< Input data register       offset 0x10 */
    volatile uint32_t ODR;      /*!< Output data register      offset 0x14 */
    volatile uint32_t BSRR;     /*!< Bit set/reset register    offset 0x18 */
    volatile uint32_t LCKR;     /*!< Configuration lock reg    offset 0x1C */
    volatile uint32_t AFRL;     /*!< Alt. function low reg     offset 0x20 */
    volatile uint32_t AFRH;     /*!< Alt. function high reg    offset 0x24 */
} GPIO_RegDef_t;

/* Port accessor macros */
#define GPIOA  ((GPIO_RegDef_t *)GPIOA_BASE)
#define GPIOB  ((GPIO_RegDef_t *)GPIOB_BASE)
#define GPIOC  ((GPIO_RegDef_t *)GPIOC_BASE)
#define GPIOD  ((GPIO_RegDef_t *)GPIOD_BASE)
#define GPIOE  ((GPIO_RegDef_t *)GPIOE_BASE)
#define GPIOF  ((GPIO_RegDef_t *)GPIOF_BASE)
#define GPIOG  ((GPIO_RegDef_t *)GPIOG_BASE)
#define GPIOH  ((GPIO_RegDef_t *)GPIOH_BASE)

/* ─── Enumerations ────────────────────────────────────────────────────────── */

/** FR-3: Pin mode */
typedef enum {
    GPIO_MODE_INPUT  = 0x00U, /*!< Digital input      */
    GPIO_MODE_OUTPUT = 0x01U, /*!< Digital output     */
    GPIO_MODE_AF     = 0x02U, /*!< Alternate function */
    GPIO_MODE_ANALOG = 0x03U  /*!< Analog             */
} GPIO_Mode_t;

/** FR-4: Output type */
typedef enum {
    GPIO_OTYPE_PP = 0U, /*!< Push-pull  */
    GPIO_OTYPE_OD = 1U  /*!< Open-drain */
} GPIO_OType_t;

/** FR-5: Output speed */
typedef enum {
    GPIO_SPEED_LOW    = 0x00U, /*!< Low speed      (≤8  MHz)  */
    GPIO_SPEED_MEDIUM = 0x01U, /*!< Medium speed   (≤50 MHz)  */
    GPIO_SPEED_HIGH   = 0x02U, /*!< High speed     (≤100 MHz) */
    GPIO_SPEED_VHIGH  = 0x03U  /*!< Very-high speed(≤180 MHz) */
} GPIO_Speed_t;

/** FR-6: Pull-up / pull-down */
typedef enum {
    GPIO_PUPD_NONE     = 0x00U, /*!< No pull         */
    GPIO_PUPD_PULLUP   = 0x01U, /*!< Pull-up         */
    GPIO_PUPD_PULLDOWN = 0x02U  /*!< Pull-down       */
} GPIO_PuPd_t;

/** Pin logical state */
typedef enum {
    GPIO_PIN_RESET = 0U,
    GPIO_PIN_SET   = 1U
} GPIO_PinState_t;

/** FR-10: Return codes */
typedef enum {
    GPIO_OK            =  0,  /*!< Operation successful        */
    GPIO_ERR_NULL_PORT = -1,  /*!< NULL port pointer           */
    GPIO_ERR_INVALID_PIN = -2 /*!< Pin number out of range     */
} GPIO_Status_t;

/* ─── Configuration structure ────────────────────────────────────────────── */
typedef struct {
    GPIO_RegDef_t *port;   /*!< GPIO port (GPIOA … GPIOH)  */
    uint8_t        pin;    /*!< Pin number 0–15             */
    GPIO_Mode_t    mode;   /*!< Pin mode                    */
    GPIO_OType_t   otype;  /*!< Output type                 */
    GPIO_Speed_t   speed;  /*!< Output speed                */
    GPIO_PuPd_t    pupd;   /*!< Pull resistor               */
} GPIO_Config_t;

/* ─── Public API ─────────────────────────────────────────────────────────── */

/**
 * @brief  Initialise a GPIO pin according to @p config.           (FR-1…FR-6)
 * @param  config  Pointer to a filled GPIO_Config_t structure.
 * @return GPIO_OK on success, negative error code on failure.     (FR-10)
 */
GPIO_Status_t GPIO_Init(const GPIO_Config_t *config);

/**
 * @brief  Write a logical level to an output pin.                 (FR-7)
 * @param  port   GPIO port pointer.
 * @param  pin    Pin number (0–15).
 * @param  state  GPIO_PIN_SET or GPIO_PIN_RESET.
 * @return GPIO_OK on success, negative error code on failure.     (FR-10)
 */
GPIO_Status_t GPIO_WritePin(GPIO_RegDef_t *port, uint8_t pin, GPIO_PinState_t state);

/**
 * @brief  Read the logical level of a pin.                        (FR-8)
 * @param  port   GPIO port pointer.
 * @param  pin    Pin number (0–15).
 * @param  state  Output parameter – receives GPIO_PIN_SET / RESET.
 * @return GPIO_OK on success, negative error code on failure.     (FR-10)
 */
GPIO_Status_t GPIO_ReadPin(GPIO_RegDef_t *port, uint8_t pin, GPIO_PinState_t *state);

/**
 * @brief  Toggle the output level of a pin.                       (FR-9)
 * @param  port  GPIO port pointer.
 * @param  pin   Pin number (0–15).
 * @return GPIO_OK on success, negative error code on failure.     (FR-10)
 */
GPIO_Status_t GPIO_TogglePin(GPIO_RegDef_t *port, uint8_t pin);

/**
 * @brief  Enable the AHB1 peripheral clock for @p port.           (FR-1)
 * @param  port  GPIO port pointer.
 * @return GPIO_OK on success, GPIO_ERR_NULL_PORT if port is NULL.
 */
GPIO_Status_t GPIO_EnableClock(GPIO_RegDef_t *port);

#endif /* GPIO_DRIVER_H */
