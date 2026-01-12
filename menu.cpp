#include "menu.h"

Menu::Menu() 
  : _currentMenu(MENU_MAIN), _currentItem(0), _targetAngle(0), 
    _targetPosition(0), _shouldSave(false), _manualAngleSet(false),
    _lastAbsoluteAngle(999), _lastMenuChangeTime(0), _digitMode(DIGIT_UNITS) {
}

int32_t Menu::angleToSteps(uint16_t angle) {
  // Конвертуємо кут (0-360) в кроки
  return (int32_t)angle * STEPS_360 / 360;
}

void Menu::updateTargetAngle(uint16_t absoluteAngle) {
  // Якщо активний режим редагування через меню - не оновлюємо
  if (_currentMenu == MENU_SET_ANGLE) {
    return;
  }
  
  // Якщо кут був встановлений вручну, перевіряємо чи абсолютний енкодер змінився значно
  if (_manualAngleSet) {
    // Якщо це перше читання - зберігаємо значення
    if (_lastAbsoluteAngle == 999) {
      _lastAbsoluteAngle = absoluteAngle;
      return;
    }
    
    // Обчислюємо різницю (враховуючи перехід через 0/360)
    int16_t diff1 = abs((int16_t)absoluteAngle - (int16_t)_lastAbsoluteAngle);
    int16_t diff2 = 360 - diff1;
    int16_t diff = (diff1 < diff2) ? diff1 : diff2;
    
    // Якщо абсолютний енкодер змінився більше ніж на 5 градусів - скидаємо прапорець
    if (diff > 5) {
      _manualAngleSet = false;
      _lastAbsoluteAngle = absoluteAngle;
      // Продовжуємо оновлення кута
    } else {
      // Невелика зміна - не оновлюємо кут
      return;
    }
  }
  
  // Оновлюємо кут з абсолютного енкодера
  _targetAngle = absoluteAngle;
  _lastAbsoluteAngle = absoluteAngle;
  
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

void Menu::resetManualAngleFlag() {
  // Скидаємо прапорець ручного встановлення
  // Викликається, коли користувач обертає абсолютний енкодер значно
  _manualAngleSet = false;
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
    case MENU_SET_ANGLE:
      handleSetAngleMenu(encoderDelta, buttonPressed);
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
  // Навігація по головному меню з затримкою для плавної навігації
  unsigned long now = millis();
  if (encoderDelta != 0 && (now - _lastMenuChangeTime >= MENU_CHANGE_DELAY_MS)) {
    // Обмежуємо крок до ±1 для плавної навігації
    int8_t step = (encoderDelta > 0) ? 1 : -1;
    _currentItem += step;
    
    // Обмежуємо в межах меню
    if (_currentItem < 0)
      _currentItem = ITEM_COUNT - 1;
    else if (_currentItem >= ITEM_COUNT)
      _currentItem = 0;
    
    // Запам'ятовуємо час зміни
    _lastMenuChangeTime = now;
  }
  
  // Обробка вибору пункту
  if (buttonPressed) {
    switch (_currentItem) {
      case ITEM_STATUS:
        _currentMenu = MENU_STATUS;
        break;
      case ITEM_SET_ANGLE:
        _currentMenu = MENU_SET_ANGLE;
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
  // При натисканні кнопки повертаємось до головного меню
  if (buttonPressed) {
    _currentMenu = MENU_MAIN;
    _currentItem = ITEM_STATUS;  // Встановлюємо вибраний пункт на Status
  }
}

void Menu::updateDigitMode(bool digitButtonPressed) {
  // Обробка натискання кнопки перемикання розрядів
  static bool lastButtonState = false;
  
  if (digitButtonPressed && !lastButtonState) {
    // Перемикаємо режим редагування розряду
    _digitMode = (DigitMode)((_digitMode + 1) % 3);
  }
  
  lastButtonState = digitButtonPressed;
}

void Menu::handleSetAngleMenu(int16_t encoderDelta, bool buttonPressed) {
  // Редагування кута через інкрементальний енкодер з затримкою
  unsigned long now = millis();
  
  // При натисканні кнопки енкодера - вихід з меню
  if (buttonPressed) {
    _manualAngleSet = true;
    _currentMenu = MENU_MAIN;
    _currentItem = ITEM_SET_ANGLE;
    return;
  }
  
  // Обробка обертання енкодера
  if (encoderDelta != 0 && (now - _lastMenuChangeTime >= MENU_CHANGE_DELAY_MS)) {
    // Визначаємо крок залежно від вибраного розряду
    int16_t step = 0;
    switch (_digitMode) {
      case DIGIT_UNITS:
        step = (encoderDelta > 0) ? 1 : -1;  // Одиниці: ±1
        break;
      case DIGIT_TENS:
        step = (encoderDelta > 0) ? 10 : -10;  // Десятки: ±10
        break;
      case DIGIT_HUNDREDS:
        step = (encoderDelta > 0) ? 100 : -100;  // Сотні: ±100
        break;
    }
    
    // Змінюємо кут з відповідним кроком
    int32_t newAngle = (int32_t)_targetAngle + step;
    
    // Обмежуємо в межах 0-360
    if (newAngle < 0) {
      newAngle = 360;
    } else if (newAngle > 360) {
      newAngle = 0;
    }
    
    _targetAngle = (uint16_t)newAngle;
    
    // Оновлюємо цільову позицію
    int32_t newTarget = angleToSteps(_targetAngle);
    if (newTarget < MIN_POS)
      newTarget = MIN_POS;
    else if (newTarget > MAX_POS)
      newTarget = MAX_POS;
    _targetPosition = newTarget;
    
    // Встановлюємо прапорець, що кут встановлений вручну
    _manualAngleSet = true;
    
    // Запам'ятовуємо час зміни
    _lastMenuChangeTime = now;
  }
}

void Menu::setTargetAngle(uint16_t angle) {
  // Встановлюємо кут вручну
  if (angle > 360) angle = 360;
  _targetAngle = angle;
  
  // Оновлюємо цільову позицію
  int32_t newTarget = angleToSteps(_targetAngle);
  if (newTarget < MIN_POS)
    newTarget = MIN_POS;
  else if (newTarget > MAX_POS)
    newTarget = MAX_POS;
  _targetPosition = newTarget;
  
  // Встановлюємо прапорець, що кут встановлений вручну
  _manualAngleSet = true;
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
