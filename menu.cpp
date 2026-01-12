#include "menu.h"

Menu::Menu() 
  : _currentMenu(MENU_MAIN), _currentItem(0), _targetAngle(0), 
    _targetPosition(0), _shouldSave(false) {
}

int32_t Menu::angleToSteps(uint16_t angle) {
  // Конвертуємо кут (0-360) в кроки
  return (int32_t)angle * STEPS_360 / 360;
}

void Menu::updateTargetAngle(uint16_t absoluteAngle) {
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
  _targetPosition = newTarget;
}

void Menu::updateNavigation(int16_t encoderDelta, bool buttonPressed) {
  // Обробка навігації залежно від поточного меню
  switch (_currentMenu) {
    case MENU_MAIN:
      handleMainMenu(encoderDelta, buttonPressed);
      break;
    case MENU_STATUS:
      handleStatusMenu(buttonPressed);
      break;
    case MENU_SETTINGS:
      handleSettingsMenu(encoderDelta, buttonPressed);
      break;
    case MENU_SAVE:
      handleSaveMenu(buttonPressed);
      break;
  }
}

void Menu::handleMainMenu(int16_t encoderDelta, bool buttonPressed) {
  // Навігація по головному меню
  if (encoderDelta != 0) {
    _currentItem += encoderDelta;
    
    // Обмежуємо в межах меню
    if (_currentItem < 0)
      _currentItem = ITEM_COUNT - 1;
    else if (_currentItem >= ITEM_COUNT)
      _currentItem = 0;
  }
  
  // Обробка вибору пункту
  if (buttonPressed) {
    switch (_currentItem) {
      case ITEM_STATUS:
        _currentMenu = MENU_STATUS;
        break;
      case ITEM_SETTINGS:
        _currentMenu = MENU_SETTINGS;
        break;
      case ITEM_SAVE:
        _currentMenu = MENU_SAVE;
        break;
    }
  }
}

void Menu::handleStatusMenu(bool buttonPressed) {
  // Повернення до головного меню
  if (buttonPressed) {
    _currentMenu = MENU_MAIN;
    _currentItem = ITEM_STATUS;
  }
}

void Menu::handleSettingsMenu(int16_t encoderDelta, bool buttonPressed) {
  // Налаштування (можна розширити)
  if (buttonPressed) {
    _currentMenu = MENU_MAIN;
    _currentItem = ITEM_SETTINGS;
  }
}

void Menu::handleSaveMenu(bool buttonPressed) {
  // Підтвердження збереження
  if (buttonPressed) {
    _shouldSave = true;
    _currentMenu = MENU_MAIN;
    _currentItem = ITEM_SAVE;
  }
}

bool Menu::isPositionReached(int32_t currentPos, int32_t remaining) const {
  // Перевіряємо, чи поточна позиція відповідає цільовій
  int32_t effectivePos = currentPos + remaining;
  int32_t diff = abs(effectivePos - _targetPosition);
  
  // Вважаємо досягнутою, якщо різниця менше 2 кроків
  return diff < 2;
}
