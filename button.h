#ifndef BUTTON_H
#define BUTTON_H

#include <Arduino.h>

class Button {
public:
  Button(uint8_t pin, unsigned long debounceMs = 500);
  void begin();
  bool isPressed();  // Перевіряє натискання з debounce
  bool wasPressed();  // Повертає true один раз після натискання
  bool isCurrentlyPressed() const;  // Перевіряє поточний стан кнопки (без debounce)
  
private:
  uint8_t _pin;
  unsigned long _debounceMs;
  unsigned long _lastPressTime;
  bool _lastState;
  bool _wasPressed;
};

#endif
