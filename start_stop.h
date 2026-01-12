#ifndef START_STOP_H
#define START_STOP_H

#include <Arduino.h>

class StartStop {
public:
  StartStop(uint8_t buttonPin, uint8_t ledPin, unsigned long debounceMs = 500);
  void begin();
  bool isPressed();  // Перевіряє натискання з debounce
  bool toggle();  // Перемикає стан (true = старт, false = стоп)
  bool getState() const { return _isRunning; }
  void setState(bool state);  // Встановлює стан
  void updateLED();  // Оновлює стан світлодіода
  
private:
  uint8_t _buttonPin;
  uint8_t _ledPin;
  unsigned long _debounceMs;
  unsigned long _lastPressTime;
  bool _lastButtonState;
  bool _isRunning;
  bool _wasPressed;
};

#endif
