#include "absolute_encoder.h"

AbsoluteEncoder::AbsoluteEncoder(uint8_t analogPin, float refVoltage, float maxAngle)
  : _analogPin(analogPin), _refVoltage(refVoltage), _maxAngle(maxAngle),
    _lastAngle(999), _lastReadTime(0), _zeroOffset(0.0) {
}

void AbsoluteEncoder::begin() {
  pinMode(_analogPin, INPUT);
  // Зчитуємо початкове значення
  _lastAngle = readAngleInt();
}

float AbsoluteEncoder::readRawAngle() {
  int sensorValue = analogRead(_analogPin);
  float voltage = sensorValue * (_refVoltage / 1023.0);
  float angle = (voltage / _refVoltage) * _maxAngle;
  
  // Обмежуємо кут в межах 0-360
  if (angle < 0) angle = 0;
  if (angle > _maxAngle) angle = _maxAngle;
  
  return angle;
}

float AbsoluteEncoder::readAngle() {
  float rawAngle = readRawAngle();
  float adjustedAngle = rawAngle - _zeroOffset;
  
  // Нормалізуємо кут до діапазону 0-360
  while (adjustedAngle < 0) {
    adjustedAngle += _maxAngle;
  }
  while (adjustedAngle >= _maxAngle) {
    adjustedAngle -= _maxAngle;
  }
  
  return adjustedAngle;
}

uint16_t AbsoluteEncoder::readAngleInt() {
  return (uint16_t)readAngle();
}

bool AbsoluteEncoder::hasChanged() {
  unsigned long now = millis();
  if (now - _lastReadTime < READ_INTERVAL_MS) {
    return false;
  }
  
  uint16_t currentAngle = readAngleInt();
  bool changed = (currentAngle != _lastAngle);
  
  if (changed) {
    _lastAngle = currentAngle;
  }
  
  _lastReadTime = now;
  return changed;
}

void AbsoluteEncoder::setZero() {
  // Встановлюємо поточне сире значення як нуль
  float currentRawAngle = readRawAngle();
  _zeroOffset = currentRawAngle;
  // Оновлюємо останній кут для уникнення помилок виявлення зміни
  _lastAngle = readAngleInt();
}
