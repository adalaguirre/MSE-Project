/******************************************************************************
* Copyright (C) 2026 by Carlos Villarreal - CETYS Universidad
*
* Redistribution, modification or use of this software in source or binary
* forms is permitted as long as the files maintain this copyright. Users are
* permitted to modify this and use it to learn about the field of embedded
* software. Carlos Villarreal and CETYS Universidad are not liable for any
* misuse of this material.
*
*****************************************************************************/
/**
* @file i2c.c
* @brief Implementacion del driver I2C1 bare-metal para STM32F446RE.
*
* Modo Fast (400 kHz), APB1 = 45 MHz.
* Los pines PB8 (SCL) y PB9 (SDA) deben estar configurados con AF4,
* open-drain y pull-up antes de llamar a i2c_init() (lo hace gpio_init()).
*
* Calculo de parametros:
*   FREQ  = 45          (APB1 en MHz)
*   CCR   = 45e6 / (3 * 400e3) = 37.5 -> 38   (Duty=0, Fast mode)
*   TRISE = (45e6 * 300ns) + 1 = 14.5 -> 15
*
* @author Team MSE - Adrian, Daniel, Carlos, Adal
* @date 2026-05-28
*
*/

/*** Includes ***/
#include "i2c.h"
#include "clock.h"   /* RCC_APB1ENR */

/*** Preprocessor Definitions ***/

/* Mapa de registros I2C1 */
typedef struct {
    volatile uint32_t CR1;
    volatile uint32_t CR2;
    volatile uint32_t OAR1;
    volatile uint32_t OAR2;
    volatile uint32_t DR;
    volatile uint32_t SR1;
    volatile uint32_t SR2;
    volatile uint32_t CCR;
    volatile uint32_t TRISE;
    volatile uint32_t FLTR;
} I2c_Regs_t;

#define I2C1_DEV    ((I2c_Regs_t *)0x40005400UL)

/* CR1 bits */
#define CR1_PE      (1U << 0U)    /* Peripheral Enable */
#define CR1_START   (1U << 8U)    /* Generar condicion START */
#define CR1_STOP    (1U << 9U)    /* Generar condicion STOP */
#define CR1_SWRST   (1U << 15U)   /* Software Reset */

/* SR1 bits */
#define SR1_SB      (1U << 0U)    /* Start Bit generado */
#define SR1_ADDR    (1U << 1U)    /* Direccion enviada/recibida */
#define SR1_BTF     (1U << 2U)    /* Byte Transfer Finished */
#define SR1_TXE     (1U << 7U)    /* TX Data Register vacio */
#define SR1_AF      (1U << 10U)   /* Acknowledge Failure */

/* SR2 bits */
#define SR2_BUSY    (1U << 1U)    /* Bus ocupado */

/* Clock parameters para 400 kHz Fast Mode con APB1 = 50 MHz (F411).
 *
 * Fast Mode, Duty=0 (tlow = 2*thigh):
 *   CCR   = Fpclk1 / (3 * Fscl) = 50e6 / (3 * 400e3) = 41.67 -> 42
 *           Bit 15 (CCR_FS) = 1 activa Fast Mode
 *   TRISE = (Fpclk1 * 300ns) + 1 = (50e6 * 300e-9) + 1 = 15 + 1 = 16
 *
 * Resultado: ~4x mas rapido que Standard Mode -> OLED fluido a ~30 FPS.
 * El tiempo entre transacciones (capturas del ADC) provee el hold time
 * suficiente para que el bus quede libre sin delay adicional.
 */
#define I2C_FREQ_MHZ    50U
#define I2C_CCR_VAL     ((1U << 15U) | 42U)   /* FS=1, CCR=42 -> 400 kHz */
#define I2C_TRISE_VAL   16U

/* RCC: habilitar I2C1 en APB1 (bit 21) */
#define RCC_I2C1EN      (1U << 21U)

/*** Local Variables ***/

/*** Function Prototypes ***/
static I2c_Status_t i2c_wait_flag(uint32_t flag);

/*** Function Definitions ***/

/**
 * @brief Espera hasta que el flag indicado aparezca en SR1.
 */
static I2c_Status_t i2c_wait_flag(uint32_t flag) {
    uint32_t timeout = I2C_TIMEOUT_DEFAULT;
    while (!(I2C1_DEV->SR1 & flag)) {
        if (--timeout == 0U) {
            return I2C_ERROR_TIMEOUT;
        }
    }
    return I2C_OK;
}

I2c_Status_t i2c_init(void) {
    /* 1. Habilitar clock de I2C1 en APB1 */
    RCC_APB1ENR |= RCC_I2C1EN;

    /* 2. Reset por software para iniciar limpio */
    I2C1_DEV->CR1 |= CR1_SWRST;
    I2C1_DEV->CR1 &= ~CR1_SWRST;

    /* 3. Frecuencia del periférico (valor en MHz de APB1) */
    I2C1_DEV->CR2 = I2C_FREQ_MHZ;

    /* 4. Fast mode (400 kHz): bit FS=1 en CCR, CCR=42 */
    I2C1_DEV->CCR = I2C_CCR_VAL;

    /* 5. Tiempo maximo de subida: 300 ns en fast mode */
    I2C1_DEV->TRISE = I2C_TRISE_VAL;

    /* 6. Encender el periferico */
    I2C1_DEV->CR1 |= CR1_PE;

    return I2C_OK;
}

I2c_Status_t i2c_write(uint8_t dev_addr, const uint8_t *data, uint32_t len) {
    I2c_Status_t status;
    uint32_t i;
    uint32_t timeout = I2C_TIMEOUT_DEFAULT;

    if ((data == (void *)0) || (len == 0U)) {
        return I2C_ERROR_INVALID;
    }

    /* 0. Esperar a que el bus quede libre antes de generar START.
     *    Esto evita que transacciones consecutivas se "atasquen". */
    while (I2C1_DEV->SR2 & SR2_BUSY) {
        if (--timeout == 0U) {
            /* Bus atascado: reset por software y reiniciar */
            I2C1_DEV->CR1 |= CR1_SWRST;
            I2C1_DEV->CR1 &= ~CR1_SWRST;
            I2C1_DEV->CR2    = I2C_FREQ_MHZ;
            I2C1_DEV->CCR    = I2C_CCR_VAL;
            I2C1_DEV->TRISE  = I2C_TRISE_VAL;
            I2C1_DEV->CR1   |= CR1_PE;
            return I2C_ERROR_TIMEOUT;
        }
    }

    /* 1. Generar condicion START */
    I2C1_DEV->CR1 |= CR1_START;
    status = i2c_wait_flag(SR1_SB);
    if (status != I2C_OK) { return status; }

    /* 2. Enviar direccion de 7 bits + bit Write (0) */
    I2C1_DEV->DR = (uint32_t)(dev_addr << 1U);
    status = i2c_wait_flag(SR1_ADDR);
    if (status != I2C_OK) {
        /* Si el device no responde, limpiar y salir */
        if (I2C1_DEV->SR1 & SR1_AF) {
            I2C1_DEV->SR1 &= ~SR1_AF;
        }
        I2C1_DEV->CR1 |= CR1_STOP;
        return (status == I2C_ERROR_TIMEOUT) ? I2C_ERROR_AF : status;
    }
    /* Limpiar flag ADDR leyendo SR1 seguido de SR2 */
    (void)I2C1_DEV->SR1;
    (void)I2C1_DEV->SR2;

    /* 3. Enviar todos los bytes */
    for (i = 0U; i < len; i++) {
        status = i2c_wait_flag(SR1_TXE);
        if (status != I2C_OK) { return status; }
        I2C1_DEV->DR = (uint32_t)data[i];
    }

    /* 4. Esperar a que el ultimo byte se transfiera completamente */
    status = i2c_wait_flag(SR1_BTF);
    if (status != I2C_OK) { return status; }

    /* 5. Generar condicion STOP */
    I2C1_DEV->CR1 |= CR1_STOP;

    /* 6. Esperar a que el bus quede libre antes de salir.
     *    Esto garantiza que la siguiente transaccion no encuentre BUSY. */
    timeout = I2C_TIMEOUT_DEFAULT;
    while (I2C1_DEV->SR2 & SR2_BUSY) {
        if (--timeout == 0U) { break; }
    }

    return I2C_OK;
}
