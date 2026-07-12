ISR(TIMER1_COMPA_vect){
   static byte state = HIGH;
   state = !state;
   digitalWrite(right_led_pin, state);
}

/**
 * Updates at 1kHz
 * To reduce frequency, add another counter
 * Fnew = 1kHz / (TOP+1)
 */
ISR(TIMER3_COMPA_vect){
  static byte state = HIGH;
  state = !state;
  digitalWrite(middle_led_pin, state);
}

/**
 * Updates at 1kHz
 * To reduce frequency, add another counter
 * Fnew = 1kHz / (TOP+1) = 20Hz
 */
ISR(TIMER4_COMPA_vect){
  static byte state = HIGH;
  state = !state;
  digitalWrite(left_led_pin, state);
}
