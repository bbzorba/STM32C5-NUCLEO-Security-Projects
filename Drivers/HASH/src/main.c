#include <string.h>
#include "../inc/hash.h"
#include "../../UART/inc/uart.h"

static USART_HandleType huart;

typedef HASH_StatusTypeDef (*HashStartFn)(HASH_HandleTypeDef *);
typedef HASH_StatusTypeDef (*HashFinalFn)(HASH_HandleTypeDef *, uint8_t *);

static void run_test(const char *label, const char *msg,
                     HashStartFn start_fn, HashFinalFn final_fn,
                     size_t digest_len, const char *expected)
{
    HASH_HandleTypeDef hhash;
    HASH_Constructor(&hhash);
    
    uint8_t digest[32];

    start_fn(&hhash);
    HASH_SHA256_Update(&hhash, (const uint8_t *)msg, strlen(msg));
    final_fn(&hhash, digest);

    USART_WriteString(&huart, label);
    print_hex(&huart, digest, digest_len);
    USART_WriteString(&huart, "\r\nExpected Output = ");
    USART_WriteString(&huart, expected);
    USART_WriteString(&huart, "\r\n\n");
}

int main(void)
{
    USART_constructor(&huart, USART_2, TX_ONLY, __115200);
    USART_WriteString(&huart, "\r\n=== Hash Tests ===\r\n\n");

    run_test("SHA-1  (\"abc\") = ",
             "abc", HASH_SHA1_Start, HASH_SHA1_Final, 20,
             "a9993e364706816aba3e25717850c26c9cd0d89d");

    run_test("SHA-224(\"abc\") = ",
             "abc", HASH_SHA224_Start, HASH_SHA224_Final, 28,
             "23097d223405d8228642a477bda255b32aadbce4bda0b3f7e36c9da7");

    run_test("SHA-256(\"abc\") = ",
             "abc", HASH_SHA256_Start, HASH_SHA256_Final, 32,
             "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad");

    run_test("SHA-256(\"abcd\") = ",
             "abcd", HASH_SHA256_Start, HASH_SHA256_Final, 32,
             "88d4266fd4e6338d13b845fcf289579d209c897823b9217da3e161936f031589");

    while (1) {}
}