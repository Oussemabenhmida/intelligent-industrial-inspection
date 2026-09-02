#ifndef HAL_GPIO_H
#define HAL_GPIO_H

#include "hal_types.h"

/* ─────────────────────────────────────────────
   GPIO Register Block

   This struct mirrors the memory layout of a
   real STM32 GPIO peripheral. Each field maps
   to a hardware register at a fixed offset.

   On real hardware you would point a pointer
   of this type at the peripheral base address.
   Here we allocate instances in RAM to simulate
   the same behaviour.
───────────────────────────────────────────── */

typedef struct {
    volatile u32 MODER;   /* 0x00 - Pin mode register        */
    volatile u32 OTYPER;  /* 0x04 - Output type register     */
    volatile u32 OSPEEDR; /* 0x08 - Output speed register    */
    volatile u32 PUPDR;   /* 0x0C - Pull-up/pull-down        */
    volatile u32 IDR;     /* 0x10 - Input data register      */
    volatile u32 ODR;     /* 0x14 - Output data register     */
    volatile u32 BSRR;    /* 0x18 - Bit set/reset register   */
    volatile u32 LCKR;    /* 0x1C - Configuration lock       */
} GPIO_Regs;

/* ─────────────────────────────────────────────
   Pin Mode — 2 bits per pin in MODER
───────────────────────────────────────────── */

typedef enum {
    GPIO_MODE_INPUT    = 0x00,
    GPIO_MODE_OUTPUT   = 0x01,
    GPIO_MODE_ALTERNATE= 0x02,
    GPIO_MODE_ANALOG   = 0x03
} GPIO_Mode;

/* ─────────────────────────────────────────────
   Pin state
───────────────────────────────────────────── */

typedef enum {
    GPIO_PIN_LOW  = 0,
    GPIO_PIN_HIGH = 1
} GPIO_PinState;

/* ─────────────────────────────────────────────
   Simulated GPIO ports.
   On real hardware these would be pointers to
   peripheral base addresses. Here they are
   statically allocated register blocks.
───────────────────────────────────────────── */

extern GPIO_Regs sim_GPIOA;
extern GPIO_Regs sim_GPIOB;
extern GPIO_Regs sim_GPIOC;

/* ─────────────────────────────────────────────
   API
───────────────────────────────────────────── */

HAL_Status GPIO_SetMode(GPIO_Regs *port, u8 pin, GPIO_Mode mode);
HAL_Status GPIO_WritePin(GPIO_Regs *port, u8 pin, GPIO_PinState state);
HAL_Status GPIO_TogglePin(GPIO_Regs *port, u8 pin);
GPIO_PinState GPIO_ReadPin(GPIO_Regs *port, u8 pin);

#endif /* HAL_GPIO_H */
