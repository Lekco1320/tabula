/**
 * @file common.c
 * @brief Common definitions for e-paper components.
 *
 * @author Lukaß Zhang <lekco_1320@qq.com>
 * @date 2025-12-11
 * @license MIT
 */

#include "epd_core/common.h"

const char* epd_err_to_str(epd_err_t err)
{
    switch (err)
    {
    case EPD_OK:
        return "OK";
    
    case EPD_FAIL:
        return "Fail";

    case EPD_ERR_NO_MEM:
        return "No available memory";

    case EPD_ERR_INVALID_ARG:
        return "Invalid argument";
    
    case EPD_ERR_INVALID_STATE:
        return "Invalid state";

    case EPD_ERR_INVALID_SIZE:
        return "Invalid size";

    case EPD_ERR_NOT_FOUND:
        return "Not found";

    case EPD_ERR_NOT_SUPPORTED:
        return "Not supported";

    case EPD_ERR_TIMEOUT:
        return "Timeout";

    case EPD_ERR_INVALID_RESPONSE:
        return "Invalid response";

    case EPD_ERR_INVALID_CRC:
        return "Invalid CRC";

    case EPD_ERR_INVALID_VERSION:
        return "Invalid version";

    case EPD_ERR_INVALID_MAC:
        return "Invalid MAC address";

    case EPD_ERR_NOT_FINISHED:
        return "Not finished yet";

    case EPD_ERR_NOT_ALLOWED:
        return "Not allowed";

    case EPD_FALLBACK:
        return "Fallback used";

    default:
        return "Unknown error";
    }
}
