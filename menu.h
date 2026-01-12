#ifndef MENU_H
#define MENU_H

#include <Arduino.h>
#include "config.h"

// Типи меню
enum MenuType {
  MENU_MAIN,           // Головне меню
  MENU_STATUS,         // Статус системи
  MENU_SETTINGS,       // Налаштування
  MENU_SAVE            // Збереження
};

// Пункти головного меню
enum MainMenuItem {
  ITEM_STATUS = 0,
  ITEM_SETTINGS = 1,
  ITEM_SAVE = 2,
  ITEM_COUNT = 3
};

class Menu {
public:
  Menu();
  
  // Оновлення меню з інкрементальним енкодером (навігація)
  void updateNavigation(int16_t encoderDelta, bool buttonPressed);
  
  // Оновлення цільового кута з абсолютного енкодера
  void updateTargetAngle(uint16_t absoluteAngle);
  
  // Отримання поточного типу меню
  MenuType getCurrentMenu() const { return _currentMenu; }
  
  // Отримання поточного пункту меню
  uint8_t getCurrentItem() const { return _currentItem; }
  
  // Отримання цільового кута
  uint16_t getTargetAngle() const { return _targetAngle; }
  
  // Отримання цільової позиції в кроках
  int32_t getTargetPosition() const { return _targetPosition; }
  
  // Перевірка, чи потрібно зберегти
  bool shouldSave() const { return _shouldSave; }
  void clearSaveFlag() { _shouldSave = false; }
  
  // Перевірка, чи позиція відповідає цільовій
  bool isPositionReached(int32_t currentPos, int32_t remaining) const;
  
private:
  MenuType _currentMenu;
  uint8_t _currentItem;
  uint16_t _targetAngle;
  int32_t _targetPosition;
  bool _shouldSave;
  
  int32_t angleToSteps(uint16_t angle);
  void handleMainMenu(int16_t encoderDelta, bool buttonPressed);
  void handleStatusMenu(bool buttonPressed);
  void handleSettingsMenu(int16_t encoderDelta, bool buttonPressed);
  void handleSaveMenu(bool buttonPressed);
};

#endif
