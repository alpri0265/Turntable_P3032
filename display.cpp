#include "display.h"
#include "config.h"
#include <string.h>

// Глобальна змінна для скидання сплеш-екрану
static bool _splashNeedReset = false;

#if LCD_MODE == 0
// Конструктор для 4-bit режиму
Display::Display(uint8_t rs, uint8_t enable, uint8_t d4, uint8_t d5, uint8_t d6, uint8_t d7)
  : _lastDeg(999), _lastTargetDeg(999),
    _lastUpdate(0), _messageShown(false), _messageStartTime(0), _isI2C(false) {
  _cols = (LCD_TYPE == 1) ? 16 : 20;
  _rows = (LCD_TYPE == 1) ? 2 : 4;
  _lcd = new LiquidCrystal(rs, enable, d4, d5, d6, d7);
}
#else
// Конструктор для I2C режиму
Display::Display(uint8_t i2cAddress, uint8_t cols, uint8_t rows)
  : _cols(cols), _rows(rows), _lastDeg(999), _lastTargetDeg(999),
    _lastUpdate(0), _messageShown(false), _messageStartTime(0), _isI2C(true) {
  _lcd = new LiquidCrystal_I2C(i2cAddress, cols, rows);
}
#endif

void Display::begin() {
  _lcd->begin(_cols, _rows);
  _lcd->clear();
  #if LCD_MODE == 1
  _lcd->backlight();
  #endif
}

void Display::printAt(uint8_t col, uint8_t row, const char* text) {
  _lcd->setCursor(col, row);
  _lcd->print(text);
}

void Display::printAt(uint8_t col, uint8_t row, uint16_t value) {
  _lcd->setCursor(col, row);
  if (value < 100) _lcd->print(' ');
  if (value < 10)  _lcd->print(' ');
  _lcd->print(value);
}

void Display::update(uint32_t position, uint32_t steps360) {
  unsigned long now = millis();
  
  // Перевіряємо, чи потрібно приховати повідомлення
  if (_messageShown && (now - _messageStartTime > SAVE_MESSAGE_MS)) {
    _messageShown = false;
    drawAngleOnly(position, steps360);
    _lastDeg = (uint32_t)position * 360 / steps360;
  }
  
  // Оновлюємо кут, якщо змінився
  if (!_messageShown && (now - _lastUpdate > LCD_UPDATE_MS)) {
    uint16_t deg = (uint32_t)position * 360 / steps360;
    if (_lastDeg != deg) {
      drawAngleOnly(position, steps360);
      _lastDeg = deg;
    }
    _lastUpdate = now;
  }
}

void Display::showMessage(const char* line0, const char* line1) {
  _lcd->setCursor(0, 0);
  _lcd->print(line0);
  _lcd->setCursor(0, 1);
  _lcd->print(line1);
  _messageShown = true;
  _messageStartTime = millis();
}

void Display::showAngle(uint32_t position, uint32_t steps360) {
  drawFull(position, steps360);
  _lastDeg = (uint32_t)position * 360 / steps360;
  _lastUpdate = millis();
}

void Display::clear() {
  _lcd->clear();
}

void Display::drawFull(uint32_t position, uint32_t steps360) {
  uint16_t deg = (uint32_t)position * 360 / steps360;

  _lcd->setCursor(0, 0);
  _lcd->print("Manual mode");
  if (_cols >= 20) _lcd->print("        ");

  _lcd->setCursor(0, 1);
  _lcd->print("Angle: ");
  printAt(7, 1, deg);
  _lcd->print((char)223); // °
  if (_cols >= 20) _lcd->print("         ");
}

void Display::drawAngleOnly(uint32_t position, uint32_t steps360) {
  uint16_t deg = (uint32_t)position * 360 / steps360;

  printAt(7, 1, deg);
  _lcd->print((char)223); // °
  if (_cols >= 20) _lcd->print("         ");
}

void Display::updateWithTarget(uint32_t position, uint32_t steps360, uint16_t targetAngle) {
  unsigned long now = millis();
  
  // Перевіряємо, чи потрібно приховати повідомлення
  if (_messageShown && (now - _messageStartTime > SAVE_MESSAGE_MS)) {
    _messageShown = false;
    drawWithTarget(position, steps360, targetAngle);
    _lastDeg = (uint32_t)position * 360 / steps360;
    _lastTargetDeg = targetAngle;
  }
  
  // Оновлюємо, якщо змінився кут або цільовий кут
  if (!_messageShown && (now - _lastUpdate > LCD_UPDATE_MS)) {
    uint16_t deg = (uint32_t)position * 360 / steps360;
    if (_lastDeg != deg || _lastTargetDeg != targetAngle) {
      drawWithTarget(position, steps360, targetAngle);
      _lastDeg = deg;
      _lastTargetDeg = targetAngle;
    }
    _lastUpdate = now;
  }
}

void Display::drawWithTarget(uint32_t position, uint32_t steps360, uint16_t targetAngle) {
  uint16_t currentDeg = (uint32_t)position * 360 / steps360;

  if (_rows >= 4) {
    // Для LCD2004 (20x4) - використовуємо більше місця
    // Рядок 0: Заголовок
    _lcd->setCursor(0, 0);
    _lcd->print("Turntable Control");
    if (_cols >= 20) _lcd->print("    ");
    
    // Рядок 1: Поточний кут
    _lcd->setCursor(0, 1);
    _lcd->print("Current: ");
    printAt(9, 1, currentDeg);
    _lcd->print((char)223); // °
    if (_cols >= 20) _lcd->print("      ");
    
    // Рядок 2: Цільовий кут
    _lcd->setCursor(0, 2);
    _lcd->print("Target:  ");
    printAt(9, 2, targetAngle);
    _lcd->print((char)223); // °
    if (_cols >= 20) _lcd->print("      ");
    
    // Рядок 3: Додаткова інформація (позиція в кроках)
    _lcd->setCursor(0, 3);
    _lcd->print("Steps: ");
    _lcd->print(position);
    if (_cols >= 20) {
      _lcd->print(" / ");
      _lcd->print(STEPS_360);
    }
  } else {
    // Для LCD1602 (16x2) - компактний вигляд
    // Перший рядок: поточний кут
    _lcd->setCursor(0, 0);
    _lcd->print("Cur: ");
    printAt(5, 0, currentDeg);
    _lcd->print((char)223); // °
    _lcd->print("  ");

    // Другий рядок: цільовий кут
    _lcd->setCursor(0, 1);
    _lcd->print("Tgt: ");
    printAt(5, 1, targetAngle);
    _lcd->print((char)223); // °
    _lcd->print("  ");
  }
}

void Display::printMenuItem(uint8_t row, uint8_t itemIndex, const char* text, bool selected) {
  _lcd->setCursor(0, row);
  if (selected) {
    _lcd->print(">");
  } else {
    _lcd->print(" ");
  }
  _lcd->print(text);
  // Очищаємо решту рядка
  uint8_t len = strlen(text);
  for (uint8_t i = len + 1; i < _cols; i++) {
    _lcd->print(" ");
  }
}

void Display::resetSplashScreen() {
  // Встановлюємо прапорець для скидання сплеш-екрану
  _splashNeedReset = true;
}

void Display::showSplashScreen(uint16_t encoderAngle, uint16_t targetAngle, bool isRunning) {
  static uint16_t lastEncoderAngle = 65535;
  static uint16_t lastTargetAngle = 65535;
  static bool lastIsRunning = false;
  static bool firstDisplay = true;
  
  // Якщо потрібно скинути (викликано resetSplashScreen) - робимо це
  if (_splashNeedReset) {
    firstDisplay = true;
    _splashNeedReset = false;
    lastEncoderAngle = 65535;
    lastTargetAngle = 65535;
    lastIsRunning = !isRunning;
  }

  if (firstDisplay) {
    _lcd->clear();
    firstDisplay = false;
    lastEncoderAngle = 65535; // Примусово оновити
    lastTargetAngle = 65535;
    lastIsRunning = !isRunning;
  }

  if (_rows >= 4) {
    // LCD2004
    // Заголовок
    if (lastEncoderAngle == 65535) {
      _lcd->setCursor(0, 0);
      _lcd->print("Turntable Ready");
      if (_cols >= 20) _lcd->print("      ");
    }

    // Кут з абсолютного енкодера (поточний стан)
    if (lastEncoderAngle == 65535) {
      _lcd->setCursor(0, 1);
      _lcd->print("Encoder: ");
    }
    if (lastEncoderAngle != encoderAngle) {
      printAt(9, 1, encoderAngle);
      _lcd->print((char)223);
      if (_cols >= 20) _lcd->print("      ");
      lastEncoderAngle = encoderAngle;
    }

    // Цільовий кут (встановлений для руху)
    if (lastTargetAngle == 65535) {
      _lcd->setCursor(0, 2);
      _lcd->print("Target: ");
    }
    if (lastTargetAngle != targetAngle) {
      printAt(8, 2, targetAngle);
      _lcd->print((char)223);
      if (_cols >= 20) _lcd->print("      ");
      lastTargetAngle = targetAngle;
    }

    // Стан та інструкції
    if (lastIsRunning != isRunning) {
      _lcd->setCursor(0, 3);
      if (isRunning) {
        _lcd->print("Status: RUNNING");
      } else {
        _lcd->print("Btn: Start/Menu");
      }
      if (_cols >= 20) _lcd->print("    ");
      lastIsRunning = isRunning;
    }
  } else {
    // LCD1602
    // Кут з абсолютного енкодера
    if (lastEncoderAngle == 65535 || lastEncoderAngle != encoderAngle) {
      _lcd->setCursor(0, 0);
      _lcd->print("Enc: ");
      printAt(5, 0, encoderAngle);
      _lcd->print((char)223);
      lastEncoderAngle = encoderAngle;
    }

    // Цільовий кут та стан
    if (lastTargetAngle == 65535 || lastTargetAngle != targetAngle || lastIsRunning != isRunning) {
      _lcd->setCursor(0, 1);
      _lcd->print("Tgt: ");
      printAt(5, 1, targetAngle);
      _lcd->print((char)223);
      if (isRunning) {
        _lcd->print(" RUN");
      } else {
        _lcd->print(" STOP");
      }
      lastTargetAngle = targetAngle;
      lastIsRunning = isRunning;
    }
  }
}

void Display::showMainMenu(uint8_t selectedItem) {
  static uint8_t lastSelectedItem = 255;
  
  // Оновлюємо тільки якщо змінився вибраний пункт
  if (lastSelectedItem != selectedItem) {
    _lcd->clear();
    lastSelectedItem = selectedItem;
  }
  
  if (_rows >= 4) {
    // LCD2004 - показуємо всі пункти
    _lcd->setCursor(0, 0);
    _lcd->print("Main Menu");
    if (_cols >= 20) _lcd->print("           ");
    
    printMenuItem(1, 0, "Home", selectedItem == 0);
    printMenuItem(2, 1, "Set Angle", selectedItem == 1);
    
    // Рядок 3: показуємо Settings або Save Position залежно від вибору
    if (selectedItem == 3) {
      // Якщо вибрано Save Position - показуємо його на рядку 3 замість Settings
      printMenuItem(3, 3, "Save Position", true);
    } else {
      // Інакше завжди показуємо Settings на рядку 3
      printMenuItem(3, 2, "Settings", selectedItem == 2);
    }
  } else {
    // LCD1602 - показуємо по 2 пункти
    if (selectedItem == 0) {
      printMenuItem(0, 0, "Home", true);
      printMenuItem(1, 1, "Set Angle", false);
    } else if (selectedItem == 1) {
      printMenuItem(0, 0, "Home", false);
      printMenuItem(1, 1, "Set Angle", true);
    } else if (selectedItem == 2) {
      printMenuItem(0, 1, "Set Angle", false);
      printMenuItem(1, 2, "Settings", true);
    } else {
      printMenuItem(0, 2, "Settings", false);
      printMenuItem(1, 3, "Save Position", true);
    }
  }
}

void Display::showSetAngleMenu(uint16_t targetAngle, uint8_t digitMode) {
  static uint16_t lastTargetAngle = 65535;
  static uint8_t lastDigitMode = 255;
  static bool firstDisplay = true;
  
  // Оновлюємо тільки якщо змінився кут, режим розряду або це перший виклик
  if (firstDisplay || lastTargetAngle != targetAngle || lastDigitMode != digitMode) {
    if (firstDisplay) {
      _lcd->clear();
      firstDisplay = false;
    }
    lastTargetAngle = targetAngle;
    lastDigitMode = digitMode;
  } else {
    return; // Нічого не змінилося
  }
  
  // Визначаємо назву режиму розряду
  const char* modeName = "";
  switch (digitMode) {
    case 0: modeName = "Units"; break;
    case 1: modeName = "Tens"; break;
    case 2: modeName = "Hundreds"; break;
  }
  
  if (_rows >= 4) {
    // LCD2004
    _lcd->setCursor(0, 0);
    _lcd->print("Set Target Angle");
    if (_cols >= 20) _lcd->print("    ");
    
    _lcd->setCursor(0, 1);
    _lcd->print("Target: ");
    printAt(8, 1, targetAngle);
    _lcd->print((char)223);
    if (_cols >= 20) _lcd->print("      ");
    
    _lcd->setCursor(0, 2);
    _lcd->print("Mode: ");
    _lcd->print(modeName);
    if (_cols >= 20) {
      for (uint8_t i = 6 + strlen(modeName); i < _cols; i++) {
        _lcd->print(" ");
      }
    }
    
    _lcd->setCursor(0, 3);
    _lcd->print("Btn: Change mode");
    if (_cols >= 20) _lcd->print("     ");
  } else {
    // LCD1602
    _lcd->setCursor(0, 0);
    _lcd->print("Tgt: ");
    printAt(5, 0, targetAngle);
    _lcd->print((char)223);
    _lcd->print(" ");
    _lcd->print(modeName);
    
    _lcd->setCursor(0, 1);
    _lcd->print("Btn: Change mode");
  }
}

void Display::showSettingsMenu(uint8_t direction) {
  // Екран вже очищено в Turntable_P3032.ino при переході в меню
  // Тут просто відображаємо вміст
  
  static uint8_t lastDirection = 255;
  bool directionChanged = (lastDirection != direction);
  lastDirection = direction;
  
  if (_rows >= 4) {
    // LCD2004
    _lcd->setCursor(0, 0);
    _lcd->print("Settings");
    if (_cols >= 20) _lcd->print("            ");
    
    _lcd->setCursor(0, 1);
    _lcd->print("Direction:");
    if (_cols >= 20) _lcd->print("          ");
    
    // Оновлюємо напрямок тільки якщо змінився
    if (directionChanged) {
      _lcd->setCursor(0, 2);
      if (direction == 0) {
        _lcd->print("> CW  (Clockwise)");
        if (_cols >= 20) _lcd->print("  ");
      } else {
        _lcd->print("> CCW (Counter-CW)");
        if (_cols >= 20) _lcd->print("");
      }
    }
    
    _lcd->setCursor(0, 3);
    _lcd->print("Rotate to change");
    if (_cols >= 20) _lcd->print("    ");
  } else {
    // LCD1602
    if (directionChanged) {
      _lcd->setCursor(0, 0);
      _lcd->print("Direction: ");
      if (direction == 0) {
        _lcd->print("CW ");
      } else {
        _lcd->print("CCW");
      }
    }
    _lcd->setCursor(0, 1);
    _lcd->print("Rotate to change");
  }
}

void Display::showSaveMenu() {
  // Екран вже очищено в Turntable_P3032.ino при переході в меню
  // Тут просто відображаємо вміст
  
  if (_rows >= 4) {
    _lcd->setCursor(0, 0);
    _lcd->print("Save Position");
    if (_cols >= 20) _lcd->print("        ");
    
    _lcd->setCursor(0, 1);
    _lcd->print("Current position");
    if (_cols >= 20) _lcd->print("     ");
    
    _lcd->setCursor(0, 2);
    _lcd->print("will be saved");
    if (_cols >= 20) _lcd->print("       ");
    
    _lcd->setCursor(0, 3);
    _lcd->print("Press to confirm");
    if (_cols >= 20) _lcd->print("    ");
  } else {
    _lcd->setCursor(0, 0);
    _lcd->print("Save Position");
    _lcd->setCursor(0, 1);
    _lcd->print("Press to confirm");
  }
}
