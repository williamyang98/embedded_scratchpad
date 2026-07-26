use esp_alloc;
use serde::ser::{Serialize, Serializer, SerializeStruct};
extern crate alloc;
use alloc::format;

pub struct HeapStats(pub &'static esp_alloc::EspHeap);

impl Serialize for HeapStats {
    fn serialize<S: Serializer>(&self, serializer: S) -> Result<S::Ok, S::Error> {
        let mut state = serializer.serialize_struct("HeapStats", 3)?;
        state.serialize_field("used", &self.0.used())?;
        state.serialize_field("free", &self.0.free())?;
        state.serialize_field("display", &format!("{0}", self.0.stats()))?;
        state.end()
    }
}
