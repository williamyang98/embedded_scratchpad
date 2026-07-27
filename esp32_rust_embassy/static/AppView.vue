<script setup>
import { ref, useTemplateRef, computed, watch, onMounted } from "vue";
import { WEBSOCKET_URL, get_bluetooth_devices, get_heap_stats } from "./api.js";

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

const bluetooth_devices = ref([]);
const is_bluetooth_devices_refreshing = ref(false);
const heap_stats = ref({});
const is_heap_stats_refreshing = ref(false);

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
        responses.value.push({ type: "text", data });
      } else {
        event.data.arrayBuffer().then((byte_data) => {
          const data = new Uint8Array(byte_data);
          responses.value.push({ type: "binary", data });
          handle_binary_response(data);
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

const ResponseHeader = {
    MissedMessages: 0x00,
    BluetoothUpdate: 0x01,
};

function handle_binary_response(data) {
  if (data.length === 0) return;
  const header = data[0];
  if (header === ResponseHeader.MissedMessages) {
    if (data.length !== 2) throw Error(`Expected length of 2`);
    let total_missed = data[1];
    console.warn(`Missed ${total_missed} client messages on websocket connection`);
    return;
  }
  if (header === ResponseHeader.BluetoothUpdate) {
    if (data.length !== 2) throw Error(`Expected length of 2`);
    let total_added = data[1];
    refresh_bluetooth_devices();
    return;
  }
  console.error(`Unhandled binary response header=${header}, data=[${data.join(',')}]`);
}

function format_bluetooth_address(addr) {
  return addr
    .map(v => v.toString())
    .map(s => s.padStart(3, "0"))
    .join(".");
}

onMounted(() => {
  refresh_bluetooth_devices();
  refresh_heap_stats();
  connect_to_websocket();
});

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
      <td v-if="row.type === 'text'">{{ row.data }}</td>
      <td v-else-if="row.type === 'binary'">[{{ row.data.join(",") }}]</td>
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

