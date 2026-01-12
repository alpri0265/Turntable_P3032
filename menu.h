#ifndef MENU_H
#define MENU_H

#include <Arduino.h>
#include "config.h"

class Menu {
public:
  Menu();
  void update(int16_t encoderDelta, bool buttonPressed, 
              int32_t& targetPosition, int32_t currentPos, int32_t remaining);
  void updateWithAbsoluteEncoder(uint16_t absoluteAngle, bool buttonPressed,
                                  int32_t& targetPosition, int32_t currentPos, int32_t remaining);
  bool shouldSave() const { return _shouldSave; }
  void clearSaveFlag() { _shouldSave = false; }
  uint16_t getTargetAngle() const { return _targetAngle; }
  void syncAngleFromPosition(int32_t position);  // Синхронізує кут з позиції
  
private:
  bool _shouldSave;
  uint16_t _targetAngle;
  int32_t angleToSteps(uint16_t angle);
};

#endif
