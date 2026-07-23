#!/bin/sh
set -x
# https://docs.espressif.com/projects/rust/book/getting-started/toolchain.html
cargo install espup --locked
espup install

# https://docs.espressif.com/projects/rust/book/getting-started/tooling/esp-generate.html
cargo install esp-generate --locked
# https://docs.espressif.com/projects/rust/book/getting-started/tooling/espflash.html
cargo install espflash --locked
# https://probe.rs/docs/getting-started/installation/
# cargo binstall probe-rs-tools --no-confirm
# Use espflash since probe-rs doesn't work with COM ports

# https://docs.espressif.com/projects/rust/book/getting-started/tooling/esp-config.html
cargo install esp-config --features=tui --locked
