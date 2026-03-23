/*
    FDCAN loopback demo for NUCLEO-C562RE.
    Uses internal loopback mode so no external CAN transceiver is needed.
    Test results are printed over USART2 (PA2/PA3) at 115200 baud.
*/

#include "../inc/bxcan.h"

int main(void)
{
    /* Initialise UART for debug output (USART2, TX-only, 115200 baud) */
    USART_HandleType usart;
    USART_constructor(&usart, USART_2, TX_ONLY, __115200);

    USART_WriteString(&usart, "\n--- FDCAN Demo Boot ---\n");

    /* Initialise FDCAN1 in loopback mode at 500 kbps. */
    CAN_HandleType can;
    CAN_constructor(&can, CAN_1, CAN_500KBPS, CAN_MODE_LOOPBACK);

    /* Run built-in loopback self-test (polling + interrupt, LD1 + UART feedback) */
    CAN_LoopbackTest(&can, &usart);

    /* Run transceiver test in normal mode (requires external CAN bus and another node) */
    CAN_HandleType can_normal;
    CAN_constructor(&can_normal, CAN_1, CAN_500KBPS, CAN_MODE_NORMAL);
    CAN_TransceiverTest(&can_normal, &usart);

    while (1) {}
}