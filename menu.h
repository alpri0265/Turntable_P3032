#ifndef MENU_H
#define MENU_H

#include <Arduino.h>
#include "config.h"
#include "direction_switch.h"

// Типи меню
enum MenuType {
  MENU_SPLASH,         // Початковий екран
  MENU_MAIN,           // Головне меню
  MENU_SET_ANGLE,      // Встановлення кута (редагування)
  MENU_SETTINGS,       // Налаштування
  MENU_SAVE            // Збереження
};

// Пункти головного меню
enum MainMenuItem {
  ITEM_HOME = 0,       // Повернення на стартовий екран
  ITEM_SET_ANGLE = 1,  // Встановлення кута
  ITEM_SETTINGS = 2,
  ITEM_SAVE = 3,
  ITEM_COUNT = 4
};

class Menu {
public:
  Menu();
  
  // Оновлення меню з інкрементальним енкодером (навігація)
  void updateNavigation(int16_t encoderDelta, bool buttonPressed);
  
  // Оновлення режиму редагування розрядів (для меню Set Angle)
  void updateDigitMode(bool digitButtonPressed);
  
  // Обробка сплеш-екрану
  void handleSplashMenu(bool buttonPressed, bool startButtonPressed);
  
  // Перевірка, чи потрібно обнулити позицію
  bool shouldResetPosition() const { return _shouldResetPosition; }
  void clearResetPositionFlag() { _shouldResetPosition = false; }
  
  // Оновлення цільового кута з абсолютного енкодера
  // Оновлює кут тільки якщо він не був встановлений вручну
  void updateTargetAngle(uint16_t absoluteAngle);
  
  // Скидання прапорця ручного встановлення (для абсолютного енкодера)
  void resetManualAngleFlag();
  
  // Встановлення цільового кута вручну (з меню)
  void setTargetAngle(uint16_t angle);
  
  // Отримання поточного типу меню
  MenuType getCurrentMenu() const { return _currentMenu; }
  
  // Отримання поточного пункту меню
  uint8_t getCurrentItem() const { return _currentItem; }
  
  // Отримання цільового кута
  uint16_t getTargetAngle() const { return _targetAngle; }
  
  // Перевірка, чи активний режим редагування кута
  bool isEditingAngle() const { return _currentMenu == MENU_SET_ANGLE; }
  
  // Отримання поточного режиму редагування розряду
  uint8_t getDigitMode() const { return _digitMode; }
  
  // Отримання цільової позиції в кроках
  int32_t getTargetPosition() const { return _targetPosition; }
  
  // Перевірка, чи потрібно зберегти
  bool shouldSave() const { return _shouldSave; }
  void clearSaveFlag() { _shouldSave = false; }
  
  // Перевірка, чи позиція відповідає цільовій
  bool isPositionReached(int32_t currentPos, int32_t remaining) const;
  
  // Перевірка, чи потрібно скинути сплеш-екран (при поверненні з меню)
  bool shouldResetSplash() const { return _shouldResetSplash; }
  void clearResetSplashFlag() { _shouldResetSplash = false; }

  // Отримання вибраного напрямку руху
  RotationDirection getDirection() const { return _selectedDirection; }
  
private:
  MenuType _currentMenu;
  uint8_t _currentItem;
  uint16_t _targetAngle;
  int32_t _targetPosition;
  bool _shouldSave;
  bool _manualAngleSet;  // Прапорець, що кут встановлений вручну
  bool _shouldResetSplash;  // Прапорець для скидання сплеш-екрану
  bool _shouldResetPosition;  // Прапорець для обнулення позиції енкодера
  uint16_t _lastAbsoluteAngle;  // Останнє значення абсолютного енкодера
  unsigned long _lastMenuChangeTime;  // Час останньої зміни пункту меню
  static const unsigned long MENU_CHANGE_DELAY_MS = 150;  // Затримка між змінами пунктів меню
  
  // Режими редагування розрядів кута
  enum DigitMode {
    DIGIT_UNITS = 0,    // Одиниці (±1)
    DIGIT_TENS = 1,     // Десятки (±10)
    DIGIT_HUNDREDS = 2  // Сотні (±100)
  };
  DigitMode _digitMode;  // Поточний режим редагування розряду
  RotationDirection _selectedDirection;  // Вибраний напрямок руху (CW/CCW)
  
  int32_t angleToSteps(uint16_t angle);
  void handleMainMenu(int16_t encoderDelta, bool buttonPressed);
  void handleSetAngleMenu(int16_t encoderDelta, bool buttonPressed);
  void handleSettingsMenu(int16_t encoderDelta, bool buttonPressed);
  void handleSaveMenu(bool buttonPressed);
};

#endif
