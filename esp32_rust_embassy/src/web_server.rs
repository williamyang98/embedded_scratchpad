use embassy_net::{
    Runner, Stack,
    tcp::TcpSocket,
    IpListenEndpoint,
};
use embassy_time::{Duration, Timer};
use embassy_futures::join::join3;
use embedded_io_async::Write;
use esp_radio::wifi::{
    Interface, WifiController,
    sta::StationConfig,
    Config as WifiConfig,
};
use log::{info, error};

extern crate alloc;
use alloc::{
    string::ToString,
    vec::Vec,
};

pub struct WebServer {
    pub wifi_controller: WifiController<'static>,
    pub net_stack: Stack<'static>,
    pub net_runner: Runner<'static, Interface<'static>>,
}

pub async fn run_web_server(mut server: WebServer) -> ! {
    let _ = join3(
        server.net_runner.run(),
        run_web_stack(server.net_stack),
        run_wifi_station(server.wifi_controller),
    ).await;

    loop {
        error!("indefinitely running web server tasks somehow all terminated");
        core::future::pending::<()>().await;
    }
}

async fn run_web_stack(net_stack: Stack<'static>) -> ! {
    let mut rx_buffer = Vec::new();
    let mut tx_buffer = Vec::new();
    rx_buffer.resize(2048, 0);
    tx_buffer.resize(2048, 0);

    info!("Waiting for network stack to connection to station...");
    net_stack.wait_config_up().await;
    let config = net_stack.config_v4();
    info!("Network stack established on {config:?}");

    const CONNECTION_TIMEOUT: Duration = Duration::from_secs(5);
    const IP_ENDPOINT: IpListenEndpoint = IpListenEndpoint {
        addr: None,
        port: 80,
    };

    loop {
        // each new connection gets a fresh tcp socket
        let mut socket = TcpSocket::new(net_stack, &mut rx_buffer, &mut tx_buffer);
        socket.set_timeout(Some(CONNECTION_TIMEOUT));

        info!("Listening for tcp connection on url={IP_ENDPOINT:?}");
        if let Err(err) = socket.accept(IP_ENDPOINT).await {
            error!("Failed to connect to tcp socket: {err:?}");
            continue;
        }

        let endpoint = socket.remote_endpoint();

        handle_socket_connection(&mut socket).await;
        socket.close();
        info!("Closing connection ip={endpoint:?}");
    }
}

async fn handle_socket_connection<'a>(socket: &mut TcpSocket<'a>) {
    let endpoint = socket.remote_endpoint();
    info!("Client connected from {:?}", endpoint);

    let mut request_buffer = [0u8; 1024];
    let mut request_size = 0;
    loop {
        let read_buffer = &mut request_buffer[request_size..];
        if read_buffer.len() == 0 {
            error!("Truncating response since it overflowed request buffer of size={0}, ip={endpoint:?}", request_buffer.len());
            break;
        }
        match socket.read(read_buffer).await {
            Ok(0) => break,
            Ok(total_bytes) => {
                let read_chunk = &read_buffer[..total_bytes];
                request_size += total_bytes;
                let is_eof = read_chunk.windows(4).any(|w| w == b"\r\n\r\n");
                if is_eof {
                    break;
                }
            }
            Err(err) => {
                error!("Error while reading request from ip={endpoint:?}, err={err:?}");
                return;
            },
        }
    }
    let request_body = core::str::from_utf8(&request_buffer[..request_size]);
    info!("Read request from connection ip={endpoint:?}");
    match request_body {
        Ok(body) => info!("Got request body length={0}\n{body}", body.len()),
        Err(ref err) => {
            error!("Bad request body: {err:?}");
            return;
        },
    }

    let response_body: &'static str = "Hello world!";
    let response_header = alloc::format!(
        "HTTP/1.0 200 OK\r\nContent-Type: text/html\r\nContent-Length: {}\r\nConnection: close\r\n\r\n",
        response_body.len()
    );

    if let Err(err) = socket.write_all(response_header.as_bytes()).await {
        error!("Failed to write response header to connection ip={endpoint:?}, err={err:?}");
        return;
    }
    if let Err(err) = socket.write_all(response_body.as_bytes()).await {
        error!("Failed to write response body to connection ip={endpoint:?}, err={err:?}");
        return;
    }
    if let Err(err) = socket.flush().await {
        error!("Failed to flush to connection ip={endpoint:?}, err={err:?}");
        return;
    }

    info!("Successfully handled socket connection ip={endpoint:?}");
}

async fn run_wifi_station(mut wifi_controller: WifiController<'static>) -> ! {
    use crate::wifi_credentials::{SSID, PASSWORD};
    let station_config = StationConfig::default()
        .with_ssid(SSID)
        .with_password(PASSWORD.to_string());

    wifi_controller.set_config(&WifiConfig::Station(station_config))
        .expect("Failed to set wifi controller station configuration");

    info!("Attempting to connect to wifi station with ssid={SSID}");

    const RETRY_DURATION: Duration = Duration::from_secs(10);
    loop {
        match wifi_controller.connect_async().await {
            Err(err) => {
                error!("Wifi failed to connect: {err:?}");
                Timer::after(RETRY_DURATION).await;
                continue;
            },
            Ok(res) => info!("Wifi connected successfully: {res:?}"),
        }
        match wifi_controller.wait_for_disconnect_async().await {
            Ok(res) => info!("Wifi disconnected gracefully: {res:?}"),
            Err(err) => error!("Wifi disconnected with an error: {err:?}"),
        }
        info!("Attempting to reconnect to wifi station after disconnecting...");
    }
}
