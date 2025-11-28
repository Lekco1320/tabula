/**
 * @file common.h
 * @brief Common definitions for e-paper components.
 *
 * @author Lukaß Zhang <lekco_1320@qq.com>
 * @date 2025-11-28
 * @license MIT
 */

#pragma once

#ifndef _EPD_CORE_COMMON_H_
#define _EPD_CORE_COMMON_H_

#include <stdbool.h>

typedef enum {
    EPD_OK                   = 0,      /*!< value indicating success (no error) */
    EPD_FAIL                 = -1,     /*!< Generic epd_err_t code indicating failure */
    EPD_ERR_NO_MEM           = 0x101,  /*!< Out of memory */
    EPD_ERR_INVALID_ARG      = 0x102,  /*!< Invalid argument */
    EPD_ERR_INVALID_STATE    = 0x103,  /*!< Invalid state */
    EPD_ERR_INVALID_SIZE     = 0x104,  /*!< Invalid size */
    EPD_ERR_NOT_FOUND        = 0x105,  /*!< Requested resource not found */
    EPD_ERR_NOT_SUPPORTED    = 0x106,  /*!< Operation or feature not supported */
    EPD_ERR_TIMEOUT          = 0x107,  /*!< Operation timed out */
    EPD_ERR_INVALID_RESPONSE = 0x108,  /*!< Received response was invalid */
    EPD_ERR_INVALID_CRC      = 0x109,  /*!< CRC or checksum was invalid */
    EPD_ERR_INVALID_VERSION  = 0x10A,  /*!< Version was invalid */
    EPD_ERR_INVALID_MAC      = 0x10B,  /*!< MAC address was invalid */
    EPD_ERR_NOT_FINISHED     = 0x10C,  /*!< Operation has not fully completed */
    EPD_ERR_NOT_ALLOWED      = 0x10D,  /*!< Operation is not allowed */
} epd_err_t;

#endif // !_EPD_CORE_COMMON_H_