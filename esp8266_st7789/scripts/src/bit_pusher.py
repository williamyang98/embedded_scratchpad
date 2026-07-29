class BitPusher:
    def __init__(self):
        self._data = [0x00]
        self.bit_index = 0

    def push(self, data, n_bits):
        self._data[-1] |= (data << self.bit_index) & 0xFF

        remaining_bits = 8-self.bit_index
        shift_bits = min(n_bits, remaining_bits)

        self.bit_index += shift_bits
        if self.bit_index == 8:
            self.bit_index = 0
            self._data.append(0x00)

        data = data >> shift_bits
        n_bits -= shift_bits
        if n_bits > 0:
            self.push(data, n_bits)

    def get_data(self):
        if self.bit_index == 0:
            return self._data[:-1]
        return self._data
