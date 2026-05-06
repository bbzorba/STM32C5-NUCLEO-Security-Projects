/*
    FDCAN loopback demo for NUCLEO-C562RE.
    Uses internal loopback mode so no external CAN transceiver is needed.
    Test results are printed over USART2 (PA2/PA3) at 115200 baud.
*/

#include "../inc/fdcan.h"

int main(void)
{
    /* Initialise UART for debug output (USART2, TX-only, 115200 baud) */
    USART_HandleType huart;
    USART_constructor(&huart, USART_2, TX_ONLY, __115200);

    USART_WriteString(&huart, "\n--- FDCAN Demo Boot ---\n");

    /* Initialise FDCAN1 in loopback mode at 500 kbps. */
    CAN_HandleType hcan;
    CAN_constructor(&hcan, CAN_1, CAN_500KBPS, CAN_MODE_LOOPBACK);

    /* Run built-in loopback self-test (polling + interrupt, LD1 + UART feedback) */
    CAN_LoopbackTest(&hcan, &huart);

    /* Run transceiver test in normal mode (requires external CAN bus and another node) */
    CAN_HandleType hcan_normal;
    CAN_constructor(&hcan_normal, CAN_1, CAN_500KBPS, CAN_MODE_NORMAL);
    CAN_TransceiverTest(&hcan_normal, &huart);

    while (1) {}
}