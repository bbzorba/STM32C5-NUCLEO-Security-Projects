/*
    Author: Baris Berk Zorba
    Date: June 2024 (ported to STM32C562RE FDCAN — 2025)
    FDCAN driver header for STM32C562RE (NUCLEO-C562RE).
    The STM32C562RE has one FDCAN instance (simplified variant)
    with dedicated message SRAM.  Provides register definitions,
    message types, and APIs for polling and interrupt-based CAN.
*/

#ifndef __BXCAN_H
#define __BXCAN_H

#include <stdint.h>
#include <stddef.h>
#include "../../GPIO/inc/gpio.h"

#ifndef __IO
#define __IO volatile
#endif
#ifndef __NVIC_PRIO_BITS
#define __NVIC_PRIO_BITS 4
#endif

/* IRQn_Type and NVIC helper API come from nvic.h (included via uart.h below). */
#include "../../UART/inc/uart.h"

////////////////////////////// PERIPHERAL BASE ADDRESSES //////////////////////////////

#define FDCAN1_BASE        0x4000A400U
#define FDCAN_CONFIG_BASE  0x4000A500U
#define SRAMCAN_BASE       0x4000AC00U   /* FDCAN message SRAM */

////////////////////////////// RCC CLOCK ENABLE / SELECT //////////////////////////////

/* APB1HENR bit 9: FDCAN peripheral clock gate */
#define RCC_APB1HENR_FDCANEN  ((uint32_t)(1UL << 9))

/* CCIPR1 bits [27:26]: FDCAN1 kernel clock source (STM32C562RE SVD)
     00 = pclk1 (default), 01 = psis_ck, 10 = psik_ck, 11 = hse_ck */
#define RCC_CCIPR1_FDCANSEL_Pos   26U
#define RCC_CCIPR1_FDCANSEL_Msk   (3UL << RCC_CCIPR1_FDCANSEL_Pos)
#define RCC_CCIPR1_FDCANSEL_PCLK1 (0UL << RCC_CCIPR1_FDCANSEL_Pos)  /* default */

/* FDCAN kernel clock frequency — default is pclk1 = HSIDIV3 = 48 MHz */
#define FDCAN_KERNEL_CLK_HZ  48000000U

////////////////////////////// FDCAN REGISTER STRUCTURE ////////////////////////////////

typedef struct {
    __IO uint32_t CREL;           /* 0x000  Core Release                      */
    __IO uint32_t ENDN;           /* 0x004  Endian                            */
         uint32_t RESERVED0;      /* 0x008                                    */
    __IO uint32_t DBTP;           /* 0x00C  Data Bit Timing & Prescaler       */
    __IO uint32_t TEST;           /* 0x010  Test                              */
    __IO uint32_t RWD;            /* 0x014  RAM Watchdog                      */
    __IO uint32_t CCCR;           /* 0x018  CC Control                        */
    __IO uint32_t NBTP;           /* 0x01C  Nominal Bit Timing & Prescaler    */
    __IO uint32_t TSCC;           /* 0x020  Timestamp Counter Configuration   */
    __IO uint32_t TSCV;           /* 0x024  Timestamp Counter Value           */
    __IO uint32_t TOCC;           /* 0x028  Timeout Counter Configuration     */
    __IO uint32_t TOCV;           /* 0x02C  Timeout Counter Value             */
         uint32_t RESERVED1[4];   /* 0x030-0x03C                              */
    __IO uint32_t ECR;            /* 0x040  Error Counter                     */
    __IO uint32_t PSR;            /* 0x044  Protocol Status                   */
    __IO uint32_t TDCR;           /* 0x048  Transmitter Delay Compensation    */
         uint32_t RESERVED2;      /* 0x04C                                    */
    __IO uint32_t IR;             /* 0x050  Interrupt Register                */
    __IO uint32_t IE;             /* 0x054  Interrupt Enable                  */
    __IO uint32_t ILS;            /* 0x058  Interrupt Line Select             */
    __IO uint32_t ILE;            /* 0x05C  Interrupt Line Enable             */
         uint32_t RESERVED3[8];   /* 0x060-0x07C                              */
    __IO uint32_t RXGFC;          /* 0x080  Global Filter Configuration       */
    __IO uint32_t XIDAM;          /* 0x084  Extended ID AND Mask              */
    __IO uint32_t HPMS;           /* 0x088  High Priority Message Status      */
         uint32_t RESERVED4;      /* 0x08C                                    */
    __IO uint32_t RXF0S;          /* 0x090  Rx FIFO 0 Status                  */
    __IO uint32_t RXF0A;          /* 0x094  Rx FIFO 0 Acknowledge             */
    __IO uint32_t RXF1S;          /* 0x098  Rx FIFO 1 Status                  */
    __IO uint32_t RXF1A;          /* 0x09C  Rx FIFO 1 Acknowledge             */
         uint32_t RESERVED5[8];   /* 0x0A0-0x0BC                              */
    __IO uint32_t TXBC;           /* 0x0C0  TX Buffer Configuration           */
    __IO uint32_t TXFQS;          /* 0x0C4  TX FIFO/Queue Status              */
    __IO uint32_t TXBRP;          /* 0x0C8  TX Buffer Request Pending         */
    __IO uint32_t TXBAR;          /* 0x0CC  TX Buffer Add Request             */
    __IO uint32_t TXBCR;          /* 0x0D0  TX Buffer Cancellation Request    */
    __IO uint32_t TXBTO;          /* 0x0D4  TX Buffer Transmission Occurred   */
    __IO uint32_t TXBCF;          /* 0x0D8  TX Buffer Cancellation Finished   */
    __IO uint32_t TXBTIE;         /* 0x0DC  TX Buffer Transmission IE         */
    __IO uint32_t TXBCIE;         /* 0x0E0  TX Buffer Cancellation Finished IE*/
    __IO uint32_t TXEFS;          /* 0x0E4  TX Event FIFO Status              */
    __IO uint32_t TXEFA;          /* 0x0E8  TX Event FIFO Acknowledge         */
} FDCAN_GlobalTypeDef;

////////////////////////////// CCCR REGISTER BITS /////////////////////////////////////

#define FDCAN_CCCR_INIT   (1U << 0)   /* Initialization                     */
#define FDCAN_CCCR_CCE    (1U << 1)   /* Configuration Change Enable        */
#define FDCAN_CCCR_ASM    (1U << 2)   /* Restricted Operation Mode          */
#define FDCAN_CCCR_CSA    (1U << 3)   /* Clock Stop Acknowledge (read-only) */
#define FDCAN_CCCR_CSR    (1U << 4)   /* Clock Stop Request                 */
#define FDCAN_CCCR_MON    (1U << 5)   /* Bus Monitoring Mode (silent)       */
#define FDCAN_CCCR_DAR    (1U << 6)   /* Disable Automatic Retransmission   */
#define FDCAN_CCCR_TEST   (1U << 7)   /* Test Mode Enable                   */
#define FDCAN_CCCR_FDOE   (1U << 8)   /* FD Operation Enable                */
#define FDCAN_CCCR_BRSE   (1U << 9)   /* Bit Rate Switch Enable             */

////////////////////////////// TEST REGISTER BITS /////////////////////////////////////

#define FDCAN_TEST_LBCK   (1U << 4)   /* Loop Back Mode */

////////////////////////////// NBTP REGISTER FIELDS ///////////////////////////////////

#define FDCAN_NBTP_NTSEG2_Pos   0U     /* bits [6:0]   Nominal Time Segment 2 */
#define FDCAN_NBTP_NTSEG1_Pos   8U     /* bits [15:8]  Nominal Time Segment 1 */
#define FDCAN_NBTP_NBRP_Pos     16U    /* bits [24:16] Nominal Bit Rate Prescaler */
#define FDCAN_NBTP_NSJW_Pos     25U    /* bits [31:25] Nominal Resync Jump Width */

////////////////////////////// IR (INTERRUPT REGISTER) BITS ///////////////////////////

#define FDCAN_IR_RF0N    (1U << 0)    /* Rx FIFO 0 New Message              */
#define FDCAN_IR_RF0F    (1U << 1)    /* Rx FIFO 0 Full                     */
#define FDCAN_IR_RF0L    (1U << 2)    /* Rx FIFO 0 Message Lost             */
#define FDCAN_IR_RF1N    (1U << 3)    /* Rx FIFO 1 New Message              */
#define FDCAN_IR_TC      (1U << 7)    /* Transmission Completed             */
#define FDCAN_IR_BO      (1U << 19)   /* Bus_Off Status                     */
#define FDCAN_IR_EW      (1U << 18)   /* Warning Status                     */
#define FDCAN_IR_EP      (1U << 17)   /* Error Passive                      */

////////////////////////////// IE (INTERRUPT ENABLE) BITS /////////////////////////////

#define FDCAN_IE_RF0NE   (1U << 0)    /* Rx FIFO 0 New Message IE           */
#define FDCAN_IE_TCE     (1U << 7)    /* Transmission Completed IE          */

////////////////////////////// ILE (INTERRUPT LINE ENABLE) BITS ///////////////////////

#define FDCAN_ILE_EINT0  (1U << 0)    /* Enable Interrupt Line 0            */
#define FDCAN_ILE_EINT1  (1U << 1)    /* Enable Interrupt Line 1            */

////////////////////////////// RXGFC REGISTER BITS ////////////////////////////////////

/* Bits [1:0]: RRFE, RRFS
   Bits [3:2]: ANFE  (Accept Non-matching Frames Extended)
   Bits [5:4]: ANFS  (Accept Non-matching Frames Standard)
   Bits [20:16]: LSS (List Size Standard, 0-28)
   Bits [27:24]: LSE (List Size Extended, 0-8) */
#define FDCAN_RXGFC_RRFE      (1U << 0)
#define FDCAN_RXGFC_RRFS      (1U << 1)
#define FDCAN_RXGFC_ANFE_Pos  2U
#define FDCAN_RXGFC_ANFE_Msk  (3U << FDCAN_RXGFC_ANFE_Pos)
#define FDCAN_RXGFC_ANFS_Pos  4U
#define FDCAN_RXGFC_ANFS_Msk  (3U << FDCAN_RXGFC_ANFS_Pos)
#define FDCAN_RXGFC_LSS_Pos   16U
#define FDCAN_RXGFC_LSS_Msk   (0x1FU << FDCAN_RXGFC_LSS_Pos)
#define FDCAN_RXGFC_LSE_Pos   24U
#define FDCAN_RXGFC_LSE_Msk   (0x0FU << FDCAN_RXGFC_LSE_Pos)

////////////////////////////// RXF0S REGISTER BITS ////////////////////////////////////

#define FDCAN_RXF0S_F0FL_Pos  0U
#define FDCAN_RXF0S_F0FL_Msk  (0x0FU << FDCAN_RXF0S_F0FL_Pos)
#define FDCAN_RXF0S_F0GI_Pos  8U
#define FDCAN_RXF0S_F0GI_Msk  (0x03U << FDCAN_RXF0S_F0GI_Pos)
#define FDCAN_RXF0S_F0F       (1U << 24)
#define FDCAN_RXF0S_RF0L      (1U << 25)

////////////////////////////// PSR (PROTOCOL STATUS) BITS /////////////////////////////

#define FDCAN_PSR_BO     (1U << 7)    /* Bus_Off */
#define FDCAN_PSR_EW     (1U << 6)    /* Warning */
#define FDCAN_PSR_EP     (1U << 5)    /* Error Passive */

////////////////////////////// TXFQS (TX FIFO/QUEUE STATUS) BITS //////////////////////

#define FDCAN_TXFQS_TFFL_Pos   0U     /* bits [2:0]  TX FIFO Free Level     */
#define FDCAN_TXFQS_TFFL_Msk   (0x07U << FDCAN_TXFQS_TFFL_Pos)
#define FDCAN_TXFQS_TFGI_Pos   8U     /* bits [9:8]  TX FIFO Get Index      */
#define FDCAN_TXFQS_TFGI_Msk   (0x03U << FDCAN_TXFQS_TFGI_Pos)
#define FDCAN_TXFQS_TFQPI_Pos  16U    /* bits [17:16] TX FIFO Put Index     */
#define FDCAN_TXFQS_TFQPI_Msk  (0x03U << FDCAN_TXFQS_TFQPI_Pos)
#define FDCAN_TXFQS_TFQF       (1U << 21)  /* TX FIFO/Queue Full          */

////////////////////////////// MESSAGE SRAM LAYOUT (SIMPLIFIED FDCAN) //////////////////

/*  Fixed allocation per FDCAN instance — 0.8 KB = 212 words (RM0522 §46.4.6):
      28 Standard ID filter elements  ×  1 word  =  28 words (112 bytes)
       8 Extended ID filter elements  ×  2 words =  16 words  (64 bytes)
       3 Rx FIFO 0 elements           × 18 words =  54 words (216 bytes)
       3 Rx FIFO 1 elements           × 18 words =  54 words (216 bytes)
       3 Tx Event FIFO elements       ×  2 words =   6 words  (24 bytes)
       3 Tx Buffer elements           × 18 words =  54 words (216 bytes)
    Total = 212 words = 848 bytes.  End offset = 0x034C.
    Element size: 18 words (72 bytes) = 2-word header + 16-word data (64B max). */

#define SRAMCAN_FLS_OFFSET  0x000U   /* Standard filter start              */
#define SRAMCAN_FLE_OFFSET  0x070U   /* Extended filter start              */
#define SRAMCAN_RF0_OFFSET  0x0B0U   /* Rx FIFO 0 start                    */
#define SRAMCAN_RF1_OFFSET  0x188U   /* Rx FIFO 1 start  (0xB0+3×72)      */
#define SRAMCAN_TEF_OFFSET  0x260U   /* Tx Event FIFO start (0x188+3×72)  */
#define SRAMCAN_TFQ_OFFSET  0x278U   /* Tx Buffers start (0x260+3×8)      */

/* Element sizes — always 18 words (72 bytes) for RX/TX on this FDCAN variant */
#define SRAMCAN_RF_ELEMENT_SIZE   72U  /* 18 words per Rx FIFO element */
#define SRAMCAN_TX_ELEMENT_SIZE   72U  /* 18 words per Tx buffer       */
#define SRAMCAN_FLS_ELEMENT_SIZE   4U  /* 1 word per std filter        */
#define SRAMCAN_FLE_ELEMENT_SIZE   8U  /* 2 words per ext filter       */
#define SRAMCAN_INSTANCE_SIZE   0x350U /* Total SRAMCAN per instance   */

////////////////////////////// RX ELEMENT WORD 0 (R0) BITS ////////////////////////////

#define FDCAN_RX_R0_XTD   (1U << 30)  /* Extended Identifier       */
#define FDCAN_RX_R0_RTR   (1U << 29)  /* Remote Transmission Req   */

////////////////////////////// RX ELEMENT WORD 1 (R1) BITS ////////////////////////////

#define FDCAN_RX_R1_DLC_Pos  16U
#define FDCAN_RX_R1_DLC_Msk  (0x0FU << FDCAN_RX_R1_DLC_Pos)

////////////////////////////// TX ELEMENT WORD 0 (T0) BITS ////////////////////////////

#define FDCAN_TX_T0_XTD   (1U << 30)
#define FDCAN_TX_T0_RTR   (1U << 29)

////////////////////////////// TX ELEMENT WORD 1 (T1) BITS ////////////////////////////

#define FDCAN_TX_T1_DLC_Pos  16U

////////////////////////////// STANDARD FILTER ELEMENT BITS ////////////////////////////

/* SFT[31:30]: 00=range, 01=dual, 10=classic, 11=disabled
   SFEC[29:27]: filter element config
   SFID1[26:16]: first ID
   SFID2[10:0]:  second ID / mask */
#define FDCAN_SFE_SFT_Pos   30U
#define FDCAN_SFE_SFEC_Pos  27U
#define FDCAN_SFE_SFID1_Pos 16U
#define FDCAN_SFE_SFID2_Pos  0U

/* SFEC values (where to store matching frame) */
#define FDCAN_SFEC_DISABLE   0U
#define FDCAN_SFEC_FIFO0     1U
#define FDCAN_SFEC_FIFO1     2U
#define FDCAN_SFEC_REJECT    3U

////////////////////////////// FDCAN PERIPHERAL INSTANCE ///////////////////////////////

#define CAN_1  ((FDCAN_GlobalTypeDef *)FDCAN1_BASE)
/* STM32C562RE has only one FDCAN instance. */

////////////////////////////// CONFIGURATION ENUMS /////////////////////////////////////

/* Standard CAN baud rates (FDCAN kernel clock = HSI16 = 16 MHz) */
typedef enum {
    CAN_500KBPS = 0,
    CAN_250KBPS,
    CAN_125KBPS,
    CAN_1MBPS
} CAN_BaudRateType;

/* CAN operating modes */
typedef enum {
    CAN_MODE_NORMAL          = 0,   /* Normal bus operation (needs transceiver) */
    CAN_MODE_LOOPBACK        = 1,   /* Internal loopback (TX→RX, no pins)      */
    CAN_MODE_SILENT          = 2,   /* Listen-only (no ACK, no TX)             */
    CAN_MODE_SILENT_LOOPBACK = 3    /* Silent + loopback (self-test)           */
} CAN_ModeType;

////////////////////////////// CAN MESSAGE STRUCTURE ///////////////////////////////////

typedef struct {
    uint32_t StdId;       /* Standard identifier (11-bit, 0..0x7FF)       */
    uint32_t ExtId;       /* Extended identifier (29-bit, 0..0x1FFFFFFF)  */
    uint8_t  IDE;         /* 0 = standard frame, 1 = extended frame       */
    uint8_t  RTR;         /* 0 = data frame, 1 = remote frame             */
    uint8_t  DLC;         /* Data length code (0..8)                      */
    uint8_t  Data[8];     /* Payload bytes                                */
} CAN_MsgType;

////////////////////////////// CALLBACK & HANDLE ///////////////////////////////////////

typedef void (*CAN_RxCallback_t)(const CAN_MsgType *msg);

typedef struct {
    CAN_BaudRateType     baudRate;
    CAN_ModeType         mode;
    FDCAN_GlobalTypeDef *regs;
    CAN_RxCallback_t     rxCallback;
} CAN_HandleType;

////////////////////////////// API DECLARATIONS ////////////////////////////////////////

/* Constructor / initialisation */
void CAN_constructor(CAN_HandleType *handle, FDCAN_GlobalTypeDef *regs,
                     CAN_BaudRateType baudrate, CAN_ModeType mode);
void CAN_Init(CAN_HandleType *handle);

/* Filter configuration (accept-all into FIFO 0) */
void CAN_FilterAcceptAll(CAN_HandleType *handle);

/* Polling TX: returns 0 on success, -1 on failure */
int  CAN_Transmit(CAN_HandleType *handle, const CAN_MsgType *msg);

/* Polling RX (FIFO 0): returns 0 if a message was read, -1 if FIFO empty */
int  CAN_Receive(CAN_HandleType *handle, CAN_MsgType *msg);

/* Interrupt-based RX (FIFO 0) */
void CAN_EnableRXInterrupt(CAN_HandleType *handle, CAN_RxCallback_t callback);
void CAN_DisableRXInterrupt(CAN_HandleType *handle);

/* Built-in test: loopback self-test (returns 0 on PASS, -1 on FAIL) */
int  CAN_LoopbackTest(CAN_HandleType *handle, USART_HandleType *uart);

/* Transceiver test: send and receive on real bus (returns 0 on PASS, -1 on FAIL) */
int  CAN_TransceiverTest(CAN_HandleType *handle, USART_HandleType *uart);

#endif /* __BXCAN_H */
