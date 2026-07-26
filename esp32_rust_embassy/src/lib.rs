#![no_std]
#![feature(impl_trait_in_assoc_type)] // for picoserve AppBuilder trait
pub mod ble_scanner;
pub mod web_server;
pub mod app;
pub mod secrets;
pub mod heap_stats;
pub mod bluetooth_device;
