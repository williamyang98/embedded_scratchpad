// 8 bit timer - TOP = 2^8 = 256
// PCLK = 16MHz / 64 = 250kHz
// MCLK = PCLK / (TOP+1) = 1kHz, TOP = 249
void init_timer_4(float target_frequency) {
  TCNT4 = 0;
  TCCR4A = 0;
  TCCR4B = 0;
  
  // turn on CTC mode
  TCCR4B |= (1 << WGM42);
  TCCR4B |= (1 << CS42) | (1 << CS40); // PR = 1024
  
  // enable timer compare interrupt
  OCR4A = calculate_match_register_value(1024, target_frequency);
  TIMSK4 |= (1 << OCIE4A);
}

// 16 bit timer - TOP = 2^16 = 65536
void init_timer_1(float target_frequency) {
  TCNT1 = 0;   // Counter  = 0
  TCCR1A = 0;  // Sets to normal timer/counter mode (pg144)            
  TCCR1B = 0;  // Create waveform on timer 1B 
  
  // Set waveform generation mode (WGM)
  // Enable CTC mode (count to counter) (pg144)
  // CTC will set TOP to OCRnA
  TCCR1B |= (1 << WGM12);                
  // PR = 1024
  TCCR1B |= ((1 << CS12) | (1 << CS10)); 
  
  // Timer 1B will count to timer 1A 
  OCR1A = calculate_match_register_value(1024, target_frequency);
  TIMSK1 |= (1 << OCIE1A);               // Enable timer compare interrupt (Timer interrupt mask register)
}

// 16 bit timer - TOP = 2^16 = 65536
void init_timer_3(float target_frequency) {
  TCNT3 = 0;   // Counter  = 0
  TCCR3A = 0;  // Sets to normal timer/counter mode (pg144)            
  TCCR3B = 0;  // Create waveform on timer 1B 
  
  // Set waveform generation mode (WGM)
  // Enable CTC mode (count to counter) (pg144)
  // CTC will set TOP to OCRnA
  TCCR3B |= (1 << WGM32);                
  // PR = 1024
  TCCR3B |= ((1 << CS32) | (1 << CS30)); 
  
  // Timer 1B will count to timer 1A 
  OCR3A = calculate_match_register_value(1024, target_frequency);
  TIMSK3 |= (1 << OCIE3A);               // Enable timer compare interrupt (Timer interrupt mask register)
}
