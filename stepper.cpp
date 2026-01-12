#include "stepper.h"

Stepper::Stepper(uint8_t stepPin, uint8_t dirPin)
  : _stepPin(stepPin), _dirPin(dirPin), _position(0), 
    _remaining(0), _lastStepTime(0), _currentDir(0) {
}

void Stepper::begin() {
  pinMode(_stepPin, OUTPUT);
  pinMode(_dirPin, OUTPUT);
  digitalWrite(_stepPin, LOW);
  digitalWrite(_dirPin, LOW);
}

void Stepper::setPosition(int32_t position) {
  _position = position;
  _remaining = 0;
}

void Stepper::update() {
  if (_remaining == 0) return;
  
  unsigned long now = micros();
  
  // Перевіряємо, чи минуло достатньо часу для наступного кроку
  if (now - _lastStepTime >= STEP_DELAY_US) {
    doStep();
    _lastStepTime = now;
  }
}

void Stepper::move(int32_t steps) {
  if (steps == 0) return;
  _remaining += steps;
  if (_lastStepTime == 0) {
    _lastStepTime = micros();
  }
}

void Stepper::doStep() {
  // Встановлюємо напрямок (тільки якщо змінився)
  int8_t newDir = (_remaining > 0) ? 1 : -1;
  if (_currentDir != newDir) {
    digitalWrite(_dirPin, newDir > 0 ? HIGH : LOW);
    _currentDir = newDir;
    // Невелика затримка для стабілізації напрямку
    unsigned long dirSetTime = micros();
    while (micros() - dirSetTime < 2) {} // ~2 мкс
  }
  
  // Формуємо імпульс STEP
  digitalWrite(_stepPin, HIGH);
  unsigned long pulseStart = micros();
  while (micros() - pulseStart < STEP_PULSE_US) {} // чекаємо 4 мкс
  digitalWrite(_stepPin, LOW);
  
  // Оновлюємо позицію
  if (_remaining > 0) {
    _remaining--;
    _position++;
  } else {
    _remaining++;
    _position--;
  }
}
