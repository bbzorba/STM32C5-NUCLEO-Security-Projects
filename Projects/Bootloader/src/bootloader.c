#include "../inc/bootloader.h"

extern uint32_t SystemCoreClock;   /* defined in startup.c */

/* ── Internal helpers ────────────────────────────────────────────── */

static void BL_SendByte(BL_HandleTypeDef *hbl, uint8_t byte)
{
    USART_x_Write(hbl->huart, byte);
}

/*
 * Polled receive with timeout (DWT cycle counter).
 * Error flags (ORE/FE/NE/PE) are cleared so a glitch on the line cannot
 * stall reception.
 */
static BL_Status BL_ReadByte(BL_HandleTypeDef *hbl, uint8_t *byte, uint32_t timeout_ms)
{
    USART_ManualType *regs = hbl->huart->regs;
    uint32_t start = DWT_CYCCNT_REG;
    uint32_t limit = timeout_ms * (SystemCoreClock / 1000U);

    while (!(regs->ISR & USART_ISR_RXNE)) {
        if (regs->ISR & USART_ISR_ERR_MASK)
            regs->ICR = USART_ICR_PECF | USART_ICR_FECF | USART_ICR_NECF | USART_ICR_ORECF;
        if ((DWT_CYCCNT_REG - start) >= limit)
            return BL_ERR_TIMEOUT;
    }
    *byte = (uint8_t)(regs->RDR & 0xFFU);
    return BL_OK;
}

static BL_Status BL_ReadBytes(BL_HandleTypeDef *hbl, uint8_t *buf, uint32_t len)
{
    for (uint32_t i = 0; i < len; i++) {
        if (BL_ReadByte(hbl, &buf[i], BL_BYTE_TIMEOUT_MS) != BL_OK)
            return BL_ERR_TIMEOUT;
    }
    return BL_OK;
}

/* Little-endian 32-bit word, matches struct.pack('<I') on the host */
static BL_Status BL_ReadWord(BL_HandleTypeDef *hbl, uint32_t *word)
{
    uint8_t b[4];
    if (BL_ReadBytes(hbl, b, 4U) != BL_OK)
        return BL_ERR_TIMEOUT;
    *word = (uint32_t)b[0] | ((uint32_t)b[1] << 8) |
            ((uint32_t)b[2] << 16) | ((uint32_t)b[3] << 24);
    return BL_OK;
}

/* Discard anything left in RDR and clear error flags */
static void BL_FlushRx(BL_HandleTypeDef *hbl)
{
    USART_ManualType *regs = hbl->huart->regs;
    regs->ICR = USART_ICR_PECF | USART_ICR_FECF | USART_ICR_NECF | USART_ICR_ORECF;
    while (regs->ISR & USART_ISR_RXNE)
        (void)regs->RDR;
}

/*
 * Program one 128-bit flash word.
 * The 4th 32-bit write fills the write buffer and starts programming by
 * itself — no FW needed, and each flash word is written exactly once (ECC).
 * The result is read back to catch silent failures.
 */
static BL_Status BL_ProgramFlashWord(BL_HandleTypeDef *hbl, uint32_t address, const uint32_t data[4])
{
    FLASH_TypeDef *flash = hbl->hflash.Instance;

    while (flash->SR & FLASH_SR_BSY);
    FLASH_CCR_REG = FLASH_CCR_CLR_EOP | FLASH_CCR_CLR_WRPERR | FLASH_CCR_CLR_PGSERR;

    flash->CR |= FLASH_CR_PG;
    for (uint32_t i = 0; i < 4U; i++)
        *(volatile uint32_t *)(address + 4U * i) = data[i];
    while (flash->SR & (FLASH_SR_BSY | FLASH_SR_WBNE));
    flash->CR &= ~FLASH_CR_PG;

    if (flash->SR & (FLASH_SR_WRPERR | FLASH_SR_PGSERR))
        return BL_ERR_FLASH;

    for (uint32_t i = 0; i < 4U; i++) {
        if (*(volatile uint32_t *)(address + 4U * i) != data[i])
            return BL_ERR_FLASH;
    }
    return BL_OK;
}

/* Erase sectors 8-30: the active slot plus its CRC tag sector */
static BL_Status BL_EraseActiveSlot(BL_HandleTypeDef *hbl)
{
    for (uint32_t addr = BL_ACTIVE_ADDR; addr <= BL_TAG_ADDR; addr += BL_SECTOR_SIZE) {
        if (FLASH_ErasePage(&hbl->hflash, addr) != FLASH_OK)
            return BL_ERR_FLASH;
    }
    return BL_OK;
}

/* Hardware CRC-32 over the full 176 KB slot (same as zlib.crc32 in seal.py) */
static uint32_t BL_ComputeSlotCrc(BL_HandleTypeDef *hbl)
{
    uint8_t out[4];
    CRC_Calculate(&hbl->hcrc, (const uint8_t *)BL_ACTIVE_ADDR, BL_ACTIVE_SIZE, out);
    /* CRC_Calculate returns the value big-endian */
    return ((uint32_t)out[0] << 24) | ((uint32_t)out[1] << 16) |
           ((uint32_t)out[2] <<  8) |  (uint32_t)out[3];
}

/* ── Public API ──────────────────────────────────────────────────── */

void BL_Init(BL_HandleTypeDef *hbl, USART_HandleType *huart)
{
    hbl->huart = huart;
    FLASH_Init(&hbl->hflash);
    CRC_Constructor(&hbl->hcrc);

    /* B1 (PC13): input with pull-up, pressed = low */
    hbl->btn_cfg.Pin       = BL_BTN_PIN;
    hbl->btn_cfg.Mode      = GPIO_MODE_INPUT;
    hbl->btn_cfg.Pull      = GPIO_PULLUP;
    hbl->btn_cfg.Speed     = GPIO_SPEED_LOW;
    hbl->btn_cfg.Alternate = 0U;
    GPIO_constructor(&hbl->hbtn, BL_BTN_PORT, &hbl->btn_cfg);

    /* LD1 (PA5): push-pull output, off */
    hbl->led_cfg.Pin       = BL_LED_PIN;
    hbl->led_cfg.Mode      = GPIO_MODE_OUTPUT_PP;
    hbl->led_cfg.Pull      = GPIO_NOPULL;
    hbl->led_cfg.Speed     = GPIO_SPEED_LOW;
    hbl->led_cfg.Alternate = 0U;
    GPIO_constructor(&hbl->hled, BL_LED_PORT, &hbl->led_cfg);
    GPIO_WritePin(&hbl->hled, BL_LED_PIN, GPIO_PIN_RESET);

    /* DWT cycle counter drives all millisecond timeouts */
    DEMCR_REG      |= DEMCR_TRCENA;
    DWT_CYCCNT_REG  = 0U;
    DWT_CTRL_REG   |= DWT_CTRL_CYCCNTENA;

    /* B1 has an RC filter: wait before the first BL_ButtonPressed() sample */
    while (DWT_CYCCNT_REG < BL_BTN_SETTLE_MS * (SystemCoreClock / 1000U)) {}
}

uint8_t BL_ButtonPressed(BL_HandleTypeDef *hbl)
{
    return GPIO_ReadPin(&hbl->hbtn, BL_BTN_PIN) == GPIO_PIN_RESET;   /* active low */
}

BL_Trigger BL_WaitForUpdateRequest(BL_HandleTypeDef *hbl, uint32_t timeout_ms)
{
    const uint32_t cycles_per_ms = SystemCoreClock / 1000U;
    const uint8_t  forever       = (timeout_ms == BL_WAIT_FOREVER);
    uint32_t start      = DWT_CYCCNT_REG;
    uint32_t blink_last = start;
    uint32_t ready_last = start;
    uint8_t  byte;

    BL_FlushRx(hbl);
    GPIO_WritePin(&hbl->hled, BL_LED_PIN, GPIO_PIN_SET);

    while (forever || (DWT_CYCCNT_REG - start) < timeout_ms * cycles_per_ms) {
        /* Short polls so the window still ends on time and B1 stays responsive */
        if (BL_ReadByte(hbl, &byte, BL_POLL_MS) == BL_OK && byte == BL_CMD_UPDATE)
            return BL_TRIGGER_UART;

        if (forever) {
            if ((DWT_CYCCNT_REG - blink_last) >= BL_LED_BLINK_MS * cycles_per_ms) {
                blink_last = DWT_CYCCNT_REG;
                GPIO_TogglePin(&hbl->hled, BL_LED_PIN);
            }
            /* A host may open the port after the first READY — keep announcing */
            if ((DWT_CYCCNT_REG - ready_last) >= BL_READY_REPEAT_MS * cycles_per_ms) {
                ready_last = DWT_CYCCNT_REG;
                USART_WriteString(hbl->huart, "[BL] READY\r\n");
            }
        } else if (BL_ButtonPressed(hbl)) {
            return BL_TRIGGER_BUTTON;
        }
    }
    return BL_TRIGGER_NONE;
}

BL_Status BL_ReceiveImage(BL_HandleTypeDef *hbl)
{
    static uint32_t chunk_words[BL_CHUNK_SIZE / 4U];   /* word-aligned chunk buffer */
    uint8_t  *chunk = (uint8_t *)chunk_words;
    uint32_t  image_size, host_crc, received = 0U;
    uint32_t  addr = BL_ACTIVE_ADDR;
    BL_Status st;

    BL_SendByte(hbl, BL_ACK);   /* acknowledge the update request */

    /* Step 1: image size */
    st = BL_ReadWord(hbl, &image_size);
    if (st != BL_OK) {
        BL_SendByte(hbl, BL_NACK);
        return st;
    }
    if (image_size == 0U || image_size > BL_ACTIVE_SIZE) {
        BL_SendByte(hbl, BL_NACK);
        return BL_ERR_SIZE;
    }

    /* Step 2: erase slot + tag — from here on the old image is gone */
    if (FLASH_Unlock(&hbl->hflash) != FLASH_OK) {
        BL_SendByte(hbl, BL_NACK);
        return BL_ERR_FLASH;
    }
    st = BL_EraseActiveSlot(hbl);
    if (st != BL_OK)
        goto fail;
    BL_SendByte(hbl, BL_ACK);

    /* Step 3: receive + program chunk by chunk (ACK = send the next one) */
    while (received < image_size) {
        uint32_t len = image_size - received;
        if (len > BL_CHUNK_SIZE)
            len = BL_CHUNK_SIZE;

        st = BL_ReadBytes(hbl, chunk, len);
        if (st != BL_OK)
            goto fail;

        /* Pad the last chunk with erased-flash bytes up to a full flash word */
        uint32_t padded = (len + BL_FLASH_WORD_SIZE - 1U) & ~(BL_FLASH_WORD_SIZE - 1U);
        memset(chunk + len, 0xFF, padded - len);

        for (uint32_t off = 0; off < padded; off += BL_FLASH_WORD_SIZE) {
            st = BL_ProgramFlashWord(hbl, addr + off, &chunk_words[off / 4U]);
            if (st != BL_OK)
                goto fail;
        }

        addr     += padded;
        received += len;
        BL_SendByte(hbl, BL_ACK);
    }

    /* Step 4: CRC-32 of the padded slot, computed offline by the host */
    st = BL_ReadWord(hbl, &host_crc);
    if (st != BL_OK)
        goto fail;
    if (BL_ComputeSlotCrc(hbl) != host_crc) {
        st = BL_ERR_CRC;
        goto fail;
    }

    /* Step 5: seal — write the tag in its own flash word, rest left erased */
    {
        const uint32_t tag_word[4] = { host_crc, 0xFFFFFFFFU, 0xFFFFFFFFU, 0xFFFFFFFFU };
        st = BL_ProgramFlashWord(hbl, BL_TAG_ADDR, tag_word);
        if (st != BL_OK)
            goto fail;
    }

    FLASH_Lock(&hbl->hflash);
    BL_SendByte(hbl, BL_ACK);
    return BL_OK;

fail:
    FLASH_Lock(&hbl->hflash);
    BL_SendByte(hbl, BL_NACK);
    return st;
}

BL_Status BL_VerifyActiveSlot(BL_HandleTypeDef *hbl)
{
    char     buf[64];
    uint32_t tag = *(volatile uint32_t *)BL_TAG_ADDR;
    uint32_t crc, msp, pc;

    USART_WriteString(hbl->huart, "[BL] Active slot — 0x08010000\r\n");

    /* Step 1: tag present? */
    snprintf(buf, sizeof(buf), "  CRC tag @ 0x0803C000 : %08lX\r\n", (unsigned long)tag);
    USART_WriteString(hbl->huart, buf);
    if (tag == BL_TAG_BLANK) {
        USART_WriteString(hbl->huart, "  CRC  : no tag — slot not sealed\r\n");
        return BL_ERR_NO_TAG;
    }

    /* Step 2: integrity */
    crc = BL_ComputeSlotCrc(hbl);
    snprintf(buf, sizeof(buf), "  Computed CRC         : %08lX\r\n", (unsigned long)crc);
    USART_WriteString(hbl->huart, buf);
    if (crc != tag) {
        USART_WriteString(hbl->huart, "  CRC  : MISMATCH — image corrupted\r\n");
        return BL_ERR_CRC;
    }
    USART_WriteString(hbl->huart, "  CRC  : OK\r\n");

    /* Step 3: vector table sanity (MSP in SRAM, Thumb reset vector in slot) */
    msp = *(volatile uint32_t *)(BL_ACTIVE_ADDR + 0U);
    pc  = *(volatile uint32_t *)(BL_ACTIVE_ADDR + 4U);
    if (msp < BL_RAM_START || msp > BL_RAM_END ||
        !(pc & 1U) || pc < BL_ACTIVE_ADDR || pc >= BL_ACTIVE_ADDR + BL_ACTIVE_SIZE) {
        snprintf(buf, sizeof(buf), "  MSP  : 0x%08lX  PC : 0x%08lX — INVALID\r\n",
                 (unsigned long)msp, (unsigned long)pc);
        USART_WriteString(hbl->huart, buf);
        return BL_ERR_NO_IMAGE;
    }
    USART_WriteString(hbl->huart, "  MSP  : valid\r\n");
    return BL_OK;
}

/*
 * Hand control to the image at app_base:
 *   - wait for the console to drain, then disable USART2 so the
 *     application's USART_constructor starts from a clean peripheral
 *   - stop SysTick, disable + clear every NVIC interrupt (FLASH IRQ included)
 *   - relocate VTOR, load the application's MSP, re-enable interrupts, branch
 */
void BL_JumpToApplication(BL_HandleTypeDef *hbl, uint32_t app_base)
{
    uint32_t app_msp = *(volatile uint32_t *)(app_base + 0U);
    uint32_t app_pc  = *(volatile uint32_t *)(app_base + 4U);

    GPIO_WritePin(&hbl->hled, BL_LED_PIN, GPIO_PIN_RESET);

    while (!(hbl->huart->regs->ISR & USART_ISR_TC));
    hbl->huart->regs->CR1 = 0U;

    __asm volatile ("cpsid i" ::: "memory");

    SYST_CSR_REG = 0U;
    for (uint32_t i = 0; i < NVIC_REG_COUNT; i++) {
        NVIC_ICER_BASE[i] = 0xFFFFFFFFU;
        NVIC_ICPR_BASE[i] = 0xFFFFFFFFU;
    }

    SCB_VTOR_REG = app_base;
    __asm volatile ("dsb\n\tisb" ::: "memory");

    /* Nothing is pending any more, so PRIMASK can be released before the branch —
       the application starts with interrupts enabled, as after a real reset */
    __asm volatile (
        "msr  msp, %0\n\t"
        "cpsie i\n\t"
        "bx   %1\n\t"
        : : "r"(app_msp), "r"(app_pc)
        : "memory"
    );

    while (1) {}   /* unreachable */
}

const char *BL_StatusString(BL_Status st)
{
    switch (st) {
    case BL_OK:           return "OK";
    case BL_ERR_TIMEOUT:  return "TIMEOUT";
    case BL_ERR_SIZE:     return "BAD SIZE";
    case BL_ERR_FLASH:    return "FLASH ERROR";
    case BL_ERR_CRC:      return "CRC MISMATCH";
    case BL_ERR_NO_TAG:   return "NO TAG";
    case BL_ERR_NO_IMAGE: return "NO IMAGE";
    default:              return "UNKNOWN";
    }
}
