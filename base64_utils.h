#pragma once
#include <stdint.h>
#include <stddef.h>
#include <Arduino.h> // Added for size_t, and for consistency with .cpp for ps_malloc related functions

// Decodes base64 input (null-terminated) into output buffer.
// Returns the number of bytes written to output, or -1 on error.
int base64_decode(const char* input, uint8_t* output, size_t output_len);

// Decodes a base64 string into a newly allocated PSRAM buffer.
// Returns a pointer to the buffer, or nullptr on error.
// The caller is responsible for freeing the returned buffer using ps_free().
// The decoded_len will be set to the length of the decoded data.
uint8_t* base64_decode_to_psram(const char* input, size_t* decoded_len);
