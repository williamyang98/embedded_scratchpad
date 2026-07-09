#pragma once
#include <stdint.h>

namespace cobs {

// https://en.wikipedia.org/wiki/Consistent_Overhead_Byte_Stuffing
// avoid additional overhead bytes by restricting maximum output bytes to 256
// this way the code byte which stores the offset to the next zero byte never exceeds 255 which is storabled in uint8
static constexpr uint8_t DELIMITER_BYTE = 0x00;
static constexpr uint8_t MAX_CODE_OFFSET = 255;

static size_t encode(const void* src_data, size_t src_length_bytes, uint8_t* dest_buffer) {
    const uint8_t* src_buffer = reinterpret_cast<const uint8_t*>(src_data);

    // the first encoded byte is the offset to the next zero byte which includes terminating byte
    // the codes essentially form a linked list that hops from one element to the next using an offset figure
    // decoding means replacing each value in this linked list with the original byte before encoding, which is the same as the delimiter byte

    dest_buffer[0] = 0;
    size_t dest_i = 1;
    size_t previous_code_dest_i = 0;
    uint8_t curr_code_offset = 1;
    for (size_t src_i = 0; src_i < src_length_bytes; src_i++) {
        const bool is_restart_block = curr_code_offset == MAX_CODE_OFFSET;
        if (is_restart_block) {
            dest_buffer[previous_code_dest_i] = curr_code_offset;
            dest_buffer[dest_i] = 0;
            previous_code_dest_i = dest_i;
            dest_i++;
            curr_code_offset = 1;
        }

        const uint8_t src_byte = src_buffer[src_i];
        const bool is_delimiter_byte = src_byte == DELIMITER_BYTE;
        if (!is_delimiter_byte) {
            dest_buffer[dest_i] = src_byte;
            dest_i++;
            curr_code_offset++;
        } else {
            dest_buffer[previous_code_dest_i] = curr_code_offset;
            dest_buffer[dest_i] = 0;
            previous_code_dest_i = dest_i;
            dest_i++;
            curr_code_offset = 1;
        }
    }
    // append zero delimiter byte
    dest_buffer[previous_code_dest_i] = curr_code_offset;
    dest_buffer[dest_i] = DELIMITER_BYTE;
    dest_i++;
    return dest_i;
}

static size_t decode(const uint8_t* src_buffer, size_t src_length_bytes, void* dest_data) {
    uint8_t* dest_buffer = reinterpret_cast<uint8_t*>(dest_data);
    size_t dest_i = 0;

    uint8_t next_code_offset = src_buffer[0];
    uint8_t curr_code_offset = 1;
    for (size_t src_i = 1; src_i < src_length_bytes; src_i++) {
        const uint8_t src_byte = src_buffer[src_i];
        if (src_byte == DELIMITER_BYTE) break;
        if (curr_code_offset == next_code_offset) {
            if (curr_code_offset != MAX_CODE_OFFSET) {
                dest_buffer[dest_i] = DELIMITER_BYTE;
                dest_i++;
            }
            curr_code_offset = 1;
            next_code_offset = src_byte;
        } else {
            dest_buffer[dest_i] = src_byte;
            dest_i++;
            curr_code_offset++;
        }
    }
    return dest_i;
}

};

