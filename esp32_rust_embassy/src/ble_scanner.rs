use core::cell::RefCell;
use bt_hci::{
    cmd::le::LeSetScanParams,
    controller::ControllerCmdSync,
    param::LeAdvReportsIter,
};
use embassy_futures::join::join;
use embassy_time::Duration;
use heapless::Deque;
use trouble_host::prelude::*;
use log::{info, error};

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

    let mut resources: HostResources<DefaultPacketPool, CONNECTIONS_MAX, L2CAP_CHANNELS_MAX> =
        HostResources::new();
    let stack = trouble_host::new(controller, &mut resources)
        .set_random_address(address);
    let host = stack.build();
    let mut runner = host.runner;
    let central = host.central;

    let printer = Printer {
        seen: RefCell::new(Deque::new()),
    };

    let run_scanner = async move || -> ! {
        let mut scanner = Scanner::new(central);
        let mut config = ScanConfig::default();
        config.active = true;
        config.phys = PhySet::M1;
        config.interval = Duration::from_secs(1);
        config.window = Duration::from_secs(1);
        let mut _session = scanner.scan(&config).await.unwrap();
        loop {
            core::future::pending::<()>().await;
            error!("bluetooth scanning loop somehow broke free from its indefinite scanning time");
        }
    };

    let _ = join(
        runner.run_with_handler(&printer),
        run_scanner(),
    ).await;

    loop {
        error!("bluetooth tasks somehow finished even though they were meant to be indefinite");
        core::future::pending::<()>().await;
    }
}

struct Printer {
    seen: RefCell<Deque<BdAddr, 128>>,
}

impl EventHandler for Printer {
    fn on_adv_reports(&self, mut it: LeAdvReportsIter<'_>) {
        let mut seen = self.seen.borrow_mut();
        while let Some(Ok(report)) = it.next() {
            if seen.iter().find(|b| b.raw() == report.addr.raw()).is_none() {
                info!("Discovered new bluetooth device with address={:?}", report.addr);
                if seen.is_full() {
                    seen.pop_front();
                }
                seen.push_back(report.addr).unwrap();
            }
        }
    }
}
