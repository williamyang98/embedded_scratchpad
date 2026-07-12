#include <ArduinoSTL.h>

#include <BasicComponents.h>
#include <BasicComponents/LED.hpp>
#include <BasicComponents/Timer.hpp>

#include <ILEDController.hpp>

#include <LEDControllers/Oscillator.hpp>
#include <CommandCLI/OscillatorCLI.hpp>
#include <PersistentObjects/OscillatorPersistence.hpp>

#include <LEDControllers/TextToLED/TextToLED.hpp>
#include <LEDControllers/TextToLED/TapCodeEncoder.hpp>
#include <LEDControllers/TextToLED/MorseCodeEncoder.hpp>
#include <CommandCLI/TextToLEDCLI.hpp>
#include <PersistentObjects/TextToLEDPersistence.hpp>

#include <LEDControllers/Slider.hpp>
#include <CommandCLI/SliderCLI.hpp>
#include <PersistentObjects/SliderPersistence.hpp>

#include <LEDControllers/SteadyState.hpp>

#include <ControllerManager/ControllerManager.hpp>

#define BAUD_RATE 9600
#define BUFFER_SIZE 256

char serialBuffer[BUFFER_SIZE] = {0};

bc::LED led(13);
const int button_pin = 2;
const int MANAGER_ADDR = 0;

// module manager
ControllerManager manager(MANAGER_ADDR); // oscillator


void setup()
{
    Serial.begin(BAUD_RATE);
    create_modules();
    init_button();
    init_timer_1(60.0f); // update at 60fps 
}

void loop()
{
    // check CLI
    if (Serial.available() > 0) {
      parse_command();
    }
    delay(100);
}

void parse_command(void) {
  int totalBytes = Serial.readBytes(serialBuffer, sizeof(serialBuffer));
  if (totalBytes > 0) {
      serialBuffer[totalBytes] = 0;
      ResponseType response = manager.ParseCommand(serialBuffer, totalBytes);
      switch (response)
      {
      case ACCEPTED:
      case DATA_CHANGED:
          manager.Save();
          break;
      case REJECTED:
          Serial.println(F("Invalid command"));
          break;
      }
  }
}
