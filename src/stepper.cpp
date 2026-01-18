#include "stepper.h"

Stepper::Stepper(uint8_t stepPin, uint8_t dirPin, uint8_t enablePin)
  : _stepPin(stepPin), _dirPin(dirPin), _enablePin(enablePin), _position(0), 
    _remaining(0), _lastStepTime(0), _currentStepDelay(STEP_DELAY_ACCEL_US), _currentDir(0), 
    _directionInvert(false), _distanceToTarget(0), _enabled(true) {
}

void Stepper::begin() {
  pinMode(_stepPin, OUTPUT);
  pinMode(_dirPin, OUTPUT);
  pinMode(_enablePin, OUTPUT);
  digitalWrite(_stepPin, LOW);
  digitalWrite(_dirPin, LOW);
  // ENABLE активний низьким рівнем (LOW = утримується, HIGH = знято з утримання)
  digitalWrite(_enablePin, LOW);  // Початково утримується
  _enabled = true;
}

void Stepper::setEnabled(bool enabled) {
  _enabled = enabled;
  // ENABLE активний низьким рівнем: LOW = утримується, HIGH = знято з утримання
  digitalWrite(_enablePin, enabled ? LOW : HIGH);
}

void Stepper::setPosition(int32_t position) {
  // Нормалізуємо позицію до діапазону 0-360 градусів (0-STEPS_360)
  while (position < MIN_POS) {
    position += STEPS_360;
  }
  while (position >= STEPS_360) {
    position -= STEPS_360;
  }
  _position = position;
  _remaining = 0;
  _currentStepDelay = STEP_DELAY_ACCEL_US;  // Скидаємо затримку до початкової
}

void Stepper::setDirectionInvert(bool invert) {
  _directionInvert = invert;
}

void Stepper::setDistanceToTarget(int32_t steps) {
  _distanceToTarget = abs(steps);
}

int8_t Stepper::getPhysicalDirection(int32_t steps) {
  // Визначаємо логічний напрямок
  int8_t logicalDir = (steps > 0) ? 1 : -1;
  
  // Застосовуємо інверсію, якщо потрібно
  if (_directionInvert) {
    logicalDir = -logicalDir;
  }
  
  return logicalDir;
}

void Stepper::update() {
  if (_remaining == 0) {
    _currentStepDelay = STEP_DELAY_ACCEL_US;  // Скидаємо затримку при зупинці
    return;
  }
  
  // Оновлюємо затримку для прискорення/заспілення
  updateStepDelay();
  
  unsigned long now = micros();
  
  // Перевіряємо, чи минуло достатньо часу для наступного кроку
  if (now - _lastStepTime >= _currentStepDelay) {
    doStep();
    _lastStepTime = now;
  }
}

void Stepper::move(int32_t steps) {
  if (steps == 0) return;
  _remaining += steps;
  _currentStepDelay = STEP_DELAY_ACCEL_US;  // Починаємо з початкової затримки (прискорення)
  if (_lastStepTime == 0) {
    _lastStepTime = micros();
  }
}

void Stepper::updateStepDelay() {
  // Перевіряємо, чи потрібно заспілення
  int32_t remainingAbs = abs(_remaining);
  
  if (_distanceToTarget > 0 && remainingAbs <= DECEL_START_STEPS) {
    // Заспілення: збільшуємо затримку при наближенні до цілі
    // Лінійне збільшення затримки від мінімуму до максимуму
    // Використовуємо цілочисельну арифметику
    unsigned long delayRange = STEP_DELAY_MAX_US - STEP_DELAY_MIN_US;
    unsigned long decelFactor = (remainingAbs * 1000) / DECEL_START_STEPS;  // 0-1000
    if (decelFactor > 1000) decelFactor = 1000;
    _currentStepDelay = STEP_DELAY_MIN_US + (delayRange * (1000 - decelFactor)) / 1000;
  } else if (_currentStepDelay > STEP_DELAY_MIN_US) {
    // Прискорення: зменшуємо затримку до мінімуму
    _currentStepDelay -= 10;  // Поступове зменшення
    if (_currentStepDelay < STEP_DELAY_MIN_US) {
      _currentStepDelay = STEP_DELAY_MIN_US;
    }
  } else {
    _currentStepDelay = STEP_DELAY_MIN_US;  // Максимальна швидкість
  }
}

void Stepper::doStep() {
  // Визначаємо фізичний напрямок з урахуванням інверсії
  int8_t newDir = getPhysicalDirection(_remaining);
  
  // Встановлюємо напрямок (тільки якщо змінився)
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
  
  // Оновлюємо позицію (логічно, без інверсії)
  if (_remaining > 0) {
    _remaining--;
    _position++;
  } else {
    _remaining++;
    _position--;
  }
  
  // Нормалізуємо позицію до діапазону 0-360 градусів (0-STEPS_360)
  if (_position < MIN_POS) {
    _position += STEPS_360;  // Додаємо один повний оберт
  } else if (_position >= STEPS_360) {
    _position -= STEPS_360;  // Віднімаємо один повний оберт
  }
}
