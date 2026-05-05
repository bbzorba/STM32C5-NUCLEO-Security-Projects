/*
    Author: Baris Berk Zorba
    Date: June 2024 (ported to STM32C562RE FDCAN — 2025)
    FDCAN driver implementation for STM32C562RE (NUCLEO-C562RE).
    The STM32C562RE has a simplified FDCAN peripheral that uses
    dedicated message SRAM instead of register-based mailboxes.
    Provides both polling and interrupt-based APIs.
*/

#include "../inc/fdcan.h"

#define NO_OF_CONT_TX_RX_TEST 500

/* GPIO handles for LD1 (PA5) and B1 button (PC13) */
static GPIO_HandleTypeDef s_ld1;
static GPIO_InitTypeDef   s_ld1_init;
static GPIO_HandleTypeDef s_b1;
static GPIO_InitTypeDef   s_b1_init;
static int s_gpio_initialized = 0;

/* Used by interrupt-based test */
static volatile uint8_t s_irq_msg_received = 0;
static CAN_MsgType      s_irq_rx_msg;

static void delay(volatile uint32_t count)
{
    while (count--) { __asm__("nop"); }
}

/* Initialise LD1 (PA5) and B1 button (PC13) using the GPIO driver */
static void CAN_GPIO_Init(void)
{
    if (s_gpio_initialized) return;

    /* LD1 Green — PA5, push-pull output */
    s_ld1_init.Pin       = GPIO_PIN_5;
    s_ld1_init.Mode      = GPIO_MODE_OUTPUT_PP;
    s_ld1_init.Pull      = GPIO_NOPULL;
    s_ld1_init.Speed     = GPIO_SPEED_MEDIUM;
    s_ld1_init.Alternate = 0;
    GPIO_constructor(&s_ld1, GPIO_A, &s_ld1_init);

    /* B1 user button — PC13, input (active-low, external pull-up on board) */
    s_b1_init.Pin       = GPIO_PIN_13;
    s_b1_init.Mode      = GPIO_MODE_INPUT;
    s_b1_init.Pull      = GPIO_NOPULL;
    s_b1_init.Speed     = GPIO_SPEED_LOW;
    s_b1_init.Alternate = 0;
    GPIO_constructor(&s_b1, GPIO_C, &s_b1_init);

    s_gpio_initialized = 1;
}

//////////////////////////////INTERNAL HELPERS//////////////////////////////

/*  Compute NBTP value for a given baud rate.
    FDCAN kernel clock = pclk1 = HSIDIV3 = 48 MHz (default on STM32C562RE).
    Uses NTSEG1=13tq, NTSEG2=2tq → bit time = 1+13+2 = 16 tq.
    Sample point = (1+13)/16 = 87.5 %.
    Baud = 48 MHz / (NBRP × 16). */
static uint32_t CAN_ComputeNBTP(CAN_BaudRateType baud)
{
    uint32_t brp;
    switch (baud) {
        case CAN_1MBPS:   brp = 3;   break;  /* 48 MHz / (3  × 16) = 1   Mbps  */
        case CAN_500KBPS: brp = 6;   break;  /* 48 MHz / (6  × 16) = 500 kbps  */
        case CAN_250KBPS: brp = 12;  break;  /* 48 MHz / (12 × 16) = 250 kbps  */
        case CAN_125KBPS: brp = 24;  break;  /* 48 MHz / (24 × 16) = 125 kbps  */
        default:          brp = 6;   break;
    }

    /* All values are 0-based in the register:
       NSJW = 1 tq  → field 0
       NTSEG2 = 2 tq → field 1
       NTSEG1 = 13 tq → field 12
       NBRP = brp → field brp-1 */
    return ((0U          << FDCAN_NBTP_NSJW_Pos)   |
            ((brp - 1U)  << FDCAN_NBTP_NBRP_Pos)   |
            (12U         << FDCAN_NBTP_NTSEG1_Pos)  |
            (1U          << FDCAN_NBTP_NTSEG2_Pos));
}

/*  Read one message from Rx FIFO 0 in the message SRAM.
    Returns 1 if a message was read, 0 if FIFO was empty. */
static int CAN_ReadFIFO(FDCAN_GlobalTypeDef *regs, CAN_MsgType *msg)
{
    uint32_t rxf0s = regs->RXF0S;
    uint32_t fill = rxf0s & FDCAN_RXF0S_F0FL_Msk;
    if (fill == 0)
        return 0;

    uint32_t get_idx = (rxf0s >> FDCAN_RXF0S_F0GI_Pos) & 0x03U;

    /* Read the 4-word Rx element from message SRAM */
    volatile uint32_t *rx_elem = (volatile uint32_t *)(SRAMCAN_BASE +
                                  SRAMCAN_RF0_OFFSET +
                                  get_idx * SRAMCAN_RF_ELEMENT_SIZE);
    uint32_t r0   = rx_elem[0];
    uint32_t r1   = rx_elem[1];
    uint32_t data0 = rx_elem[2];
    uint32_t data1 = rx_elem[3];

    /* Acknowledge (frees this FIFO slot) */
    regs->RXF0A = get_idx;

    /* Parse identifier */
    msg->IDE = (r0 & FDCAN_RX_R0_XTD) ? 1 : 0;
    msg->RTR = (r0 & FDCAN_RX_R0_RTR) ? 1 : 0;
    if (msg->IDE) {
        msg->ExtId = r0 & 0x1FFFFFFFU;
        msg->StdId = 0;
    } else {
        msg->StdId = (r0 >> 18) & 0x7FFU;
        msg->ExtId = 0;
    }

    /* Parse data length */
    msg->DLC = (r1 >> FDCAN_RX_R1_DLC_Pos) & 0x0FU;
    if (msg->DLC > 8) msg->DLC = 8;

    /* Unpack data bytes (little-endian layout) */
    msg->Data[0] = (uint8_t)(data0 >>  0);
    msg->Data[1] = (uint8_t)(data0 >>  8);
    msg->Data[2] = (uint8_t)(data0 >> 16);
    msg->Data[3] = (uint8_t)(data0 >> 24);
    msg->Data[4] = (uint8_t)(data1 >>  0);
    msg->Data[5] = (uint8_t)(data1 >>  8);
    msg->Data[6] = (uint8_t)(data1 >> 16);
    msg->Data[7] = (uint8_t)(data1 >> 24);

    return 1;
}
////////////////////////////////////////////////////////////////////////////////////////////



//////////////////////////////CONSTRUCTOR & INIT////////////////////////////////////////

void CAN_constructor(CAN_HandleType *handle, FDCAN_GlobalTypeDef *regs,
                     CAN_BaudRateType baudrate, CAN_ModeType mode)
{
    handle->regs       = regs;
    handle->baudRate   = baudrate;
    handle->mode       = mode;
    handle->rxCallback = NULL;

    CAN_Init(handle);
}

void CAN_Init(CAN_HandleType *handle)
{
    /* FDCAN kernel clock = pclk1 (default FDCAN1SEL=0 in CCIPR1).
       pclk1 = HSIDIV3 = 48 MHz on STM32C562RE after reset.
       No oscillator enable or CCIPR1 write needed. */

    /* 1. Enable FDCAN peripheral clock (APB1 high) */
    RCC->APB1HENR |= RCC_APB1HENR_FDCANEN;
    __asm__("dsb"); __asm__("nop"); __asm__("nop");   /* read-back delay */

    /* 2. Configure GPIO: PA11 = FDCAN1_RX, PA12 = FDCAN1_TX (AF9) */
    RCC->AHB2ENR |= RCC_AHB2ENR_GPIOAEN;
    __asm__("dsb"); __asm__("nop"); __asm__("nop");

    GPIO_A->MODER  &= ~(MODER_PIN11_MASK | MODER_PIN12_MASK);
    GPIO_A->MODER  |=  (MODER_PIN11_SET  | MODER_PIN12_SET);    /* AF mode */
    GPIO_A->AFR[1] &= ~(AFRH_PIN11_MASK  | AFRH_PIN12_MASK);
    GPIO_A->AFR[1] |=  (AFRH_PIN11_SET_AF9 | AFRH_PIN12_SET_AF9);
    /* Pull-up on RX pin so floating line reads as recessive */
    GPIO_A->PUPDR  &= ~PUPDR_PIN11_MASK;
    GPIO_A->PUPDR  |=  (GPIO_PULLUP << 22);  /* PA11 pull-up */

    /* 3. Request FDCAN initialisation mode */
    handle->regs->CCCR |= FDCAN_CCCR_INIT;
    while (!(handle->regs->CCCR & FDCAN_CCCR_INIT)) { /* wait */ }

    /* 4. Enable configuration change */
    handle->regs->CCCR |= FDCAN_CCCR_CCE;

    /* 4b. Zero the message SRAM for this FDCAN instance so filters/FIFOs
       start from a clean state rather than uninitialized garbage. */
    {
        volatile uint32_t *p = (volatile uint32_t *)SRAMCAN_BASE;
        for (uint32_t i = 0; i < SRAMCAN_INSTANCE_SIZE / 4U; i++)
            p[i] = 0U;
    }

    /* 5. Configure nominal bit timing */
    handle->regs->NBTP = CAN_ComputeNBTP(handle->baudRate);

    /* 6. Set operating mode */
    uint32_t cccr = handle->regs->CCCR;
    cccr &= ~(FDCAN_CCCR_MON | FDCAN_CCCR_TEST);
    cccr |= FDCAN_CCCR_DAR;  /* disable automatic retransmission */
    if (handle->mode == CAN_MODE_LOOPBACK || handle->mode == CAN_MODE_SILENT_LOOPBACK) {
        cccr |= FDCAN_CCCR_TEST;
    }
    if (handle->mode == CAN_MODE_SILENT || handle->mode == CAN_MODE_SILENT_LOOPBACK) {
        cccr |= FDCAN_CCCR_MON;
    }
    handle->regs->CCCR = cccr;

    /* For loopback: set TEST.LBCK (TEST register writable only when CCCR.TEST=1) */
    if (handle->mode == CAN_MODE_LOOPBACK || handle->mode == CAN_MODE_SILENT_LOOPBACK) {
        handle->regs->TEST |= FDCAN_TEST_LBCK;
    }

    /* 7. Configure accept-all filter */
    CAN_FilterAcceptAll(handle);

    /* 8. Leave initialisation mode → enter operational mode */
    handle->regs->CCCR &= ~FDCAN_CCCR_INIT;
    while (handle->regs->CCCR & FDCAN_CCCR_INIT) { /* wait */ }
}
////////////////////////////////////////////////////////////////////////////////////////////



//////////////////////////////FILTER CONFIGURATION//////////////////////////////////////

/*  Configure RXGFC to accept ALL frames (standard and extended)
    into Rx FIFO 0 as non-matching frames.  No filter elements are
    needed — LSS=0, LSE=0, ANFS=00, ANFE=00. */
void CAN_FilterAcceptAll(CAN_HandleType *handle)
{
    /* ANFS=00 (accept std non-matching → FIFO 0)
       ANFE=00 (accept ext non-matching → FIFO 0)
       LSS=0, LSE=0, RRFE=0, RRFS=0 */
    handle->regs->RXGFC = 0x00000000U;
}
////////////////////////////////////////////////////////////////////////////////////////////



//////////////////////////////POLLING TX API////////////////////////////////////////////

/*  Transmit one CAN message via the TX FIFO.
    The simplified FDCAN on STM32C562RE uses a 3-element TX FIFO
    (no dedicated TX buffers).  Use TXFQS.TFQPI for the put index
    and poll TXBTO for completion (IR.TC requires TXBTIE).
    Returns 0 on success, -1 if FIFO full or TX timed out. */
int CAN_Transmit(CAN_HandleType *handle, const CAN_MsgType *msg)
{
    /* Check TX FIFO is not full */
    uint32_t txfqs = handle->regs->TXFQS;
    if (txfqs & FDCAN_TXFQS_TFQF)
        return -1;

    /* Get the put index from TX FIFO */
    uint8_t buf_idx = (txfqs >> FDCAN_TXFQS_TFQPI_Pos) & 0x03U;

    /* Build TX element words */
    uint32_t t0 = 0;
    if (msg->IDE) {
        t0 = (msg->ExtId & 0x1FFFFFFFU) | FDCAN_TX_T0_XTD;
    } else {
        t0 = ((msg->StdId & 0x7FFU) << 18);
    }
    if (msg->RTR) t0 |= FDCAN_TX_T0_RTR;

    uint32_t t1 = ((uint32_t)(msg->DLC & 0x0FU)) << FDCAN_TX_T1_DLC_Pos;
    /* EFC=0: no TX event stored; MM=0 */

    uint32_t data0 = ((uint32_t)msg->Data[0]      ) |
                     ((uint32_t)msg->Data[1] <<  8) |
                     ((uint32_t)msg->Data[2] << 16) |
                     ((uint32_t)msg->Data[3] << 24);
    uint32_t data1 = ((uint32_t)msg->Data[4]      ) |
                     ((uint32_t)msg->Data[5] <<  8) |
                     ((uint32_t)msg->Data[6] << 16) |
                     ((uint32_t)msg->Data[7] << 24);

    /* Write into message SRAM */
    volatile uint32_t *tx_elem = (volatile uint32_t *)(SRAMCAN_BASE +
                                  SRAMCAN_TFQ_OFFSET +
                                  buf_idx * SRAMCAN_TX_ELEMENT_SIZE);
    tx_elem[0] = t0;
    tx_elem[1] = t1;
    tx_elem[2] = data0;
    tx_elem[3] = data1;

    /* Request transmission */
    handle->regs->TXBAR = (1U << buf_idx);

    /* Wait for Transmission Occurred — TXBTO bit is set when TXBRP clears
       after a successful transmission.  Does NOT require TXBTIE. */
    volatile uint32_t timeout = 1000000U;
    while (!(handle->regs->TXBTO & (1U << buf_idx))) {
        if (--timeout == 0) return -1;  /* TX timed out */
    }

    return 0;
}
////////////////////////////////////////////////////////////////////////////////////////////



//////////////////////////////POLLING RX API////////////////////////////////////////////

/*  Receive one CAN message from FIFO 0 (polling).
    Returns 0 if a message was read, -1 if FIFO was empty. */
int CAN_Receive(CAN_HandleType *handle, CAN_MsgType *msg)
{
    return CAN_ReadFIFO(handle->regs, msg) ? 0 : -1;
}
////////////////////////////////////////////////////////////////////////////////////////////



//////////////////////////////INTERRUPT API//////////////////////////////////////////////

/*
 * Internal callback registered with the NVIC dispatch table.
 * Invoked by nvic.c's FDCAN1_IT0_IRQHandler when the interrupt fires.
 * 'arg' is the CAN_HandleType pointer passed to CAN_EnableRXInterrupt().
 */
static void fdcan_it0_handler(void *arg)
{
    CAN_HandleType *handle = (CAN_HandleType *)arg;
    if (handle->regs->IR & FDCAN_IR_RF0N) {
        handle->regs->IR = FDCAN_IR_RF0N;   /* write-1-to-clear */
        CAN_MsgType msg;
        if (CAN_ReadFIFO(handle->regs, &msg) && handle->rxCallback) {
            handle->rxCallback(&msg);
        }
    }
}

void CAN_EnableRXInterrupt(CAN_HandleType *handle, CAN_RxCallback_t callback)
{
    handle->rxCallback = callback;
    handle->regs->IE  |= FDCAN_IE_RF0NE;    /* Rx FIFO 0 new-message interrupt */
    handle->regs->ILS &= ~FDCAN_IR_RF0N;    /* route RF0N to interrupt line 0  */
    handle->regs->ILE |= FDCAN_ILE_EINT0;   /* enable interrupt line 0         */
    /* Register callback — nvic.c owns FDCAN1_IT0_IRQHandler and dispatches here */
    NVIC_RegisterHandler(FDCAN1_IT0_IRQn, fdcan_it0_handler, handle, 0);
}

void CAN_DisableRXInterrupt(CAN_HandleType *handle)
{
    handle->regs->IE &= ~FDCAN_IE_RF0NE;
    NVIC_UnregisterHandler(FDCAN1_IT0_IRQn);
}
////////////////////////////////////////////////////////////////////////////////////////////



//////////////////////////////BUILT-IN TESTS////////////////////////////////////////////

/* Internal RX callback used by CAN_LoopbackTest (stores message & sets flag) */
static void loopback_irq_callback(const CAN_MsgType *msg)
{
    s_irq_rx_msg = *msg;
    s_irq_msg_received = 1;
}

/* Helper: print a CAN message in hex over UART */
static void CAN_PrintMsg(USART_HandleType *uart, const char *label, const CAN_MsgType *msg)
{
    char buf[128];
    snprintf(buf, sizeof(buf), "  %s: ID=0x%03lX DLC=%u Data=",
             label, (unsigned long)(msg->IDE ? msg->ExtId : msg->StdId), msg->DLC);
    USART_WriteString(uart, buf);
    for (uint8_t i = 0; i < msg->DLC; i++) {
        snprintf(buf, sizeof(buf), "%02X ", msg->Data[i]);
        USART_WriteString(uart, buf);
    }
    USART_WriteString(uart, "\n");
}

/*  Self-test using internal loopback.
    Phase 1: Polling TX → Polling RX     (LD1 on = pass)
    Phase 2: Polling TX → Interrupt RX   (LD1 blink = pass)
    Phase 3: Continuous loopback blink   (LD1 toggles)
    LD1 off after each phase reset.
    All results are printed over UART.
    Returns 0 on overall PASS, -1 on FAIL. */
int CAN_LoopbackTest(CAN_HandleType *handle, USART_HandleType *uart)
{
    CAN_GPIO_Init();
    GPIO_WritePin(&s_ld1, GPIO_PIN_5, GPIO_PIN_RESET);

    char buf[128];
    int pass = 0;

    USART_WriteString(uart, "\n========================================\n");
    USART_WriteString(uart, " FDCAN Loopback Self-Test (STM32C562RE)\n");
    USART_WriteString(uart, "========================================\n");

    /* Print configuration */
    snprintf(buf, sizeof(buf), "Mode: LOOPBACK | Baud: %s\n",
             (handle->baudRate == CAN_1MBPS)   ? "1 Mbps"   :
             (handle->baudRate == CAN_500KBPS)  ? "500 kbps" :
             (handle->baudRate == CAN_250KBPS)  ? "250 kbps" :
             (handle->baudRate == CAN_125KBPS)  ? "125 kbps" : "unknown");
    USART_WriteString(uart, buf);
    snprintf(buf, sizeof(buf), "CCCR=0x%08lX  NBTP=0x%08lX  PSR=0x%08lX\n",
             (unsigned long)handle->regs->CCCR,
             (unsigned long)handle->regs->NBTP,
             (unsigned long)handle->regs->PSR);
    USART_WriteString(uart, buf);
    snprintf(buf, sizeof(buf), "TEST=0x%08lX  RCC_CR1=0x%08lX\n",
             (unsigned long)handle->regs->TEST,
             (unsigned long)RCC->CR1);
    USART_WriteString(uart, buf);

    /* ---- Phase 1: Polling TX → Polling RX ---- */
    USART_WriteString(uart, "\n--- Phase 1: Polling TX -> Polling RX ---\n");

    CAN_MsgType tx_msg = {0};
    tx_msg.StdId   = 0x123;
    tx_msg.IDE     = 0;
    tx_msg.RTR     = 0;
    tx_msg.DLC     = 4;
    tx_msg.Data[0] = 0xDE;
    tx_msg.Data[1] = 0xAD;
    tx_msg.Data[2] = 0xBE;
    tx_msg.Data[3] = 0xEF;

    CAN_PrintMsg(uart, "TX", &tx_msg);

    if (CAN_Transmit(handle, &tx_msg) != 0) {
        USART_WriteString(uart, "  TX FAILED!\n");
        snprintf(buf, sizeof(buf), "  PSR=0x%08lX\n", (unsigned long)handle->regs->PSR);
        USART_WriteString(uart, buf);
        USART_WriteString(uart, "  RESULT: FAIL\n");
        return -1;
    }
    USART_WriteString(uart, "  TX OK\n");

    CAN_MsgType rx_msg;
    while (CAN_Receive(handle, &rx_msg) != 0) { /* spin */ }

    CAN_PrintMsg(uart, "RX", &rx_msg);

    if (rx_msg.StdId == 0x123 &&
        rx_msg.Data[0] == 0xDE && rx_msg.Data[1] == 0xAD &&
        rx_msg.Data[2] == 0xBE && rx_msg.Data[3] == 0xEF)
    {
        USART_WriteString(uart, "  RX MATCH: PASS\n");
        GPIO_WritePin(&s_ld1, GPIO_PIN_5, GPIO_PIN_SET);
    } else {
        USART_WriteString(uart, "  RX MISMATCH: FAIL\n");
        pass = -1;
    }

    delay(3000000);
    GPIO_WritePin(&s_ld1, GPIO_PIN_5, GPIO_PIN_RESET);

    /* ---- Phase 2: Interrupt-based RX ---- */
    USART_WriteString(uart, "\n--- Phase 2: Polling TX -> Interrupt RX ---\n");

    s_irq_msg_received = 0;
    CAN_EnableRXInterrupt(handle, loopback_irq_callback);
    USART_WriteString(uart, "  RX interrupt enabled (FIFO 0)\n");

    tx_msg.StdId   = 0x456;
    tx_msg.DLC     = 2;
    tx_msg.Data[0] = 0xCA;
    tx_msg.Data[1] = 0xFE;

    CAN_PrintMsg(uart, "TX", &tx_msg);

    if (CAN_Transmit(handle, &tx_msg) != 0) {
        USART_WriteString(uart, "  TX FAILED!\n");
        pass = -1;
    } else {
        USART_WriteString(uart, "  TX OK, waiting for IRQ callback...\n");
    }

    while (!s_irq_msg_received) { /* spin */ }
    USART_WriteString(uart, "  IRQ callback fired!\n");

    CAN_PrintMsg(uart, "IRQ RX", &s_irq_rx_msg);

    if (s_irq_rx_msg.StdId == 0x456 &&
        s_irq_rx_msg.Data[0] == 0xCA && s_irq_rx_msg.Data[1] == 0xFE)
    {
        USART_WriteString(uart, "  IRQ RX MATCH: PASS\n");
        GPIO_WritePin(&s_ld1, GPIO_PIN_5, GPIO_PIN_SET);
    } else {
        USART_WriteString(uart, "  IRQ RX MISMATCH: FAIL\n");
        pass = -1;
    }

    CAN_DisableRXInterrupt(handle);

    delay(3000000);
    GPIO_WritePin(&s_ld1, GPIO_PIN_5, GPIO_PIN_RESET);

    /* ---- Overall result ---- */
    USART_WriteString(uart, "\n========================================\n");
    if (pass == 0)
        USART_WriteString(uart, " LOOPBACK TEST: ALL PASSED\n");
    else
        USART_WriteString(uart, " LOOPBACK TEST: FAILED\n");
    USART_WriteString(uart, "========================================\n");

    /* ---- Phase 3: Continuous loopback (press B1 on PC13 to exit) ---- */
    USART_WriteString(uart, "\nPhase 3: Continuous loopback (press B1 button to exit)\n");

    uint8_t counter = 0;
    for (;;) {
        tx_msg.StdId   = 0x100;
        tx_msg.DLC     = 1;
        tx_msg.Data[0] = counter++;

        if (CAN_Transmit(handle, &tx_msg) == 0) {
            while (CAN_Receive(handle, &rx_msg) != 0) { /* spin */ }

            snprintf(buf, sizeof(buf), "  Loop #%u: TX=0x%02X RX=0x%02X %s\n",
                     (unsigned)(counter - 1), tx_msg.Data[0], rx_msg.Data[0],
                     (tx_msg.Data[0] == rx_msg.Data[0]) ? "OK" : "MISMATCH");
            USART_WriteString(uart, buf);

            GPIO_TogglePin(&s_ld1, GPIO_PIN_5);
        } else {
            USART_WriteString(uart, "  Loop TX failed\n");
            pass = -1;
        }
        delay(1000000);
        /* B1 user button on PC13 — active LOW (pressed=0) */
        if (GPIO_ReadPin(&s_b1, GPIO_PIN_13) == GPIO_PIN_RESET) break;
    }

    USART_WriteString(uart, "Phase 3 completed cont. test \n");
    GPIO_WritePin(&s_ld1, GPIO_PIN_5, GPIO_PIN_RESET);
    return pass;
}

/*  Transceiver test for normal bus operation.
    Sends a test message and waits for an incoming message.
    Requires a CAN transceiver (e.g. MCP2551/SN65HVD230) and
    another CAN node on the bus.
    All results are printed over UART.
    Returns 0 on overall PASS, -1 on FAIL. */
int CAN_TransceiverTest(CAN_HandleType *handle, USART_HandleType *uart)
{
    CAN_GPIO_Init();
    GPIO_WritePin(&s_ld1, GPIO_PIN_5, GPIO_PIN_RESET);

    char buf[128];
    int pass = 0;

    USART_WriteString(uart, "\n========================================\n");
    USART_WriteString(uart, " FDCAN Transceiver Test (Normal Mode)\n");
    USART_WriteString(uart, "========================================\n");

    snprintf(buf, sizeof(buf), "Mode: NORMAL | Baud: %s\n",
             (handle->baudRate == CAN_1MBPS)   ? "1 Mbps"   :
             (handle->baudRate == CAN_500KBPS)  ? "500 kbps" :
             (handle->baudRate == CAN_250KBPS)  ? "250 kbps" :
             (handle->baudRate == CAN_125KBPS)  ? "125 kbps" : "unknown");
    USART_WriteString(uart, buf);
    snprintf(buf, sizeof(buf), "CCCR=0x%08lX  NBTP=0x%08lX  PSR=0x%08lX\n",
             (unsigned long)handle->regs->CCCR,
             (unsigned long)handle->regs->NBTP,
             (unsigned long)handle->regs->PSR);
    USART_WriteString(uart, buf);

    /* ---- Phase 1: Polling TX ---- */
    USART_WriteString(uart, "\n--- Phase 1: Polling TX ---\n");

    CAN_MsgType tx_msg = {0};
    tx_msg.StdId   = 0x200;
    tx_msg.IDE     = 0;
    tx_msg.RTR     = 0;
    tx_msg.DLC     = 3;
    tx_msg.Data[0] = 'C';
    tx_msg.Data[1] = 'A';
    tx_msg.Data[2] = 'N';

    CAN_PrintMsg(uart, "TX", &tx_msg);

    if (CAN_Transmit(handle, &tx_msg) == 0) {
        USART_WriteString(uart, "  TX OK\n");
        GPIO_WritePin(&s_ld1, GPIO_PIN_5, GPIO_PIN_SET);
    } else {
        USART_WriteString(uart, "  TX FAILED!\n");
        snprintf(buf, sizeof(buf), "  PSR=0x%08lX\n",
                 (unsigned long)handle->regs->PSR);
        USART_WriteString(uart, buf);
        pass = -1;
    }

    /* ---- Phase 2: Polling RX (wait for any message from another node) ---- */
    USART_WriteString(uart, "\n--- Phase 2: Polling RX (waiting for remote node) ---\n");

    CAN_MsgType rx_msg;
    while (CAN_Receive(handle, &rx_msg) != 0) { /* spin until a message arrives */ }

    USART_WriteString(uart, "  Message received!\n");
    CAN_PrintMsg(uart, "RX", &rx_msg);

    delay(3000000);
    GPIO_WritePin(&s_ld1, GPIO_PIN_5, GPIO_PIN_RESET);

    /* ---- Phase 3: Interrupt-based RX ---- */
    USART_WriteString(uart, "\n--- Phase 3: Interrupt RX (waiting for remote echo) ---\n");

    s_irq_msg_received = 0;
    CAN_EnableRXInterrupt(handle, loopback_irq_callback);
    USART_WriteString(uart, "  RX interrupt enabled (FIFO 0)\n");

    tx_msg.StdId   = 0x201;
    tx_msg.DLC     = 1;
    tx_msg.Data[0] = 0x42;

    CAN_PrintMsg(uart, "TX", &tx_msg);
    CAN_Transmit(handle, &tx_msg);
    USART_WriteString(uart, "  Waiting for IRQ callback...\n");

    while (!s_irq_msg_received) { /* spin until IRQ callback fires */ }
    USART_WriteString(uart, "  IRQ callback fired!\n");
    CAN_PrintMsg(uart, "IRQ RX", &s_irq_rx_msg);
    GPIO_WritePin(&s_ld1, GPIO_PIN_5, GPIO_PIN_SET);

    CAN_DisableRXInterrupt(handle);

    delay(3000000);
    GPIO_WritePin(&s_ld1, GPIO_PIN_5, GPIO_PIN_RESET);

    /* ---- Overall result ---- */
    USART_WriteString(uart, "\n========================================\n");
    if (pass == 0)
        USART_WriteString(uart, " TRANSCEIVER TEST: ALL PASSED\n");
    else
        USART_WriteString(uart, " TRANSCEIVER TEST: FAILED\n");
    USART_WriteString(uart, "========================================\n");

    /* ---- Phase 4: Continuous TX+RX ---- */
    USART_WriteString(uart, "\nPhase 4: TX+RX cont. test (LD1 toggles)\n");

    uint8_t counter = 0;
    for (uint16_t i = 0; i < NO_OF_CONT_TX_RX_TEST; i++) {
        tx_msg.StdId   = 0x100;
        tx_msg.DLC     = 1;
        tx_msg.Data[0] = counter++;

        CAN_Transmit(handle, &tx_msg);

        if (CAN_Receive(handle, &rx_msg) == 0) {
            snprintf(buf, sizeof(buf), "  Loop #%u: TX=0x%02X RX=0x%02X\n",
                     (unsigned)(counter - 1), tx_msg.Data[0], rx_msg.Data[0]);
            USART_WriteString(uart, buf);
            GPIO_TogglePin(&s_ld1, GPIO_PIN_5);
        }
        delay(1000000);
    }

    USART_WriteString(uart, "Phase 4 cont. test complete\n");
    GPIO_WritePin(&s_ld1, GPIO_PIN_5, GPIO_PIN_RESET);
    return pass;
}
////////////////////////////////////////////////////////////////////////////////////////////
