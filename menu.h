#ifndef MENU_H
#define MENU_H

#include <Arduino.h>
#include "config.h"

class Menu {
public:
  Menu();
  void update(int16_t encoderDelta, bool buttonPressed, 
              int32_t& targetPosition, int32_t currentPos, int32_t remaining);
  bool shouldSave() const { return _shouldSave; }
  void clearSaveFlag() { _shouldSave = false; }
  
private:
  bool _shouldSave;
};

#endif
