<script setup>
import { ref, watchEffect } from "vue";

const emits = defineEmits(["command"]);

const temperature_celcius = ref(254);
const humidity_percent = ref(105);
const wind_kph = ref(52);
const time_24_hour = ref(1635);
const time_show_24_hour = ref(false);
const time_show_leading_zeros = ref(false);

function emit_command(command) {
    emits("command", command);
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

watchEffect(set_temperature);
watchEffect(set_wind_kph);
watchEffect(set_humidity);
watchEffect(set_time_24_hour);

function trigger_render() {
    emit_command({
        "type": "trigger_render",
    });
}

function refresh_all() {
    set_temperature();
    set_wind_kph();
    set_humidity();
    set_time_24_hour();
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
    <label>Humdity: </label>
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
</form>
</template>
