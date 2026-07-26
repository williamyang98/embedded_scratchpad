use bt_hci::{
    cmd::le::LeSetScanParams,
    controller::ControllerCmdSync,
    param::LeAdvReportsIter,
};
use embassy_futures::join::join;
use embassy_time::{Duration, Timer};
use trouble_host::prelude::*;

extern crate alloc;
use alloc::{
    boxed::Box,
    sync::Arc,
};
use crate::app::App;

/// Max number of connections
const CONNECTIONS_MAX: usize = 1;
const L2CAP_CHANNELS_MAX: usize = 1;

pub async fn ble_scanner_run<C>(controller: C, app: Arc<App>) -> !
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

    let run_listener = async move || {
        let device_tracker = DeviceTracker::new(app);
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
    app: Arc<App>,
}

impl DeviceTracker {
    pub fn new(app: Arc<App>) -> Self {
        Self {
            app,
        }
    }
}

impl EventHandler for DeviceTracker {
    fn on_adv_reports(&self, reports: LeAdvReportsIter<'_>) {
        self.app.read_bluetooth_reports(reports);
    }
}
