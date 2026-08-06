#![no_std]
#![no_main]
#![deny(
    clippy::mem_forget,
    reason = "mem::forget is generally not safe to do with esp_hal types, especially those \
    holding buffers for the duration of a data transfer."
)]
#![deny(clippy::large_stack_frames)]
// NOTE: We get a query depth limit reached when the compiler is attempting to compute the size of
// the embassy async task web_server_task because of picoserve's heavy use of traits which requires
// the compiler to parse the AST extremely deep
#![recursion_limit = "256"]

use esp_alloc as _;
use esp_backtrace as _;
use esp_hal::{
    Async,
    clock::CpuClock,
    time::{Duration as EspDuration, Instant},
    timer::timg::{TimerGroup, MwdtStage, Wdt},
    interrupt::software::SoftwareInterruptControl,
    system::Stack,
    rng::Rng,
    gpio::{Output, Level, OutputConfig},
    peripherals::{TIMG0, TIMG1},
    spi::master::{Spi, Config as SpiMasterConfig},
};
// rtos
use esp_rtos::embassy::Executor;
use embassy_executor::Spawner;
use embassy_time::{Duration, Timer};
use embassy_sync::{
    channel::Channel,
    blocking_mutex::raw::CriticalSectionRawMutex,
};
// wifi and bluetooth radio
use bt_hci::controller::ExternalController;
use esp_radio::{
    wifi,
    wifi::{
        Interface, WifiController,
        sta::StationConfig,
        Config as WifiConfig,
    },
    ble::controller::BleConnector,
};
// web server
use embassy_net as net;
use picoserve::{AppRouter, AppBuilder, Config as ServerConfig};
use esp32_d0wd_v3::{
    ble_scanner::ble_scanner_run,
    app::{App, WebsocketWatchValue, MAX_WEBSOCKET_SUBSCRIBERS},
    led_controller::LedController,
    web_server::{WebServer, run_server},
    secrets::{WIFI_SSID, WIFI_PASSWORD},
};

extern crate alloc;
use alloc::{
    boxed::Box,
    sync::Arc,
    string::{String, ToString},
    format,
};
use static_cell::StaticCell;

esp_bootloader_esp_idf::esp_app_desc!();

type MessageChannel = Channel<CriticalSectionRawMutex, u32, 64>;
const CORE_1_STACK_SIZE: usize = 8192;
static CORE_1_STACK: StaticCell<Box<Stack<CORE_1_STACK_SIZE>>> = StaticCell::new();
static CORE_1_EXECUTOR: StaticCell<Executor> = StaticCell::new();
static CHANNEL_MESSAGE: StaticCell<MessageChannel> = StaticCell::new();

// spare server connection to handle only http requests along side multiple websocket connections
static MAX_SERVER_CONNECTIONS: usize = MAX_WEBSOCKET_SUBSCRIBERS+1;
// network socket for net runner required
static MAX_NETWORK_SOCKETS: usize = MAX_SERVER_CONNECTIONS+1; 
static NET_STACK_RESOURCES: StaticCell<net::StackResources<MAX_NETWORK_SOCKETS>> = StaticCell::new();
static WEB_SERVER_ROUTER: StaticCell<AppRouter<WebServer>> = StaticCell::new();
static WEB_SERVER_CONFIG: ServerConfig = ServerConfig::const_default().keep_connection_alive();

#[allow(
    clippy::large_stack_frames,
    reason = "it's not unusual to allocate larger buffers etc. in main"
)]
#[esp_rtos::main]
async fn main_core_0(spawner: Spawner) -> ! {
    esp_println::logger::init_logger_from_env();

    let clock_speed = CpuClock::max();
    // With the radio on for bluetooth and wifi the default board has insufficient power decoupling
    // Run at a lower speed to prevent bad reads from SPI flash (external) and random crashes
    // let clock_speed = CpuClock::_160MHz;

    let config = esp_hal::Config::default().with_cpu_clock(clock_speed);
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
    // esp_alloc::heap_allocator!(size: 64 * 1024);

    let timer_group_0 = TimerGroup::new(peripherals.TIMG0);
    let timer_group_1 = TimerGroup::new(peripherals.TIMG1);
    let software_interrupt_control = SoftwareInterruptControl::new(peripherals.SW_INTERRUPT);

    esp_rtos::start(timer_group_0.timer0, software_interrupt_control.software_interrupt0);
    log::info!("esp_rtos started first executor on core 0");

    // https://docs.espressif.com/projects/rust/esp-radio/0.18.0/esp32/esp_radio/index.html#running-on-the-second-core
    // esp_radio::init() must be called on the first core
    log::info!("Start initialising esp radio");
    let (wifi_controller, interfaces) = wifi::new(peripherals.WIFI, Default::default())
        .expect("Failed to initialize Wi-Fi controller");
    let ble_connector = BleConnector::new(peripherals.BT, Default::default())
        .expect("Failed to initialize BLE connector");

    let net_config = net::Config::dhcpv4(Default::default());
    let net_stack_resources = NET_STACK_RESOURCES.init(net::StackResources::new());
    let net_seed: u64 = {
        let rng = Rng::new();
        let seed: u64 = (rng.random() as u64) << 32 | (rng.random() as u64);
        seed
    };
    let (net_stack, net_runner) = net::new(
        interfaces.station,
        net_config,
        net_stack_resources,
        net_seed,
    );
    log::info!("Finished initialising esp radio");

    let mut led = Output::new(peripherals.GPIO2, Level::Low, OutputConfig::default());
    led.set_low();
    // led.set_high();
    let led_controller = LedController::new(peripherals.LEDC, led);
    let app = Arc::new(App::new(led_controller));

    let spi = Spi::new(peripherals.SPI2, SpiMasterConfig::default())
        .expect("Failed to initialise SPI2")
        .with_sck(peripherals.GPIO18)
        .with_miso(peripherals.GPIO19)
        .with_mosi(peripherals.GPIO23)
        .with_cs(peripherals.GPIO5)
        .into_async();

    let messages_channel = &*CHANNEL_MESSAGE.init(Channel::new());
    let core_1_stack = CORE_1_STACK.init(Box::new(Stack::new()));
    esp_rtos::start_second_core(
        peripherals.CPU_CTRL,
        software_interrupt_control.software_interrupt1,
        core_1_stack,
        {
            let app = app.clone();
            let watchdog_timer = timer_group_1.wdt;
            move || {
                let core_1_executor = CORE_1_EXECUTOR.init(Executor::new());
                core_1_executor.run(move |spawner| {
                    spawner.spawn(receive_messages_task(app, messages_channel).unwrap());
                    spawner.spawn(print_heap_stats().unwrap());
                    spawner.spawn(watchdog_timer_core_1_task(watchdog_timer).unwrap());
                    log::info!("core 1 running all tasks");
                });
            }
        },
    );
    log::info!("esp_rtos started second executor on core 1");

    // FIXME: Trying to start a trouBLE scan session after attaching listener hangs on core 1 which doesn't make sense
    spawner.spawn(ble_scanner_task(ble_connector, app.clone()).unwrap());
    spawner.spawn(send_messages_task(messages_channel).unwrap());
    spawner.spawn(run_network_stack_task(net_runner).unwrap());
    spawner.spawn(run_wifi_station_task(wifi_controller).unwrap());
    spawner.spawn(run_led_spi_write_task(spi).unwrap());
    spawner.spawn(watchdog_timer_core_0_task(timer_group_0.wdt).unwrap());
    log::info!("core 0 running all tasks");

    log::info!("Waiting for network stack to connection to station...");
    net_stack.wait_config_up().await;
    let config = net_stack.config_v4();
    log::info!("Network stack established on {config:?}");

    // web server allocates a large stack for picoserve::Server so do this inside main where we permit it
    let web_server = WebServer { app };
    let web_server_router = WEB_SERVER_ROUTER.init(web_server.build_app());
    let web_server_config = &WEB_SERVER_CONFIG;
    log::info!("Spawning server instances to handle {MAX_SERVER_CONNECTIONS} connections");
    for server_id in 0..MAX_SERVER_CONNECTIONS {
        let server_id = format!("Server connection {server_id}");
        spawner.spawn(run_server_task(server_id, web_server_router, web_server_config, net_stack).unwrap());
    }

    log::info!("core 0 main now idling after spawning all tasks");
    loop {
        core::future::pending::<()>().await;
    }
}

static WATCHDOG_TIMER_TIMEOUT: EspDuration = EspDuration::from_secs(5);
static WATCHDOG_TIMER_FEED_PERIOD: Duration = Duration::from_secs(1);

#[embassy_executor::task]
async fn watchdog_timer_core_0_task(mut watchdog_timer: Wdt<TIMG0<'static>>) -> ! {
    watchdog_timer.set_timeout(MwdtStage::Stage0, WATCHDOG_TIMER_TIMEOUT);
    log::info!("Started watchdog timer on core 0");
    watchdog_timer.enable();
    loop {
        watchdog_timer.feed();
        Timer::after(WATCHDOG_TIMER_FEED_PERIOD).await;
    }
}

#[embassy_executor::task]
async fn watchdog_timer_core_1_task(mut watchdog_timer: Wdt<TIMG1<'static>>) -> ! {
    watchdog_timer.set_timeout(MwdtStage::Stage0, WATCHDOG_TIMER_TIMEOUT);
    log::info!("Started watchdog timer on core 1");
    watchdog_timer.enable();
    loop {
        watchdog_timer.feed();
        Timer::after(WATCHDOG_TIMER_FEED_PERIOD).await;
    }
}

#[embassy_executor::task]
async fn ble_scanner_task(ble_connector: BleConnector<'static>, app: Arc<App>) -> ! {
    let ble_controller = ExternalController::<_, 1>::new(ble_connector);
    ble_scanner_run(ble_controller, app).await;
}

#[embassy_executor::task]
async fn receive_messages_task(app: Arc<App>, messages_channel: &'static MessageChannel) -> ! {
    let mut counter: u32 = 0;
    loop {
        let message = messages_channel.receive().await;
        log::info!("Hello world counter={counter}, message={message}!");
        match app.get_websocket_publisher() {
            Ok(publisher) => publisher.publish_immediate(WebsocketWatchValue::BackgroundTaskMessage { message }),
            Err(err) => log::error!("Couldn't acquire publisher to send background message: {err:?}"),
        };
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

struct Dhms(pub EspDuration);

impl core::fmt::Display for Dhms {
    fn fmt(&self, f: &mut core::fmt::Formatter<'_>) -> core::fmt::Result {
        let total_seconds = self.0.as_secs();
        let seconds = total_seconds % 60;
        let minutes = (total_seconds / 60) % 60;
        let hours = (total_seconds / 3600) % 24;
        let days = total_seconds / 86400;
        let mut has_digit = false;
        if days > 0 {
            has_digit = true;
            write!(f, "{days}d")?;
        }
        if hours > 0 || has_digit {
            has_digit = true;
            write!(f, "{hours}h")?;
        }
        if minutes > 0 || has_digit {
            write!(f, "{minutes}m")?;
        }
        write!(f, "{seconds}s")
    }
}

#[embassy_executor::task]
async fn print_heap_stats() -> ! {
    const POLL_PERIOD: Duration = Duration::from_secs(60);
    loop {
        let stats = esp_alloc::HEAP.stats();
        log::info!("Heap stats\n{stats}");
        let uptime = Instant::now();
        log::info!("Uptime is {}", Dhms(uptime.duration_since_epoch()));
        Timer::after(POLL_PERIOD).await;
    }
}

#[embassy_executor::task]
async fn run_network_stack_task(mut net_runner: net::Runner<'static, Interface<'static>>) -> ! {
    net_runner.run().await
}

#[embassy_executor::task]
async fn run_wifi_station_task(mut wifi_controller: WifiController<'static>) -> ! {
    let station_config = StationConfig::default()
        .with_ssid(WIFI_SSID)
        .with_password(WIFI_PASSWORD.to_string());

    wifi_controller.set_config(&WifiConfig::Station(station_config))
        .expect("Failed to set wifi controller station configuration");

    log::info!("Attempting to connect to wifi station with ssid={WIFI_SSID}");

    const RETRY_DURATION: Duration = Duration::from_secs(10);
    loop {
        match wifi_controller.connect_async().await {
            Err(err) => {
                log::error!("Wifi failed to connect: {err:?}");
                Timer::after(RETRY_DURATION).await;
                continue;
            },
            Ok(res) => log::info!("Wifi connected successfully: {res:?}"),
        }
        match wifi_controller.wait_for_disconnect_async().await {
            Ok(res) => log::info!("Wifi disconnected gracefully: {res:?}"),
            Err(err) => log::error!("Wifi disconnected with an error: {err:?}"),
        }
        log::info!("Attempting to reconnect to wifi station after disconnecting...");
    }
}

#[allow(
    clippy::large_stack_frames,
    reason = "The picoserver instance is quite large and uses heapless to do stack allocations"
)]
#[embassy_executor::task(pool_size=MAX_SERVER_CONNECTIONS)]
pub async fn run_server_task(id: String, router: &'static AppRouter<WebServer>, config: &'static ServerConfig, net_stack: net::Stack<'static>) -> ! {
    run_server(&id, router, config, net_stack).await
}

#[embassy_executor::task]
pub async fn run_led_spi_write_task(mut spi: Spi<'static, Async>) -> ! {
    const BLINK_PERIOD: Duration = Duration::from_millis(100);
    let mut counter: u8 = 0;
    loop {
        let mut data: [u8; 1] = [counter];
        if let Err(err) = spi.transfer(&mut data) {
            log::error!("Failed to perform spi transfer: {err:?}");
        };
        counter = counter.wrapping_add(1);
        Timer::after(BLINK_PERIOD).await;
    }
}
