#pragma once
#include <stdint.h>
#include <stddef.h>

// Decodes base64 input (null-terminated) into output buffer.
// Returns the number of bytes written to output, or -1 on error.
int base64_decode(const char* input, uint8_t* output, size_t output_len);
