#include "start_stop.h"

StartStop::StartStop(uint8_t buttonPin, uint8_t ledPin, unsigned long debounceMs)
  : _buttonPin(buttonPin), _ledPin(ledPin), _debounceMs(debounceMs),
    _lastPressTime(0), _lastButtonState(HIGH), _isRunning(false), _wasPressed(false) {
}

void StartStop::begin() {
  pinMode(_buttonPin, INPUT_PULLUP);
  pinMode(_ledPin, OUTPUT);
  digitalWrite(_ledPin, LOW);
}

bool StartStop::isPressed() {
  bool currentState = !digitalRead(_buttonPin);  // інвертуємо, бо INPUT_PULLUP
  unsigned long now = millis();
  
  if (currentState && !_lastButtonState && (now - _lastPressTime > _debounceMs)) {
    _lastPressTime = now;
    _lastButtonState = currentState;
    _wasPressed = true;
    return true;
  }
  
  _lastButtonState = currentState;
  return false;
}

bool StartStop::toggle() {
  if (isPressed()) {
    _isRunning = !_isRunning;
    updateLED();
    return _isRunning;
  }
  return _isRunning;
}

void StartStop::setState(bool state) {
  _isRunning = state;
  updateLED();
}

void StartStop::updateLED() {
  digitalWrite(_ledPin, _isRunning ? HIGH : LOW);
}
