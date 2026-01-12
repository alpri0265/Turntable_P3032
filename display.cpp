#include "display.h"
#include "config.h"

Display::Display(uint8_t rs, uint8_t enable, uint8_t d4, uint8_t d5, uint8_t d6, uint8_t d7)
  : _lcd(rs, enable, d4, d5, d6, d7), _lastDeg(999), 
    _lastUpdate(0), _messageShown(false), _messageStartTime(0) {
}

void Display::begin() {
  _lcd.begin(16, 2);
  _lcd.clear();
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
  _lcd.setCursor(0, 0);
  _lcd.print(line0);
  _lcd.setCursor(0, 1);
  _lcd.print(line1);
  _messageShown = true;
  _messageStartTime = millis();
}

void Display::showAngle(uint32_t position, uint32_t steps360) {
  drawFull(position, steps360);
  _lastDeg = (uint32_t)position * 360 / steps360;
  _lastUpdate = millis();
}

void Display::clear() {
  _lcd.clear();
}

void Display::drawFull(uint32_t position, uint32_t steps360) {
  uint16_t deg = (uint32_t)position * 360 / steps360;

  _lcd.setCursor(0, 0);
  _lcd.print("Manual mode   ");

  _lcd.setCursor(0, 1);
  _lcd.print("Angle: ");
  if (deg < 100) _lcd.print(' ');
  if (deg < 10)  _lcd.print(' ');
  _lcd.print(deg);
  _lcd.print((char)223); // °
  _lcd.print("   ");
}

void Display::drawAngleOnly(uint32_t position, uint32_t steps360) {
  uint16_t deg = (uint32_t)position * 360 / steps360;

  _lcd.setCursor(7, 1);
  if (deg < 100) _lcd.print(' ');
  if (deg < 10)  _lcd.print(' ');
  _lcd.print(deg);
  _lcd.print((char)223); // °
  _lcd.print("   ");
}
