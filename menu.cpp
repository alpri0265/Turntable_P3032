#include "menu.h"

Menu::Menu() : _shouldSave(false) {
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
    }
  }
  
  // Обробка кнопки
  if (buttonPressed) {
    _shouldSave = true;
  }
}
