#define CLK_FREQ (float)(16000000.0f)

/**
 * Calculate the match register value given the prescaler and target frequency
 **
 * Frequency equations
 * PCLK = CLK / PR
 * MCLK = PCLK / (TOP+1)
 ** 
 * Symbol definitions
 * PCLK = timer frequency
 * MCLK = match frequency
 * CLK = clock frequency (16MHz)
 * PR = prescaler value              (Use TCCRnm, timer counter control register for timer n match channel m)
 * TOP = match register value        (Use OCRnm, overflow control register for timer n match channel m)
 */

unsigned int calculate_match_register_value(unsigned int prescaler, float match_frequency) {
  // PCLK = CLK / PR
  float PCLK = (float)CLK_FREQ / (float)prescaler;
  // MCLK = PCLK / (TOP+1)
  // TOP = PCLK/MCLK -1
  float top = PCLK/match_frequency - 1.0f;
  return (unsigned int)top;
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
