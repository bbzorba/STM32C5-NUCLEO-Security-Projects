/*
 * Image identification record "IMGID:<module>" — linked into every build.
 * The Makefile passes the PROJECT_DIR folder name (-DIMAGE_NAME=App_Demo …);
 * linker/sections.ld keeps the record right after the vector table, where
 * `make read-flash` (tools/read_flash.py) finds it to name what is programmed.
 */
#define STR_(x)  #x
#define STR(x)   STR_(x)

__attribute__((section(".image_id"), used))
const char image_id[] = "IMGID:" STR(IMAGE_NAME);
