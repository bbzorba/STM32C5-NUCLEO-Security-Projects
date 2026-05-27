#ifndef __RSA_H
#define __RSA_H

#include <stdint.h>

/* Supported key sizes, selected at runtime via RSA_KeySizeTypeDef. */
typedef enum {
    RSA_KEY_512  = 0,
    RSA_KEY_1024 = 1,
    RSA_KEY_2048 = 2
} RSA_KeySizeTypeDef;

typedef enum {
    RSA_OK    = 0,
    RSA_ERROR = 1
} RSA_StatusTypeDef;

/* Bignum: always allocated for the maximum size (2048-bit = 64 limbs).
 * Limbs are little-endian: d[0] = least significant 32-bit word.
 * For smaller keys (512 / 1024-bit) the upper limbs are zero. */
#define RSA_MAX_LIMBS  64             /* 2048 / 32                    */
#define RSA_MAX_BYTES  256            /* 2048 / 8                     */
#define RSA_KEY_BYTES(key_size)  (64U << (unsigned)(key_size))  /* 64, 128, 256  */

typedef struct {
    uint32_t d[RSA_MAX_LIMBS];
} bn_t;

typedef struct {
    bn_t              n;         /* modulus         */
    bn_t              e;         /* public exponent */
    RSA_KeySizeTypeDef key_size;
} RSA_PubKey_t;

typedef struct {
    bn_t              n;         /* modulus          */
    bn_t              d;         /* private exponent */
    RSA_KeySizeTypeDef key_size;
} RSA_PrivKey_t;

/* Compute out = in^e mod n  (encrypt / verify).  in/out: RSA_KEY_BYTES(key->key_size) bytes, big-endian. */
RSA_StatusTypeDef RSA_PubOp (const RSA_PubKey_t  *pub,  const uint8_t *in, uint8_t *out);
/* Compute out = in^d mod n  (decrypt / sign).   in/out: RSA_KEY_BYTES(key->key_size) bytes, big-endian. */
RSA_StatusTypeDef RSA_PrivOp(const RSA_PrivKey_t *priv, const uint8_t *in, uint8_t *out);

#endif /* __RSA_H */