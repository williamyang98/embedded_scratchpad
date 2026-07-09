import argparse
import cobs
from response_parser import ResponseHandler
from devices import ProcessDevice
import subprocess
import sys
import os

class CustomResponseHandler(ResponseHandler):
    def acknowledge_command(self, header, is_success):
        print(f"> Acknowledged received: header=0x{header:02X}, is_success={is_success}")

    def render_status(self, is_busy):
        print(f"> Render status: is_busy={is_busy}")

    def log_message(self, message):
        print(f"> Logged Message: {message}")

    def debug_message(self, message):
        print(f"> Debug Message: {message}")

    def debug_frame(self, frame):
        print(f"> Frame: width={frame.width}, height={frame.height}, label='{frame.label}'")

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("executable", nargs="?", default="../build/st7789.exe")
    args = parser.parse_args()

    if not os.path.isfile(args.executable):
        logger.error(f"'{args.executable}' is not a valid executable")
        return 1

    try:
        proc = subprocess.Popen(
            [args.executable],
            stdin=subprocess.PIPE,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
        )
    except FileNotFoundError as ex:
        print(f"[ERROR]: Failed to launch process: {ex}")
        return 1

    response_handler = CustomResponseHandler()
    device = ProcessDevice(proc, response_handler)
    command_sender = device.get_command_sender()

    def write_serial():
        temperature = 100
        humidity = 200
        time = 1537
        wind = 100
        while True:
            try:
                c = input("")
            except KeyboardInterrupt:
                print("Exiting on keyboard interrupt...")
                return

            if c == "t":
                command_sender.set_temperature(temperature)
                temperature += 1
            elif c == "r":
                command_sender.trigger_render()
            elif c == "h":
                command_sender.set_humidity(humidity)
                humidity += 1
            elif c == "T":
                command_sender.set_24_hour_time(time, False, False)
                time += 1
            elif c == "w":
                command_sender.set_wind_kph(wind)
                wind += 1
            elif c == "exit":
                print("Exiting...")
                return
            else:
                print(f"Unknown command: {c}")

    write_serial()
    device.wait()

if __name__ == "__main__":
    rv = main()
    sys.exit(rv)
