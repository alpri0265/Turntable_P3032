#ifndef STEPPER_H
#define STEPPER_H

#include <Arduino.h>
#include "config.h"

class Stepper {
public:
  Stepper(uint8_t stepPin, uint8_t dirPin);
  void begin();
  void update();  // Неблокуюче оновлення
  void move(int32_t steps);  // Додає кроки до черги
  void setPosition(int32_t position);  // Встановлює поточну позицію
  void setDirectionInvert(bool invert);  // Інвертує напрямок руху
  int32_t getPosition() const { return _position; }
  int32_t getRemaining() const { return _remaining; }
  bool isDirectionInverted() const { return _directionInvert; }
  
private:
  uint8_t _stepPin;
  uint8_t _dirPin;
  int32_t _position;
  int32_t _remaining;
  unsigned long _lastStepTime;
  int8_t _currentDir;
  bool _directionInvert;  // Інверсія напрямку
  
  void doStep();
  int8_t getPhysicalDirection(int32_t steps);  // Отримує фізичний напрямок з урахуванням інверсії
};

#endif
