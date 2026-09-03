#include "hal_uart.h"

/* ─────────────────────────────────────────────
   Simulated UART instances.
   SR is pre-set with TXE so the transmitter
   appears ready immediately after init,
   matching real hardware behaviour after enable.
───────────────────────────────────────────── */

UART_Regs sim_UART1 = { .SR = UART_SR_TXE, 0 };
UART_Regs sim_UART2 = { .SR = UART_SR_TXE, 0 };

/* ─────────────────────────────────────────────
   UART_Init

   Configures the UART peripheral.
   In simulation we store the baud rate in BRR
   and enable TX+RX in CR1 — same fields a real
   STM32 driver would configure.
───────────────────────────────────────────── */

HAL_Status UART_Init(UART_Regs *uart, const UART_Config *config)
{
    if (uart == NULL || config == NULL) {
        return HAL_ERROR;
    }

    if (config->baud_rate == 0) {
        return HAL_ERROR;
    }

    /* Store baud rate in BRR (simulated) */
    uart->BRR = config->baud_rate;

    /* Enable UART, transmitter, and receiver */
    uart->CR1 = UART_CR1_UE | UART_CR1_TE | UART_CR1_RE;

    /* Mark transmitter ready */
    uart->SR |= UART_SR_TXE;

    return HAL_OK;
}

/* ─────────────────────────────────────────────
   UART_IsWritable / UART_IsReadable

   Thin flag checks — used internally and
   exposed for polling from application code.
───────────────────────────────────────────── */

bool UART_IsWritable(UART_Regs *uart)
{
    if (uart == NULL) return false;
    return (uart->SR & UART_SR_TXE) != 0;
}

bool UART_IsReadable(UART_Regs *uart)
{
    if (uart == NULL) return false;
    return (uart->SR & UART_SR_RXNE) != 0;
}

/* ─────────────────────────────────────────────
   UART_TransmitByte

   Waits for TXE then writes to DR.
   In real hardware writing DR clears TXE
   and the hardware shifts the byte out.
   In simulation we clear TXE on write and
   immediately restore it to simulate
   instantaneous transmission.

   timeout: maximum poll iterations before
   returning HAL_TIMEOUT.
───────────────────────────────────────────── */

HAL_Status UART_TransmitByte(UART_Regs *uart, u8 data, u32 timeout)
{
    if (uart == NULL) return HAL_ERROR;

    /* Wait for TX register to be empty */
    u32 count = 0;
    while (!UART_IsWritable(uart)) {
        if (++count >= timeout) {
            return HAL_TIMEOUT;
        }
    }

    /* Write data — clears TXE in real hardware */
    uart->SR &= ~UART_SR_TXE;
    uart->DR  = data;

    /* Simulate transmission complete — restore TXE */
    uart->SR |= UART_SR_TXE;
    uart->SR |= UART_SR_TC;

    return HAL_OK;
}

/* ─────────────────────────────────────────────
   UART_ReceiveByte

   Waits for RXNE then reads DR.
   In real hardware reading DR clears RXNE.
   The caller must provide a valid pointer.
───────────────────────────────────────────── */

HAL_Status UART_ReceiveByte(UART_Regs *uart, u8 *data, u32 timeout)
{
    if (uart == NULL || data == NULL) return HAL_ERROR;

    /* Wait for received data */
    u32 count = 0;
    while (!UART_IsReadable(uart)) {
        if (++count >= timeout) {
            return HAL_TIMEOUT;
        }
    }

    /* Read data — clears RXNE in real hardware */
    *data = (u8)(uart->DR & 0xFFU);
    uart->SR &= ~UART_SR_RXNE;

    return HAL_OK;
}

/* ─────────────────────────────────────────────
   UART_TransmitBuffer

   Transmits len bytes from buf.
   Returns HAL_TIMEOUT if any byte times out.
───────────────────────────────────────────── */

HAL_Status UART_TransmitBuffer(UART_Regs *uart,
                                const u8 *buf,
                                u16 len,
                                u32 timeout)
{
    if (uart == NULL || buf == NULL || len == 0) return HAL_ERROR;

    for (u16 i = 0; i < len; i++) {
        HAL_Status s = UART_TransmitByte(uart, buf[i], timeout);
        if (s != HAL_OK) return s;
    }

    return HAL_OK;
}

/* ─────────────────────────────────────────────
   UART_ReceiveBuffer

   Receives len bytes into buf.
   Returns HAL_TIMEOUT if any byte times out.
───────────────────────────────────────────── */

HAL_Status UART_ReceiveBuffer(UART_Regs *uart,
                               u8 *buf,
                               u16 len,
                               u32 timeout)
{
    if (uart == NULL || buf == NULL || len == 0) return HAL_ERROR;

    for (u16 i = 0; i < len; i++) {
        HAL_Status s = UART_ReceiveByte(uart, &buf[i], timeout);
        if (s != HAL_OK) return s;
    }

    return HAL_OK;
}
