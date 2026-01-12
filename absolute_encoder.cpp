#include "absolute_encoder.h"

AbsoluteEncoder::AbsoluteEncoder(uint8_t analogPin, float refVoltage, float maxAngle)
  : _analogPin(analogPin), _refVoltage(refVoltage), _maxAngle(maxAngle),
    _lastAngle(999), _lastReadTime(0), _zeroOffset(0.0), _filterIndex(0) {
  // Ініціалізуємо буфер фільтрації
  for (uint8_t i = 0; i < FILTER_SAMPLES; i++) {
    _filterBuffer[i] = 0.0;
  }
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

float AbsoluteEncoder::readRawAngleFiltered() {
  // Читаємо кілька зразків і усереднюємо (просте ковзне середнє)
  float newValue = readRawAngle();
  _filterBuffer[_filterIndex] = newValue;
  _filterIndex = (_filterIndex + 1) % FILTER_SAMPLES;
  
  // Обчислюємо середнє значення з буфера
  float sum = 0.0;
  for (uint8_t i = 0; i < FILTER_SAMPLES; i++) {
    sum += _filterBuffer[i];
  }
  float average = sum / FILTER_SAMPLES;
  
  return average;
}

float AbsoluteEncoder::readAngle() {
  // Використовуємо фільтроване значення для стабільності
  float rawAngle = readRawAngleFiltered();
  float adjustedAngle = rawAngle - _zeroOffset;
  
  // Якщо кут дуже близький до нуля (шум), встановлюємо точно 0
  if (adjustedAngle < 1.0 && adjustedAngle > -1.0) {
    adjustedAngle = 0.0;
  }
  
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
  // Очищаємо буфер перед зчитуваннями
  for (uint8_t i = 0; i < FILTER_SAMPLES; i++) {
    _filterBuffer[i] = 0.0;
  }
  _filterIndex = 0;
  
  // Затримка для стабілізації АЦП
  delay(50);
  
  // Робимо багато зчитувань для точного усереднення
  float sum = 0.0;
  const uint8_t samples = 128;  // Ще більше зразків
  for (uint8_t i = 0; i < samples; i++) {
    int sensorValue = analogRead(_analogPin);
    float voltage = sensorValue * (_refVoltage / 1023.0);
    float angle = (voltage / _refVoltage) * _maxAngle;
    if (angle < 0) angle = 0;
    if (angle > _maxAngle) angle = _maxAngle;
    sum += angle;
    delayMicroseconds(1000);  // Більша затримка для стабілізації
  }
  
  // Обчислюємо середнє та встановлюємо offset
  float averageRawAngle = sum / samples;
  _zeroOffset = averageRawAngle;
  
  // Заповнюємо буфер offset для миттєвої стабільності після обнулення
  for (uint8_t i = 0; i < FILTER_SAMPLES; i++) {
    _filterBuffer[i] = _zeroOffset;
  }
  _filterIndex = 0;
  
  // Оновлюємо останній кут
  _lastAngle = 0;
}
