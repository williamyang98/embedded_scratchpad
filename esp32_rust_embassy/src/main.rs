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
use embassy_sync::{
    channel::Channel,
    blocking_mutex::raw::CriticalSectionRawMutex,
};
use embassy_net as net;
use esp_alloc as _;
use esp_backtrace as _;
use esp_hal::{
    clock::CpuClock,
    timer::timg::TimerGroup,
    interrupt::software::SoftwareInterruptControl,
    system::Stack,
    rng::Rng,
    time::Rate,
    gpio::{Output, Level, OutputConfig, DriveMode},
    ledc::{
        Ledc, LSGlobalClkSource, LowSpeed,
        timer as ledc_timer,
        timer::TimerIFace,
        channel as ledc_channel,
        channel::ChannelIFace,
    },
};
use esp_rtos::embassy::Executor;
use esp_radio::ble::controller::BleConnector;
use static_cell::StaticCell;
use log::{info, error};
use esp32_d0wd_v3::{
    ble_scanner::ble_scanner_run,
    web_server::{run_web_server, WebServer},
};

extern crate alloc;

esp_bootloader_esp_idf::esp_app_desc!();

type MessageChannel = Channel<CriticalSectionRawMutex, u32, 64>;

static CORE_1_STACK: StaticCell<Stack<8192>> = StaticCell::new();
static CORE_1_EXECUTOR: StaticCell<Executor> = StaticCell::new();
static CHANNEL_MESSAGE: StaticCell<MessageChannel> = StaticCell::new();
static LED_LOW_SPEED_TIMER_0: StaticCell<ledc_timer::Timer<'static, LowSpeed>> = StaticCell::new();
static NET_STACK_RESOURCES: StaticCell<net::StackResources<3>> = StaticCell::new();

#[allow(
    clippy::large_stack_frames,
    reason = "it's not unusual to allocate larger buffers etc. in main"
)]
#[esp_rtos::main]
async fn main_core_0(spawner: Spawner) -> ! {
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
    info!("esp_rtos started first executor on core 0");

    let (wifi_controller, interfaces) = esp_radio::wifi::new(peripherals.WIFI, Default::default())
        .expect("Failed to initialize Wi-Fi controller");
    let ble_connector = BleConnector::new(peripherals.BT, Default::default())
        .expect("Failed to initialize BLE connector");

    let net_config = net::Config::dhcpv4(Default::default());
    let net_stack_resources = NET_STACK_RESOURCES.init(net::StackResources::new());
    let net_seed: u64 = {
        let rng = esp_hal::rng::Rng::new();
        let seed: u64 = (rng.random() as u64) << 32 | (rng.random() as u64);
        seed
    };
    let (net_stack, net_runner) = net::new(
        interfaces.station,
        net_config,
        net_stack_resources,
        net_seed,
    );
    let web_server = WebServer {
        wifi_controller,
        net_stack,
        net_runner,
    };

    // pwm controller for leds that handles fading the pwm duty cycle
    let mut led = Output::new(peripherals.GPIO2, Level::Low, OutputConfig::default());
    led.set_low();
    // led.set_high();

    // led_controller -> low_speed_timer -> low_speed_channel -> led_output
    let mut led_controller = Ledc::new(peripherals.LEDC);
    led_controller.set_global_slow_clock(LSGlobalClkSource::APBClk);
    let led_low_speed_timer_0 = LED_LOW_SPEED_TIMER_0.init(led_controller.timer::<LowSpeed>(ledc_timer::Number::Timer0));
    led_low_speed_timer_0.configure(ledc_timer::config::Config {
        duty: ledc_timer::config::Duty::Duty5Bit,
        clock_source: ledc_timer::LSClockSource::APBClk,
        frequency: Rate::from_khz(24),
    }).expect("Failed to configure led controller low speed timer");

    let mut led_channel_0 = led_controller.channel::<LowSpeed>(ledc_channel::Number::Channel0, led);
    led_channel_0.configure(ledc_channel::config::Config {
        timer: &*led_low_speed_timer_0,
        duty_pct: 0,
        drive_mode: DriveMode::PushPull,
    }).expect("Failed to configure led controller low speed channel");

    info!("Configured all peripherals");

    let messages_channel = &*CHANNEL_MESSAGE.init(Channel::new());

    let core_1_stack = CORE_1_STACK.init(Stack::new());
    esp_rtos::start_second_core_with_stack_guard_offset(
        peripherals.CPU_CTRL,
        software_interrupt_control.software_interrupt1,
        core_1_stack,
        None,
        move || {
            let core_1_executor = CORE_1_EXECUTOR.init(Executor::new());
            core_1_executor.run(|spawner| {
                spawner.spawn(hello_world_task(messages_channel).unwrap());
                spawner.spawn(send_messages_task(messages_channel).unwrap());
                info!("core 1 running all tasks");
            });
        },
    );
    info!("esp_rtos started second executor on core 1");

    // https://docs.espressif.com/projects/rust/esp-radio/0.18.0/esp32/esp_radio/index.html#running-on-the-second-core
    // esp_radio::init() needs special core considerations
    // So we have wifi and bluetooth on core 0 so all the interrupts actually work
    spawner.spawn(web_server_task(web_server).unwrap());
    spawner.spawn(ble_scanner_task(ble_connector).unwrap());
    spawner.spawn(led_blink_task(led_channel_0).unwrap());
    info!("core 0 running all tasks");

    loop {
        core::future::pending::<()>().await;
        error!("core 0 main somehow woke up from being indefinitely idle");
    }
}

#[embassy_executor::task]
async fn web_server_task(web_server: WebServer) -> ! {
    run_web_server(web_server).await;
}

#[embassy_executor::task]
async fn ble_scanner_task(ble_connector: BleConnector<'static>) -> ! {
    let ble_controller = ExternalController::<_, 1>::new(ble_connector);
    ble_scanner_run(ble_controller).await;
}

#[embassy_executor::task]
async fn hello_world_task(messages_channel: &'static MessageChannel) -> ! {
    let mut counter: u32 = 0;
    loop {
        let message = messages_channel.receive().await;
        info!("Hello world counter={counter}, message={message}!");
        counter += 1;
    }
}

#[embassy_executor::task]
async fn send_messages_task(messages_channel: &'static MessageChannel) -> ! {
    let rng = Rng::new();
    loop {
        let message: u32 = rng.random();
        messages_channel.send(message).await;
        Timer::after(Duration::from_secs(1)).await;
    }
}

#[embassy_executor::task]
async fn led_blink_task(led_channel_0: ledc_channel::Channel<'static, LowSpeed>) -> ! {
    const WAIT_DELAY: Duration = Duration::from_millis(10);
    const MIN_DUTY_CYCLE: u8 = 0;
    const MAX_DUTY_CYCLE: u8 = 50;
    loop {
        for i in MIN_DUTY_CYCLE..=MAX_DUTY_CYCLE {
            led_channel_0.set_duty(i).unwrap();
            Timer::after(WAIT_DELAY).await;
        }
        for i in (MIN_DUTY_CYCLE..=MAX_DUTY_CYCLE).rev() {
            led_channel_0.set_duty(i).unwrap();
            Timer::after(WAIT_DELAY).await;
        }
    }
}
