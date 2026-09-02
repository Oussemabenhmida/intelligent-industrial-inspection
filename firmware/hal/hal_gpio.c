#include "hal_gpio.h"

/* ─────────────────────────────────────────────
   Simulated GPIO port instances.
   These live in RAM and act as our fake
   hardware registers.
───────────────────────────────────────────── */

GPIO_Regs sim_GPIOA = {0};
GPIO_Regs sim_GPIOB = {0};
GPIO_Regs sim_GPIOC = {0};

/* ─────────────────────────────────────────────
   GPIO_SetMode

   MODER uses 2 bits per pin.
   Pin 0 → bits [1:0]
   Pin 1 → bits [3:2]
   Pin N → bits [(2N+1):(2N)]

   To set a pin's mode:
   1. Clear the 2 bits for that pin
   2. Write the new mode into those 2 bits
───────────────────────────────────────────── */

HAL_Status GPIO_SetMode(GPIO_Regs *port, u8 pin, GPIO_Mode mode)
{
    if (port == NULL || pin > 15) {
        return HAL_ERROR;
    }

    /* Clear the 2 bits for this pin */
    port->MODER &= ~(0x3U << (pin * 2U));

    /* Write the new mode */
    port->MODER |= ((u32)mode << (pin * 2U));

    return HAL_OK;
}

/* ─────────────────────────────────────────────
   GPIO_WritePin

   Uses BSRR — the Bit Set/Reset Register.
   This is the correct way to set/clear pins
   on STM32. It is atomic — no read-modify-write
   needed, which matters in interrupt contexts.

   Bits [15:0]  → set the corresponding pin
   Bits [31:16] → reset the corresponding pin
───────────────────────────────────────────── */

HAL_Status GPIO_WritePin(GPIO_Regs *port, u8 pin, GPIO_PinState state)
{
    if (port == NULL || pin > 15) {
        return HAL_ERROR;
    }

    if (state == GPIO_PIN_HIGH) {
        /* Set: write 1 to bit position in lower half */
        port->BSRR = (1U << pin);
        port->ODR |= (1U << pin);   /* keep ODR in sync for simulation */
    } else {
        /* Reset: write 1 to bit position in upper half */
        port->BSRR = (1U << (pin + 16U));
        port->ODR &= ~(1U << pin);  /* keep ODR in sync for simulation */
    }

    return HAL_OK;
}

/* ─────────────────────────────────────────────
   GPIO_TogglePin

   XOR the pin bit in ODR.
   Flips the bit regardless of current state.
───────────────────────────────────────────── */

HAL_Status GPIO_TogglePin(GPIO_Regs *port, u8 pin)
{
    if (port == NULL || pin > 15) {
        return HAL_ERROR;
    }

    port->ODR ^= (1U << pin);

    return HAL_OK;
}

/* ─────────────────────────────────────────────
   GPIO_ReadPin

   Reads IDR — the Input Data Register.
   Shifts the target bit down to position 0
   and masks it to get a clean 0 or 1.
───────────────────────────────────────────── */

GPIO_PinState GPIO_ReadPin(GPIO_Regs *port, u8 pin)
{
    if (port == NULL || pin > 15) {
        return GPIO_PIN_LOW;
    }

    return ((port->IDR >> pin) & 0x1U) ? GPIO_PIN_HIGH : GPIO_PIN_LOW;
}
