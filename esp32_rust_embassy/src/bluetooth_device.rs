use bt_hci::param::{LeAdvEventKind, AddrKind, BdAddr};
use serde::ser::{Serialize, Serializer, SerializeStruct};

#[derive(Debug, Clone, Copy)]
pub struct BluetoothDevice {
    pub event_kind: LeAdvEventKind,
    pub addr_kind: AddrKind,
    pub addr: BdAddr,
    pub rssi: i8,
}

impl Serialize for BluetoothDevice {
    fn serialize<S: Serializer>(&self, serializer: S) -> Result<S::Ok, S::Error> {
        let mut state = serializer.serialize_struct("BluetoothDevice", 4)?;

        let event_kind: &'static str = match self.event_kind {
            LeAdvEventKind::AdvInd => "ind",
            LeAdvEventKind::AdvDirectInd => "direct_ind",
            LeAdvEventKind::AdvScanInd => "scan_ind",
            LeAdvEventKind::AdvNonconnInd => "non_connection_ind",
            LeAdvEventKind::ScanRsp => "scan_response",
        };
        state.serialize_field("event_kind", event_kind)?;

        let addr_kind: Option<&'static str> = match self.addr_kind {
            AddrKind::PUBLIC => Some("public"),
            AddrKind::RANDOM => Some("random"),
            AddrKind::RESOLVABLE_PRIVATE_OR_PUBLIC => Some("resolvable_private_or_public"),
            AddrKind::RESOLVABLE_PRIVATE_OR_RANDOM => Some("resolvable_private_or_random"),
            AddrKind::ANONYMOUS_ADV => Some("anonymous_adv"),
            _ => None,
        };
        if let Some(addr_kind) = addr_kind {
            state.serialize_field("addr_kind", addr_kind)?;
        } else {
            state.serialize_field("addr_kind", &self.addr_kind.as_raw())?;
        }
        state.serialize_field("addr", self.addr.raw())?;
        state.serialize_field("rssi", &self.rssi)?;
        state.end()
    }
}
