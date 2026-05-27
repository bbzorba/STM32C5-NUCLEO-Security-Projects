#include "rsa.h"
#include <string.h>

/* Number of active limbs for each key size: 16 / 32 / 64 */
static int key_limbs(RSA_KeySizeTypeDef key_size) { return 16 << (int)key_size; }

/* ── Byte ↔ bignum conversions (big-endian, nl active limbs) ────────── */

static void bn_from_be(bn_t *r, const uint8_t *src, int num_limbs)
{
    int num_bytes = num_limbs * 4;
    memset(r->d, 0, RSA_MAX_LIMBS * sizeof(uint32_t));
    for (int i = 0; i < num_bytes; i++) {
        int limb  = (num_bytes - 1 - i) / 4;
        int shift = ((num_bytes - 1 - i) % 4) * 8;
        r->d[limb] |= (uint32_t)src[i] << shift;
    }
}

static void bn_to_be(uint8_t *dst, const bn_t *a, int num_limbs)
{
    int num_bytes = num_limbs * 4;
    for (int i = 0; i < num_bytes; i++) {
        int limb  = (num_bytes - 1 - i) / 4;
        int shift = ((num_bytes - 1 - i) % 4) * 8;
        dst[i] = (uint8_t)(a->d[limb] >> shift);
    }
}

/* ── Schoolbook multiply: r[2*nl] = a[nl] × b[nl] ───────────────────── */

static void bn_mul(uint32_t *r, const bn_t *a, const bn_t *b, int num_limbs)
{
    memset(r, 0, 2 * num_limbs * sizeof(uint32_t));
    for (int i = 0; i < num_limbs; i++) {
        uint64_t carry = 0;
        for (int j = 0; j < num_limbs; j++) {
            uint64_t p = (uint64_t)a->d[i] * b->d[j] + r[i + j] + carry;
            r[i + j]   = (uint32_t)p;
            carry       = p >> 32;
        }
        r[i + num_limbs] += (uint32_t)carry;
    }
}

/* ── Shift-and-subtract reduction: r = a_in[2*nl] mod n[nl] ─────────── */

static void bn_reduce(bn_t *r, const uint32_t *a_in, const bn_t *n, int num_limbs)
{
    /* Working buffer: full double-width, upper part zeroed */
    uint32_t acc[2 * RSA_MAX_LIMBS];
    memcpy(acc, a_in, 2 * num_limbs * sizeof(uint32_t));
    memset(acc + 2 * num_limbs, 0, (2 * RSA_MAX_LIMBS - 2 * num_limbs) * sizeof(uint32_t));

    /* Locate MSB of acc */
    int acc_msb = -1;
    for (int i = 2 * num_limbs - 1; i >= 0 && acc_msb < 0; i--) {
        if (acc[i]) {
            uint32_t w = acc[i]; int bit = 31;
            while (bit > 0 && !(w >> bit)) bit--;
            acc_msb = 32 * i + bit;
        }
    }

    /* Locate MSB of n */
    int n_msb = -1;
    for (int i = num_limbs - 1; i >= 0 && n_msb < 0; i--) {
        if (n->d[i]) {
            uint32_t w = n->d[i]; int bit = 31;
            while (bit > 0 && !(w >> bit)) bit--;
            n_msb = 32 * i + bit;
        }
    }

    if (acc_msb < 0 || n_msb < 0) goto done;

    /* Align n to MSB of acc and subtract down */
    for (int shift = acc_msb - n_msb; shift >= 0; shift--) {
        int word_shift = shift / 32, bit_shift = shift % 32;
        int top = num_limbs + word_shift;

        /* Compare acc vs (n << shift): scan from top down */
        int cmp = 0;
        for (int i = top; i >= word_shift && !cmp; i--) {
            int j = i - word_shift;
            uint32_t ni = 0;
            if (j >= 0 && j < num_limbs)              ni  = bit_shift ? (n->d[j] << bit_shift) : n->d[j];
            if (j > 0  && j <= num_limbs && bit_shift) ni |= n->d[j - 1] >> (32 - bit_shift);
            if      (acc[i] < ni) cmp = -1;
            else if (acc[i] > ni) cmp =  1;
        }

        if (cmp < 0) continue;          /* acc < n<<shift, skip */

        /* acc -= (n << shift) */
        uint64_t borrow = 0;
        for (int i = word_shift; i <= top + 1 && i < 2 * RSA_MAX_LIMBS; i++) {
            int j = i - word_shift;
            uint32_t ni = 0;
            if (j >= 0 && j < num_limbs)              ni  = bit_shift ? (n->d[j] << bit_shift) : n->d[j];
            if (j > 0  && j <= num_limbs && bit_shift) ni |= n->d[j - 1] >> (32 - bit_shift);
            uint64_t sub = (uint64_t)acc[i] - ni - borrow;
            acc[i]  = (uint32_t)sub;
            borrow  = (sub >> 63) & 1U;
        }
    }

done:
    memcpy(r->d, acc, num_limbs * sizeof(uint32_t));
    memset(r->d + num_limbs, 0, (RSA_MAX_LIMBS - num_limbs) * sizeof(uint32_t));
}

/* ── Modular multiply: r = (a × b) mod n ────────────────────────────── */

static void bn_mulmod(bn_t *r, const bn_t *a, const bn_t *b, const bn_t *n, int num_limbs)
{
    uint32_t tmp[2 * RSA_MAX_LIMBS];
    bn_mul(tmp, a, b, num_limbs);
    bn_reduce(r, tmp, n, num_limbs);
}

/* ── Modular exponentiation: r = base^exp mod n (square-and-multiply) ── */

static void bn_powmod(bn_t *r, const bn_t *base, const bn_t *exp, const bn_t *n, int num_limbs)
{
    bn_t result, sq;
    memset(&result, 0, sizeof(result)); result.d[0] = 1U;
    memcpy(sq.d, base->d, RSA_MAX_LIMBS * sizeof(uint32_t));

    /* Find MSB of exponent to limit the iteration count */
    int msb = -1;
    for (int i = num_limbs - 1; i >= 0 && msb < 0; i--) {
        if (exp->d[i]) {
            uint32_t w = exp->d[i]; int bit = 31;
            while (bit > 0 && !(w >> bit)) bit--;
            msb = 32 * i + bit;
        }
    }
    if (msb < 0) { memcpy(r->d, result.d, RSA_MAX_LIMBS * sizeof(uint32_t)); return; }

    for (int i = 0; i <= msb; i++) {
        if ((exp->d[i / 32] >> (i % 32)) & 1U)
            bn_mulmod(&result, &result, &sq, n, num_limbs);
        if (i < msb)
            bn_mulmod(&sq, &sq, &sq, n, num_limbs);
    }
    memcpy(r->d, result.d, RSA_MAX_LIMBS * sizeof(uint32_t));
}

/* ── Public API ─────────────────────────────────────────────────────── */

RSA_StatusTypeDef RSA_PubOp(const RSA_PubKey_t *pub, const uint8_t *in, uint8_t *out)
{
    int num_limbs = key_limbs(pub->key_size);
    bn_t m, c;
    bn_from_be(&m, in, num_limbs);
    bn_powmod(&c, &m, &pub->e, &pub->n, num_limbs);
    bn_to_be(out, &c, num_limbs);
    return RSA_OK;
}

RSA_StatusTypeDef RSA_PrivOp(const RSA_PrivKey_t *priv, const uint8_t *in, uint8_t *out)
{
    int num_limbs = key_limbs(priv->key_size);
    bn_t c, m;
    bn_from_be(&c, in, num_limbs);
    bn_powmod(&m, &c, &priv->d, &priv->n, num_limbs);
    bn_to_be(out, &m, num_limbs);
    return RSA_OK;
}
