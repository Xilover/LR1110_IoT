/*!
 * \file      lr11xx_hal.c
 *
 * \brief     Implements the lr11xx radio HAL functions
 *
 * The Clear BSD License
 * Copyright Semtech Corporation 2021. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted (subject to the limitations in the disclaimer
 * below) provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of the Semtech corporation nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 * NO EXPRESS OR IMPLIED LICENSES TO ANY PARTY'S PATENT RIGHTS ARE GRANTED BY
 * THIS LICENSE. THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND
 * CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT
 * NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL SEMTECH CORPORATION BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * -----------------------------------------------------------------------------
 * --- DEPENDENCIES ------------------------------------------------------------
 */

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#include "lr11xx_hal.h"


/*
 * -----------------------------------------------------------------------------
 * --- PRIVATE MACROS-----------------------------------------------------------
 */

/*
 * -----------------------------------------------------------------------------
 * --- PRIVATE CONSTANTS -------------------------------------------------------
 */

/*
 * -----------------------------------------------------------------------------
 * --- PRIVATE TYPES -----------------------------------------------------------
 */

/*
 * -----------------------------------------------------------------------------
 * --- PRIVATE VARIABLES -------------------------------------------------------
 */

/*
 * -----------------------------------------------------------------------------
 * --- PRIVATE FUNCTIONS DECLARATION -------------------------------------------
 */

/**
 * @brief Wait until radio busy pin returns to 0
 */
void lr11xx_hal_wait_on_busy( const lr11xx_hal_context_t* radio );

/*
 * -----------------------------------------------------------------------------
 * --- PUBLIC FUNCTIONS DEFINITION ---------------------------------------------
 */

lr11xx_hal_status_t lr11xx_hal_reset(const lr11xx_hal_context_t* context)
{
    // Pull RESET pin low to reset the LR11xx
    HAL_GPIO_WritePin(context->reset_port, context->reset_pin, GPIO_PIN_RESET);

    // Hold RESET for 1 ms
    HAL_Delay(1);

    // Pull RESET pin high to complete the reset
    HAL_GPIO_WritePin(context->reset_port, context->reset_pin, GPIO_PIN_SET);

    return LR11XX_HAL_STATUS_OK;
}


lr11xx_hal_status_t lr11xx_hal_wakeup(const lr11xx_hal_context_t* context)
{
    // Pull NSS low
    HAL_GPIO_WritePin(context->nss_port, context->nss_pin, GPIO_PIN_RESET);

    // Short delay to wake up the modem
    HAL_Delay(1);

    // Pull NSS high to complete wakeup
    HAL_GPIO_WritePin(context->nss_port, context->nss_pin, GPIO_PIN_SET);

    return LR11XX_HAL_STATUS_OK;
}



lr11xx_hal_status_t lr11xx_hal_write(const lr11xx_hal_context_t* context, const uint8_t* command,
                                     uint16_t command_length, const uint8_t* data, uint16_t data_length)
{
    // Wait for the modem to be ready (BUSY pin low)
    lr11xx_hal_wait_on_busy(context);

    // Pull NSS low to start communication
    HAL_GPIO_WritePin(context->nss_port, context->nss_pin, GPIO_PIN_RESET);

    // Transmit the command over SPI
    HAL_SPI_Transmit(context->hspi, (uint8_t*)command, command_length, HAL_MAX_DELAY);

    // If data needs to be transmitted, send it
    if (data_length > 0)
    {
        HAL_SPI_Transmit(context->hspi, (uint8_t*)data, data_length, HAL_MAX_DELAY);
    }

    // Pull NSS high to end communication
    HAL_GPIO_WritePin(context->nss_port, context->nss_pin, GPIO_PIN_SET);

    return LR11XX_HAL_STATUS_OK;
}



lr11xx_hal_status_t lr11xx_hal_read(const lr11xx_hal_context_t* context, const uint8_t* command,
                                    uint16_t command_length, uint8_t* data, uint16_t data_length)
{
    uint8_t dummy_byte = 0x00;

    // Wait for the modem to be ready (BUSY pin low)
    lr11xx_hal_wait_on_busy(context);

    // Pull NSS low to start communication
    HAL_GPIO_WritePin(context->nss_port, context->nss_pin, GPIO_PIN_RESET);

    // Transmit the command over SPI
    HAL_SPI_Transmit(context->hspi, (uint8_t*)command, command_length, HAL_MAX_DELAY);

    // Pull NSS high after sending the command
    HAL_GPIO_WritePin(context->nss_port, context->nss_pin, GPIO_PIN_SET);

    // Wait again for the modem to be ready (BUSY pin low)
    lr11xx_hal_wait_on_busy(context);

    // Pull NSS low again to start receiving data
    HAL_GPIO_WritePin(context->nss_port, context->nss_pin, GPIO_PIN_RESET);

    // Send a dummy byte and receive data
    HAL_SPI_TransmitReceive(context->hspi, &dummy_byte, data, data_length, HAL_MAX_DELAY);

    // Pull NSS high after receiving the data
    HAL_GPIO_WritePin(context->nss_port, context->nss_pin, GPIO_PIN_SET);

    return LR11XX_HAL_STATUS_OK;
}


lr11xx_hal_status_t lr11xx_hal_direct_read(const lr11xx_hal_context_t* context, uint8_t* data, uint16_t data_length)
{
    // Wait for the modem to be ready (BUSY pin low)
    lr11xx_hal_wait_on_busy(context);

    // Pull NSS low to start communication
    HAL_GPIO_WritePin(context->nss_port, context->nss_pin, GPIO_PIN_RESET);

    // Receive data over SPI
    HAL_SPI_Receive(context->hspi, data, data_length, HAL_MAX_DELAY);

    // Pull NSS high to end communication
    HAL_GPIO_WritePin(context->nss_port, context->nss_pin, GPIO_PIN_SET);

    return LR11XX_HAL_STATUS_OK;
}


/*
 * -----------------------------------------------------------------------------
 * --- PRIVATE FUNCTIONS DEFINITION --------------------------------------------
 */

void lr11xx_hal_wait_on_busy(const lr11xx_hal_context_t* context)
{
    // Wait for the BUSY pin to go low
    while (HAL_GPIO_ReadPin(context->busy_port, context->busy_pin) == GPIO_PIN_SET)
    {
        // Busy pin is high, waiting for it to go low
    }
}


/* --- EOF ------------------------------------------------------------------ */
