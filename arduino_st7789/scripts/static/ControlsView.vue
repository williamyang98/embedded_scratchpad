<script setup>
import { ref, watchEffect } from "vue";

const emits = defineEmits(["command"]);

const temperature_celcius = ref(254);
const humidity_percent = ref(105);
const wind_kph = ref(52);
const time_24_hour = ref(1635);
const time_show_24_hour = ref(false);
const time_show_leading_zeros = ref(false);
const location = ref("Sydney");
const weather_description = ref("cloudy rain");
const weather_icon = ref(0);
const moon_phase = ref(0);

function emit_command(command) {
  emits("command", command);
}

function trigger_render() {
  emit_command({
    "type": "trigger_render",
  });
}

function set_temperature() {
  emit_command({
    "type": "set_temperature",
    "temperature": temperature_celcius.value,
  });
}

function set_wind_kph() {
  emit_command({
    "type": "set_wind_kph",
    "wind_kph": wind_kph.value,
  });
}

function set_humidity() {
  emit_command({
    "type": "set_humidity",
    "humidity": humidity_percent.value,
  });
}

function set_time_24_hour() {
  emit_command({
    "type": "set_time_24_hour",
    "time_24_hour": time_24_hour.value,
    "show_24_hour": time_show_24_hour.value,
    "show_leading_zeros": time_show_leading_zeros.value,
  });
}

function set_location() {
  emit_command({
    "type": "set_location",
    "location": location.value,
  });
}

function set_weather_description() {
  emit_command({
    "type": "set_weather_description",
    "description": weather_description.value,
  });
}

function set_weather_icon() {
  emit_command({
    "type": "set_weather_icon",
    "icon": weather_icon.value,
  });
}

function set_moon_phase() {
  emit_command({
    "type": "set_moon_phase",
    "phase": moon_phase.value,
  });
}

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
  trigger_render();
}

defineExpose({
  submit() {
    refresh_all();
  }
})

</script>

<template>
<form>
  <button @click.stop="trigger_render" type="button">Trigger Render</button>
  <button @click.stop="refresh_all" type="button">Refresh All</button>
  <div>
    <label>Temperature: </label>
    <input type="number" v-model.number="temperature_celcius" min="-999", max="900">
  </div>
  <div>
    <label>Humidity: </label>
    <input type="number" v-model.number="humidity_percent" min="0", max="1000">
  </div>
  <div>
    <label>Wind: </label>
    <input type="number" v-model.number="wind_kph" min="0", max="1000">
  </div>
  <div>
    <label>Time: </label>
    <input type="number" v-model.number="time_24_hour" min="0", max="2400">
  </div>
  <div>
    <label>Time leading zeros: </label>
    <input type="checkbox" v-model.boolean="time_show_leading_zeros"/>
  </div>
  <div>
    <label>Time 24 hour: </label>
    <input type="checkbox" v-model.boolean="time_show_24_hour"/>
  </div>
  <div>
    <label>Location: </label>
    <input type="text" v-model.text="location"/>
  </div>
  <div>
    <label>Weather description: </label>
    <input type="text" v-model.text="weather_description"/>
  </div>
  <div>
    <label>Moon phase: </label>
    <select v-model.number="moon_phase">
      <option value="0">new moon</option>
      <option value="1">waxing crescent</option>
      <option value="2">first quarter</option>
      <option value="3">waxing gibbous</option>
      <option value="4">full moon</option>
      <option value="5">waning gibbous</option>
      <option value="6">third quarter</option>
      <option value="7">waning crescent</option>
    </select>
  </div>
  <div>
    <label>Weather icon: </label>
    <select v-model.number="weather_icon">
      <option value="0">winter</option>
      <option value="1">lightning storm</option>
      <option value="2">heavy rain</option>
      <option value="3">partly cloudy</option>
      <option value="4">sunny</option>
    </select>
  </div>
</form>
</template>
