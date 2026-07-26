use bt_hci::{
    cmd::le::LeSetScanParams,
    controller::ControllerCmdSync,
    param::LeAdvReportsIter,
};
use embassy_futures::join::join3;
use embassy_time::{Duration, Timer};
use embassy_sync::{
    channel::Channel,
    mutex::Mutex,
    blocking_mutex::raw::NoopRawMutex,
};
use trouble_host::prelude::*;

extern crate alloc;
use alloc::{
    boxed::Box,
    rc::Rc,
    sync::Arc,
    vec::Vec,
};
use crate::{
    app::App,
    bluetooth_device::BluetoothDevice,
};

/// Max number of connections
const CONNECTIONS_MAX: usize = 1;
const L2CAP_CHANNELS_MAX: usize = 1;

type NotificationChannel = Channel<NoopRawMutex, usize, 32>;
type NewDevices = Mutex<NoopRawMutex, Vec<BluetoothDevice>>;

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

    let notification_channel = Rc::new(NotificationChannel::new());
    let new_devices = Rc::new(NewDevices::new(Vec::new()));
    let device_tracker = DeviceTracker {
        new_devices: new_devices.clone(),
        notification_channel: notification_channel.clone(),
    };

    let mut run_listener = async move || {
        log::info!("Starting running trouBLE listener");
        if let Err(err) = runner.run_with_handler(&device_tracker).await {
            log::error!("trouBLE listener ended with an error: {err:?}");
        } else {
            log::info!("trouBLE listener closed down gracefully");
        }
    };

    // the bluetooth device listener is a synchronous interrupt
    // have it run on the same core as the asynchronous pump loop
    // sync_interrupt -> push_reports -> notify_async_loop -> async_pump_runs -> ...
    // this way we can move reports to an asynchronous context without dropping any reports
    let run_report_pump = async move || -> ! {
        loop {
            let _total_reports_received = notification_channel.receive().await;
            let mut new_devices = new_devices.lock().await;
            app.extend_bluetooth_devices(new_devices.drain(..)).await;
        }
    };

    let _ = join3(
        run_scanner(),
        run_report_pump(),
        run_listener(),
    ).await;

    loop {
        core::future::pending::<()>().await;
    }
}

struct DeviceTracker {
    notification_channel: Rc<NotificationChannel>,
    new_devices: Rc<NewDevices>,
}

impl EventHandler for DeviceTracker {
    fn on_adv_reports(&self, reports: LeAdvReportsIter<'_>) {
        let mut new_devices = match self.new_devices.try_lock() {
            Ok(new_devices) => new_devices,
            Err(err) => {
                log::error!("Attempted to add devices while run_pump acquired it on same core: {err:?}");
                return;
            },
        };

        let mut total_added = 0;
        for report in reports {
            match report {
                Ok(report) => {
                    let new_device = BluetoothDevice {
                        event_kind: report.event_kind,
                        addr_kind: report.addr_kind,
                        addr: report.addr,
                        rssi: report.rssi,
                    };
                    new_devices.push(new_device);
                    total_added += 1;
                },
                Err(err) => {
                    log::error!("Got a mangled trouBLE report: {err:?}");
                },
            }
        }
        drop(new_devices);

        if total_added > 0 && let Err(err) = self.notification_channel.try_send(total_added) {
            log::error!("Failed to notify run_pump throgh channel: {err:?}");
        }
    }
}
