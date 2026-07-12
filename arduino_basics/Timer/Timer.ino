const int right_led_pin = 13;
const int left_led_pin = 8;
const int middle_led_pin = 5;

const int led_pin = 13;
const int button_pin = 2; // can be attached as hardware interrupt
const int button_measure_pin = A0;

 
void setup() {
  Serial.begin(9600);
  // timer interrupt
  pinMode(left_led_pin, OUTPUT);
  pinMode(right_led_pin, OUTPUT);
  pinMode(middle_led_pin, OUTPUT);

  // hardware interrupt
  pinMode(led_pin, OUTPUT);
  pinMode(button_pin, INPUT);
  pinMode(button_measure_pin, INPUT);
  attachInterrupt(digitalPinToInterrupt(button_pin), &on_button_press, RISING);

  // timers
  init_timer_1(10.0f);
  init_timer_3(10.0f);
  init_timer_4(10.0f);
}
 
 
void loop() {
  delay(1000);
}



 
 
