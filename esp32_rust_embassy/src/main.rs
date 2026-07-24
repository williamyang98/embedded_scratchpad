#![no_std]
#![no_main]
#![deny(
    clippy::mem_forget,
    reason = "mem::forget is generally not safe to do with esp_hal types, especially those \
    holding buffers for the duration of a data transfer."
)]
#![deny(clippy::large_stack_frames)]

use bt_hci::controller::ExternalController;
use embassy_executor::Spawner;
use embassy_time::{Duration, Timer};
extern crate alloc;
use esp_alloc as _;
use esp_backtrace as _;
use esp_hal::{
    clock::CpuClock,
    timer::timg::TimerGroup,
    interrupt::software::SoftwareInterruptControl,
    gpio::{Output, Level, OutputConfig},
};
use esp_radio::ble::controller::BleConnector;
use log::info;
use esp32_d0wd_v3::ble_scanner::ble_scanner_run;

esp_bootloader_esp_idf::esp_app_desc!();

#[allow(
    clippy::large_stack_frames,
    reason = "it's not unusual to allocate larger buffers etc. in main"
)]
#[esp_rtos::main]
async fn main(spawner: Spawner) {
    esp_println::logger::init_logger_from_env();

    let config = esp_hal::Config::default().with_cpu_clock(CpuClock::max());
    let peripherals = esp_hal::init(config);

    // The following pins are used to bootstrap the chip. They are available
    // for use, but check the datasheet of the module for more information on them.
    // - GPIO0
    // - GPIO2
    // - GPIO5
    // - GPIO12
    // - GPIO15
    // These GPIO pins are in use by some feature of the module and should not be used.
    let _ = peripherals.GPIO6;
    let _ = peripherals.GPIO7;
    let _ = peripherals.GPIO8;
    let _ = peripherals.GPIO9;
    let _ = peripherals.GPIO10;
    let _ = peripherals.GPIO11;
    let _ = peripherals.GPIO16;
    let _ = peripherals.GPIO20;

    esp_alloc::heap_allocator!(#[esp_hal::ram(reclaimed)] size: 98768);
    // COEX needs more RAM - so we've added some more
    esp_alloc::heap_allocator!(size: 64 * 1024);

    let timer_group_0 = TimerGroup::new(peripherals.TIMG0);
    let software_interrupt_control = SoftwareInterruptControl::new(peripherals.SW_INTERRUPT);
    esp_rtos::start(timer_group_0.timer0, software_interrupt_control.software_interrupt0);
    info!("esp_rtos and embassy initialised");

    let (mut _wifi_controller, _interfaces) = esp_radio::wifi::new(peripherals.WIFI, Default::default())
        .expect("Failed to initialize Wi-Fi controller");
    let ble_connector = BleConnector::new(peripherals.BT, Default::default())
        .expect("Failed to initialize BLE connector");
    let led = Output::new(peripherals.GPIO2, Level::Low, OutputConfig::default());

    spawner.spawn(ble_scanner_task(ble_connector).unwrap());
    spawner.spawn(hello_world_task().unwrap());
    spawner.spawn(led_blink_task(led).unwrap());
}

#[embassy_executor::task]
async fn ble_scanner_task(ble_connector: BleConnector<'static>) {
    let ble_controller = ExternalController::<_, 1>::new(ble_connector);
    ble_scanner_run(ble_controller).await;
}

#[embassy_executor::task]
async fn hello_world_task() {
    let mut counter: u32 = 0;
    loop {
        info!("Hello world {counter}!");
        counter += 1;
        Timer::after(Duration::from_secs(1)).await;
    }
}

#[embassy_executor::task]
async fn led_blink_task(mut led: Output<'static>) {
    loop {
        led.set_high();
        Timer::after(Duration::from_millis(100)).await;
        led.set_low();
        Timer::after(Duration::from_millis(100)).await;
    }
}
