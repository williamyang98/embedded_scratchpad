void init_button(void) {
  pinMode(button_pin, INPUT);
  attachInterrupt(digitalPinToInterrupt(button_pin), &on_button_press, FALLING);
}

void on_button_press(void) {
  int next = (manager.GetCurrentModuleIndex() + 1) % manager.GetTotalModules();
  if (manager.SelectModule(next)) {
    Serial.println(F("Button selected next module"));
  } else {
    Serial.println(F("Button couldn't change to next module"));  
  }
}
