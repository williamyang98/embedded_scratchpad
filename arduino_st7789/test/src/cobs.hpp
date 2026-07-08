#pragma once
#include <stdint.h>

// https://en.wikipedia.org/wiki/Consistent_Overhead_Byte_Stuffing
// avoid additional overhead bytes by restricting maximum output bytes to 256
// this way the code byte which stores the offset to the next zero byte never exceeds 255 which is storabled in uint8
static uint16_t cobs_encode(const void* src_data, uint8_t src_length_bytes, uint8_t* dest_buffer) {
    if (src_length_bytes > 254) {
        src_length_bytes = 254;
    }
    const uint8_t* src_buffer = reinterpret_cast<const uint8_t*>(src_data);

    // the first encoded byte is the offset to the next zero byte which includes terminating byte
    dest_buffer[0] = 0;

    uint16_t dest_i = 1;
    uint16_t previous_code_dest_i = 0;
    uint8_t curr_code_offset = 1;
    for (uint16_t src_i = 0; src_i < src_length_bytes; src_i++) {
        const uint8_t src_byte = src_buffer[src_i];
        if (src_byte != 0) {
            dest_buffer[dest_i] = src_byte;
        } else {
            dest_buffer[previous_code_dest_i] = curr_code_offset;
            previous_code_dest_i = dest_i;
            curr_code_offset = 0;
        }
        dest_i++;
        curr_code_offset++;
    }
    // append zero delimiter byte
    dest_buffer[previous_code_dest_i] = curr_code_offset;
    dest_i++;

    return dest_i;
}

static uint8_t cobs_decode(const uint8_t* src_buffer, uint16_t src_length_bytes, void* dest_data) {
    if (src_length_bytes > 256) {
        src_length_bytes = 256;
    }
    // ignore delimiter byte when writing
    src_length_bytes--;

    uint8_t* dest_buffer = reinterpret_cast<uint8_t*>(dest_data);
    uint8_t dest_i = 0;

    uint8_t next_code_offset = src_buffer[0];
    uint8_t curr_code_offset = 1;
    for (uint16_t src_i = 1; src_i < src_length_bytes; src_i++) {
        const uint8_t src_byte = src_buffer[src_i];
        if (curr_code_offset == next_code_offset) {
            curr_code_offset = 0;
            next_code_offset = src_byte;
            dest_buffer[dest_i] = 0;
        } else {
            dest_buffer[dest_i] = src_byte;
        }
        dest_i++;
        curr_code_offset++;
    }
    return dest_i;
}
