#ifndef HAL_UART_H
#define HAL_UART_H

#include "hal_types.h"

/* ─────────────────────────────────────────────
   UART Status Register bit positions.
   These mirror STM32 USART_SR bit definitions.
───────────────────────────────────────────── */

#define UART_SR_TXE     (1U << 7)   /* TX Data Register Empty      */
#define UART_SR_TC      (1U << 6)   /* Transmission Complete        */
#define UART_SR_RXNE    (1U << 5)   /* RX Data Register Not Empty   */
#define UART_SR_ORE     (1U << 3)   /* Overrun Error                */
#define UART_SR_FE      (1U << 1)   /* Framing Error                */
#define UART_SR_PE      (1U << 0)   /* Parity Error                 */

/* ─────────────────────────────────────────────
   UART Control Register 1 bit positions.
───────────────────────────────────────────── */

#define UART_CR1_UE     (1U << 13)  /* UART Enable                  */
#define UART_CR1_TE     (1U << 3)   /* Transmitter Enable           */
#define UART_CR1_RE     (1U << 2)   /* Receiver Enable              */

/* ─────────────────────────────────────────────
   UART Register Block.
   Mirrors STM32 USART peripheral layout.
───────────────────────────────────────────── */

typedef struct {
    volatile u32 SR;    /* 0x00 - Status register           */
    volatile u32 DR;    /* 0x04 - Data register             */
    volatile u32 BRR;   /* 0x08 - Baud rate register        */
    volatile u32 CR1;   /* 0x0C - Control register 1        */
    volatile u32 CR2;   /* 0x10 - Control register 2        */
    volatile u32 CR3;   /* 0x14 - Control register 3        */
} UART_Regs;

/* ─────────────────────────────────────────────
   UART Configuration struct.
   Passed to UART_Init to configure the port.
───────────────────────────────────────────── */

typedef struct {
    u32 baud_rate;      /* e.g. 115200                      */
    u8  word_length;    /* 8 or 9 bits                      */
    u8  stop_bits;      /* 1 or 2                           */
    u8  parity;         /* 0=none, 1=even, 2=odd            */
} UART_Config;

/* ─────────────────────────────────────────────
   Simulated UART instances.
───────────────────────────────────────────── */

extern UART_Regs sim_UART1;
extern UART_Regs sim_UART2;

/* ─────────────────────────────────────────────
   Timeout sentinel for polling loops.
───────────────────────────────────────────── */

#define UART_TIMEOUT_DEFAULT  1000U

/* ─────────────────────────────────────────────
   API
───────────────────────────────────────────── */

HAL_Status UART_Init(UART_Regs *uart, const UART_Config *config);
HAL_Status UART_TransmitByte(UART_Regs *uart, u8 data, u32 timeout);
HAL_Status UART_ReceiveByte(UART_Regs *uart, u8 *data, u32 timeout);
HAL_Status UART_TransmitBuffer(UART_Regs *uart, const u8 *buf, u16 len, u32 timeout);
HAL_Status UART_ReceiveBuffer(UART_Regs *uart, u8 *buf, u16 len, u32 timeout);
bool       UART_IsReadable(UART_Regs *uart);
bool       UART_IsWritable(UART_Regs *uart);

#endif /* HAL_UART_H */
