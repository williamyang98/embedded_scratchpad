// counterpart to src/web_socket.rs
export const CommandHeader = {
  SetLedDutyCycle: 0x00,
  GetLedDutyCycle: 0x01,
  Ping: 0x02,
};

export class WebsocketCommandCreator {
  set_led_duty_cycle(cycle) {
    return new Uint8Array([CommandHeader.SetLedDutyCycle, cycle]);
  }
  get_led_duty_cycle() {
    return new Uint8Array([CommandHeader.GetLedDutyCycle]);
  }
  send_ping(value) {
    let command = new Uint8Array(5);
    command[0] = CommandHeader.Ping;
    let view = new DataView(command.buffer);
    let value_offset = 1;
    const IS_LITTLE_ENDIAN = true;
    view.setUint32(value_offset, value, IS_LITTLE_ENDIAN);
    return command;
  }
}

export const ResponseHeader = {
  MissedMessages: 0x00,
  BluetoothUpdate: 0x01,
  GetLedDutyCycle: 0x02,
  BackgroundTaskMessage: 0x03,
  Pong: 0x04,
  // errors
  EmptyCommand: 0xFC,
  BadCommandLength: 0xFD,
  BadCommandValue: 0xFE,
  UnhandledCommand: 0xFF,
};

export class BadLengthError extends Error {
  constructor(header, expected, given) {
    let message = `Header ${header} expected length of ${expected} but was given ${given}`
    super(message);
    this.header = header;
    this.expected = expected;
    this.given = given;
  }
}

export class ServerError extends Error {
  constructor(header, data) {
    let message = `Server responded with command parse error: header=${header}, data=[${data.join(',')}]`;
    super(message);
    this.header = header;
    this.data = data;
  }
}

export class UnhandledResponse extends Error {
  constructor(header, data) {
    let message = `Unhandled binary response header=${header}, data=[${data.join(',')}]`;
    super(message);
    this.header = header;
    this.data = data;
  }
}

export function parse_websocket_response(data) {
  if (data.length === 0) {
    return Error("Empty frame");
  }

  const header = data[0];
  if (header === ResponseHeader.MissedMessages) {
    const EXPECTED_LENGTH = 2;
    if (data.length !== EXPECTED_LENGTH) throw BadLengthError(header, EXPECTED_LENGTH, data.length);
    let total_missed = data[1];
    console.warn(`Missed ${total_missed} client messages on websocket connection`);
    return { type: "missed_messages", total_missed }
  }
  if (header === ResponseHeader.BluetoothUpdate) {
    const EXPECTED_LENGTH = 2;
    if (data.length !== EXPECTED_LENGTH) throw BadLengthError(header, EXPECTED_LENGTH, data.length);
    let total_added = data[1];
    return { type: "bluetooth_update", total_added };
  }
  if (header === ResponseHeader.GetLedDutyCycle) {
    const EXPECTED_LENGTH = 2;
    if (data.length !== EXPECTED_LENGTH) throw BadLengthError(header, EXPECTED_LENGTH, data.length);
    let duty_cycle = data[1];
    return { type: "get_led_duty_cycle", duty_cycle };
  }
  if (header === ResponseHeader.BackgroundTaskMessage) {
    const EXPECTED_LENGTH = 5;
    if (data.length !== EXPECTED_LENGTH) throw BadLengthError(header, EXPECTED_LENGTH, data.length);
    let view = new DataView(data.buffer);
    const IS_LITTLE_ENDIAN = true;
    let message_offset = 1;
    let message = view.getUint32(message_offset, IS_LITTLE_ENDIAN);
    return { type: "background_task_message", message };
  }
  if (header === ResponseHeader.Pong) {
    const EXPECTED_LENGTH = 5;
    if (data.length !== EXPECTED_LENGTH) throw BadLengthError(header, EXPECTED_LENGTH, data.length);
    let view = new DataView(data.buffer);
    const IS_LITTLE_ENDIAN = true;
    let value_offset = 1;
    let value = view.getUint32(value_offset, IS_LITTLE_ENDIAN);
    return { type: "pong", value };
  }
  if (header === ResponseHeader.EmptyCommand) throw ServerError(header, data);
  if (header === ResponseHeader.BadCommandLength) throw ServerError(header, data);
  if (header === ResponseHeader.BadCommandValue) throw ServerError(header, data);
  if (header === ResponseHeader.UnhandledCommand) throw ServerError(header, data);

  throw UnhandledResponse(header, data);
}
