#![no_std]
#![feature(impl_trait_in_assoc_type)] // for picoserve AppBuilder trait
#![recursion_limit = "256"]
pub mod ble_scanner;
pub mod web_server;
pub mod web_socket;
pub mod app;
pub mod secrets;
pub mod heap_stats;
pub mod bluetooth_device;
pub mod led_controller;
