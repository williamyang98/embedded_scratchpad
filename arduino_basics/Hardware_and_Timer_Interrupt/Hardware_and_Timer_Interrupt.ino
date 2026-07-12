const int left_led_pin = 5;
const int middle_led_pin = 8;
const int right_led_pin = 10;



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
  // when button pressed, toggle led async
  pinMode(led_pin, OUTPUT);
  pinMode(button_pin, INPUT);
  pinMode(button_measure_pin, INPUT);
  attachInterrupt(digitalPinToInterrupt(button_pin), &on_button_press, RISING);

  // left: 2Hz, middle: 10Hz, right: 20Hz
  init_timer_1(2.0f); 
  init_timer_3(10.0f);
  init_timer_4(20.0f);
}

void loop() {
  // continuous loop to print info about button/led combo
  int voltage = analogRead(button_measure_pin);
  Serial.print(voltage);
  Serial.print(',');
  Serial.print(digitalRead(led_pin) * 1024);
  Serial.print(',');
  Serial.println(512);
  delay(10);
}

void on_button_press(void) {
  digitalWrite(led_pin, !digitalRead(led_pin));
}
