/*
 * RSA demo / test for STM32C562RE.
 * Uses PKCS#1 v2.1 NIST test vectors from rsa_vectors.h.
 * For each vector: raw RSA roundtrip test pub_op(priv_op(sig)) == sig.
 */
#include "rsa.h"
#include "rsa_vectors.h"
#include "uart.h"
#include <string.h>
#include <stdio.h>

static USART_HandleType huart;

/* Build RSA key structs from the byte-array vectors and run a roundtrip test.
 * Input:  sig (known PSS signature, used as raw RSA plaintext)
 * Test:   enc = pub_op(sig)  then  dec = priv_op(enc)
 * Pass:   dec == sig
 */
static void run_test(const rsa_test_vec_t *v)
{
    unsigned num_bytes = RSA_KEY_BYTES(v->key_size);
    char buf[32];
    snprintf(buf, sizeof(buf), "\r\n--- RSA-%s ---\r\n", v->name);
    USART_WriteString(&huart, buf);

    /* Build key structs from big-endian byte arrays */
    RSA_PubKey_t  pub;
    RSA_PrivKey_t priv;
    pub.key_size  = v->key_size;
    priv.key_size = v->key_size;
    pub.e.d[0] = 0x00010001U;   /* e = 65537 */
    for (unsigned i = 1; i < RSA_MAX_LIMBS; i++) pub.e.d[i] = 0;

    /* Import n and d from big-endian byte arrays */
    int num_limbs = (int)(num_bytes / 4);
    for (int i = 0; i < num_limbs; i++) {
        int src = (num_limbs - 1 - i) * 4;
        uint32_t w = ((uint32_t)v->n_be[src]   << 24)
                   | ((uint32_t)v->n_be[src+1] << 16)
                   | ((uint32_t)v->n_be[src+2] <<  8)
                   | ((uint32_t)v->n_be[src+3]);
        pub.n.d[i] = priv.n.d[i] = w;
    }
    for (int i = num_limbs; i < RSA_MAX_LIMBS; i++) pub.n.d[i] = priv.n.d[i] = 0;

    for (int i = 0; i < num_limbs; i++) {
        int src = (num_limbs - 1 - i) * 4;
        priv.d.d[i] = ((uint32_t)v->d_be[src]   << 24)
                    | ((uint32_t)v->d_be[src+1] << 16)
                    | ((uint32_t)v->d_be[src+2] <<  8)
                    | ((uint32_t)v->d_be[src+3]);
    }
    for (int i = num_limbs; i < RSA_MAX_LIMBS; i++) priv.d.d[i] = 0;

    /* Roundtrip: enc = pub_op(sig), dec = priv_op(enc), verify dec == sig */
    static uint8_t enc[RSA_MAX_BYTES];
    static uint8_t dec[RSA_MAX_BYTES];

    RSA_PubOp (&pub,  v->sig, enc);
    RSA_PrivOp(&priv, enc,    dec);

    int pass = (memcmp(dec, v->sig, num_bytes) == 0);
    USART_WriteString(&huart, "Roundtrip: ");
    USART_WriteString(&huart, pass ? "PASS\r\n" : "FAIL\r\n");

    if (!pass) {
        USART_WriteString(&huart, "Sig (head):");
        print_hex(&huart, v->sig, 16);
        USART_WriteString(&huart, "\r\nDec (head):");
        print_hex(&huart, dec, 16);
        USART_WriteString(&huart, "\r\n");
    }
}

int main(void)
{
    __asm volatile("cpsie i" ::: "memory");
    USART_constructor(&huart, USART_2, TX_ONLY, __115200);

    USART_WriteString(&huart, "\r\n=== RSA NIST VECTORS ===\r\n");
    USART_WriteString(&huart, "Source: PKCS#1 v2.1 pss-vect.txt\r\n");
    USART_WriteString(&huart, "Test:   pub_op(priv_op(sig)) == sig\r\n");

    for (unsigned i = 0; i < RSA_NUM_VECTORS; i++)
        run_test(&rsa_vectors[i]);

    USART_WriteString(&huart, "\r\n=== DONE ===\r\n");
    while (1);
}
