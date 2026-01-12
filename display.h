#ifndef DISPLAY_H
#define DISPLAY_H

#include <Arduino.h>
#include "config.h"

// Умовна компіляція для вибору бібліотеки
#if LCD_MODE == 0
  #include <LiquidCrystal.h>
#else
  #include <Wire.h>
  #include <LiquidCrystal_I2C.h>
#endif

class Display {
public:
  // Конструктор для 4-bit режиму
  Display(uint8_t rs, uint8_t enable, uint8_t d4, uint8_t d5, uint8_t d6, uint8_t d7);
  
  // Конструктор для I2C режиму
  Display(uint8_t i2cAddress, uint8_t cols, uint8_t rows);
  
  void begin();
  void update(uint32_t position, uint32_t steps360);
  void updateWithTarget(uint32_t position, uint32_t steps360, uint16_t targetAngle);
  void showMessage(const char* line0, const char* line1);
  void showAngle(uint32_t position, uint32_t steps360);
  void clear();
  
  // Відображення меню
  void showMainMenu(uint8_t selectedItem);
  void showStatusMenu(uint32_t position, uint32_t steps360, uint16_t targetAngle, bool positionReached, bool directionCCW);
  void showSetAngleMenu(uint16_t targetAngle, uint8_t digitMode);
  void showSettingsMenu();
  void showSaveMenu();
  
private:
  #if LCD_MODE == 0
    LiquidCrystal* _lcd;
  #else
    LiquidCrystal_I2C* _lcd;
  #endif
  
  uint8_t _cols;
  uint8_t _rows;
  uint16_t _lastDeg;
  uint16_t _lastTargetDeg;
  unsigned long _lastUpdate;
  bool _messageShown;
  unsigned long _messageStartTime;
  bool _isI2C;
  
  void drawFull(uint32_t position, uint32_t steps360);
  void drawAngleOnly(uint32_t position, uint32_t steps360);
  void drawWithTarget(uint32_t position, uint32_t steps360, uint16_t targetAngle);
  void printAt(uint8_t col, uint8_t row, const char* text);
  void printAt(uint8_t col, uint8_t row, uint16_t value);
  void printMenuItem(uint8_t row, uint8_t itemIndex, const char* text, bool selected);
};

#endif
