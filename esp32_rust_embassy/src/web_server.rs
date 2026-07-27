use embassy_net as net;
use picoserve::{
    Config as ServerConfig, Router, Server, AppBuilder, AppRouter,
    routing::{get, get_service, PathRouter},
    response::{ws, File, Redirect, Json},
};
use itertools::Itertools;

extern crate alloc;
use alloc::{
    vec,
    sync::Arc,
    boxed::Box,
};
use crate::{
    app::App,
    heap_stats::HeapStats,
    web_socket::WebsocketHandler,
};

pub struct WebServer {
    pub app: Arc<App>,
}

impl AppBuilder for WebServer {
    type PathRouter = impl PathRouter;

    fn build_app(self) -> Router<Self::PathRouter> {
        Router::new()
            .route("/", get(async || Redirect::to("/index.html")))
            .route("/AppView.vue", get_service(File::html(include_str!("../static/AppView.vue"))))
            .route("/index.html", get_service(File::html(include_str!("../static/index.html"))))
            .route("/index.css", get_service(File::css(include_str!("../static/index.css"))))
            .route("/loader.js", get_service(File::javascript(include_str!("../static/loader.js"))))
            .route("/api.js", get_service(File::javascript(include_str!("../static/api.js"))))
            .route("/web_socket.js", get_service(File::javascript(include_str!("../static/web_socket.js"))))
            .route("/utility.js", get_service(File::javascript(include_str!("../static/utility.js"))))
            .route("/api/v1/websocket", get({
                let app = self.app.clone();
                async move |upgrade: ws::WebSocketUpgrade| {
                    log::info!("Got websocket connection requesting upgrade with protocols={0:?}", upgrade.protocols().map(|p| p.format(",")));
                    // https://developer.mozilla.org/en-US/docs/Web/API/WebSocket/WebSocket#protocols
                    // https://www.iana.org/assignments/websocket/websocket.xml#subprotocol-name
                    // In javascript: ```let ws = new WebSocket("ws://<HOSTNAME>:<PORT>/ws", ["soap"])```
                    // static WEBSOCKET_PROTOCOL: &str = "soap";
                    let handler = WebsocketHandler { app: app.clone() };
                    upgrade
                        .on_upgrade_using_state(handler)
                        // Not specifying this means we accept all protocols that are compatible with RFC6455
                        // .with_protocol(WEBSOCKET_PROTOCOL)
                }
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

