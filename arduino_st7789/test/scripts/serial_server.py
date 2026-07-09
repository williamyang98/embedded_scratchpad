import argparse
from command_creator import CommandSender, COBS_DELIMITER_BYTE
import threading

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

    writer = lambda data: ser.write(data)
    sender = CommandSender(writer)

    def read_serial():
        while True:
            try:
                data = ser.read_until(expected=bytes([COBS_DELIMITER_BYTE]))
            except Exception as ex:
                print(f"Exiting reader thread: {ex}")
                return
            if len(data) == 0:
                continue
            print(f"> {data}")

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
                sender.set_temperature(temperature)
                temperature += 1
            elif c == "r":
                sender.trigger_render()
            elif c == "h":
                sender.set_humidity(humidity)
                humidity += 1
            elif c == "T":
                sender.set_24_hour_time(time, False, False)
                time += 1
            elif c == "w":
                sender.set_wind_kph(wind)
                wind += 1
            elif c == "exit":
                print("Exiting...")
                return
            else:
                print(f"Unknown command: {c}")

    read_thread = threading.Thread(target=read_serial)
    read_thread.start()
    write_serial()
    ser.close()

if __name__ == "__main__":
    main()
