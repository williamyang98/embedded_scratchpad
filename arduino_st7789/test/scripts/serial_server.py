import argparse
import cobs
from response_parser import ResponseHandler
from devices import SerialDevice

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
        pass

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--port", default=None, type=str, help="COM port to connect to")
    parser.add_argument("--baudrate", default=9600, type=int, help="Rate to communicate with device")
    parser.add_argument("--list-ports", action="store_true", help="List all COM ports connected to computer")
    parser.add_argument("--reset", action="store_true", help="Reset arduino on connection")
    args = parser.parse_args()


    if args.list_ports:
        import serial.tools.list_ports
        ports = serial.tools.list_ports.comports()
        for port in ports:
            print(port.device)
        return

    import serial
    port_name = args.port
    if port_name is None:
        import serial.tools.list_ports
        ports = serial.tools.list_ports.comports()
        if len(ports) == 0:
            print("No ports available to choose by default")
            return
        port = ports[0]
        print(f"Choosing port '{port.device}' by default")
        port_name = port.device

    ser = serial.Serial()
    ser.port = port_name
    ser.baudrate = args.baudrate
    ser.dtr = args.reset
    ser.open()

    response_handler = CustomResponseHandler()
    device = SerialDevice(ser, response_handler)
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
    main()
