#ifndef ABSOLUTE_ENCODER_H
#define ABSOLUTE_ENCODER_H

#include <Arduino.h>

class AbsoluteEncoder {
public:
  AbsoluteEncoder(uint8_t analogPin, float refVoltage = 5.0, float maxAngle = 360.0);
  void begin();
  float readAngle();  // Читає кут в градусах (0-360)
  uint16_t readAngleInt();  // Читає кут як ціле число (0-360)
  bool hasChanged();  // Перевіряє, чи змінився кут
  void setZero();  // Встановлює поточне положення як нуль (0°)
  
private:
  uint8_t _analogPin;
  float _refVoltage;
  float _maxAngle;
  uint16_t _lastAngle;
  unsigned long _lastReadTime;
  float _zeroOffset;  // Зсув для встановлення нуля
  static const unsigned long READ_INTERVAL_MS = 10;  // Інтервал читання
  static const uint8_t FILTER_SAMPLES = 8;  // Кількість зразків для фільтрації
  float _filterBuffer[FILTER_SAMPLES];  // Буфер для фільтрації
  uint8_t _filterIndex;  // Індекс для буфера фільтрації
  
  float readRawAngle();  // Читає сире значення кута без урахування offset
  float readRawAngleFiltered();  // Читає сире значення з фільтрацією
};

#endif
