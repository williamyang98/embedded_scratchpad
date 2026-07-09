# python version of ../src/cobs.hpp
DELIMITER_BYTE = 0x00
MAX_CODE_OFFSET = 255

def get_maximum_encoded_size(decoded_size):
    total_overhead_bytes = (decoded_size+254-1)//254 + 1
    return total_overhead_bytes + decoded_size

def get_maximum_decoded_size(encoded_size):
    minimum_overhead_bytes = 2
    if encoded_size <= minimum_overhead_bytes:
        return 0
    return encoded_size-minimum_overhead_bytes

def encode(src_buffer: bytearray):
    src_length_bytes = len(src_buffer)
    assert src_length_bytes > 0

    max_dest_bytes = get_maximum_encoded_size(src_length_bytes)
    dest_buffer = bytearray(max_dest_bytes)

    dest_buffer[0] = 0
    dest_i = 1
    previous_code_dest_i = 0
    curr_code_offset = 1
    for src_i in range(src_length_bytes):
        is_restart_block = curr_code_offset == MAX_CODE_OFFSET
        if is_restart_block:
            dest_buffer[previous_code_dest_i] = curr_code_offset
            dest_buffer[dest_i] = 0
            previous_code_dest_i = dest_i
            dest_i += 1
            curr_code_offset = 1

        src_byte = src_buffer[src_i]
        is_delimiter_byte = src_byte == DELIMITER_BYTE
        if not is_delimiter_byte:
            dest_buffer[dest_i] = src_byte
            dest_i += 1
            curr_code_offset += 1
        else:
            dest_buffer[previous_code_dest_i] = curr_code_offset
            dest_buffer[dest_i] = 0
            previous_code_dest_i = dest_i
            dest_i += 1
            curr_code_offset = 1

    dest_buffer[previous_code_dest_i] = curr_code_offset
    dest_buffer[dest_i] = DELIMITER_BYTE
    dest_i += 1
    return dest_buffer[:dest_i]

def decode(src_buffer: bytearray):
    src_length_bytes = len(src_buffer)
    assert src_length_bytes > 0

    max_dest_bytes = get_maximum_decoded_size(src_length_bytes)
    dest_buffer = bytearray(max_dest_bytes)
    dest_i = 0

    next_code_offset = src_buffer[0]
    curr_code_offset = 1
    for src_i in range(1, src_length_bytes):
        src_byte = src_buffer[src_i]
        if src_byte == DELIMITER_BYTE:
            break
        if curr_code_offset == next_code_offset:
            if curr_code_offset != MAX_CODE_OFFSET:
                dest_buffer[dest_i] = DELIMITER_BYTE
                dest_i += 1
            curr_code_offset = 1
            next_code_offset = src_byte
        else:
            dest_buffer[dest_i] = src_byte
            dest_i += 1
            curr_code_offset += 1
    return dest_buffer[:dest_i]

