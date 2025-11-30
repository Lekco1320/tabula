/**
 * @file math.h
 * @brief Math utilities functions.
 *
 * @author Lukaß Zhang <lekco_1320@qq.com>
 * @date 2025-11-30
 * @license MIT
 */

#pragma once

#ifndef _EPD_CORE_MATH_H_
#define _EPD_CORE_MATH_H_

#include <stdint.h>

#define EPD_MIN(A, B) (((A) <= (B)) ? (A) : (B))
#define EPD_MAX(A, B) (((A) >= (B)) ? (A) : (B))

/**
 * @brief Saturating add for uint16_t.
 *
 * @param a Left operand.
 * @param b Right operand.
 * @return Sum of `a + b`, saturated at `0xFFFF` on overflow.
 */
static inline uint16_t epd_sat_add_uint16(uint16_t a, uint16_t b)
{
    uint32_t s = (uint32_t)a + (uint32_t)b;
    uint32_t carry = s >> 16;
    s |= -carry;
    return (uint16_t)s;
}

/**
 * @brief Saturating subtract for uint16_t.
 *
 * @param a Minuend.
 * @param b Subtrahend.
 * @return `a - b`, saturated at `0` on underflow.
 */
static inline uint16_t epd_sat_sub_uint16(uint16_t a, uint16_t b)
{
    uint32_t d = (uint32_t)a - (uint32_t)b;
    uint32_t borrow = (a < b);
    return (uint16_t)(d & ~(-borrow));
}

#endif // !_EPD_CORE_MATH_H_
