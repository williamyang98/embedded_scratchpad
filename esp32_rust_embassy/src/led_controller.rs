use esp_hal::time::Rate;
use esp_hal::gpio::{Output, DriveMode};
use esp_hal::ledc::{
    Ledc, LSGlobalClkSource, LowSpeed,
    timer::{
        Timer as LedTimer, TimerIFace, Number as TimerNumber, LSClockSource,
        config::{Config as TimerConfig, Duty as TimerDuty},
    },
    channel::{
        Channel as LedChannel, ChannelIFace, Number as ChannelNumber,
        config::Config as ChannelConfig,
        Error as ChannelError,
    },
};
use esp_hal::peripherals::LEDC;
use core::sync::atomic::{AtomicU8, Ordering};

// rtos
extern crate alloc;
use alloc::rc::Rc;

pub struct LedController {
    _led_controller: Ledc<'static>,
    _led_timer: Rc<LedTimer<'static, LowSpeed>>,
    led_channel: LedChannel<'static, LowSpeed>,
    duty: AtomicU8,
}

impl LedController {
    pub fn new(ledc: LEDC<'static>, led_output: Output<'static>) -> LedController {
        // led_controller -> low_speed_timer -> low_speed_channel -> led_output
        let mut led_controller = Ledc::new(ledc);
        led_controller.set_global_slow_clock(LSGlobalClkSource::APBClk);
        let mut led_timer = led_controller.timer::<LowSpeed>(TimerNumber::Timer0);
        led_timer.configure(TimerConfig {
            duty: TimerDuty::Duty5Bit,
            clock_source: LSClockSource::APBClk,
            frequency: Rate::from_khz(24),
        }).expect("Failed to configure led controller low speed timer");

        let led_timer = Rc::new(led_timer);
        let mut led_channel = led_controller.channel::<LowSpeed>(ChannelNumber::Channel0, led_output);
        led_channel.configure(ChannelConfig {
            // LEAK: store in permanently leaked Rc
            timer: unsafe { &*Rc::into_raw(led_timer.clone()) },
            duty_pct: 0,
            drive_mode: DriveMode::PushPull,
        }).expect("Failed to configure led controller low speed channel");

        LedController {
            _led_controller: led_controller,
            _led_timer: led_timer,
            led_channel,
            duty: AtomicU8::new(0),
        }
    }

    pub fn set_duty_cycle(&self, duty: u8) -> Result<(), ChannelError> {
        match self.led_channel.set_duty(duty) {
            Ok(()) => {
                self.duty.store(duty, Ordering::Relaxed);
                Ok(())
            },
            Err(err) => Err(err),
        }
    }

    pub fn get_duty_cycle(&self) -> u8 {
        self.duty.load(Ordering::Relaxed)
    }
}

// treat it as an atomic variable
unsafe impl Send for LedController { }
unsafe impl Sync for LedController { }
