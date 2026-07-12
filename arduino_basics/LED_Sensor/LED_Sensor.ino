// Reads an analog input on pin 0 from photoresistor connected to voltage divider to get light level

#define TOTAL_SAMPLES 3
int moving_average[TOTAL_SAMPLES] = {0};
int current_index = 0;

// the setup routine runs once when you press reset:
void setup() {
  // initialize serial communication at 9600 bits per second:
  Serial.begin(9600);
}

// the loop routine runs over and over again forever:
void loop() {
  int sensorValue = analogRead(A0);
  push_value(sensorValue);
  
  float avg = get_moving_average();
  Serial.println(avg);
  delay(1);        // delay in between reads for stability
}

void push_value(int value) {
  moving_average[current_index] = value;
  current_index = (current_index+1)%TOTAL_SAMPLES;
}

float get_moving_average() {
  float sum = 0;
  for (int i = 0; i < TOTAL_SAMPLES; i++) {
    sum += moving_average[i];
  }
  float avg = sum / (float)TOTAL_SAMPLES;
  return avg;
}
