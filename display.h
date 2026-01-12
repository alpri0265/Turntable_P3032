#ifndef DISPLAY_H
#define DISPLAY_H

#include <LiquidCrystal.h>
#include <Arduino.h>

class Display {
public:
  Display(uint8_t rs, uint8_t enable, uint8_t d4, uint8_t d5, uint8_t d6, uint8_t d7);
  void begin();
  void update(uint32_t position, uint32_t steps360);
  void showMessage(const char* line0, const char* line1);
  void showAngle(uint32_t position, uint32_t steps360);
  void clear();
  
private:
  LiquidCrystal _lcd;
  uint16_t _lastDeg;
  unsigned long _lastUpdate;
  bool _messageShown;
  unsigned long _messageStartTime;
  
  void drawFull(uint32_t position, uint32_t steps360);
  void drawAngleOnly(uint32_t position, uint32_t steps360);
};

#endif
