#include <stdio.h>
#include <string.h>
#include "hal_uart.h"

static void print_uart_sr(const char *label, UART_Regs *uart)
{
    printf("\n[%s] SR=0x%08X  TXE=%d  RXNE=%d  TC=%d  ORE=%d\n",
           label,
           uart->SR,
           (uart->SR & UART_SR_TXE)  ? 1 : 0,
           (uart->SR & UART_SR_RXNE) ? 1 : 0,
           (uart->SR & UART_SR_TC)   ? 1 : 0,
           (uart->SR & UART_SR_ORE)  ? 1 : 0);
}

int main(void)
{
    printf("=== UART Simulation Smoke Test ===\n");

    UART_Config cfg = {
        .baud_rate   = 115200,
        .word_length = 8,
        .stop_bits   = 1,
        .parity      = 0
    };

    HAL_Status s = UART_Init(&sim_UART1, &cfg);
    printf("\nUART_Init returned: %s\n", s == HAL_OK ? "HAL_OK" : "HAL_ERROR");
    printf("BRR = %u (expect 115200)\n", sim_UART1.BRR);
    print_uart_sr("After Init", &sim_UART1);

    printf("\n--- Transmit byte 0xA5 ---");
    s = UART_TransmitByte(&sim_UART1, 0xA5, UART_TIMEOUT_DEFAULT);
    printf("\nUART_TransmitByte returned: %s\n", s == HAL_OK ? "HAL_OK" : "HAL_ERROR");
    printf("DR = 0x%02X (expect 0xA5)\n", sim_UART1.DR);
    print_uart_sr("After TX", &sim_UART1);

    printf("\n--- Simulate incoming byte 0x3C ---");
    sim_UART1.DR  = 0x3C;
    sim_UART1.SR |= UART_SR_RXNE;
    print_uart_sr("Before RX", &sim_UART1);

    u8 received = 0;
    s = UART_ReceiveByte(&sim_UART1, &received, UART_TIMEOUT_DEFAULT);
    printf("UART_ReceiveByte returned: %s\n", s == HAL_OK ? "HAL_OK" : "HAL_ERROR");
    printf("Received = 0x%02X (expect 0x3C)\n", received);
    print_uart_sr("After RX", &sim_UART1);

    printf("\n--- RX timeout (no data) ---\n");
    sim_UART1.SR &= ~UART_SR_RXNE;
    u8 dummy = 0;
    s = UART_ReceiveByte(&sim_UART1, &dummy, 10U);
    printf("UART_ReceiveByte (no data) returned: %s\n",
           s == HAL_TIMEOUT ? "HAL_TIMEOUT (correct)" : "WRONG");

    printf("\n--- NULL guards ---\n");
    s = UART_TransmitByte(NULL, 0xFF, 10U);
    printf("TransmitByte(NULL) = %s\n", s == HAL_ERROR ? "HAL_ERROR (correct)" : "WRONG");

    s = UART_ReceiveByte(&sim_UART1, NULL, 10U);
    printf("ReceiveByte(NULL data ptr) = %s\n", s == HAL_ERROR ? "HAL_ERROR (correct)" : "WRONG");

    printf("\n=== UART smoke test complete ===\n");
    return 0;
}
