#ifndef __PLATFORM_TEGRA_UART_H
#define __PLATFORM_TEGRA_UART_H

#define UART_LSR_0                              0x14
#define UART_LSR_0_THRE_FIELD                   0x20
#define UART_LSR_0_RDR_FIELD                    0x1
#define UART_LSR_0_TMTY_FIELD                   0x40

#define UART_THR_DLAB_0_0                       0x0

#define UART_LCR_0                              0xc
#define UART_LCR_0_DLAB_RANGE                   7:7
#define UART_LCR_0_PAR_RANGE                    3:3
#define UART_LCR_0_STOP_RANGE                   2:2
#define UART_LCR_0_WD_SIZE_RANGE                1:0

#define UART_IIR_FCR_0                          0x8
#define UART_IIR_FCR_0_TX_CLR_RANGE             2:2
#define UART_IIR_FCR_0_RX_CLR_RANGE             1:1
#define UART_IIR_FCR_0_FCR_EN_FIFO_RANGE        0:0

#define UART_IER_DLAB_0_0                       0x4

#define UART_MCR_0                              0x10
#define UART_MCR_0_SW_DEFAULT_VAL               0x0


#define UART_MSR_0                              0x18
#define UART_MSR_0_SW_DEFAULT_VAL               0x0

#define UART_SPR_0                              0x1c
#define UART_SPR_0_SW_DEFAULT_VAL               0x0

#define UART_IRDA_CSR_0                         0x20
#define UART_IRDA_CSR_0_SW_DEFAULT_VAL          0x0

#define UART_ASR_0                              0x3c
#define UART_ASR_0_SW_DEFAULT_VAL               0x0

#define UART_RHR_0                              0x0


/**
 * @brief Initialize the tegra UART port
 *
 * @param base Base address of the UART to be used.
 *
 * @return NO_ERROR on success, appropriate error code otherwise 
 */
int tegra_uart_init(uintptr_t base);

/**
 * @brief Outputs a character to the initialized UART port
 *
 * @param c Character to be printed.
 *
 * @return NO_ERROR on success, appropriate error code otherwise
 */
int tegra_uart_putc(char c);

/**
 * @brief Reads a character from the initialized UART port
 *
 * @param wait Specifies whether to do a blocking read of the UART port
 *
 * @return The character read from the UART port.
 */
int tegra_uart_getc(bool wait);

#endif
