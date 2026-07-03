<script setup>
import { ref, useTemplateRef, computed, watch, onMounted } from "vue";
import FrameView from "./FrameView.vue";

const frame_elem = useTemplateRef("frame");
const frames = ref([]);
const selected_frame_index = ref(0);
const selected_frame = computed(() => frames.value[selected_frame_index.value]);
const is_running = ref(false);
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
    const websocket = new WebSocket(websocket_url);
    websocket.addEventListener("open", () => {
      is_running.value = true;
    });
    websocket.addEventListener("message", (event) => {
      if (typeof event.data === "string") {
        previous_frame_header = JSON.parse(event.data);
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
    websocket.addEventListener("close", (event) => {
      is_running.value = false;
    });
  } catch {
    is_running.value = false;
  }
}

onMounted(() => {
  launch_process();
});

watch(selected_frame, (selected_frame) => {
  if (selected_frame === undefined) return;
  if (frame_elem.value === null) return;
  const { header, image } = selected_frame;
  if (header === null) return;
  frame_elem.value.update_image(image, header.image_width, header.image_height);
}, );

</script>

<template>
<button @click="launch_process" :disabled="is_running">Run</button>
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
  <table v-if="selected_frame.header !== null">
    <tr><td>x_start</td><td>{{ selected_frame.header.x_start }}</td></tr>
    <tr><td>x_end</td><td>{{ selected_frame.header.x_end }}</td></tr>
    <tr><td>y_start</td><td>{{ selected_frame.header.y_start }}</td></tr>
    <tr><td>y_end</td><td>{{ selected_frame.header.y_end }}</td></tr>
    <tr><td>x_cursor</td><td>{{ selected_frame.header.x_cursor }}</td></tr>
    <tr><td>y_cursor</td><td>{{ selected_frame.header.y_cursor }}</td></tr>
    <tr><td>image_width</td><td>{{ selected_frame.header.image_width }}</td></tr>
    <tr><td>image_height</td><td>{{ selected_frame.header.image_height }}</td></tr>
    <tr><td>label</td><td><div>{{ selected_frame.header.label || "?" }}</td></tr>
  </table>
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
