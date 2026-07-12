ISR(TIMER1_COMPA_vect){
    int value = manager.GetValue();
    led.SetValue(value);
    manager.Update();
}
