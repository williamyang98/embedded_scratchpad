const int button_pin = 2;
const int voltage_pin = A0;

const float R1 = 50700; // 50.8k
const int total_samples = 10;


void setup() {
  Serial.begin(9600);
  
  pinMode(button_pin, INPUT);
  pinMode(voltage_pin, INPUT);
  attachInterrupt(digitalPinToInterrupt(button_pin), &measure_resistance, RISING);
}

void loop() {
  delay(1000);
}

void measure_resistance(void) {
  float sum_resistance = 0;
  for (int i = 0; i < total_samples; i++) {
    sum_resistance += get_resistance();
  }
  float R2 = sum_resistance / (float)total_samples;
  Serial.println(R2);
}

float get_resistance(void) {
  // Vout = Vin * R2 / (R1 + R2)
  // Vout/Vin = R2/(R1+R2)
  // Vin/Vout = R1/R2 + 1
  // R1/R2 = Vin/Vout - 1
  // R2 = R1/(Vin/Vout - 1)

  // Since Vout = val * 5/1023
  // Vin/Vout = 5/(val * 5/1023)
  // Vin/Vout = 1023/val

  int value = analogRead(voltage_pin);
  float Vin_Vout = 1023.0f / (float)value;
  float R2 = R1 / (Vin_Vout - 1.0f);
  return R2;
}
