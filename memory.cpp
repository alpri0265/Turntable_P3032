#include "memory.h"

const int Memory::EEPROM_ADDRESS;

Memory::Memory(int32_t minPos, int32_t maxPos)
  : _minPos(minPos), _maxPos(maxPos) {
}

void Memory::load(int32_t& position) {
  EEPROM.get(EEPROM_ADDRESS, position);
  if (position < _minPos || position > _maxPos) {
    position = 0;
  }
}

void Memory::save(int32_t position) {
  EEPROM.put(EEPROM_ADDRESS, position);
}
