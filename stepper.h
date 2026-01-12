#ifndef STEPPER_H
#define STEPPER_H

#include <Arduino.h>
#include "config.h"

class Stepper {
public:
  Stepper(uint8_t stepPin, uint8_t dirPin, uint8_t enablePin);
  void begin();
  void update();  // Неблокуюче оновлення
  void move(int32_t steps);  // Додає кроки до черги
  void setPosition(int32_t position);  // Встановлює поточну позицію
  void setDirectionInvert(bool invert);  // Інвертує напрямок руху
  void setEnabled(bool enabled);  // Встановлює утримання двигуна (true = утримується, false = знято з утримання)
  bool isEnabled() const { return _enabled; }  // Повертає стан утримання
  int32_t getPosition() const { return _position; }
  int32_t getRemaining() const { return _remaining; }
  bool isDirectionInverted() const { return _directionInvert; }
  void setDistanceToTarget(int32_t steps);  // Встановлює відстань до цілі для заспілення
  
private:
  uint8_t _stepPin;
  uint8_t _dirPin;
  uint8_t _enablePin;
  bool _enabled;  // Стан утримання (true = утримується, false = знято)
  int32_t _position;
  int32_t _remaining;
  unsigned long _lastStepTime;
  unsigned long _currentStepDelay;  // Поточна затримка між кроками
  int8_t _currentDir;
  bool _directionInvert;  // Інверсія напрямку
  int32_t _distanceToTarget;  // Відстань до цілі для заспілення
  static const unsigned long STEP_DELAY_MIN_US = 400;  // Мінімальна затримка (максимальна швидкість)
  static const unsigned long STEP_DELAY_MAX_US = 2000;  // Максимальна затримка (мінімальна швидкість)
  static const unsigned long STEP_DELAY_ACCEL_US = 1500;  // Початкова затримка при старті
  static const int32_t DECEL_START_STEPS = 160;  // Почати заспілення за ~18 градусів (160 кроків при 16 микростепах)
  
  void doStep();
  int8_t getPhysicalDirection(int32_t steps);  // Отримує фізичний напрямок з урахуванням інверсії
  void updateStepDelay();  // Оновлює затримку для прискорення/заспілення
};

#endif
