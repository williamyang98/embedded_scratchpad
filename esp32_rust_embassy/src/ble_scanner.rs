use core::cell::RefCell;
use bt_hci::{
    cmd::le::LeSetScanParams,
    controller::ControllerCmdSync,
    param::{LeAdvReport, LeAdvReportsIter},
};
use embassy_futures::join::join;
use embassy_time::{Duration, Timer};
use trouble_host::prelude::*;

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
    log::info!("Assigned our bluetooth address={:?}", address);

    type TroubleResources = HostResources<DefaultPacketPool, CONNECTIONS_MAX, L2CAP_CHANNELS_MAX>;
    let mut resources: Box<TroubleResources> = Box::new(TroubleResources::new());
    log::info!("trouBLE host resources takes up {0} bytes", core::mem::size_of::<TroubleResources>());

    let stack = Box::new(trouble_host::new(controller, &mut resources))
        .set_random_address(address);
    let host = stack.build();
    let mut runner = host.runner;
    let central = host.central;

    let run_scanner = async move || {
        let mut scanner = Scanner::new(central);
        let config = ScanConfig {
            active: true,
            phys: PhySet::M1,
            interval: Duration::from_secs(1),
            window: Duration::from_secs(1),
            ..Default::default()
        };

        log::info!("Attempting to start a trouBLE scan session");
        let mut total_fails: u32 = 0;
        let _session = loop {
            match scanner.scan(&config).await {
                Ok(session) => {
                    // FIXME: For some reason no session is created when running on core 1
                    log::info!("Started an active trouBLE scan session");
                    break session;
                },
                Err(err) => {
                    total_fails += 1;
                    log::error!("trouBLE scan session failed to start with err={err:?}, total_fails={total_fails}. Retrying...");
                    Timer::after(Duration::from_secs(5)).await;
                },
            }
        };

        // keep session active for listener
        loop {
            core::future::pending::<()>().await;
        }
    };

    let mut run_listener = async move || {
        let device_tracker = DeviceTracker::new(128);
        log::info!("Starting running trouBLE listener");
        if let Err(err) = runner.run_with_handler(&device_tracker).await {
            log::error!("trouBLE listener ended with an error: {err:?}");
        } else {
            log::info!("trouBLE listener closed down gracefully");
        }
    };

    let _ = join(
        run_scanner(),
        run_listener(),
    ).await;

    loop {
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
    fn on_adv_reports(&self, reports: LeAdvReportsIter<'_>) {
        let mut addresses = match self.addresses.try_borrow_mut() {
            Ok(addresses) => addresses,
            Err(err) => {
                log::error!("Received BLE device reports while in the middle of processing previous reports: {err:?}");
                return;
            },
        };

        let mut push_report = move |report: LeAdvReport<'_>| {
            let report_address = report.addr;
            let is_new_address = addresses.iter().find(|address| address.raw() == report_address.raw()).is_none();
            if is_new_address {
                log::info!("Adding new bluetooth device with address=0x{0} to existing total_devices={1}",
                    display_address(&report_address), addresses.len());
                let is_full = addresses.len() == addresses.capacity();
                if is_full && let Some(old_address) = addresses.pop_front() {
                    log::info!("Removing oldest seen bluetooth device with address=0x{0}", display_address(&old_address));
                }
            }
            addresses.push_back(report.addr);
        };

        for report in reports {
            match report {
                Ok(report) => push_report(report),
                Err(err) => {
                    log::error!("Got a mangled trouBLE report: {err:?}");
                },
            }
        }
    }
}
