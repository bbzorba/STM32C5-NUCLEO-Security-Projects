#ifndef __ECDSA_H
#define __ECDSA_H

#include <string.h>
#include <stdint.h>
#include <stddef.h>
#include "../../GPIO/inc/gpio.h"   /* RCC_TypeDef / RCC */
#include "../../NVIC/inc/nvic.h"
#include "../../UART/inc/uart.h"
#include "../../HASH/inc/hash.h"   /* For hashing messages before signing */

#define ECC_BYTES 32

typedef struct {
    uint8_t pubkey_x[ECC_BYTES];
    uint8_t pubkey_y[ECC_BYTES];
    HASH_HandleTypeDef *hhash;
} ECDSA_HandleTypeDef;

/* Init with public key + hash driver */
void ECDSA_Init(ECDSA_HandleTypeDef *hecdsa,
                HASH_HandleTypeDef *hhash,
                const uint8_t *pub_x,
                const uint8_t *pub_y);

/* Verify signature of message */
int ECDSA_Verify(ECDSA_HandleTypeDef *hecdsa,
                 const uint8_t *msg,
                 size_t len,
                 const uint8_t *r,
                 const uint8_t *s);

#endif /* __ECDSA_H */