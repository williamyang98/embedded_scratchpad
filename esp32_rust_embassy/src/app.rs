use embassy_sync::{
    mutex::Mutex,
    blocking_mutex::raw::CriticalSectionRawMutex,
    pubsub::{PubSubChannel, Subscriber, Publisher, Error as PubSubError},
};
use bt_hci::param::BdAddr;
use itertools::Itertools;
use core::fmt::Display;
extern crate alloc;
use alloc::{
    sync::Arc,
    boxed::Box,
};
use core::ops::Deref;
use heapless::Deque;
use crate::{
    bluetooth_device::BluetoothDevice,
    led_controller::LedController,
};

#[derive(Debug, Clone, Copy)]
pub enum WebsocketWatchValue {
    ForceClose,
    BluetoothDevicesUpdated { total_added: usize },
    BackgroundTaskMessage { message: u32 },
}

const MAX_WEBSOCKET_VALUES: usize = 16;
pub const MAX_WEBSOCKET_SUBSCRIBERS: usize = 3;
const MAX_WEBSOCKET_PUBLISHERS: usize = 2;
// list of application publishers
// 1. bluetooth scanner
// 2. random number generator

pub type WebsocketChannel = PubSubChannel<CriticalSectionRawMutex, WebsocketWatchValue, MAX_WEBSOCKET_VALUES, MAX_WEBSOCKET_SUBSCRIBERS, MAX_WEBSOCKET_PUBLISHERS>;
pub type WebsocketPublisher<'a> = Publisher<'a, CriticalSectionRawMutex, WebsocketWatchValue, MAX_WEBSOCKET_VALUES, MAX_WEBSOCKET_SUBSCRIBERS, MAX_WEBSOCKET_PUBLISHERS>;
pub type WebsocketSubscriber<'a> = Subscriber<'a, CriticalSectionRawMutex, WebsocketWatchValue, MAX_WEBSOCKET_VALUES, MAX_WEBSOCKET_SUBSCRIBERS, MAX_WEBSOCKET_PUBLISHERS>;
pub type BluetoothDevices = Box<Deque<BluetoothDevice, 128>>;

pub struct App {
    websocket_channel: Arc<WebsocketChannel>,
    // https://docs.embassy.dev/embassy-sync/0.8.0/default/blocking_mutex/struct.Mutex.html#method.lock_mut
    // critical_mutex.lock_mut is not inherently safe which is why we use a RefCell as well
    bluetooth_devices: Arc<Mutex<CriticalSectionRawMutex, BluetoothDevices>>,
    pub led_controller: Arc<LedController>,
}

fn display_address(addr: &BdAddr) -> impl Display {
    addr.raw().iter().format_with("", |byte, f| f(&format_args!("{byte:02X}")))
}

impl App {
    pub fn new(led_controller: LedController) -> Self {
        Self {
            websocket_channel: Arc::new(WebsocketChannel::new()),
            bluetooth_devices: Arc::new(Mutex::new(Box::new(Deque::new()))),
            led_controller: Arc::new(led_controller),
        }
    }

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

    pub fn get_websocket_publisher(&self) -> Result<WebsocketPublisher<'_>, PubSubError> {
        self.websocket_channel.publisher()
    }
}
