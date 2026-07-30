<script setup>
import { DebugFrame } from "./debug_frame.js";
const props = defineProps({
  frame: DebugFrame,
});
</script>

<template>
<table>
  <tr><td>x_start</td><td>{{ frame.x_start }}</td></tr>
  <tr><td>x_end</td><td>{{ frame.x_end }}</td></tr>
  <tr><td>y_start</td><td>{{ frame.y_start }}</td></tr>
  <tr><td>y_end</td><td>{{ frame.y_end }}</td></tr>
  <tr><td>x_cursor</td><td>{{ frame.x_cursor }}</td></tr>
  <tr><td>y_cursor</td><td>{{ frame.y_cursor }}</td></tr>
  <tr><td>width</td><td>{{ frame.width }}</td></tr>
  <tr><td>height</td><td>{{ frame.height }}</td></tr>
  <tr><td>brightness</td><td>{{ frame.brightness }}</td></tr>
  <tr><td>hardware_reset</td><td>{{ frame.hardware_reset }}</td></tr>
  <tr><td>label</td><td><div>{{ frame.label || "?" }}</td></tr>
</table>
</template>
