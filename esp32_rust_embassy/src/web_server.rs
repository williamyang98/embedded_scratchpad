use embassy_net::{Runner, Stack};
use embassy_time::{Duration, Timer};
use embassy_futures::join::join3;
use embassy_sync::{
    blocking_mutex::raw::CriticalSectionRawMutex,
    signal::Signal,
};
use esp_radio::wifi::{
    Interface, WifiController,
    sta::StationConfig,
    Config as WifiConfig,
};
use picoserve::{
    routing::{get, get_service},
    response::{ws, File, Redirect},
    io,
};
use itertools::Itertools;

extern crate alloc;
use alloc::{
    string::ToString,
    vec,
    sync::Arc,
};

pub struct WebServer {
    pub wifi_controller: WifiController<'static>,
    pub net_stack: Stack<'static>,
    pub net_runner: Runner<'static, Interface<'static>>,
}

impl WebServer {
    // NOTE: this uses a massive amount of stack space which can cause stack overflows
    //       specifically run_web_stack(...) which instantiates and runs picoserve::Server
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

#[derive(Debug, Clone, Copy)]
enum WebsocketControlSignal {
    ForceClose,
}

struct AppState {
    websocket_signal: Arc<Signal<CriticalSectionRawMutex, WebsocketControlSignal>>,
}

impl Default for AppState {
    fn default() -> Self {
        Self {
            websocket_signal: Arc::new(Signal::new()),
        }
    }
}

struct WebsocketHandler;

impl ws::WebSocketCallbackWithState<AppState> for WebsocketHandler {
    async fn run_with_state<R: io::Read, W: io::Write<Error = R::Error>>(
        self,
        app_state: &AppState,
        mut rx: ws::SocketRx<R>,
        mut tx: ws::SocketTx<W>,
    ) -> Result<(), W::Error> {
        use picoserve::{
            response::ws::Message,
            futures::Either,
        };

        let mut message_buffer = vec![0; 32];

        type WebsocketCloseReason<'a> = (u16, &'a str);
        let close_reason: Option<WebsocketCloseReason> = loop {
            let message = rx
                .next_message(&mut message_buffer, app_state.websocket_signal.wait())
                .await?;

            let message: Message = match message {
                Either::First(res) => match res {
                    Ok(message) => message,
                    Err(err) => {
                        log::warn!("Websocket reception error: {err:?}");
                        break Some((err.code(), "Websocket reception error"));
                    },
                },
                Either::Second(signal) => match signal {
                    WebsocketControlSignal::ForceClose => {
                        // https://websocket.org/reference/close-codes/
                        break Some((1000, "Websocket forcefully closed by host"));
                    },
                },
            };

            log::info!("Message: {message:?}");
            match message {
                Message::Text(new_message) => {
                    if let Err(err) = tx.send_text(new_message).await {
                        log::error!("Websocket transmission error: {err:?}");
                        return Err(err);
                    }
                },
                Message::Binary(message) => {
                    log::info!("Ignoring binary message: {message:?}")
                },
                Message::Close(reason) => {
                    log::info!("Websocket close reason: {reason:?}");
                    break Some((1000, "Websocket acknowledging close message"));
                },
                Message::Ping(ping) => {
                    if let Err(err) = tx.send_pong(ping).await {
                        log::error!("Websocket failed to reply to ping with pong: {err:?}");
                        return Err(err);
                    }
                },
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

    let app_state = AppState::default();
    let router = picoserve::Router::new()
        .route("/", get(async || Redirect::to("/index.html")))
        .route("/index.html", get_service(File::html(include_str!("../static/index.html"))))
        .route("/index.css", get_service(File::css(include_str!("../static/index.css"))))
        .route("/loader.js", get_service(File::javascript(include_str!("../static/loader.js"))))
        .route("/AppView.vue", get_service(File::html(include_str!("../static/AppView.vue"))))
        .route("/ws", get(async |upgrade: ws::WebSocketUpgrade| {
            log::info!("Got websocket connection requesting upgrade with protocols={0:?}", upgrade.protocols().map(|p| p.format(",")));
            // https://developer.mozilla.org/en-US/docs/Web/API/WebSocket/WebSocket#protocols
            // https://www.iana.org/assignments/websocket/websocket.xml#subprotocol-name
            // In javascript: ```let ws = new WebSocket("ws://<HOSTNAME>:<PORT>/ws", ["soap"])```
            // static WEBSOCKET_PROTOCOL: &str = "soap";
            upgrade
                .on_upgrade_using_state(WebsocketHandler)
                // Not specifying this means we accept all protocols that are compatible with RFC6455
                // .with_protocol(WEBSOCKET_PROTOCOL)
        }))
        .with_state(app_state);

    let mut http_buffer = vec![0; 2048];
    let config = picoserve::Config::const_default().keep_connection_alive();
    let server = picoserve::Server::new(&router, &config, &mut http_buffer);

    let port = 80;
    let mut tcp_rx_buffer = vec![0; 1024];
    let mut tcp_tx_buffer = vec![0; 1024];
    let task_id: usize = 0;
    server.listen_and_serve(task_id, net_stack, port, &mut tcp_rx_buffer, &mut tcp_tx_buffer)
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
