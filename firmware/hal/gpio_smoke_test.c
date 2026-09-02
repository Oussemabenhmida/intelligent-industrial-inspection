#include <stdio.h>
#include "hal_gpio.h"

/* Helper — prints the relevant registers of a port */
static void print_port(const char *name, GPIO_Regs *port, u8 pin)
{
    printf("\n[%s] pin=%u\n", name, pin);
    printf("  MODER : 0x%08X\n", port->MODER);
    printf("  ODR   : 0x%08X\n", port->ODR);
    printf("  IDR   : 0x%08X\n", port->IDR);
    printf("  BSRR  : 0x%08X\n", port->BSRR);
    printf("  PinState (from ODR): %s\n",
           (port->ODR & (1U << pin)) ? "HIGH" : "LOW");
}

int main(void)
{
    printf("=== GPIO Simulation Smoke Test ===\n");

    /* ── Test 1: Set pin 5 as output ── */
    printf("\n--- Set GPIOA pin 5 as OUTPUT ---");
    GPIO_SetMode(&sim_GPIOA, 5, GPIO_MODE_OUTPUT);
    print_port("GPIOA", &sim_GPIOA, 5);

    /* ── Test 2: Write HIGH ── */
    printf("\n--- Write pin 5 HIGH ---");
    GPIO_WritePin(&sim_GPIOA, 5, GPIO_PIN_HIGH);
    print_port("GPIOA", &sim_GPIOA, 5);

    /* ── Test 3: Write LOW ── */
    printf("\n--- Write pin 5 LOW ---");
    GPIO_WritePin(&sim_GPIOA, 5, GPIO_PIN_LOW);
    print_port("GPIOA", &sim_GPIOA, 5);

    /* ── Test 4: Toggle ── */
    printf("\n--- Toggle pin 5 (expect HIGH) ---");
    GPIO_TogglePin(&sim_GPIOA, 5);
    print_port("GPIOA", &sim_GPIOA, 5);

    printf("\n--- Toggle pin 5 again (expect LOW) ---");
    GPIO_TogglePin(&sim_GPIOA, 5);
    print_port("GPIOA", &sim_GPIOA, 5);

    /* ── Test 5: Change mode — verify clear+set works ── */
    printf("\n--- Change pin 5 from OUTPUT to ALTERNATE ---");
    GPIO_SetMode(&sim_GPIOA, 5, GPIO_MODE_ALTERNATE);
    print_port("GPIOA", &sim_GPIOA, 5);

    printf("\n--- Change pin 5 from ALTERNATE back to OUTPUT ---");
    GPIO_SetMode(&sim_GPIOA, 5, GPIO_MODE_OUTPUT);
    print_port("GPIOA", &sim_GPIOA, 5);

    /* ── Test 6: NULL guard ── */
    printf("\n--- NULL port guard ---\n");
    HAL_Status s = GPIO_WritePin(NULL, 5, GPIO_PIN_HIGH);
    printf("  GPIO_WritePin(NULL) returned: %s\n",
           s == HAL_ERROR ? "HAL_ERROR (correct)" : "WRONG");

    /* ── Test 7: Invalid pin guard ── */
    printf("\n--- Invalid pin guard ---\n");
    s = GPIO_WritePin(&sim_GPIOA, 16, GPIO_PIN_HIGH);
    printf("  GPIO_WritePin(pin=16) returned: %s\n",
           s == HAL_ERROR ? "HAL_ERROR (correct)" : "WRONG");

    printf("\n=== Smoke test complete ===\n");
    return 0;
}
