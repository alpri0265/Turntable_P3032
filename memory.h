#ifndef MEMORY_H
#define MEMORY_H

#include <EEPROM.h>
#include <Arduino.h>

class Memory {
public:
  Memory(int32_t minPos, int32_t maxPos);
  void load(int32_t& position);
  void save(int32_t position);
  
private:
  int32_t _minPos;
  int32_t _maxPos;
  static const int EEPROM_ADDRESS = 0;
};

#endif
