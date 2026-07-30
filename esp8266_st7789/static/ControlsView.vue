<script setup>
import { ref, watchEffect } from "vue";
import { CommandCreator, WeatherIcon, MoonPhase } from "./web_socket.js";
import { debounce_timeout } from "./utility.js";

const emits = defineEmits(["command"]);
const command_creator = new CommandCreator();

const temperature_celcius = ref(254);
const humidity_percent = ref(105);
const wind_kph = ref(52);
const time_24_hour = ref(1635);
const time_show_24_hour = ref(false);
const time_show_leading_zeros = ref(false);
const location = ref("Sydney");
const weather_description = ref("cloudy rain");
const weather_icon = ref(WeatherIcon.WINTER);
const moon_phase = ref(MoonPhase.NEW_MOON);
const screen_brightness = ref(50);

function emit_command(command) {
  emits("command", command);
}

function trigger_render() {
  emit_command(command_creator.trigger_render());
}

// bound to slider which emits too many events at once
const _set_screen_brightness = debounce_timeout((brightness) => {
  emit_command(command_creator.set_screen_brightness(brightness));
}, 10);
function set_screen_brightness() {
  _set_screen_brightness(screen_brightness.value);
}

function set_temperature() {
  emit_command(command_creator.set_temperature(temperature_celcius.value));
}

function set_humidity() {
  emit_command(command_creator.set_humidity(humidity_percent.value));
}

function set_time_24_hour() {
  emit_command(command_creator.set_24_hour_time(
    time_24_hour.value,
    time_show_24_hour.value,
    time_show_leading_zeros.value,
  ));
}

function set_wind_kph() {
  emit_command(command_creator.set_wind_kph(wind_kph.value));
}

function set_location() {
  emit_command(command_creator.set_location(location.value.toUpperCase()));
}

function set_weather_description() {
  emit_command(command_creator.set_weather_description(weather_description.value.toUpperCase()));
}

function set_weather_icon() {
  emit_command(command_creator.set_weather_icon(weather_icon.value));
}

function set_moon_phase() {
  emit_command(command_creator.set_moon_phase(moon_phase.value));
}

watchEffect(set_screen_brightness);
watchEffect(set_temperature);
watchEffect(set_wind_kph);
watchEffect(set_humidity);
watchEffect(set_time_24_hour);
watchEffect(set_location);
watchEffect(set_weather_description);
watchEffect(set_weather_icon);
watchEffect(set_moon_phase);

function refresh_all() {
  set_temperature();
  set_wind_kph();
  set_humidity();
  set_time_24_hour();
  set_location();
  set_weather_description();
  set_weather_icon();
  set_moon_phase();
  set_screen_brightness();
  trigger_render();
}

defineExpose({
  submit() {
    refresh_all();
  }
})

</script>

<template>
<table>
<tbody>
  <tr>
    <td colspan=2>
      <button @click.stop="trigger_render" type="button">Trigger Render</button>
      <button @click.stop="refresh_all" type="button">Refresh All</button>
    </td>
  </tr>
  <tr>
    <td><label>Screen brightness</label></td>
    <td><input type="range" v-model.number="screen_brightness" min="0", max="255"></td>
  </tr>
  <tr>
    <td><label>Temperature</label></td>
    <td><input type="number" v-model.number="temperature_celcius" min="-999", max="900">
  </tr>
  <tr>
    <td><label>Humidity</label></td>
    <td><input type="number" v-model.number="humidity_percent" min="0", max="1000"></td>
  </tr>
  <tr>
    <td><label>Wind</label></td>
    <td><input type="number" v-model.number="wind_kph" min="0", max="1000"></td>
  </tr>
  <tr>
    <td><label>Time</label></td>
      <td><input type="number" v-model.number="time_24_hour" min="0", max="2400"></td>
  </tr>
  <tr>
    <td><label>Time leading zeros</label></td>
      <td><input type="checkbox" v-model.boolean="time_show_leading_zeros"/></td>
  </tr>
  <tr>
    <td><label>Time 24 hour</label></td>
    <td><input type="checkbox" v-model.boolean="time_show_24_hour"/></td>
  </tr>
  <tr>
    <td><label>Location</label></td>
    <td><input type="text" v-model.text="location"/></td>
  </tr>
  <tr>
    <td><label>Weather description</label></td>
    <td><input type="text" v-model.text="weather_description"/></td>
  </tr>
  <tr>
    <td><label>Moon phase</label></td>
    <td>
      <select v-model.number="moon_phase">
        <option :value="MoonPhase.NEW_MOON">new moon</option>
        <option :value="MoonPhase.WAXING_CRESCENT">waxing crescent</option>
        <option :value="MoonPhase.FIRST_QUARTER">first quarter</option>
        <option :value="MoonPhase.WAXING_GIBBOUS">waxing gibbous</option>
        <option :value="MoonPhase.FULL_MOON">full moon</option>
        <option :value="MoonPhase.WANING_GIBBOUS">Waning gibbous</option>
        <option :value="MoonPhase.THIRD_QUARTER">third quarter</option>
        <option :value="MoonPhase.WANING_CRESCENT">waning crescent</option>
      </select>
    </td>
  </tr>
  <tr>
    <td><label>Weather icon</label></td>
    <td>
      <select v-model.number="weather_icon">
        <option :value="WeatherIcon.WINTER">winter</option>
        <option :value="WeatherIcon.LIGHTNING_STORM">lightning storm</option>
        <option :value="WeatherIcon.HEAVY_RAIN">heavy rain</option>
        <option :value="WeatherIcon.PARTLY_CLOUDY">partly cloudy</option>
        <option :value="WeatherIcon.SUNNY">sunny</option>
      </select>
    </td>
  </tr>
</tbody>
</table>
</template>
