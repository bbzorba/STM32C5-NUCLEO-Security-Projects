#include "../inc/aes.h"

/* ── One-shot mode functions ─────────────────────────────────────────────
 * These set up the handle, run the full operation, and return.
 * Use these for bulk data (e.g. encrypting a file buffer in one call).
 * Use _Init + _Process directly if you need to feed data in chunks.
 * ----------------------------------------------------------------------- */
static int run_operation(AES_ModeTypeDef mode, AES_KeySizeTypeDef ks, int enc,
               const uint8_t *key, const uint8_t *iv,
               const uint8_t *in, uint8_t *out, uint32_t len)
{
    AES_HandleTypeDef aes_handler = { .mode = mode, .keysize = ks };
    AES_constructor(&aes_handler);
    int r = enc ? AES_Encrypt_Init(&aes_handler, key, iv) : AES_Decrypt_Init(&aes_handler, key, iv);
    if (r) return r;
    return AES_Process(&aes_handler, in, out, len);
}

int AES_ECB_Encrypt(const uint8_t *key, AES_KeySizeTypeDef ks, const uint8_t *in, uint8_t *out, uint32_t len) {
    return run_operation(AES_MODE_ECB, ks, 1, key, NULL, in, out, len);
}
int AES_ECB_Decrypt(const uint8_t *key, AES_KeySizeTypeDef ks, const uint8_t *in, uint8_t *out, uint32_t len) {
    return run_operation(AES_MODE_ECB, ks, 0, key, NULL, in, out, len);
}
int AES_CBC_Encrypt(const uint8_t *key, AES_KeySizeTypeDef ks, const uint8_t *iv, const uint8_t *in, uint8_t *out, uint32_t len) {
    return run_operation(AES_MODE_CBC, ks, 1, key, iv, in, out, len);
}
int AES_CBC_Decrypt(const uint8_t *key, AES_KeySizeTypeDef ks, const uint8_t *iv, const uint8_t *in, uint8_t *out, uint32_t len) {
    return run_operation(AES_MODE_CBC, ks, 0, key, iv, in, out, len);
}
int AES_CTR_Crypt(const uint8_t *key, AES_KeySizeTypeDef ks, const uint8_t *nonce, const uint8_t *in, uint8_t *out, uint32_t len) {
    return run_operation(AES_MODE_CTR, ks, 1, key, nonce, in, out, len);  /* CTR encrypt == decrypt */
}

