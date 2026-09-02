#ifndef HAL_TYPES_H
#define HAL_TYPES_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

/* ─────────────────────────────────────────────
   Fixed-width types used across the entire HAL.
   These mirror what you would find in a real
   STM32 CMSIS header.
───────────────────────────────────────────── */

typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef int8_t   s8;
typedef int16_t  s16;
typedef int32_t  s32;

/* ─────────────────────────────────────────────
   HAL return codes.
   Every HAL function returns one of these.
───────────────────────────────────────────── */

typedef enum {
    HAL_OK      = 0,
    HAL_ERROR   = 1,
    HAL_BUSY    = 2,
    HAL_TIMEOUT = 3
} HAL_Status;

/* ─────────────────────────────────────────────
   Register access macro.
   Treats an address as a pointer to a volatile
   32-bit register — exactly how real firmware
   accesses memory-mapped hardware.
───────────────────────────────────────────── */

#define REG32(addr)  (*(volatile u32 *)(addr))

#endif /* HAL_TYPES_H */
