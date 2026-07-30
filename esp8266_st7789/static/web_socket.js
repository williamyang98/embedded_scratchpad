import { DebugFrame } from "./debug_frame.js";

// main/main.cpp in g_websocket.uri
export const DEFAULT_WEBSOCKET_URL = `ws://${document.location.host}/api/v1/websocket`;

// javascript port of scripts/src/command_creator.py
// transmitter for components/st7789/app/commands.hpp
export const CommandHeader = {
  TRIGGER_RENDER: 0x00,
  // weather page
  SET_TEMPERATURE: 0x01,
  SET_HUMIDITY: 0x02,
  SET_TIME_24_HOUR: 0x03,
  SET_WIND_KPH: 0x04,
  SET_WEATHER_ICON: 0x05,
  SET_LOCATION: 0x06,
  SET_WEATHER_DESCRIPTION: 0x07,
  SET_MOON_PHASE: 0x08,
  // set page
  SET_SCREEN_BRIGHTNESS: 0xFE,
  SET_PAGE: 0xFF,
};

export const WeatherIcon = {
  WINTER: 0,
  LIGHTNING_STORM: 1,
  HEAVY_RAIN: 2,
  PARTLY_CLOUDY: 3,
  SUNNY: 4,
};

export const MoonPhase = {
  NEW_MOON: 0,
  WAXING_CRESCENT: 1,
  FIRST_QUARTER: 2,
  WAXING_GIBBOUS: 3,
  FULL_MOON: 4,
  WANING_GIBBOUS: 5,
  THIRD_QUARTER: 6,
  WANING_CRESCENT: 7,
};

export const AppPage = {
  LANDING_SCREEN: 0,
  WEATHER_PAGE: 1,
};

export class CommandCreator {
  trigger_render() {
    return new Uint8Array([CommandHeader.TRIGGER_RENDER]);
  }
  set_page(page) {
    return new Uint8Array([CommandHeader.SET_PAGE, page & 0xFF]);
  }
  set_screen_brightness(brightness) {
    return new Uint8Array([CommandHeader.SET_SCREEN_BRIGHTNESS, brightness & 0xFF]);
  }
  set_temperature(temperature) {
    return new Uint8Array([
      CommandHeader.SET_TEMPERATURE,
      (temperature >> 8) & 0xFF,
      temperature & 0xFF,
    ]);
  }
  set_humidity(humidity) {
    return new Uint8Array([
      CommandHeader.SET_HUMIDITY,
      (humidity >> 8) & 0xFF,
      humidity & 0xFF,
    ]);
  }
  set_24_hour_time(time_24_hour, is_show_24_hour, is_show_leading_zero) {
    return new Uint8Array([
      CommandHeader.SET_TIME_24_HOUR,
      (time_24_hour >> 8) & 0xFF,
      time_24_hour & 0xFF,
      is_show_24_hour ? 0xFF : 0x00,
      is_show_leading_zero ? 0xFF : 0x00,
    ]);
  }
  set_wind_kph(wind_kph) {
    return new Uint8Array([
      CommandHeader.SET_WIND_KPH,
      (wind_kph >> 8) & 0xFF,
      wind_kph & 0xFF,
    ]);
  }
  set_location(location_string) {
    const encoder = new TextEncoder();
    const buffer = encoder.encode(location_string);
    return new Uint8Array([
      CommandHeader.SET_LOCATION,
      ...buffer,
    ]);
  }
  set_weather_description(weather_description) {
    const encoder = new TextEncoder();
    const buffer = encoder.encode(weather_description);
    return new Uint8Array([
      CommandHeader.SET_WEATHER_DESCRIPTION,
      ...buffer,
    ]);
  }
  set_weather_icon(weather_icon) {
    return new Uint8Array([
      CommandHeader.SET_WEATHER_ICON,
      weather_icon,
    ]);
  }
  set_moon_phase(moon_phase) {
    return new Uint8Array([
      CommandHeader.SET_MOON_PHASE,
      moon_phase,
    ]);
  }
}

// javascript port of scripts/src/response_parser.py
// receiver for components/st7789/app/response.hpp
export const ResponseHeader = {
  ACKNOWLEDGE_COMMAND: 0x00,
  RENDER_STATUS: 0x01,
  LOG_MESSAGE: 0x02,
  DEBUG_MESSAGE: 0x03,
  DEBUG_FRAME: 0x04,
};

export class BadLengthError extends Error {
  constructor(header, expected, given) {
    const message = `Header ${header} expected length of ${expected} but was given ${given}`;
    super(message);
    this.header = header;
    this.expected = expected;
    this.given = given;
  }
}

export class UnhandledResponse extends Error {
  constructor(header, data) {
    const message = `Unhandled response with header=${header} with data body containing ${data.length-1} bytes`;
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
  if (header === ResponseHeader.ACKNOWLEDGE_COMMAND) {
    const EXPECTED_LENGTH = 3;
    if (data.length !== EXPECTED_LENGTH) throw new BadLengthError(header, EXPECTED_LENGTH, data.length);
    const ack_header = data[1];
    const is_success = data[2] != 0x00;
    return { type: "acknowledge_command", header: ack_header, is_success }
  }
  if (header === ResponseHeader.RENDER_STATUS) {
    const EXPECTED_LENGTH = 2;
    if (data.length !== EXPECTED_LENGTH) throw new BadLengthError(header, EXPECTED_LENGTH, data.length);
    const is_busy = data[1] != 0x00;
    return { type: "render_status", is_busy };
  }
  if (header === ResponseHeader.LOG_MESSAGE) {
    const view = data.subarray(1);
    const decoder = new TextDecoder("utf-8");
    const message = decoder.decode(view);
    return { type: "log_message", message };
  }
  if (header === ResponseHeader.DEBUG_MESSAGE) {
    const view = data.subarray(1);
    const decoder = new TextDecoder("utf-8");
    const message = decoder.decode(view);
    return { type: "debug_message", message };
  }
  if (header === ResponseHeader.DEBUG_FRAME) {
    const frame_data = data.subarray(1);
    const frame = new DebugFrame(frame_data);
    return { type: "debug_frame", frame };
  }
  throw new UnhandledResponse(header, data);
}
