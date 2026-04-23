#ifndef __HASH_H
#define __HASH_H

#include <stddef.h>
#include "../../GPIO/inc/gpio.h"  /* RCC_TypeDef, RCC */

#define __IO volatile

/* HASH base address and RCC clock enable (verified: STM32C562.svd) */
#define HASH_BASE          0x42000000U
#define RCC_AHB2ENR_HASHEN (1U << 16)

/*HASH_CR bits*/
#define HASH_CR_INIT 0x00000004U
#define HASH_CR_DMAE 0x00000008U
#define HASH_CR_ALG_SHA1 0x00000000U
#define HASH_CR_ALG_SHA224 0x00040000U
#define HASH_CR_ALG_SHA256 0x00060000U

/* HASH_IMR bits */
#define HASH_IMR_DINIE 0x00000001U

/* HASH_SR bits */
#define HASH_SR_DINIS 0x00000001U
#define HASH_SR_DCIS  0x00000002U
#define HASH_SR_DMAS  0x00000004U
#define HASH_SR_BUSY 0x00000008U

/* Sizes and return codes */
#define HASH_BLOCK_SIZE       64

typedef enum {
    HASH_OK = 0,
    HASH_ERROR = 1
} HASH_StatusTypeDef;

typedef enum {
    HASH_SHA1 = 0,
    HASH_SHA224,
    HASH_SHA256
} HASH_AlgorithmTypeDef;

typedef struct{
    
}HASH_ManualTypeDef;

typedef struct{

}HASH_DMATypeDef;

typedef struct{

}HASH_HandleTypeDef;

void Hash_Constructor(HASH_HandleTypeDef *hash);
void Hash_Init(HASH_HandleTypeDef *hash);
HASH_StatusTypeDef Hash_Update(HASH_HandleTypeDef *hash, const unsigned char *data, size_t len);
HASH_StatusTypeDef Hash_Final(HASH_HandleTypeDef *hash, unsigned char *hash_output);


#endif /* __HASH_H */