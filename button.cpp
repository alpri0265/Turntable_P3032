#include "button.h"

Button::Button(uint8_t pin, unsigned long debounceMs)
  : _pin(pin), _debounceMs(debounceMs), _lastPressTime(0), 
    _lastState(HIGH), _wasPressed(false) {
}

void Button::begin() {
  pinMode(_pin, INPUT_PULLUP);
}

bool Button::isPressed() {
  bool currentState = !digitalRead(_pin);  // інвертуємо, бо INPUT_PULLUP
  unsigned long now = millis();
  
  if (currentState && !_lastState && (now - _lastPressTime > _debounceMs)) {
    _lastPressTime = now;
    _lastState = currentState;
    _wasPressed = true;
    return true;
  }
  
  _lastState = currentState;
  return false;
}

bool Button::wasPressed() {
  if (_wasPressed) {
    _wasPressed = false;
    return true;
  }
  return false;
}
