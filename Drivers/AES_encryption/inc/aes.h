// AES header file for STM32C562RE Nucleo board
#include <stdint.h>
#include <string.h>
#include "../../GPIO/inc/gpio.h"

#ifndef __AES_H
#define __AES_H

///////////////////////////////////////ADDRESS DEFINITIONS//////////////////////////////////////////////////
#define __IO volatile

// STM32C562RE base addresses
#define AES_BASE 0x42090000U // Base address for AES
#define SAES_BASE 0x420C0C00U // Base address for Secure AES

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
// RCC peripheral declarations and direct port access use RCC_TypeDef struct below (RCC->AHB2ENR etc.)



//////////////////////////////////////REGISTER BIT DEFINITIONS/////////////////////////////////////////////////
/* AES control register (CR) bit definitions */
#define AES_CR_EN      (1U << 0)   // AES enable
#define AES_CR_DATATYPE_NONE  (0U << 1)  // No data swap
#define AES_CR_MODE_ENCRYPT   (0U << 3)  // Encryption mode (MODE[1:0] = 00)
#define AES_CR_MODE_KEYDER    (1U << 3)  // Key derivation (MODE[1:0] = 01)
#define AES_CR_MODE_DECRYPT   (2U << 3)  // Decryption (MODE[1:0] = 10)
#define AES_CR_CHMOD_ECB      (0U << 5)  // ECB chaining (CHMOD = 000)
#define AES_CR_CHMOD_CBC      (1U << 5)  // CBC chaining (CHMOD = 001)
#define AES_CR_CCFC           (1U << 7)  // Computation complete flag clear
#define AES_CR_ERRC           (1U << 8)  // Error clear
#define AES_CR_CCFIE          (1U << 9)  // CCF interrupt enable
#define AES_CR_ERRIE          (1U << 10) // Error interrupt enable
#define AES_CR_DMAINEN        (1U << 11) // DMA input enable
#define AES_CR_DMAOUTEN       (1U << 12) // DMA output enable

/* AES status register (SR) bit definitions */
#define AES_SR_CCF     (1U << 0)  // Computation complete flag
#define AES_SR_RDERR   (1U << 1)  // Read error flag
#define AES_SR_WRERR   (1U << 2)  // Write error flag
#define AES_SR_BUSY    (1U << 3)  // Busy flag
#define AES_SR_TIMEOUT (1U << 8)  // Timeout flag (SAES only)

//////////////////////////////////////////////////////////////////////////////////////////////////////////////



///////////////////////////////////CONFIGURATION DEFINITIONS//////////////////////////////////////////////////
#define AES_BLOCK_SIZE 16 // AES block size in bytes
#define AES_KEY_SIZE 16   // AES key size in bytes (128 bits)
#define AES_IV_SIZE 16    // AES initialization vector size in bytes (128 bits)
#define AES_MAX_DATA_SIZE 1024 // Maximum data size for encryption/decryption (must be multiple of AES_BLOCK_SIZE)
#define AES_SUCCESS 0
#define AES_ERROR -1
#define AES_INVALID_KEY -2
#define AES_INVALID_IV -3
#define AES_INVALID_DATA_SIZE -4
#define AES_BUSY -5
#define AES_TIMEOUT -6
#define AES_ERROR_BUSY -7
#define AES_ERROR_TIMEOUT -8
#define AES_ERROR_INVALID_PARAM -9
#define AES_ERROR_UNKNOWN -10
#define AES_ERROR_NONE 0
//////////////////////////////////////////////////////////////////////////////////////////////////////////////



/////////////////////////////////GPIO STRUCTURES AND ENUMERATIONS/////////////////////////////////////////////
/* RCC register definition structure — STM32C562RE
 * Field offsets from RCC_BASE (0x44020C00):
 *   AHB2ENR @ 0x08C  (GPIOA..E, GPIOH clock enable)
 * Verified against STM32C562RE CMSIS header (stm32c562xx.h, stm32c5xx-dfp). */
/* AES hardware register map — STM32C562RE RM0522 §28
 * AES_BASE  = 0x42090000
 * SAES_BASE = 0x420C0C00  (Secure AES, TrustZone) */
typedef struct
{
    __IO uint32_t CR;        // 0x00: Control Register
    __IO uint32_t SR;        // 0x04: Status Register
    __IO uint32_t DINR;      // 0x08: Data Input Register (write 4 words to feed one block)
    __IO uint32_t DOUTR;     // 0x0C: Data Output Register (read 4 words after CCF)
    __IO uint32_t KEYR[8];  // 0x10-0x2C: Key Registers [0]=MSW … [7]=LSW (256-bit)
    __IO uint32_t IVR[4];   // 0x30-0x3C: IV Registers  [0]=MSW … [3]=LSW (128-bit)
    __IO uint32_t SUSP[8];  // 0x40-0x5C: Suspend registers (context save)
} AES_ManualTypeDef;

/* AES handle structure */
typedef struct {
    AES_ManualTypeDef *regs;  // Pointer to AES peripheral base address
} AES_HandleTypeDef;

void AES_constructor(AES_HandleTypeDef *AESx);


// Peripheral declarations
#define AES ((AES_ManualTypeDef *)AES_BASE)
#define SAES ((AES_ManualTypeDef *)SAES_BASE)
///////////////////////////////////////////////////////////////////////////////////////////////////////////////



/////////////////////////////////////AES FUNCTION PROTOTYPES/////////////////////////////////////////////////
/* Function prototypes */
int AES_Encrypt_Init(AES_HandleTypeDef *AESx, uint8_t *key, uint8_t *iv);
int AES_Encrypt(AES_HandleTypeDef *AESx, uint8_t *input, uint8_t *output, uint32_t length);
int AES_Decrypt_Init(AES_HandleTypeDef *AESx, uint8_t *key, uint8_t *iv);
int AES_Decrypt(AES_HandleTypeDef *AESx, uint8_t *input, uint8_t *output, uint32_t length);
void Init_Sample_Test_Vector(uint8_t *key, uint8_t *iv, uint8_t *plaintext);

/* SAES (Secure AES) — same API, different peripheral base */
void SAES_constructor(AES_HandleTypeDef *AESx);
int SAES_Encrypt_Init(AES_HandleTypeDef *AESx, uint8_t *key, uint8_t *iv);
int SAES_Encrypt(AES_HandleTypeDef *AESx, uint8_t *input, uint8_t *output, uint32_t length);
int SAES_Decrypt_Init(AES_HandleTypeDef *AESx, uint8_t *key, uint8_t *iv);
int SAES_Decrypt(AES_HandleTypeDef *AESx, uint8_t *input, uint8_t *output, uint32_t length);
///////////////////////////////////////////////////////////////////////////////////////////////////////////////


#endif // __AES_H