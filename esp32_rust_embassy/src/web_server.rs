use embassy_net::{Runner, Stack};
use embassy_time::{Duration, Timer};
use embassy_futures::join::{join, join3};
use embassy_sync::{
    blocking_mutex::raw::NoopRawMutex,
    channel::Channel,
};
use esp_radio::wifi::{
    Interface, WifiController,
    sta::StationConfig,
    Config as WifiConfig,
};
use picoserve::{
    routing::get,
    response::ws,
    io,
};
use itertools::Itertools;

extern crate alloc;
use alloc::{
    string::{String, ToString},
    vec,
    boxed::Box,
};

pub struct WebServer {
    pub wifi_controller: WifiController<'static>,
    pub net_stack: Stack<'static>,
    pub net_runner: Runner<'static, Interface<'static>>,
}

impl WebServer {
    pub async fn run(mut self) -> ! {
        let _ = join3(
            self.net_runner.run(),
            run_web_stack(self.net_stack),
            run_wifi_station(self.wifi_controller),
        ).await;

        loop {
            log::error!("indefinitely running web server tasks somehow all terminated");
            core::future::pending::<()>().await;
        }
    }
}

// https://developer.mozilla.org/en-US/docs/Web/API/WebSocket/WebSocket#protocols
// https://www.iana.org/assignments/websocket/websocket.xml#subprotocol-name
// In javascript: ```let ws = new WebSocket("ws://<HOSTNAME>:<PORT>/ws", ["soap"])```
static WEBSOCKET_PROTOCOL: &str = "soap";

struct WebsocketHandler;

impl ws::WebSocketCallback for WebsocketHandler {
    async fn run<R: io::Read, W: io::Write<Error = R::Error>>(
        self,
        mut rx: ws::SocketRx<R>,
        mut tx: ws::SocketTx<W>,
    ) -> Result<(), W::Error> {
        use picoserve::{
            response::ws::Message,
            futures::Either,
        };

        let messages_channel = Box::new(Channel::<NoopRawMutex, String, 3>::new());
        let mut message_buffer = vec![0; 128];

        type ExitCode<'a> = (u16, &'a str);
        let close_reason: Option<ExitCode> = loop {
            let message = rx
                .next_message(&mut message_buffer, messages_channel.receive())
                .await?;

            let message: Message = match message {
                Either::First(res) => match res {
                    Ok(message) => message,
                    Err(err) => {
                        log::warn!("Websocket reception error: {err:?}");
                        break Some((err.code(), "Websocket reception error"));
                    },
                },
                Either::Second(res) => {
                    log::info!("Websocket async channel result: {res:?}");
                    continue;
                },
            };

            log::info!("Message: {message:?}");
            match message {
                Message::Text(new_message) => {
                    let (_, tx_res) = join(
                        messages_channel.send(new_message.into()),
                        tx.send_text(new_message),
                    ).await;
                    if let Err(err) = tx_res {
                        log::error!("Websocket transmission error: {err:?}");
                        break None;
                    }
                },
                Message::Binary(message) => {
                    log::info!("Ignoring binary message: {message:?}")
                },
                Message::Close(reason) => {
                    log::info!("Websocket close reason: {reason:?}");
                    break None;
                },
                Message::Ping(ping) => tx.send_pong(ping).await?,
                Message::Pong(pong) => log::info!("Websocket pong: {pong:?}"),
            };
        };

        tx.close(close_reason).await?;
        Ok(())
    }
}

async fn run_web_stack(net_stack: Stack<'static>) -> ! {
    log::info!("Waiting for network stack to connection to station...");
    net_stack.wait_config_up().await;
    let config = net_stack.config_v4();
    log::info!("Network stack established on {config:?}");

    let router = Box::new(picoserve::Router::new())
        .route("/", get(async || { "Hello World!" }))
        .route("/ws", get(async |upgrade: ws::WebSocketUpgrade| {
            log::info!("Got websocket connection requesting protocols: {0:?}", upgrade.protocols().map(|p| p.format(",")));
            upgrade
                .on_upgrade(WebsocketHandler)
                .with_protocol(WEBSOCKET_PROTOCOL)
        }));

    let port = 80;
    let mut tcp_rx_buffer = vec![0; 1024];
    let mut tcp_tx_buffer = vec![0; 1024];
    let mut http_buffer = vec![0; 2048];

    let config = picoserve::Config::const_default().keep_connection_alive();
    let task_id: usize = 0;

    Box::new(picoserve::Server::new(&router, &config, &mut http_buffer))
        .listen_and_serve(task_id, net_stack, port, &mut tcp_rx_buffer, &mut tcp_tx_buffer)
        .await
        .into_never()
}

async fn run_wifi_station(mut wifi_controller: WifiController<'static>) -> ! {
    use crate::wifi_credentials::{SSID, PASSWORD};
    let station_config = StationConfig::default()
        .with_ssid(SSID)
        .with_password(PASSWORD.to_string());

    wifi_controller.set_config(&WifiConfig::Station(station_config))
        .expect("Failed to set wifi controller station configuration");

    log::info!("Attempting to connect to wifi station with ssid={SSID}");

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
