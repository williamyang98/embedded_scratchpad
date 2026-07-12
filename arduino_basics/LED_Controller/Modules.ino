void create_modules(void) {
  // oscillator
  Oscillator            *oscillator            = new Oscillator();
  OscillatorCLI         *oscillatorCLI         = new OscillatorCLI(oscillator);
  OscillatorPersistence *oscillatorPersistence = new OscillatorPersistence(oscillator, manager.GetAddrEnd());
  // slider
  Slider                *slider               = new Slider();
  SliderCLI             *sliderCLI            = new SliderCLI(slider);
  SliderPersistence     *sliderPersistence    = new SliderPersistence(slider, oscillatorPersistence->GetAddrEnd());
  // morse code
  MorseCodeEncoder     *morseCodeEncoder      = new MorseCodeEncoder(StandardMorseCodeTable);
  TextToLED            *morseCode             = new TextToLED(morseCodeEncoder);
  TextToLEDCLI         *morseCodeCLI          = new TextToLEDCLI(morseCode);
  TextToLEDPersistence *morseCodePersistence  = new TextToLEDPersistence(morseCode, sliderPersistence->GetAddrEnd());
  // tap code
  TapCodeEncoder       *tapCodeEncoder        = new TapCodeEncoder(&TapCodeTable);
  TextToLED            *tapCode               = new TextToLED(tapCodeEncoder);
  TextToLEDCLI         *tapCodeCLI            = new TextToLEDCLI(tapCode);
  TextToLEDPersistence *tapCodePersistence    = new TextToLEDPersistence(tapCode, morseCodePersistence->GetAddrEnd());
  // off
  SteadyState *off = new SteadyState(0);
  // on
  SteadyState *on = new SteadyState(255);
  
  manager.AddModule((ControllerModule){ off,        nullptr,       nullptr,               "off" });
  manager.AddModule((ControllerModule){ on,         nullptr,       nullptr,               "on" });
  manager.AddModule((ControllerModule){ slider,     sliderCLI,     sliderPersistence,     "slider" });
  manager.AddModule((ControllerModule){ morseCode,  morseCodeCLI,  morseCodePersistence,  "morse code" });
  manager.AddModule((ControllerModule){ tapCode,    tapCodeCLI,    tapCodePersistence,    "tap code" });
  manager.AddModule((ControllerModule){ oscillator, oscillatorCLI, oscillatorPersistence, "oscillator" });
  manager.Load();
}
