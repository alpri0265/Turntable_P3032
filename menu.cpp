#include "menu.h"

Menu::Menu() : _shouldSave(false), _targetAngle(0) {
}

int32_t Menu::angleToSteps(uint16_t angle) {
  // Конвертуємо кут (0-360) в кроки
  return (int32_t)angle * STEPS_360 / 360;
}

void Menu::syncAngleFromPosition(int32_t position) {
  // Конвертуємо позицію (кроки) в кут
  _targetAngle = (uint32_t)position * 360 / STEPS_360;
}

void Menu::update(int16_t encoderDelta, bool buttonPressed, 
                  int32_t& targetPosition, int32_t currentPos, int32_t remaining) {
  // Обробка енкодера
  if (encoderDelta != 0) {
    // Обчислюємо цільову позицію з урахуванням поточної позиції та черги кроків
    int32_t newTarget = currentPos + remaining + encoderDelta;

    // Обмежуємо позицію в межах дозволеного діапазону
    if (newTarget < MIN_POS)
      newTarget = MIN_POS;
    else if (newTarget > MAX_POS)
      newTarget = MAX_POS;

    // Обчислюємо скільки кроків потрібно додати до черги
    int32_t stepsToAdd = newTarget - currentPos - remaining;
    
    if (stepsToAdd != 0) {
      targetPosition = newTarget;
      _targetAngle = (uint32_t)newTarget * 360 / STEPS_360;
    }
  }
  
  // Обробка кнопки
  if (buttonPressed) {
    _shouldSave = true;
  }
}

void Menu::updateWithAbsoluteEncoder(uint16_t absoluteAngle, bool buttonPressed,
                                     int32_t& targetPosition, int32_t currentPos, int32_t remaining) {
  // Встановлюємо цільовий кут з абсолютного енкодера
  _targetAngle = absoluteAngle;
  
  // Конвертуємо кут в позицію (кроки)
  int32_t newTarget = angleToSteps(absoluteAngle);
  
  // Обмежуємо в межах дозволеного діапазону
  if (newTarget < MIN_POS)
    newTarget = MIN_POS;
  else if (newTarget > MAX_POS)
    newTarget = MAX_POS;
  
  // Встановлюємо цільову позицію
  targetPosition = newTarget;
  
  // Обробка кнопки
  if (buttonPressed) {
    _shouldSave = true;
  }
}
