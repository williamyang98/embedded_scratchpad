import struct from "./struct.js";

export class FrameBodyTooShort extends Error {
  constructor(minimum_length, data, part) {
    const given_length = data.length;
    const message = `Frame body too short to contain '${part}' expected minimum length of ${minimum_length} bytes but got ${given_length} bytes`;
    super(message);
    this.minimum_length = minimum_length;
    this.given_length = given_length;
    this.data = data;
    this.part = part;
  }
}

const header_format = struct("<HHHHHHHHBBI");

export class DebugFrame {
  constructor(data) {
    if (data.length < header_format.size) {
      throw new FrameBodyTooShort(header_format.size, data, "HEADER");
    }
    const header_data = data.subarray(0, header_format.size);
    const header = header_format.unpack_from(header_data.buffer, header_data.byteOffset);
    const [x_start, x_end, y_start, y_end] = header.slice(0, 4);
    const [x_cursor, y_cursor] = header.slice(4, 6);
    const [width, height] = header.slice(6, 8);
    const [brightness, hardware_reset] = header.slice(8, 10);
    const label_length = header[10];

    data = data.subarray(header_format.size);
    if (data.length < label_length) {
      throw new FrameBodyTooShort(label_length, data, "LABEL");
    }
    const label_data = data.subarray(0, label_length);
    const label_decoder = new TextDecoder("utf-8");
    const label = label_decoder.decode(label_data);

    data = data.subarray(label_length);
    const total_pixels = width*height;
    const rgb565_size = 2;
    const total_pixels_bytes = total_pixels*rgb565_size;
    if (data.length < total_pixels_bytes) {
      throw new FrameBodyTooShort(total_pixel_bytes, data, "PIXEL_DATA");
    }
    // need to copy since uint16 array needs to be aligned to 2 byte boundary and throws error no unaligned byte offset
    let pixel_data = new Uint8Array(total_pixels_bytes);
    pixel_data.set(data.subarray(0, total_pixels_bytes));
    pixel_data = new Uint16Array(pixel_data.buffer);

    data = data.subarray(total_pixels_bytes);
    if (data.length != 0) {
      console.warn(`Got ${data.length} extra unhandled bytes in frame body`);
    }

    this.x_start = x_start;
    this.x_end = x_end;
    this.y_start = y_start;
    this.y_end = y_end;
    this.x_cursor = x_cursor;
    this.y_cursor = y_cursor;
    this.width = width;
    this.height = height;
    this.brightness = brightness;
    this.hardware_reset = hardware_reset;
    this.label = label;
    this.pixel_data = pixel_data;
  }
}
