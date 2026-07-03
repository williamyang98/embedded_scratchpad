<script setup>
import { ref, useTemplateRef, computed, watch, defineExpose } from "vue";

const props = defineProps(["scale"]);
const canvas = useTemplateRef("canvas");
const canvas_context = ref(null);
const image_data = ref(null);
const scale = computed(() => {
    if (props.scale === undefined) return 1.0;
    return props.scale;
});

watch(canvas, (canvas) => {
  if(canvas === null) return;
  canvas_context.value = canvas.getContext("2d");
}, { immediate: true });

function update_image(rgb565_data, width, height) {
  const total_pixels = width*height;
  if (total_pixels != rgb565_data.length) {
    throw Error(`Total number of pixels ${rgb565_data} in rgb565 data doesn't match resolution width=${width}, height=${height}, total_pixels=${total_pixels}`);
  }

  if (canvas.value === null) return;
  if (canvas_context.value === null) return;

  if (image_data.value === null || canvas.value.width !== width || canvas.value.height !== height) {
    canvas.value.width = width;
    canvas.value.height = height;
    canvas.value.style.width = `${Math.round(width*scale.value)}px`;
    canvas.value.style.height = `${Math.round(height*scale.value)}px`;
    image_data.value = canvas_context.value.createImageData(width, height);
  }

  const rgba8 = image_data.value.data;
  for (let i = 0; i < total_pixels; i++) {
    const j = i*4;
    const rgb565 = rgb565_data[i];
    let r = (rgb565 & 0b11111_000000_00000) >> 11;
    let g = (rgb565 & 0b00000_111111_00000) >> 5;
    let b = (rgb565 & 0b00000_000000_11111);
    r = Math.floor(r / 31 * 255);
    g = Math.floor(g / 63 * 255);
    b = Math.floor(b / 31 * 255);
    rgba8[j+0] = r;
    rgba8[j+1] = g;
    rgba8[j+2] = b;
    rgba8[j+3] = 255;
  }
  canvas_context.value.putImageData(image_data.value, 0, 0);
}

watch(scale, (scale) => {
  if (canvas.value === null) return;
  const width = Math.round(canvas.value.width*scale);
  const height = Math.round(canvas.value.height*scale);
  canvas.value.style.width = `${width}px`;
  canvas.value.style.height = `${height}px`;
});

defineExpose({
  update_image,
});
</script>

<template>
<canvas ref="canvas"></canvas>
</template>

<style scoped>
canvas {
  display: block;
  image-rendering: pixelated;
  image-rendering: crisp-edges;
}
</style>
