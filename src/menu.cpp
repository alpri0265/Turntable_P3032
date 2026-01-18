#include "menu.h"

Menu::Menu()
  : _currentMenu(MENU_SPLASH), _currentItem(0), _targetAngle(0),
    _targetPosition(0), _shouldSave(false), _manualAngleSet(false),
    _shouldResetSplash(false), _shouldResetPosition(false), _lastAbsoluteAngle(999), _lastMenuChangeTime(0), _digitMode(DIGIT_UNITS), _selectedDirection(DIR_CW), _stepperZeroPosition(0) {
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
  
  // Якщо кут був встановлений вручну - НЕ оновлюємо з абсолютного енкодера
  // Кут залишається таким, який був встановлений вручну, поки користувач не скине прапорець
  if (_manualAngleSet) {
    // Просто оновлюємо останнє значення абсолютного енкодера для відстеження
    if (_lastAbsoluteAngle == 999) {
      _lastAbsoluteAngle = absoluteAngle;
    }
    // Не оновлюємо цільовий кут - залишаємо встановлений вручну
    // Але оновлюємо цільову позицію на основі збереженого кута
    int32_t angleInSteps = angleToSteps(_targetAngle);
    int32_t newTarget = _stepperZeroPosition + angleInSteps;
    while (newTarget < 0) newTarget += STEPS_360;
    while (newTarget >= STEPS_360) newTarget -= STEPS_360;
    _targetPosition = newTarget;
    return;
  }
  
  // Оновлюємо кут з абсолютного енкодера
  _targetAngle = absoluteAngle;
  _lastAbsoluteAngle = absoluteAngle;
  
  // Конвертуємо кут в позицію (кроки) відносно нульової позиції
  int32_t angleInSteps = angleToSteps(absoluteAngle);
  int32_t newTarget = _stepperZeroPosition + angleInSteps;
  
  // Нормалізуємо до діапазону 0-360 градусів
  while (newTarget < 0) {
    newTarget += STEPS_360;
  }
  while (newTarget >= STEPS_360) {
    newTarget -= STEPS_360;
  }
  
  // Встановлюємо цільову позицію
  _targetPosition = newTarget;
}

void Menu::resetManualAngleFlag() {
  // Скидаємо прапорець ручного встановлення
  // Викликається, коли користувач обертає абсолютний енкодер значно
  _manualAngleSet = false;
}

void Menu::setDirection(RotationDirection direction) {
  _selectedDirection = direction;
}

void Menu::setStepperZeroPosition(int32_t zeroPosition) {
  // Встановлюємо нульову позицію двигуна (відносно якої обчислюється цільовий кут)
  // Нормалізуємо до діапазону 0-360 градусів
  while (zeroPosition < 0) {
    zeroPosition += STEPS_360;
  }
  while (zeroPosition >= STEPS_360) {
    zeroPosition -= STEPS_360;
  }
  _stepperZeroPosition = zeroPosition;
}

void Menu::handleSplashMenu(bool buttonPressed, bool startButtonPressed) {
  // Кнопка енкодера - перехід в головне меню
  if (buttonPressed) {
    _currentMenu = MENU_MAIN;
    _currentItem = 0;
    return;
  }
  
  // Кнопка старт-стоп - запуск двигуна (якщо не запущений)
  if (startButtonPressed) {
    // Логіка старту обробляється в Turntable_P3032.ino через startStop.setState()
    // Тут просто залишаємося на сплеш-екрані
  }
}

void Menu::updateNavigation(int16_t encoderDelta, bool buttonPressed) {
  // Обробка навігації залежно від поточного меню
  switch (_currentMenu) {
    case MENU_SPLASH:
      // Сплаш-екран обробляється окремо через handleSplashMenu
      break;
      
    case MENU_MAIN:
      handleMainMenu(encoderDelta, buttonPressed);
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
    // Скидаємо затримку при натисканні кнопки для негайної реакції
    _lastMenuChangeTime = now;
    
    switch (_currentItem) {
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
  
  // При натисканні кнопки енкодера - повернення на стартовий екран
  if (buttonPressed) {
    _manualAngleSet = true;
    _currentMenu = MENU_SPLASH;
    _currentItem = 0;
    _shouldResetSplash = true;  // Встановлюємо прапорець для скидання сплеш-екрану
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
    
    // Обмежуємо в межах 0-359
    if (newAngle < 0) {
      newAngle = 359;
    } else if (newAngle >= 360) {
      newAngle = 0;
    }
    
    _targetAngle = (uint16_t)newAngle;
    
    // Оновлюємо цільову позицію відносно нульової позиції
    int32_t angleInSteps = angleToSteps(_targetAngle);
    int32_t newTarget = _stepperZeroPosition + angleInSteps;
    
    // Нормалізуємо до діапазону 0-360 градусів
    while (newTarget < 0) {
      newTarget += STEPS_360;
    }
    while (newTarget >= STEPS_360) {
      newTarget -= STEPS_360;
    }
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
  
  // Оновлюємо цільову позицію відносно нульової позиції
  int32_t angleInSteps = angleToSteps(_targetAngle);
  int32_t newTarget = _stepperZeroPosition + angleInSteps;
  
  // Нормалізуємо до діапазону 0-360 градусів
  while (newTarget < 0) {
    newTarget += STEPS_360;
  }
  while (newTarget >= STEPS_360) {
    newTarget -= STEPS_360;
  }
  _targetPosition = newTarget;
  
  // Встановлюємо прапорець, що кут встановлений вручну
  _manualAngleSet = true;
}

void Menu::handleSettingsMenu(int16_t encoderDelta, bool buttonPressed) {
  // Обробка перемикання напрямку (CW/CCW)
  unsigned long now = millis();
  
  if (encoderDelta != 0 && (now - _lastMenuChangeTime >= MENU_CHANGE_DELAY_MS)) {
    // Перемикаємо напрямок
    _selectedDirection = (_selectedDirection == DIR_CW) ? DIR_CCW : DIR_CW;
    _lastMenuChangeTime = now;
  }
  
  // При натисканні кнопки повертаємось на стартовий екран
  if (buttonPressed) {
    _currentMenu = MENU_SPLASH;
    _currentItem = 0;
    _shouldResetSplash = true;  // Встановлюємо прапорець для скидання сплеш-екрану
    _lastMenuChangeTime = now;
  }
}

void Menu::handleSaveMenu(bool buttonPressed) {
  // При натисканні кнопки зберігаємо позицію та повертаємось на стартовий екран
  if (buttonPressed) {
    _shouldSave = true;
    _currentMenu = MENU_SPLASH;
    _currentItem = 0;
    _shouldResetSplash = true;  // Встановлюємо прапорець для скидання сплеш-екрану
    _lastMenuChangeTime = millis();
  }
}

bool Menu::isPositionReached(int32_t currentPos, int32_t remaining) const {
  // Перевіряємо, чи поточна позиція відповідає цільовій
  int32_t effectivePos = currentPos + remaining;
  int32_t diff = abs(effectivePos - _targetPosition);
  
  // Вважаємо досягнутою, якщо різниця менше 2 кроків
  return diff < 2;
}

void Menu::handleLongPress() {
  // Довге натискання в будь-якому меню (крім сплеш-екрану) - повертаємося на сплеш-екран
  if (_currentMenu != MENU_SPLASH) {
    // Зберігаємо інформацію про те, чи були в меню Set Angle (перед зміною меню)
    bool wasInSetAngle = (_currentMenu == MENU_SET_ANGLE);
    
    _currentMenu = MENU_SPLASH;
    _currentItem = 0;
    _shouldResetSplash = true;
    _lastMenuChangeTime = millis();
    
    // Зберігаємо встановлений вручну кут, якщо були в меню Set Angle
    if (wasInSetAngle) {
      // _manualAngleSet залишається true
    }
  }
}
