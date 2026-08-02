<script setup>
import { ref, reactive, useTemplateRef, computed, watch, onMounted } from "vue";
import FrameView from "./FrameView.vue";
import ControlsView from "./ControlsView.vue";
import FrameHeaderTable from "./FrameHeaderTable.vue"
import {
  parse_websocket_response, dht11_read_status_to_string,
  CommandCreator,
  DEFAULT_WEBSOCKET_URL,
} from "./web_socket.js";
import { milliseconds_to_dhms } from "./utility.js";

const frame_elem = useTemplateRef("frame");
const controls_elem = useTemplateRef("controls");

const frames = ref([]);
const selected_frame_index = ref(0);
const selected_frame = computed(() => frames.value[selected_frame_index.value]);

const websocket = ref(null);
const websocket_state = ref(WebSocket.CLOSED);
const is_running = ref(false);
const can_send_commands = computed(() => {
  if (websocket.value === null) return false;
  return websocket_state.value === WebSocket.OPEN;
});
const websocket_url = ref(DEFAULT_WEBSOCKET_URL);
const command_creator = new CommandCreator();

const is_pin_frame = ref(false);
const frame_scale = ref(1.0);
const is_render_busy = ref(false);
const acknowledged_commands = ref({});
const dht11 = ref({
  temperature: null,
  humidity: null,
  error_code: null,
});
const device_uptime = ref(null);

function count_acknowledged_command(header, is_success) {
  let counter = acknowledged_commands.value[header];
  if (counter === undefined) {
    counter = reactive({ successes: 0, fails: 0 });
    acknowledged_commands.value[header] = counter;
  }
  if (is_success) {
    counter.successes++;
  } else {
    counter.fails++;
  }
}

function clear_acknowledged_commands() {
  for (const counter of Object.values(acknowledged_commands.value)) {
    counter.successes = 0;
    counter.fails = 0;
  }
}

function handle_response(response) {
  if (response.type === "debug_frame") {
    frames.value.push(response.frame);
    if (!is_pin_frame.value) {
      selected_frame_index.value = frames.value.length-1;
    }
  } else if (response.type === "render_status") {
    is_render_busy.value = response.is_busy;
  } else if (response.type === "acknowledge_command") {
    count_acknowledged_command(response.header, response.is_success);
  } else if (response.type === "dht11") {
    if (response.is_success) {
      dht11.value.temperature = response.temperature;
      dht11.value.humidity = response.humidity;
      dht11.value.error_code = null;
    } else {
      dht11.value.error_code = response.error_code;
    }
  } else if (response.type === "uptime") {
    device_uptime.value = response.timestamp_ms;
  } else {
    console.log(response);
  }
}

function launch_process() {
  is_running.value = true;
  frames.value = [];

  if (!is_pin_frame.value) {
    selected_frame_index.value = 0;
  }

  try {
    websocket.value = new WebSocket(websocket_url.value);
    websocket_state.value = WebSocket.CONNECTING;
    websocket.value.addEventListener("open", () => {
      websocket_state.value = WebSocket.OPEN;
    });
    websocket.value.addEventListener("message", (event) => {
      if (typeof event.data === "string") {
        console.log(`Got websocket message: ${event.data}`);
      } else {
        event.data.arrayBuffer()
          .then((array_buffer) => {
            const buffer = new Uint8Array(array_buffer);
            const response = parse_websocket_response(buffer);
            handle_response(response);
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

function end_process() {
  if (websocket.value === null) return;
  if (websocket_state.value !== WebSocket.OPEN) return;
  websocket.value.close();
}

function on_command(command) {
  if (websocket.value === null) return;
  if (websocket_state.value !== WebSocket.OPEN) return;
  websocket.value.send(command);
}

onMounted(() => {
  launch_process();
});

watch(selected_frame, (frame) => {
  if (frame === undefined) return;
  if (frame_elem.value === null) return;
  frame_elem.value.update_image(frame.pixel_data, frame.width, frame.height);
});

function refresh_dht11() {
  on_command(command_creator.get_dht11());
}

function refresh_uptime() {
  on_command(command_creator.get_uptime());
}

const background_task_uptime_interval_id = ref(null);

function stop_background_task_uptime() {
  if (background_task_uptime_interval_id.value !== null) {
    clearInterval(background_task_uptime_interval_id.value);
    background_task_uptime_interval_id.value = null;
  }
}

function start_background_task_uptime() {
  stop_background_task_uptime();
  background_task_uptime_interval_id.value = setInterval(() => {
    refresh_uptime();
  }, 60000);
}

function format_uptime(milliseconds) {
  const dhms = milliseconds_to_dhms(milliseconds);
  let text = "";
  if (text.length > 0 || dhms.days > 0) text += `${dhms.days}d`;
  if (text.length > 0 || dhms.hours > 0) text += ` ${dhms.hours}h`;
  if (text.length > 0 || dhms.minutes > 0) text += ` ${dhms.minutes}m`;
  text += ` ${dhms.seconds}s`;
  // text += ` ${dhms.milliseconds}ms`;
  return text;
}

watch(websocket_state, (websocket_state) => {
  if (websocket_state === WebSocket.OPEN) {
    if (controls_elem.value !== null) {
      controls_elem.value.submit();
    }
    refresh_dht11();
    refresh_uptime();
    start_background_task_uptime();
  } else if (websocket_state === WebSocket.CLOSED) {
    stop_background_task_uptime();
  }
});

</script>

<template>
<div style="margin-bottom: 1rem">
  <input type="text" v-model="websocket_url" style="min-width: 20rem"/>
  <button v-if="!can_send_commands" @click="launch_process" :disabled="is_running">Connect</button>
  <button v-else @click="end_process">Disconnect</button>
</div>
<div class="d-flex flex-row">
  <div class="d-flex flex-col" style="margin-right: 1rem">
    <span><b>Controls</b></span>
    <ControlsView ref="controls" @command="on_command"/>
    <br>
    <div>
      <span style="margin-right: 1rem"><b>DHT11</b></span>
      <button @click="refresh_dht11" :disabled="!can_send_commands">Refresh</button>
    </div>
    <table>
      <thead>
        <tr><th>Temperature</th><th>Humidity</th><th>Status</th></tr>
      </thead>
      <tbody>
        <tr>
          <td style="width: 33%">{{ dht11.temperature === null ? '?' : `${dht11.temperature}°C` }}</td>
          <td style="width: 33%">{{ dht11.humidity === null ? '?' : `${dht11.humidity}%` }}</td>
          <td>{{ dht11.error_code === null ? 'Good' : `Error(${dht11_read_status_to_string(dht11.error_code)})` }}</td>
        </tr>
      </tbody>
    </table>
    <br>
    <span><b>Device Status</b></span>
    <table>
      <colgroup>
        <col style="width: 30%;">
        <col style="width: 50%;">
        <col style="width: 20%;">
      </colgroup>
      <tbody>
        <tr>
          <td><b>Rendering</b></td>
          <td>{{ is_render_busy }}</td>
          <td></td>
        </tr>
        <tr>
          <td><b>Uptime</b></td>
          <td>{{ device_uptime === null ? '?' : format_uptime(device_uptime) }}</td>
          <td><button @click="refresh_uptime" :disabled="!can_send_commands">Refresh</button></td>
        </tr>
      </tbody>
    </table>
    <br>
    <div>
      <span style="margin-right: 1rem"><b>Acknowledged commands</b></span>
      <button @click="clear_acknowledged_commands">Clear</button>
    </div>
    <table>
      <thead>
        <tr><th>Header</th><th>Successes</th><th>Fails</th></tr>
      </thead>
      <tbody>
        <tr v-for="[header, counter] of Object.entries(acknowledged_commands)" :key="header">
          <td style="width: 33%">{{ header }}</td>
          <td style="width: 33%">{{ counter.successes }}</td>
          <td>{{ counter.fails }}</td>
        </tr>
      </tbody>
    </table>
  </div>
  <div class="d-flex flex-col">
    <div>
      <label>Pin frame</label>
      <input type="checkbox" v-model.boolean="is_pin_frame"/>
    </div>
    <form>
      <input type="number" v-model.number="selected_frame_index" min="0", :max="frames.length-1" :disabled="frames.length === 0"/>/<span>{{ frames.length > 0 ? frames.length-1 : '?' }}</span>
      <input type="range" v-model.number="selected_frame_index" min="0" :max="frames.length-1" :disabled="frames.length === 0" step="1"/>
    </form>
    <div v-if="selected_frame === undefined">Waiting for frame</div>
    <FrameHeaderTable v-else :frame="selected_frame"/>
    <br>
    <div>
      <label>Scale: {{ frame_scale.toFixed(1) }}</label>
      <input type="range" v-model.number="frame_scale" min="0" max="4" step="0.1"/>
    </div>
    <div class="frame" :class="{ 'hidden': selected_frame === undefined }">
      <FrameView ref="frame" :scale="frame_scale"/>
    </div>
  </div>
</div>
</template>

<style scoped>
.frame {
  border: solid 1px black;
  display: inline-block;
  padding: 0;
  margin: 0;
}
</style>
