use embassy_sync::{
    mutex::Mutex,
    blocking_mutex::raw::CriticalSectionRawMutex,
    pubsub::{PubSubChannel, Subscriber, Publisher, Error as PubSubError},
};
use bt_hci::param::{LeAdvEventKind, AddrKind, BdAddr};
use itertools::Itertools;
use core::fmt::Display;
use serde::ser::{Serialize, Serializer, SerializeStruct};
extern crate alloc;
use alloc::{
    sync::Arc,
    boxed::Box,
};
use core::ops::Deref;
use heapless::Deque;

#[derive(Debug, Clone, Copy)]
pub enum WebsocketWatchValue {
    ForceClose,
    BluetoothDevicesUpdated { total_added: usize },
}


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

const MAX_WEBSOCKET_VALUES: usize = 16;
const MAX_WEBSOCKET_PUBLISHERS: usize = 2;
pub const MAX_WEBSOCKET_SUBSCRIBERS: usize = 4;

pub type WebsocketChannel = PubSubChannel<CriticalSectionRawMutex, WebsocketWatchValue, MAX_WEBSOCKET_VALUES, MAX_WEBSOCKET_SUBSCRIBERS, MAX_WEBSOCKET_PUBLISHERS>;
pub type WebsocketPublisher<'a> = Publisher<'a, CriticalSectionRawMutex, WebsocketWatchValue, MAX_WEBSOCKET_VALUES, MAX_WEBSOCKET_SUBSCRIBERS, MAX_WEBSOCKET_PUBLISHERS>;
pub type WebsocketSubscriber<'a> = Subscriber<'a, CriticalSectionRawMutex, WebsocketWatchValue, MAX_WEBSOCKET_VALUES, MAX_WEBSOCKET_SUBSCRIBERS, MAX_WEBSOCKET_PUBLISHERS>;
pub type BluetoothDevices = Box<Deque<BluetoothDevice, 128>>;

pub struct App {
    websocket_channel: Arc<WebsocketChannel>,
    // https://docs.embassy.dev/embassy-sync/0.8.0/default/blocking_mutex/struct.Mutex.html#method.lock_mut
    // critical_mutex.lock_mut is not inherently safe which is why we use a RefCell as well
    bluetooth_devices: Arc<Mutex<CriticalSectionRawMutex, BluetoothDevices>>,
}

impl Default for App {
    fn default() -> Self {
        Self {
            websocket_channel: Arc::new(WebsocketChannel::new()),
            bluetooth_devices: Arc::new(Mutex::new(Box::new(Deque::new()))),
        }
    }
}

fn display_address(addr: &BdAddr) -> impl Display {
    addr.raw().iter().format_with("", |byte, f| f(&format_args!("{byte:02X}")))
}

impl App {
    pub async fn extend_bluetooth_devices(&self, new_devices: impl Iterator<Item=BluetoothDevice>) {
        let mut devices = self.bluetooth_devices.lock().await;
        let mut total_added = 0;
        for new_device in new_devices {
            let is_new_address = devices.iter().find(|device| device.addr.raw() == new_device.addr.raw()).is_none();
            if is_new_address {
                log::info!("Adding new bluetooth device with address=0x{0} to existing total_devices={1}",
                    display_address(&new_device.addr), devices.len());
                if devices.is_full() && let Some(old_device) = devices.pop_front() {
                    log::info!("Removing oldest seen bluetooth device with address=0x{0}", display_address(&old_device.addr));
                }
                devices.push_back(new_device).expect("There should always be room to push back a new bluetooth device");
                total_added += 1;
            }
        }
        drop(devices);

        if total_added > 0 {
            match self.websocket_channel.publisher() {
                Ok(publisher) => publisher.publish_immediate(WebsocketWatchValue::BluetoothDevicesUpdated { total_added }),
                Err(err) => log::error!("Couldn't acquire publisher to send bluetooth devices changed notification: {err:?}"),
            }
        }
    }

    pub async fn get_bluetooth_devices(&self) -> impl Deref<Target=BluetoothDevices> {
        self.bluetooth_devices.lock().await
    }

    pub fn get_websocket_subscriber(&self) -> Result<WebsocketSubscriber<'_>, PubSubError> {
        self.websocket_channel.subscriber()
    }
}
