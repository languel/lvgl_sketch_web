#include "base64_utils.h"
#include <Arduino.h> // For ps_malloc, ps_free
#include <string.h>  // For strlen
#include <esp_heap_caps.h> // Added for heap_caps_malloc and heap_caps_free

static const int8_t b64_table[256] = {
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, // 0-15
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, // 16-31
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,62,-1,-1,-1,63, // 32-47
    52,53,54,55,56,57,58,59,60,61,-1,-1,-1, 0,-1,-1, // 48-63
    -1, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14, // 64-79
    15,16,17,18,19,20,21,22,23,24,25,-1,-1,-1,-1,-1, // 80-95
    -1,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40, // 96-111
    41,42,43,44,45,46,47,48,49,50,51,-1,-1,-1,-1,-1, // 112-127
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, // 128-143
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, // 144-159
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, // 160-175
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, // 176-191
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, // 192-207
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, // 208-223
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, // 224-239
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1  // 240-255
};

int base64_decode(const char* input, uint8_t* output, size_t output_len) {
    size_t in_len = 0;
    while (input[in_len] && input[in_len] != '=') in_len++;
    size_t out_idx = 0;
    int val = 0, valb = -8;
    for (size_t i = 0; input[i] && input[i] != '='; ++i) {
        int8_t c = b64_table[(uint8_t)input[i]];
        if (c == -1) continue;
        val = (val << 6) + c;
        valb += 6;
        if (valb >= 0) {
            if (out_idx >= output_len) return -1; // Output buffer too small
            output[out_idx++] = (uint8_t)((val >> valb) & 0xFF);
            valb -= 8;
        }
    }
    return out_idx;
}

// Decodes a base64 string into a newly allocated PSRAM buffer.
// Returns a pointer to the buffer, or nullptr on error.
// The caller is responsible for freeing the returned buffer using heap_caps_free().
// The out_decoded_len will be set to the length of the decoded data.
uint8_t* base64_decode_to_psram(const char* input, size_t* out_decoded_len) {
    if (!input || !out_decoded_len) {
        if (out_decoded_len) *out_decoded_len = 0;
        return nullptr;
    }

    size_t input_len = strlen(input);
    if (input_len == 0) {
        *out_decoded_len = 0;
        return nullptr;
    }

    // Calculate maximum possible decoded length.
    // Each 4 base64 chars correspond to 3 bytes. Padding chars adjust this.
    // A robust formula for max output size: (input_len * 3 + 3) / 4
    size_t max_decoded_buffer_size = (input_len * 3 + 3) / 4;
    if (max_decoded_buffer_size == 0 && input_len > 0) { // Should not happen if input_len > 0
        max_decoded_buffer_size = 1; // Minimal allocation for safety, though logic implies >0
    }

    uint8_t* decoded_buffer = (uint8_t*)heap_caps_malloc(max_decoded_buffer_size, MALLOC_CAP_SPIRAM); // Changed to heap_caps_malloc
    if (!decoded_buffer) {
        *out_decoded_len = 0;
        return nullptr; 
    }

    int bytes_decoded = base64_decode(input, decoded_buffer, max_decoded_buffer_size);

    if (bytes_decoded < 0) {
        heap_caps_free(decoded_buffer); // Changed to heap_caps_free
        *out_decoded_len = 0;
        return nullptr; 
    }

    *out_decoded_len = (size_t)bytes_decoded;
    return decoded_buffer;
}
