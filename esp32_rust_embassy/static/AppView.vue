<script setup>
import { ref, useTemplateRef, computed, watch, onMounted } from "vue";
import { WEBSOCKET_URL, get_bluetooth_devices, get_heap_stats } from "./api.js";
import { parse_websocket_response, WebsocketCommandCreator } from "./web_socket.js";
import { format_bluetooth_address, format_object_to_string, debounce_timeout } from "./utility.js";

const websocket = ref(null);
const websocket_state = ref(WebSocket.CLOSED);
const is_running = ref(false);
const is_websocket_open = computed(() => {
  if (websocket.value === null) return false;
  return websocket_state.value === WebSocket.OPEN;
});
const websocket_url = ref(WEBSOCKET_URL);
const responses = ref([]);
const message = ref("Hello World");
const led_duty_cycle = ref(0);

const bluetooth_devices = ref([]);
const is_bluetooth_devices_refreshing = ref(false);
const heap_stats = ref({
  free: "?",
  used: "?",
});
const is_heap_stats_refreshing = ref(false);
const websocket_command_creator = new WebsocketCommandCreator();

function connect_to_websocket() {
  is_running.value = true;
  try {
    websocket.value = new WebSocket(websocket_url.value);
    websocket_state.value = WebSocket.CONNECTING;
    websocket.value.addEventListener("open", () => {
      websocket_state.value = WebSocket.OPEN;
    });
    websocket.value.addEventListener("message", (event) => {
      if (typeof event.data === "string") {
        const data = event.data;
        responses.value.push({ type: "message", data });
      } else {
        event.data.arrayBuffer().then((byte_data) => {
          const data = new Uint8Array(byte_data);
          handle_websocket_response(data);
        });
      }
    });
    websocket.value.addEventListener("close", (event) => {
      websocket_state.value = WebSocket.CLOSED;
      websocket.value = null;
      is_running.value = false;
    });
  } catch {
    websocket_state.value = WebSocket.CLOSED;
    websocket.value = null;
    is_running.value = false;
  }
}

function disconnect_from_websocket() {
  if (websocket.value === null) return;
  if (websocket_state.value !== WebSocket.OPEN) return;
  websocket.value.close();
}

function clear_responses() {
  responses.value.length = 0;
}

function send_message() {
  if (websocket.value === null) return;
  if (websocket_state.value !== WebSocket.OPEN) return;
  websocket.value.send(message.value);
}

async function refresh_bluetooth_devices() {
  if (is_bluetooth_devices_refreshing.value) return;
  is_bluetooth_devices_refreshing.value = true;
  try {
    bluetooth_devices.value = await get_bluetooth_devices();
  } catch (err) {
    console.error(`Failed to fetch bluetooth devices: ${err}`);
  } finally {
    is_bluetooth_devices_refreshing.value = false;
  }
}

async function refresh_heap_stats() {
  if (is_heap_stats_refreshing.value) return;
  is_heap_stats_refreshing.value = true;
  try {
    heap_stats.value = await get_heap_stats();
  } catch (err) {
    console.error(`Failed to fetch heap stats: ${err}`);
  } finally {
    is_heap_stats_refreshing.value = false;
  }
}

function handle_websocket_response(data) {
  try {
    let res = parse_websocket_response(data);
    if (res.type === "missed_messages") {
      responses.value.push({ type: "missed_messages", data: { total_missed: res.total_missed } });
      refresh_bluetooth_devices();
    } else if (res.type === "bluetooth_update") {
      responses.value.push({ type: "bluetooth_update", data: { total_added: res.total_added } });
      refresh_bluetooth_devices();
    } else if (res.type === "get_led_duty_cycle") {
      led_duty_cycle.value = res.duty_cycle;
    } else {
      console.log(res);
    }
  } catch (err) {
    console.error(err);
  }
}

function refresh_led_duty_cycle() {
  if (websocket.value === null) return;
  let c = websocket_command_creator;
  websocket.value.send(c.get_led_duty_cycle());
}

onMounted(() => {
  connect_to_websocket();
  refresh_bluetooth_devices();
  refresh_heap_stats();
});

watch(is_websocket_open, (is_open) => {
  if (!is_open) return;
  refresh_led_duty_cycle();
});

const set_led_duty_cycle = debounce_timeout((led_duty_cycle) => {
  if (websocket.value === null) return;
  let c = websocket_command_creator;
  websocket.value.send(c.set_led_duty_cycle(led_duty_cycle));
}, 10);

watch(led_duty_cycle, (led_duty_cycle) => {
  set_led_duty_cycle(led_duty_cycle);
})

</script>

<template>
<div>
  <input type="text" v-model="websocket_url">
  <button v-if="!is_websocket_open" @click="connect_to_websocket" :disabled="is_running">Connect</button>
  <button v-else @click="disconnect_from_websocket">Disconnect</button>
</div>
<div>
  <input type="text" v-model="message">
  <button @click="send_message" :disabled="!is_websocket_open">Send</button>
</div>
<div>
  <span>LED: </button>
  <input type="range" v-model.number="led_duty_cycle" min="0" max="100">
  <button @click="refresh_led_duty_cycle" :disabled="!is_websocket_open">Refresh</button>
</div>
<br>
<div>
  <b>Responses ({{ responses.length }})</b>
  <button @click="clear_responses">Clear</button>
</div>
<table>
  <thead>
    <tr><th>index</th><th>type</th><th>payload</th></tr>
  </thead>
  <tbody>
    <tr v-for="(row, index) in responses" :key="index">
      <td>{{ index }}</td>
      <td>{{ row.type }}</td>
      <td v-if="typeof(row.data) === 'string'">{{ row.data }}</td>
      <td v-else-if="row.data.join !== undefined">[{{ row.data.join(",") }}]</td>
      <td v-else>{{ format_object_to_string(row.data) }}</td>
    </tr>
    <tr v-if="responses.length === 0">
      <td colspan="4">No responses</td>
    </tr>
  </tbody>
</table>
<br>
<div>
  <b>Bluetooth devices ({{ bluetooth_devices.length }})</b>
  <button @click="refresh_bluetooth_devices" :disabled="is_bluetooth_devices_refreshing">Refresh</button>
  <button @click="() => bluetooth_devices.length = 0">Clear</button>
</div>
<table>
  <thead>
    <tr><th>index</th><th>event kind</th><th>address kind</th><th>address</th><th>rssi</th></tr>
  </thead>
  <tbody>
    <tr v-for="(device, index) in bluetooth_devices" :key="index">
      <td>{{ index }}</td>
      <td>{{ device.event_kind }}</td>
      <td>{{ device.addr_kind }}</td>
      <td>{{ format_bluetooth_address(device.addr) }}</td>
      <td>{{ device.rssi }}</td>
    </tr>
    <tr v-if="bluetooth_devices.length === 0">
      <td colspan="5">No bluetooth devices</td>
    </tr>
  </tbody>
</table>
<br>
<div>
  <b>Heap stats</b>
  <button @click="refresh_heap_stats" :disabled="is_heap_stats_refreshing">Refresh</button>
</div>
<table>
  <thead>
    <tr><th>key</th><th>value</th></tr>
  </thead>
  <tbody>
    <tr v-for="[key, value] in Object.entries(heap_stats)" :key="key">
      <td>{{ key }}</td>
      <td><span style="white-space: pre-line">{{ value }}</span></td>
    </tr>
  </tbody>
</table>
</template>

<style scoped>

</style>

