#ifndef DIRECTION_SWITCH_H
#define DIRECTION_SWITCH_H

#include <Arduino.h>

// Напрямки обертання
enum RotationDirection {
  DIR_CW = 0,   // За годинниковою стрілкою (Clockwise)
  DIR_CCW = 1   // Проти годинникової стрілки (Counter-Clockwise)
};

class DirectionSwitch {
public:
  DirectionSwitch(uint8_t pin);
  void begin();
  RotationDirection read();  // Читає поточний напрямок
  bool isCW() const { return _direction == DIR_CW; }
  bool isCCW() const { return _direction == DIR_CCW; }
  bool hasChanged();  // Перевіряє, чи змінився напрямок
  
private:
  uint8_t _pin;
  RotationDirection _direction;
  RotationDirection _lastDirection;
  unsigned long _lastReadTime;
  static const unsigned long READ_INTERVAL_MS = 50;  // Інтервал читання
};

#endif
