use embassy_sync::{
    blocking_mutex::{
        raw::CriticalSectionRawMutex,
        CriticalSectionMutex,
    },
    pubsub::{PubSubChannel, Subscriber, Publisher, Error as PubSubError},
};
use bt_hci::param::{LeAdvEventKind, AddrKind, BdAddr, LeAdvReport, LeAdvReportsIter};
use itertools::Itertools;
use core::{
    cell::RefCell,
    fmt::Display,
};
extern crate alloc;
use alloc::{
    sync::Arc,
    collections::VecDeque,
};

#[derive(Debug, Clone, Copy)]
pub enum WebsocketWatchValue {
    ForceClose,
    BluetoothDevicesUpdated { total_added: usize },
}

pub struct BluetoothDevice {
    pub event_kind: LeAdvEventKind,
    pub addr_kind: AddrKind,
    pub addr: BdAddr,
    pub rssi: i8,
}

const MAX_WEBSOCKET_VALUES: usize = 16;
const MAX_WEBSOCKET_PUBLISHERS: usize = 2;
pub const MAX_WEBSOCKET_SUBSCRIBERS: usize = 4;

pub type WebsocketChannel = PubSubChannel<CriticalSectionRawMutex, WebsocketWatchValue, MAX_WEBSOCKET_VALUES, MAX_WEBSOCKET_SUBSCRIBERS, MAX_WEBSOCKET_PUBLISHERS>;
pub type WebsocketPublisher<'a> = Publisher<'a, CriticalSectionRawMutex, WebsocketWatchValue, MAX_WEBSOCKET_VALUES, MAX_WEBSOCKET_SUBSCRIBERS, MAX_WEBSOCKET_PUBLISHERS>;
pub type WebsocketSubscriber<'a> = Subscriber<'a, CriticalSectionRawMutex, WebsocketWatchValue, MAX_WEBSOCKET_VALUES, MAX_WEBSOCKET_SUBSCRIBERS, MAX_WEBSOCKET_PUBLISHERS>;

pub struct App {
    websocket_channel: Arc<WebsocketChannel>,
    // https://docs.embassy.dev/embassy-sync/0.8.0/default/blocking_mutex/struct.Mutex.html#method.lock_mut
    // critical_mutex.lock_mut is not inherently safe which is why we use a RefCell as well
    bluetooth_devices: Arc<CriticalSectionMutex<RefCell<VecDeque<BluetoothDevice>>>>,
}

impl Default for App {
    fn default() -> Self {
        Self {
            websocket_channel: Arc::new(WebsocketChannel::new()),
            bluetooth_devices: Arc::new(CriticalSectionMutex::new(RefCell::new(VecDeque::with_capacity(128)))),
        }
    }
}

fn display_address(addr: &BdAddr) -> impl Display {
    addr.raw().iter().format_with("", |byte, f| f(&format_args!("{byte:02X}")))
}

impl App {
    pub fn read_bluetooth_reports(&self, reports: LeAdvReportsIter<'_>) {
        let total_added = self.bluetooth_devices.lock(move |devices| {
            let mut devices = match devices.try_borrow_mut() {
                Ok(devices) => devices,
                Err(err) => {
                    log::error!("Tried to access bluetooth devices twice in critical section mutex: {err}");
                    return 0;
                },
            };

            let mut push_report = move |report: LeAdvReport<'_>| {
                let report_address = report.addr;
                let is_new_address = devices.iter().find(|device| device.addr.raw() == report_address.raw()).is_none();
                if is_new_address {
                    log::info!("Adding new bluetooth device with address=0x{0} to existing total_devices={1}",
                        display_address(&report_address), devices.len());
                    let is_full = devices.len() == devices.capacity();
                    if is_full && let Some(old_device) = devices.pop_front() {
                        log::info!("Removing oldest seen bluetooth device with address=0x{0}", display_address(&old_device.addr));
                    }
                }
                devices.push_back(BluetoothDevice {
                    event_kind: report.event_kind,
                    addr_kind: report.addr_kind,
                    addr: report.addr,
                    rssi: report.rssi,
                });
            };

            let mut total_added = 0;
            for report in reports {
                match report {
                    Ok(report) => {
                        push_report(report);
                        total_added += 1;
                    },
                    Err(err) => {
                        log::error!("Got a mangled trouBLE report: {err:?}");
                    },
                }
            }
            total_added
        });

        if total_added > 0 {
            match self.websocket_channel.publisher() {
                Ok(publisher) => publisher.publish_immediate(WebsocketWatchValue::BluetoothDevicesUpdated { total_added }),
                Err(err) => log::error!("Couldn't acquire publisher to send bluetooth devices changed notification: {err:?}"),
            }
        }
    }

    pub fn get_websocket_subscriber(&self) -> Result<WebsocketSubscriber<'_>, PubSubError> {
        self.websocket_channel.subscriber()
    }
}
