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
  
  // Затримка для ініціалізації дисплея (особливо важливо для I2C)
  delay(50);
  
  // Створюємо кастомний символ градуса (0) в CGRAM
  // Символ градуса: маленьке коло вгорі
  uint8_t degreeChar[8] = {
    0b01100,  //  **
    0b10010,  // *  *
    0b10010,  // *  *
    0b01100,  //  **
    0b00000,  //
    0b00000,  //
    0b00000,  //
    0b00000   //
  };
  _lcd->createChar(0, degreeChar);
  
  // Затримка після createChar (може зміщувати курсор)
  delay(10);
  
  _lcd->clear();
  
  // Затримка після clear (особливо важливо для I2C)
  delay(10);
  
  // Явно встановлюємо курсор на початок після clear
  _lcd->setCursor(0, 0);
  
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

void Display::printAt(uint8_t col, uint8_t row, float value, uint8_t decimals) {
  _lcd->setCursor(col, row);
  // Обмежуємо значення до 0-360
  if (value < 0.0) value = 0.0;
  if (value >= 360.0) value = 359.99;
  // Форматуємо з заданою кількістю знаків після коми
  _lcd->print(value, decimals);
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
  // Нормалізуємо позицію до діапазону 0-360 градусів
  int32_t normalizedPosition = position;
  while (normalizedPosition < 0) {
    normalizedPosition += steps360;
  }
  while (normalizedPosition >= steps360) {
    normalizedPosition -= steps360;
  }
  uint16_t deg = (uint32_t)normalizedPosition * 360 / steps360;

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
  // Нормалізуємо позицію до діапазону 0-360 градусів
  int32_t normalizedPosition = position;
  while (normalizedPosition < 0) {
    normalizedPosition += steps360;
  }
  while (normalizedPosition >= steps360) {
    normalizedPosition -= steps360;
  }
  uint16_t deg = (uint32_t)normalizedPosition * 360 / steps360;

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
  // Нормалізуємо позицію до діапазону 0-360 градусів
  int32_t normalizedPosition = position;
  while (normalizedPosition < 0) {
    normalizedPosition += steps360;
  }
  while (normalizedPosition >= steps360) {
    normalizedPosition -= steps360;
  }
  uint16_t currentDeg = (uint32_t)normalizedPosition * 360 / steps360;

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
    // Для "Set Angle" (itemIndex == 0) - дві стрілки, для інших - одна
    if (itemIndex == 0 && strcmp(text, "Set Angle") == 0) {
      _lcd->print(">>");
    } else {
      _lcd->print(">");
    }
  } else {
    _lcd->print(" ");
  }
  _lcd->print(text);
  // Очищаємо решту рядка
  uint8_t len = strlen(text);
  uint8_t arrowLen = (selected && itemIndex == 0 && strcmp(text, "Set Angle") == 0) ? 2 : 1;
  for (uint8_t i = len + arrowLen; i < _cols; i++) {
    _lcd->print(" ");
  }
}

void Display::resetSplashScreen() {
  // Встановлюємо прапорець для скидання сплеш-екрану
  _splashNeedReset = true;
}

void Display::showSplashScreen(float encoderAngle, uint16_t targetAngle, bool isRunning, bool motorEnabled) {
  static float lastEncoderAngle = -1.0;
  static float filteredEncoderAngle = -1.0;  // Фільтроване значення для стабільності
  static uint16_t lastTargetAngle = 65535;
  static bool lastIsRunning = false;
  static bool lastMotorEnabled = true;
  static bool firstDisplay = true;
  static bool firstRow3Display = true;  // Для першого відображення рядка 3
  
  // Якщо потрібно скинути (викликано resetSplashScreen) - робимо це
  if (_splashNeedReset) {
    firstDisplay = true;
    firstRow3Display = true;
    _splashNeedReset = false;
    lastEncoderAngle = -1.0;
    filteredEncoderAngle = -1.0;
    lastTargetAngle = 65535;
    lastIsRunning = !isRunning;
    lastMotorEnabled = !motorEnabled;  // Примусово оновити
  }

  if (firstDisplay) {
    _lcd->clear();
    firstDisplay = false;
    lastEncoderAngle = -1.0; // Примусово оновити
    filteredEncoderAngle = encoderAngle;  // Ініціалізуємо фільтроване значення
    lastTargetAngle = 65535;
    lastIsRunning = !isRunning;
    lastMotorEnabled = !motorEnabled;  // Примусово оновити
  }
  
  // Фільтрація значення кута для стабільності відображення (експоненційне усереднення)
  if (filteredEncoderAngle < 0.0) {
    filteredEncoderAngle = encoderAngle;
  } else {
    // Експоненційне усереднення з коефіцієнтом 0.7 (30% нового значення, 70% старого)
    filteredEncoderAngle = filteredEncoderAngle * 0.7f + encoderAngle * 0.3f;
  }

  if (_rows >= 4) {
    // LCD2004
    // Заголовок - стан утримання двигуна
    if (lastEncoderAngle == 65535 || lastMotorEnabled != motorEnabled) {
      _lcd->setCursor(0, 0);
      if (motorEnabled) {
        _lcd->print("Motor:Hold ON");
      } else {
        _lcd->print("Motor:Released");
      }
      if (_cols >= 20) _lcd->print("    ");
      lastMotorEnabled = motorEnabled;
    }

    // Кут з абсолютного енкодера (поточний стан)
    if (lastEncoderAngle < 0.0) {
      _lcd->setCursor(0, 1);
      _lcd->print("Encoder: ");
    }
    // Оновлюємо якщо змінився фільтрований кут (з більшою толерантністю для стабільності)
    float diff = (lastEncoderAngle < 0.0) ? 1.0 : ((filteredEncoderAngle > lastEncoderAngle) ? (filteredEncoderAngle - lastEncoderAngle) : (lastEncoderAngle - filteredEncoderAngle));
    if (lastEncoderAngle < 0.0 || diff > 0.1f) {  // Поріг збільшено до 0.1 градуса для стабільності
      printAt(9, 1, filteredEncoderAngle, 2);
      _lcd->write((uint8_t)0);  // Кастомний символ градуса
      if (_cols >= 20) _lcd->print("      ");
      lastEncoderAngle = filteredEncoderAngle;
    }

    // Цільовий кут (встановлений для руху)
    if (lastTargetAngle == 65535) {
      _lcd->setCursor(0, 2);
      _lcd->print("Target: ");
    }
    if (lastTargetAngle != targetAngle) {
      printAt(8, 2, targetAngle);
      _lcd->write((uint8_t)0);  // Кастомний символ градуса
      if (_cols >= 20) _lcd->print("      ");
      lastTargetAngle = targetAngle;
    }

    // Стан та інструкції (рядок 3) - завжди виводимо для надійності
    _lcd->setCursor(0, 3);
    if (isRunning) {
      _lcd->print("Status: RUNNING    ");
    } else {
      _lcd->print("Menu:Ok Btn:Start  ");
    }
    // Заповнюємо решту рядка пробілами
    if (_cols >= 20) {
      _lcd->print(" ");
    }
    lastIsRunning = isRunning;
  } else {
    // LCD1602
    // Кут з абсолютного енкодера
    float diff = (lastEncoderAngle < 0.0) ? 1.0 : ((filteredEncoderAngle > lastEncoderAngle) ? (filteredEncoderAngle - lastEncoderAngle) : (lastEncoderAngle - filteredEncoderAngle));
    if (lastEncoderAngle < 0.0 || diff > 0.1f) {  // Поріг збільшено до 0.1 градуса для стабільності
      _lcd->setCursor(0, 0);
      _lcd->print("Enc: ");
      printAt(5, 0, filteredEncoderAngle, 2);
      _lcd->write((uint8_t)0);  // Кастомний символ градуса
      lastEncoderAngle = filteredEncoderAngle;
    }

    // Цільовий кут та стан
    if (lastTargetAngle == 65535 || lastTargetAngle != targetAngle || lastIsRunning != isRunning) {
      _lcd->setCursor(0, 1);
      _lcd->print("Tgt: ");
      printAt(5, 1, targetAngle);
      _lcd->write((uint8_t)0);  // Кастомний символ градуса
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
    
    printMenuItem(1, 0, "Set Angle", selectedItem == 0);
    printMenuItem(2, 1, "Settings", selectedItem == 1);
    
    // Рядок 3: показуємо Save Position
    printMenuItem(3, 2, "Save Position", selectedItem == 2);
  } else {
    // LCD1602 - показуємо по 2 пункти
    if (selectedItem == 0) {
      printMenuItem(0, 0, "Set Angle", true);
      printMenuItem(1, 1, "Settings", false);
    } else if (selectedItem == 1) {
      printMenuItem(0, 0, "Set Angle", false);
      printMenuItem(1, 1, "Settings", true);
    } else {
      printMenuItem(0, 1, "Settings", false);
      printMenuItem(1, 2, "Save Position", true);
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
    // Нічого не змінилося - виводимо тільки перший рядок
    if (_rows >= 4) {
      _lcd->setCursor(0, 0);
      _lcd->print("Target: ");
      if (targetAngle < 100) _lcd->print(' ');
      if (targetAngle < 10) _lcd->print(' ');
      _lcd->print(targetAngle);
      _lcd->write((uint8_t)0);
      if (_cols >= 20) _lcd->print("       ");
    }
    return;
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
    // Перший рядок - виводимо завжди (як рядок 3 на першому екрані)
    _lcd->setCursor(0, 0);
    _lcd->print("Target: ");
    // Виводимо кут вручну для надійності
    if (targetAngle < 100) _lcd->print(' ');
    if (targetAngle < 10) _lcd->print(' ');
    _lcd->print(targetAngle);
    _lcd->write((uint8_t)0);  // Кастомний символ градуса
    if (_cols >= 20) _lcd->print("       ");
    
    _lcd->setCursor(0, 1);
    _lcd->print("Mode: ");
    _lcd->print(modeName);
    if (_cols >= 20) {
      for (uint8_t i = 6 + strlen(modeName); i < _cols; i++) {
        _lcd->print(" ");
      }
    }
    
    _lcd->setCursor(0, 3);
    _lcd->print("Btn:Ok");
    if (_cols >= 20) _lcd->print("                  ");
  } else {
    // LCD1602
    _lcd->setCursor(0, 0);
    _lcd->print("Tgt: ");
    printAt(5, 0, targetAngle);
    _lcd->write((uint8_t)0);  // Кастомний символ градуса
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
    // Оновлюємо напрямок тільки якщо змінився
    if (directionChanged) {
      _lcd->setCursor(0, 1);
      if (direction == 0) {
        _lcd->print("> CW  (Clockwise)");
        if (_cols >= 20) _lcd->print("  ");
      } else {
        _lcd->print("> CCW (Counter-CW)");
        if (_cols >= 20) _lcd->print("");
      }
    }
    
    _lcd->setCursor(0, 3);
    _lcd->print("Btn:Ok");
    if (_cols >= 20) _lcd->print("                  ");
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
    _lcd->setCursor(0, 1);
    _lcd->print("Save Position");
    if (_cols >= 20) _lcd->print("        ");
    
    _lcd->setCursor(0, 3);
    _lcd->print("Btn:Ok");
    if (_cols >= 20) _lcd->print("                  ");
  } else {
    _lcd->setCursor(0, 0);
    _lcd->print("Save Position");
    _lcd->setCursor(0, 1);
    _lcd->print("Press to confirm");
  }
}
