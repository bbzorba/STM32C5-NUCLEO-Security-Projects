#ifndef __HASH_H
#define __HASH_H

#include <stddef.h>
#include <stdint.h>
#include "../../GPIO/inc/gpio.h"  /* RCC_TypeDef, RCC */

#define __IO volatile

/* HASH base address (STM32C562RE, verified from STM32C562.svd) */
#define HASH_BASE           0x420C0400U

/* RCC AHB2 clock enable bit for HASH (bit 17, verified from STM32C562.svd) */
#define RCC_AHB2ENR_HASHEN  (1U << 17)

/* RCC AHB1 clock enable bit for LPDMA1 (bit 0, verified from STM32C562.svd) */
#define RCC_AHB1ENR_LPDMA1EN (1U << 0)

/* HASH_CR bits (bitOffset values from STM32C562.svd)
 *   bit  2       : INIT       - reset hash engine (self-clearing)
 *   bit  3       : DMAE       - enable DMA data-input requests
 *   bits [5:4]   : DATATYPE   - 00=32-bit no swap, 10=8-bit byte swap
 *   bit 13       : MDMAT      - multiple DMA transfers mode
 *   bits [18:17] : ALGO       - 00=SHA1, 10=SHA224, 11=SHA256       */
#define HASH_CR_INIT             0x00000004U
#define HASH_CR_DMAE             0x00000008U   /* bit  3                  */
#define HASH_CR_DATATYPE         0x00000030U   /* bits [5:4] mask         */
#define HASH_CR_DATATYPE_BYTE    0x00000020U   /* DATATYPE=10: byte swap  */
#define HASH_CR_MDMAT            0x00002000U   /* bit 13                  */
#define HASH_CR_ALGO             0x00060000U   /* bits [18:17] mask       */
#define HASH_CR_ALGO_SHA1        0x00000000U   /* ALGO = 00               */
#define HASH_CR_ALGO_SHA224      0x00040000U   /* ALGO = 10               */
#define HASH_CR_ALGO_SHA256      0x00060000U   /* ALGO = 11               */

/* HASH_STR bits
 *   bits [4:0] : NBLW - valid bits in last word written to DIN
 *   bit  8     : DCAL - trigger digest calculation (self-clearing) */
#define HASH_STR_NBLW_MASK  0x0000001FU
#define HASH_STR_DCAL       0x00000100U

/* HASH_SR bits */
#define HASH_SR_DINIS       0x00000001U   /* bit 0: data-input ready  */
#define HASH_SR_DCIS        0x00000002U   /* bit 1: digest complete   */
#define HASH_SR_BUSY        0x00000008U   /* bit 3: engine busy       */

/* HASH register structure — offsets verified against STM32C562.svd:
 *   CR   @ 0x000   DIN  @ 0x004   STR  @ 0x008
 *   HRA0-4 @ 0x00C..0x01C  (read-access interim words)
 *   IMR  @ 0x020   SR   @ 0x024
 *
 * SHA-256 digest output (HR0-HR7) lives at HASH_BASE + 0x310.
 * Use HASH_HR_OUT below — do NOT read from HRA[].                  */
typedef struct {
    __IO uint32_t CR;      /* 0x000  Control Register               */
    __IO uint32_t DIN;     /* 0x004  Data Input Register            */
    __IO uint32_t STR;     /* 0x008  Start Register (NBLW + DCAL)   */
    __IO uint32_t HRA[5];  /* 0x00C..0x01C  read-access hash words  */
    __IO uint32_t IMR;     /* 0x020  Interrupt Mask Register        */
    __IO uint32_t SR;      /* 0x024  Status Register                */
} HASH_ManualTypeDef;

/* SHA-256/SHA-224/SHA-1 digest output registers HR0-HR7 at offset 0x310 */
#define HASH_HR_OUT  ((volatile uint32_t *)(HASH_BASE + 0x310U))
#define HASH_regs    ((HASH_ManualTypeDef *)HASH_BASE)

/* -----------------------------------------------------------------------
 * LPDMA1 Channel 0 registers
 * Base: 0x40020000 + 0x50 = 0x40020050 (verified from STM32C562.svd)
 * LPDMA1_REQUEST_HASH_IN = 63 (verified from stm32c5xx_ll_dma.h)
 * ----------------------------------------------------------------------- */
#define LPDMA1_BASE              0x40020000U
#define LPDMA1_C0_BASE           (LPDMA1_BASE + 0x50U)

#define LPDMA1_C0CFCR  (*(volatile uint32_t *)(LPDMA1_C0_BASE + 0x0CU)) /* Flag clear  */
#define LPDMA1_C0SR    (*(volatile uint32_t *)(LPDMA1_C0_BASE + 0x10U)) /* Status      */
#define LPDMA1_C0CR    (*(volatile uint32_t *)(LPDMA1_C0_BASE + 0x14U)) /* Control     */
#define LPDMA1_C0TR1   (*(volatile uint32_t *)(LPDMA1_C0_BASE + 0x40U)) /* Transfer 1  */
#define LPDMA1_C0TR2   (*(volatile uint32_t *)(LPDMA1_C0_BASE + 0x44U)) /* Transfer 2  */
#define LPDMA1_C0BR1   (*(volatile uint32_t *)(LPDMA1_C0_BASE + 0x48U)) /* Block reg 1 */
#define LPDMA1_C0SAR   (*(volatile uint32_t *)(LPDMA1_C0_BASE + 0x4CU)) /* Src addr    */
#define LPDMA1_C0DAR   (*(volatile uint32_t *)(LPDMA1_C0_BASE + 0x50U)) /* Dst addr    */
#define LPDMA1_C0LLR   (*(volatile uint32_t *)(LPDMA1_C0_BASE + 0x7CU)) /* Linked list */

/* LPDMA1 CCR bits */
#define LPDMA1_CCR_EN            (1U << 0)  /* Channel enable          */
#define LPDMA1_CCR_RESET         (1U << 1)  /* Channel reset           */

/* LPDMA1 CTR1: SDW_LOG2[1:0]=2(word), SINC[3]=1, DDW_LOG2[17:16]=2(word), DINC[19]=0 */
#define LPDMA1_CTR1_MEM_TO_PERIPH_WORD  ((2U << 0) | (1U << 3) | (2U << 16))

/* LPDMA1 CTR2: REQSEL=63 (HASH_IN), SWREQ=0, BREQ=0 */
#define LPDMA1_REQUEST_HASH_IN   63U

/* LPDMA1 C0SR flags */
#define LPDMA1_C0SR_TCF          (1U << 8)  /* Transfer complete flag  */

/* HASH_DIN address (used as DMA destination) */
#define HASH_DIN_ADDR            (HASH_BASE + 0x004U)

typedef enum {
    HASH_OK    = 0,
    HASH_ERROR = 1
} HASH_StatusTypeDef;

typedef enum {
    HASH_SHA1   = 0,
    HASH_SHA224 = 1,
    HASH_SHA256 = 2
} HASH_AlgorithmTypeDef;

typedef struct {
    HASH_ManualTypeDef    *regs;
    HASH_AlgorithmTypeDef  algorithm;
    HASH_StatusTypeDef     state;
    size_t                 msg_len;       /* bytes fed, used for NBLW */
    unsigned char          hash_output[32];
} HASH_HandleTypeDef;

void               Hash_Constructor (HASH_HandleTypeDef *hash, HASH_AlgorithmTypeDef algorithm);
void               Hash_Init        (HASH_HandleTypeDef *hash);
void               Hash_Reset       (HASH_HandleTypeDef *hash);
HASH_StatusTypeDef Hash_Process_Data(HASH_HandleTypeDef *hash, const unsigned char *data, size_t len);
HASH_StatusTypeDef Hash_Final       (HASH_HandleTypeDef *hash, unsigned char *hash_output);

#endif /* __HASH_H */