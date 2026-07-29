<script setup>
const props = defineProps({
  header: Object,
});
</script>

<template>
<table>
  <tr><td>x_start</td><td>{{ header.x_start }}</td></tr>
  <tr><td>x_end</td><td>{{ header.x_end }}</td></tr>
  <tr><td>y_start</td><td>{{ header.y_start }}</td></tr>
  <tr><td>y_end</td><td>{{ header.y_end }}</td></tr>
  <tr><td>x_cursor</td><td>{{ header.x_cursor }}</td></tr>
  <tr><td>y_cursor</td><td>{{ header.y_cursor }}</td></tr>
  <tr><td>width</td><td>{{ header.width }}</td></tr>
  <tr><td>height</td><td>{{ header.height }}</td></tr>
  <tr><td>brightness</td><td>{{ header.brightness }}</td></tr>
  <tr><td>hardware_reset</td><td>{{ header.hardware_reset }}</td></tr>
  <tr><td>label</td><td><div>{{ header.label || "?" }}</td></tr>
</table>
</template>
