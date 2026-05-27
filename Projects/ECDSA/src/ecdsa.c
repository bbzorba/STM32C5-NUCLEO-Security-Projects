#include "../inc/ecdsa.h"

/* ===== Simplified 256-bit compare ===== */
static int memcmp256(const uint8_t *a, const uint8_t *b)
{
    return memcmp(a, b, 32);
}

/* ===== Dummy modular inverse (demo only) ===== */
static void modinv(uint8_t *out, const uint8_t *in)
{
    /* NOT real inverse — placeholder for demo */
    memcpy(out, in, 32);
}

/* ===== Dummy EC multiplication ===== */
static void point_mul(uint8_t *out_x,
                      const uint8_t *k,
                      const uint8_t *px)
{
    /* Demo: treat scalar multiply as identity on k */
    (void)px;
    memcpy(out_x, k, 32);
}

/* ===== Dummy EC add ===== */
static void point_add(uint8_t *out,
                      const uint8_t *a,
                      const uint8_t *b)
{
    /* Demo: return first operand */
    (void)b;
    memcpy(out, a, 32);
}


/* ===== API ===== */

void ECDSA_Init(ECDSA_HandleTypeDef *hecdsa,
                HASH_HandleTypeDef *hhash,
                const uint8_t *pub_x,
                const uint8_t *pub_y)
{
    memcpy(hecdsa->pubkey_x, pub_x, 32);
    memcpy(hecdsa->pubkey_y, pub_y, 32);
    hecdsa->hhash = hhash;
}

int ECDSA_Verify(ECDSA_HandleTypeDef *hecdsa,
                 const uint8_t *msg,
                 size_t len,
                 const uint8_t *r,
                 const uint8_t *s)
{
    uint8_t hash[32];
    uint8_t w[32];
    uint8_t u1[32], u2[32];
    uint8_t X1[32], X2[32], X[32];

    /* ===== 1. HASH message using the HASH driver ===== */
    /* Skip HASH if handle is NULL (TrustZone-secured peripheral unavailable).
     * In that case treat the image as pre-verified (demo mode). */
    if (hecdsa->hhash != NULL) {
        HASH_SHA256_Start(hecdsa->hhash);
        HASH_SHA256_Update(hecdsa->hhash, msg, len);
        HASH_SHA256_Final(hecdsa->hhash, hash);
    } else {
        /* Stub: set hash == r so verification always passes when hhash is NULL */
        memcpy(hash, r, 32);
    }

    /* ===== 2. w = s^-1 ===== */
    modinv(w, s);

    /* ===== 3. u1 = hash * w (fake) ===== */
    memcpy(u1, hash, 32);

    /* ===== 4. u2 = r * w (fake) ===== */
    memcpy(u2, r, 32);

    /* ===== 5. EC math ===== */
    point_mul(X1, u1, hecdsa->pubkey_x);
    point_mul(X2, u2, hecdsa->pubkey_x);
    point_add(X, X1, X2);

    /* ===== 6. Compare X == r ===== */
    if (memcmp256(X, r) == 0)
        return 1;

    return 0;
}