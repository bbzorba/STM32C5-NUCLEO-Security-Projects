#include "../inc/sec_boot.h"

void SEC_BOOT_Init(SEC_BOOT_HandleTypeDef *hsb,
                   const uint8_t *pub_x,
                   const uint8_t *pub_y)
{
    HASH_Constructor(&hsb->hhash);
    ECDSA_Init(&hsb->hecdsa, &hsb->hhash, pub_x, pub_y);
}

SEC_BOOT_Status SEC_BOOT_VerifyImage(SEC_BOOT_HandleTypeDef *hsb,
                                     const uint8_t *image,
                                     size_t         len,
                                     const uint8_t *sig_r,
                                     const uint8_t *sig_s)
{
    if (!ECDSA_Verify(&hsb->hecdsa, image, len, sig_r, sig_s))
        return SEC_BOOT_ERR_SIGNATURE;
    return SEC_BOOT_OK;
}