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
