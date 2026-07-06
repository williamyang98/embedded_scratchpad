import numpy as np
from bit_pusher import BitPusher

class Encoding:
    BINARY_RLE_U8 = "binary_rle_u8"
    GRAYSCALE_RLE_U4 = "grayscale_rle_u4"

# encoding scheme
# 1. quantize grayscale 0-255 to 0-3 inclusive
# 2. store pixel value as 2 bits tightly packed, with successive bits packed into higher bits and bytes
# 3. if pixel value is 0 or 3 store running value as 8 bit value also tightly packed
# 4. if running value reaches 255, push the pixel value again and restart running value from 0
def encode_to_grayscale_running_length_encoded_u4(image):
    assert image.dtype == np.uint8

    U8_MAX_VALUE = 255
    U4_MAX_VALUE = 3
    image = image.astype(np.float32)
    image = image/U8_MAX_VALUE
    image = image*U4_MAX_VALUE
    image = np.round(image)
    image = np.clip(image, 0, U4_MAX_VALUE)
    image = image.astype(np.uint8)

    bits = BitPusher()
    running_length = None
    running_value = None

    image = image.flatten()
    for pixel in image:
        if pixel == running_value:
            running_length += 1
            if running_length == 255:
                bits.push(running_length, 8)
                running_length = None
                running_value = None
            continue

        if running_length != None:
            bits.push(running_length, 8)
            running_length = None
            running_value = None

        bits.push(pixel, 2)
        if pixel == 0 or pixel == 3:
            running_length = 1
            running_value = pixel
        else:
            running_length = None
            running_value = None

    if running_length != None:
        bits.push(running_length, 8)

    encoding = np.array(bits.get_data(), dtype=np.uint8)
    return encoding

def encode_to_binary_running_length_encoded_u8(image):
    assert image.dtype == np.uint8

    image = (image > 127).flatten()

    running_length = None
    running_value = None
    encoding = []
    for pixel in image:
        if running_value == pixel:
            running_length += 1
            if running_length == 127:
                pixel_bit = 0b10000000 if running_value else 0b00000000
                encoding.append(pixel_bit | running_length)
                running_length = None
                running_value = None
            continue

        if running_length != None:
            pixel_bit = 0b10000000 if running_value else 0b00000000
            encoding.append(pixel_bit | running_length)
            running_length = None
            running_value = None

        running_value = pixel
        running_length = 1

    if running_length != None:
        pixel_bit = 0b10000000 if running_value else 0b00000000
        encoding.append(pixel_bit | running_length)

    encoding = np.array(encoding, dtype=np.uint8)
    return encoding

