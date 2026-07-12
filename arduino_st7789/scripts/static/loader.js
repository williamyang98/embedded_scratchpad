import { defineAsyncComponent } from "vue";
import * as Vue from "vue";
import { loadModule } from "vue3-sfc-loader";

const vue3_sfc_loader_options = {
  moduleCache: { vue: Vue },
  async getFile(url) {
    const res = await fetch(url);
    if (!res.ok) {
      throw Object.assign(new Error(res.statusText + ' ' + url), { res });
    }
    return {
      getContentData: asBinary => asBinary ? res.arrayBuffer() : res.text(),
    };
  },
  addStyle(text_content) {
    const style_elem = document.getElementById('vue_generated_styles');
    if (style_elem === null) return;
    style_elem.innerHTML += `\n${text_content}`;
  },
};

export function import_component(path) {
  const async_component = defineAsyncComponent(() => loadModule(path, vue3_sfc_loader_options));
  return async_component;
}
