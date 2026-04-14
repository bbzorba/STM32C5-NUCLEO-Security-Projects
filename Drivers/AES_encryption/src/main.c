#include "../inc/aes.h"

void delay_fn(volatile int count) {
    for (volatile int i = 0; i < count; i++);
}

int main(void) {
    AES_HandleTypeDef aes_handle;
    AES_constructor(&aes_handle);

    uint8_t key[AES_KEY_SIZE];
    uint8_t iv[AES_IV_SIZE];
    uint8_t plaintext[AES_BLOCK_SIZE];
    Init_Sample_Test_Vector(key, iv, plaintext);

    uint8_t encrypted_data[AES_BLOCK_SIZE];
    uint8_t decrypted_data[AES_BLOCK_SIZE];

    while (1) {
        AES_Encrypt_Init(&aes_handle, key, iv);
        AES_Encrypt(&aes_handle, plaintext, encrypted_data, AES_BLOCK_SIZE);

        AES_Decrypt_Init(&aes_handle, key, iv);
        AES_Decrypt(&aes_handle, encrypted_data, decrypted_data, AES_BLOCK_SIZE);

        delay_fn(1000000);
    }
}

