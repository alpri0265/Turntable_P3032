#include "direction_switch.h"

DirectionSwitch::DirectionSwitch(uint8_t pin)
  : _pin(pin), _direction(DIR_CW), _lastDirection(DIR_CW), _lastReadTime(0) {
}

void DirectionSwitch::begin() {
  pinMode(_pin, INPUT_PULLUP);
  // Читаємо початковий стан
  _direction = (digitalRead(_pin) == LOW) ? DIR_CW : DIR_CCW;
  _lastDirection = _direction;
}

RotationDirection DirectionSwitch::read() {
  unsigned long now = millis();
  
  // Читаємо стан перемикача (з debounce)
  if (now - _lastReadTime >= READ_INTERVAL_MS) {
    // LOW = CW (за годинниковою), HIGH = CCW (проти годинникової)
    // (залежить від підключення, можна інвертувати)
    _direction = (digitalRead(_pin) == LOW) ? DIR_CW : DIR_CCW;
    _lastReadTime = now;
  }
  
  return _direction;
}

bool DirectionSwitch::hasChanged() {
  RotationDirection current = read();
  bool changed = (current != _lastDirection);
  if (changed) {
    _lastDirection = current;
  }
  return changed;
}
