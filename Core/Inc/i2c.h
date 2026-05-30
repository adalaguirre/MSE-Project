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
* @file i2c.h
* @brief Driver bare-metal para I2C1 en STM32F446RE.
*
* Configura I2C1 en modo rapido (400 kHz) con APB1 = 45 MHz.
* Pines: PB8 = SCL, PB9 = SDA (AF4, open-drain con pull-up).
* Usado principalmente para comunicacion con la pantalla OLED SSD1306.
*
* @author Team MSE - Adrian, Daniel, Carlos, Adal
* @date 2026-05-28
*
*/

#ifndef __I2C_H__
#define __I2C_H__

/*** Includes ***/
#include <stdint.h>

/*** Preprocessor Definitions ***/
#define I2C_TIMEOUT_DEFAULT  100000U

/*** Type Prototypes ***/
typedef enum {
    I2C_OK            = 0U,
    I2C_ERROR_TIMEOUT = 1U,
    I2C_ERROR_AF      = 2U,   /* Acknowledge Failure — device no responde */
    I2C_ERROR_INVALID = 3U
} I2c_Status_t;

/*** Function Prototypes ***/

/**
 * @brief Inicializa I2C1 en modo Fast (400 kHz).
 *        Requiere que gpio_init() ya haya configurado PB8 y PB9.
 * @return I2C_OK si fue exitoso.
 */
I2c_Status_t i2c_init(void);

/**
 * @brief Escribe 'len' bytes a un dispositivo I2C.
 * @param dev_addr  Direccion de 7 bits del dispositivo (e.g. 0x3C para SSD1306).
 * @param data      Puntero al arreglo de bytes a enviar.
 * @param len       Numero de bytes a enviar.
 * @return I2C_OK si fue exitoso.
 */
I2c_Status_t i2c_write(uint8_t dev_addr, const uint8_t *data, uint32_t len);

#endif /* __I2C_H__ */
