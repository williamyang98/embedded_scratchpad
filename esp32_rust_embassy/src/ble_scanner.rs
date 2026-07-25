use core::cell::RefCell;
use bt_hci::{
    cmd::le::LeSetScanParams,
    controller::ControllerCmdSync,
    param::LeAdvReportsIter,
};
use embassy_futures::join::join;
use embassy_time::Duration;
use trouble_host::prelude::*;
use log::{info, error};

extern crate alloc;
use alloc::{
    boxed::Box,
    collections::VecDeque,
};
use itertools::Itertools;
use core::fmt::Display;

/// Max number of connections
const CONNECTIONS_MAX: usize = 1;
const L2CAP_CHANNELS_MAX: usize = 1;

pub async fn ble_scanner_run<C>(controller: C) -> !
where
    C: Controller + ControllerCmdSync<LeSetScanParams>,
{
    // Using a fixed "random" address can be useful for testing. In real scenarios, one would
    // use e.g. the MAC 6 byte array as the address (how to get that varies by the platform).
    let address: Address = Address::random([0xff, 0x8f, 0x1b, 0x05, 0xe4, 0xff]);
    info!("Assigned our bluetooth address={:?}", address);

    let mut resources: Box<HostResources<DefaultPacketPool, CONNECTIONS_MAX, L2CAP_CHANNELS_MAX>> =
        Box::new(HostResources::new());
    let stack = Box::new(trouble_host::new(controller, &mut resources))
        .set_random_address(address);
    let host = stack.build();
    let mut runner = host.runner;
    let central = host.central;

    let run_scanner = async move || -> ! {
        let mut scanner = Box::new(Scanner::new(central));
        let config = ScanConfig {
            active: true,
            phys: PhySet::M1,
            interval: Duration::from_secs(1),
            window: Duration::from_secs(1),
            ..Default::default()
        };
        let mut _session = scanner.scan(&config).await.unwrap();
        loop {
            core::future::pending::<()>().await;
            error!("bluetooth scanning loop somehow broke free from its indefinite scanning time");
        }
    };

    let device_tracker = DeviceTracker::new(128);
    let _ = join(
        runner.run_with_handler(&device_tracker),
        run_scanner(),
    ).await;

    loop {
        error!("bluetooth tasks somehow finished even though they were meant to be indefinite");
        core::future::pending::<()>().await;
    }
}

struct DeviceTracker {
    addresses: RefCell<VecDeque<BdAddr>>,
}

impl DeviceTracker {
    pub fn new(capacity: usize) -> Self {
        Self {
            addresses: RefCell::new(VecDeque::with_capacity(capacity))
        }
    }
}

fn display_address(addr: &BdAddr) -> impl Display {
    addr.raw().iter().format_with("", |byte, f| f(&format_args!("{byte:02X}")))
}

impl EventHandler for DeviceTracker {
    fn on_adv_reports(&self, mut it: LeAdvReportsIter<'_>) {
        let mut addresses = match self.addresses.try_borrow_mut() {
            Ok(addresses) => addresses,
            Err(err) => {
                error!("Received BLE device reports while in the middle of processing previous reports: {err:?}");
                return;
            },
        };
        while let Some(Ok(report)) = it.next() {
            let report_address = report.addr;
            let is_new_address = addresses.iter().find(|address| address.raw() == report_address.raw()).is_none();
            if is_new_address {
                info!("Discovered new bluetooth device with address=0x{0}", display_address(&report_address));
                let is_full = addresses.len() == addresses.capacity();
                if is_full && let Some(old_address) = addresses.pop_front() {
                    info!("Removing oldest seen bluetooth device with address=0x{0}", display_address(&old_address));
                }
            }
            addresses.push_back(report.addr);
        }
    }
}
