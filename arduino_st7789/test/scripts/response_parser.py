import cobs

class ResponseParser:
    def __init__(self):
        pass

    def read_bytes(self, encoded_data: bytes):
        if len(encoded_data) == 0:
            return
        data = cobs.decode(encoded_data)
        print(f"> {data}")
