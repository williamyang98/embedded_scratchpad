use embassy_net as net;
use embassy_sync::pubsub::WaitResult;
use picoserve::{
    Config as ServerConfig, Router, Server, AppBuilder, AppRouter,
    routing::{get, get_service, PathRouter},
    response::{ws, File, Redirect, Json},
    io,
};
use itertools::Itertools;

extern crate alloc;
use alloc::{
    vec,
    sync::Arc,
    boxed::Box,
};
use crate::{
    app::{App, WebsocketWatchValue},
    heap_stats::HeapStats,
};

struct WebsocketHandler;

#[repr(u8)]
enum ResponseHeader {
    MissedMessages = 0x00,
    BluetoothUpdate = 0x01,
}

impl ws::WebSocketCallbackWithState<App> for WebsocketHandler {
    async fn run_with_state<R: io::Read, W: io::Write<Error = R::Error>>(
        self,
        app: &App,
        mut rx: ws::SocketRx<R>,
        mut tx: ws::SocketTx<W>,
    ) -> Result<(), W::Error> {
        use picoserve::{
            response::ws::Message,
            futures::Either,
        };

        // https://websocket.org/reference/close-codes/
        type WebsocketCloseReason<'a> = (u16, &'a str);

        let mut message_buffer = vec![0; 32];
        let mut websocket_subscriber = match app.get_websocket_subscriber() {
            Ok(subscriber) => subscriber,
            Err(err) => {
                log::warn!("Rejecting websocket connection because server is overloaded: {err:?}");
                let close_reason: WebsocketCloseReason = (1013, "Server is busy with other clients");
                return tx.close(Some(close_reason)).await;
            },
        };

        let close_reason: Option<WebsocketCloseReason> = loop {
            let message = rx
                .next_message(&mut message_buffer, websocket_subscriber.next_message())
                .await?;

            let message: Message = match message {
                Either::First(rx_result) => match rx_result {
                    Ok(message) => message,
                    Err(err) => {
                        log::warn!("Websocket reception error: {err:?}");
                        break Some((err.code(), "Websocket reception error"));
                    },
                },
                Either::Second(subscriber_result) => match subscriber_result {
                    WaitResult::Lagged(total_missed) => {
                        log::info!("Websocket missed {total_missed} messages from host");
                        tx.send_binary(&[ResponseHeader::MissedMessages as u8, total_missed as u8]).await?;
                        continue;
                    },
                    WaitResult::Message(signal) => match signal {
                        WebsocketWatchValue::ForceClose => {
                            break Some((1000, "Websocket forcefully closed by host"));
                        },
                        WebsocketWatchValue::BluetoothDevicesUpdated { total_added } => {
                            tx.send_binary(&[ResponseHeader::BluetoothUpdate as u8, total_added as u8]).await?;
                            continue;
                        },
                    },
                },
            };

            log::info!("Message: {message:?}");
            match message {
                Message::Text(new_message) => {
                    tx.send_text(new_message).await?;
                },
                Message::Binary(message) => {
                    log::info!("Ignoring binary message: {message:?}")
                },
                Message::Close(reason) => {
                    log::info!("Websocket close reason: {reason:?}");
                    break Some((1000, "Websocket acknowledging close message"));
                },
                Message::Ping(ping) => {
                    tx.send_pong(ping).await?;
                },
                Message::Pong(pong) => log::info!("Websocket pong: {pong:?}"),
            };
        };

        tx.close(close_reason).await?;
        Ok(())
    }
}

pub struct WebServer {
    pub app: Arc<App>,
}

impl AppBuilder for WebServer {
    type PathRouter = impl PathRouter;

    fn build_app(self) -> Router<Self::PathRouter> {
        Router::new()
            .route("/", get(async || Redirect::to("/index.html")))
            .route("/index.html", get_service(File::html(include_str!("../static/index.html"))))
            .route("/index.css", get_service(File::css(include_str!("../static/index.css"))))
            .route("/loader.js", get_service(File::javascript(include_str!("../static/loader.js"))))
            .route("/api.js", get_service(File::javascript(include_str!("../static/api.js"))))
            .route("/AppView.vue", get_service(File::html(include_str!("../static/AppView.vue"))))
            .route("/api/v1/ws", get(async |upgrade: ws::WebSocketUpgrade| {
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
            .route("/api/v1/bluetooth_devices", get({
                let app = self.app.clone();
                async move || {
                    let devices = app.get_bluetooth_devices().await;
                    Json(devices.clone())
                }
            }))
            .route("/api/v1/heap_stats", get({
                async || {
                    Json(HeapStats(&esp_alloc::HEAP))
                }
            }))
            .with_state(self.app)
    }
}

pub async fn run_server(id: &str, router: &AppRouter<WebServer>, config: &ServerConfig, net_stack: net::Stack<'_>) -> ! {
    let mut http_buffer = vec![0; 2048];
    let mut tcp_rx_buffer = vec![0; 1024];
    let mut tcp_tx_buffer = vec![0; 1024];
    let server = Box::new(Server::new(router, config, &mut http_buffer));
    let port = 80;
    server.listen_and_serve(id, net_stack, port, &mut tcp_rx_buffer, &mut tcp_tx_buffer)
        .await
        .into_never()
}

