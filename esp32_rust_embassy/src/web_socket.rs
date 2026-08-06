use esp_hal::time::Instant;
use embassy_sync::pubsub::WaitResult;
use picoserve::{
    response::ws,
    io,
};
use num_enum::{TryFromPrimitive, IntoPrimitive};
use derive_more::From;

extern crate alloc;
use alloc::{
    vec,
    vec::Vec,
    sync::Arc,
};
use crate::app::{App, WebsocketWatchValue};

pub struct WebsocketHandler {
    pub app: Arc<App>,
}

#[repr(u8)]
#[derive(Debug, Clone, Copy, TryFromPrimitive, IntoPrimitive)]
enum CommandHeader {
    SetLedDutyCycle = 0x00,
    GetLedDutyCycle = 0x01,
    Ping = 0x02,
    GetUptime = 0x03,
}

#[repr(u8)]
#[derive(Debug, Clone, Copy)]
enum ResponseHeader {
    MissedMessages = 0x00,
    BluetoothUpdate = 0x01,
    GetLedDutyCycle = 0x02,
    BackgroundTaskMessage = 0x03,
    Pong = 0x04,
    GetUptime = 0x05,
    // errors
    EmptyCommand = 0xFC,
    BadCommandLength = 0xFD,
    BadCommandValue = 0xFE,
    UnhandledCommand = 0xFF,
}

#[derive(Debug, Clone, From)]
enum BinaryMessageResponse {
    None,
    StaticMessage(&'static [u8]),
    HeapMessage(Vec<u8>),
}

impl BinaryMessageResponse {
    fn as_slice(&self) -> &[u8] {
        match self {
            Self::None => &[],
            Self::StaticMessage(message) => message,
            Self::HeapMessage(message) => message.as_slice(),
        }
    }

    fn from_bad_length(command: CommandHeader, expected_length: usize, given_length: usize) -> Self {
        vec![ResponseHeader::BadCommandLength as u8, command as u8, expected_length as u8, given_length as u8].into()
    }

    fn from_bad_value(command: CommandHeader, index: u8, value: u8) -> Self {
        vec![ResponseHeader::BadCommandValue as u8, command as u8, index, value].into()
    }
}

async fn handle_binary_message(app: &App, message: &[u8]) -> BinaryMessageResponse {
    if message.is_empty() {
        return [ResponseHeader::EmptyCommand as u8].as_slice().into()
    }
    let Ok(header) = CommandHeader::try_from(message[0]) else {
        return vec![ResponseHeader::UnhandledCommand as u8, message[0]].into();
    };

    match header {
        CommandHeader::SetLedDutyCycle => {
            const EXPECTED_LENGTH: usize = 2;
            if message.len() != EXPECTED_LENGTH {
                return BinaryMessageResponse::from_bad_length(header, EXPECTED_LENGTH, message.len());
            }
            let duty_cycle = message[1];
            if app.led_controller.set_duty_cycle(duty_cycle).is_err() {
                return BinaryMessageResponse::from_bad_value(header, 1, duty_cycle);
            }
            BinaryMessageResponse::None
        },
        CommandHeader::GetLedDutyCycle => {
            const EXPECTED_LENGTH: usize = 1;
            if message.len() != EXPECTED_LENGTH {
                return BinaryMessageResponse::from_bad_length(header, EXPECTED_LENGTH, message.len());
            }
            let duty_cycle = app.led_controller.get_duty_cycle();
            vec![ResponseHeader::GetLedDutyCycle as u8, duty_cycle].into()
        },
        CommandHeader::Ping => {
            const EXPECTED_LENGTH: usize = 5;
            if message.len() != EXPECTED_LENGTH {
                return BinaryMessageResponse::from_bad_length(header, EXPECTED_LENGTH, message.len());
            }
            let mut response = vec![0u8; 5];
            response[0] = ResponseHeader::Pong as u8;
            response[1..5].copy_from_slice(&message[1..5]);
            response.into()
        },
        CommandHeader::GetUptime => {
            const EXPECTED_LENGTH: usize = 1;
            if message.len() != EXPECTED_LENGTH {
                return BinaryMessageResponse::from_bad_length(header, EXPECTED_LENGTH, message.len());
            }
            let mut response = vec![0u8; 9];
            response[0] = ResponseHeader::GetUptime as u8;
            let uptime_micros: u64 = Instant::now().duration_since_epoch().as_micros();
            response[1..9].copy_from_slice(&uptime_micros.to_le_bytes());
            response.into()
        },
    }
}

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

        // https://websocket.org/reference/close-codes/
        type WebsocketCloseReason<'a> = (u16, &'a str);

        let mut message_buffer = vec![0; 32];
        let mut websocket_subscriber = match self.app.get_websocket_subscriber() {
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
                        WebsocketWatchValue::BackgroundTaskMessage { message } => {
                            let mut response = [0u8; 5];
                            response[0] = ResponseHeader::BackgroundTaskMessage as u8;
                            response[1..5].copy_from_slice(&message.to_le_bytes());
                            tx.send_binary(&response).await?;
                            continue;
                        },
                    },
                },
            };

            // log::info!("Message: {message:?}");
            match message {
                Message::Text(new_message) => {
                    tx.send_text(new_message).await?;
                },
                Message::Binary(message) => {
                    let res = handle_binary_message(&self.app, message).await;
                    let binary_response = res.as_slice();
                    if !binary_response.is_empty() {
                        tx.send_binary(binary_response).await?;
                    }
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
