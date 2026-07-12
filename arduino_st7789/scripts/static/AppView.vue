<script setup>
import { ref, useTemplateRef, computed, watch, onMounted } from "vue";
import FrameView from "./FrameView.vue";
import ControlsView from "./ControlsView.vue";
import FrameHeaderTable from "./FrameHeaderTable.vue"

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

const is_pin_frame = ref(false);
const frame_scale = ref(1.0);

function launch_process() {
  is_running.value = true;
  const websocket_url = `ws://${document.location.host}/websocket`;
  frames.value = [];

  if (!is_pin_frame.value) {
    selected_frame_index.value = 0;
  }

  let previous_frame_header = null;
  try {
    websocket.value = new WebSocket(websocket_url);
    websocket_state.value = WebSocket.CONNECTING;
    websocket.value.addEventListener("open", () => {
      websocket_state.value = WebSocket.OPEN;
      if (controls_elem.value === null) return;
      controls_elem.value.submit();
    });
    websocket.value.addEventListener("message", (event) => {
      if (typeof event.data === "string") {
        const data = JSON.parse(event.data);
        if (data.type === "debug_frame") {
          previous_frame_header = data;
        } else {
          console.log(data);
        }
      } else {
        const header = previous_frame_header;
        previous_frame_header = null;
        event.data.arrayBuffer()
          .then((byte_data) => {
            const image = new Uint16Array(byte_data);
            frames.value.push({ header, image });
            if (!is_pin_frame.value) {
              selected_frame_index.value = frames.value.length-1;
            }
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
    websocket.value.send(JSON.stringify(command));
}

onMounted(() => {
  launch_process();
});

watch(selected_frame, (selected_frame) => {
  if (selected_frame === undefined) return;
  if (frame_elem.value === null) return;
  const { header, image } = selected_frame;
  if (header === null) return;
  frame_elem.value.update_image(image, header.width, header.height);
}, );

</script>

<template>
<button @click="launch_process" :disabled="is_running">Run</button>
<button @click="end_process" :disabled="!can_send_commands">End</button>
<ControlsView ref="controls" @command="on_command"/>
<div>
  <label>Pin frame</label>
  <input type="checkbox" v-model.boolean="is_pin_frame"/>
</div>
<form>
  <input type="number" v-model.number="selected_frame_index" min="0", :max="frames.length-1" :disabled="frames.length === 0"/>/<span>{{ frames.length > 0 ? frames.length-1 : '?' }}</span>
  <input type="range" v-model.number="selected_frame_index" min="0" :max="frames.length-1" :disabled="frames.length === 0" step="1"/>
</form>
<div v-if="selected_frame === undefined">Waiting for frame</div>
<div v-else>
  <FrameHeaderTable v-if="selected_frame.header !== null" :header="selected_frame.header"/>
  <div v-else>Missing header data</div>
</div>
<div>
    <label>Scale: {{ frame_scale.toFixed(1) }}</label>
    <input type="range" v-model.number="frame_scale" min="0" max="4" step="0.1"/>
</div>
<div class="frame" :class="{ 'hidden': selected_frame === undefined }">
  <FrameView ref="frame" :scale="frame_scale"/>
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
