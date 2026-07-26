<script setup>
import { ref, useTemplateRef, computed, watch, onMounted } from "vue";

const websocket = ref(null);
const websocket_state = ref(WebSocket.CLOSED);
const is_running = ref(false);
const is_websocket_open = computed(() => {
  if (websocket.value === null) return false;
  return websocket_state.value === WebSocket.OPEN;
});
const websocket_url = ref(`ws://${document.location.host}/ws`);
const responses = ref([]);
const message = ref("Hello World");

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
  <b>Responses ({{ responses.length }})</b>
  <button @click="clear_responses">Clear</button>
</div>
<table>
  <thead>
    <tr><th>Index</th><th>Type</th><th>Value</th></tr>
  </thead>
  <tbody>
    <tr v-for="(row, index) in responses" :key="index">
      <td>{{ index }}</td>
      <td>{{ row.type }}</td>
      <td v-if="row.type === 'text'">{{ row.data }}</td>
      <td v-else-if="row.type === 'binary'">[{{ row.data.join(",") }}]</td>
    </tr>
  </tbody>
</table>
<ol>
</ol>
</template>

<style scoped>

</style>

