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

uint8_t Memory::calculateChecksum(const SettingsData& data) {
  // Обчислюємо checksum на основі полів структури (без самого checksum)
  uint8_t sum = 0;
  // XOR всіх байтів полів position, direction, stepperZero
  const uint8_t* posBytes = (const uint8_t*)&data.position;
  for (size_t i = 0; i < sizeof(data.position); i++) {
    sum ^= posBytes[i];
  }
  sum ^= data.direction;
  const uint8_t* zeroBytes = (const uint8_t*)&data.stepperZero;
  for (size_t i = 0; i < sizeof(data.stepperZero); i++) {
    sum ^= zeroBytes[i];
  }
  return sum;
}

void Memory::loadSettings(int32_t& position, uint8_t& direction, int32_t& stepperZero) {
  SettingsData data;
  EEPROM.get(EEPROM_ADDRESS, data);
  
  // Перевіряємо checksum
  uint8_t calculatedChecksum = calculateChecksum(data);
  if (data.checksum != calculatedChecksum) {
    // Дані пошкоджені - використовуємо значення за замовчуванням
    position = 0;
    direction = 0;  // DIR_CW
    stepperZero = 0;
    return;
  }
  
  // Перевіряємо позицію
  if (data.position < _minPos || data.position > _maxPos) {
    data.position = 0;
  }
  
  position = data.position;
  direction = (data.direction > 1) ? 0 : data.direction;  // Захист від некоректних значень
  stepperZero = data.stepperZero;
}

void Memory::saveSettings(int32_t position, uint8_t direction, int32_t stepperZero) {
  SettingsData data;
  data.position = position;
  data.direction = direction;
  data.stepperZero = stepperZero;
  data.checksum = calculateChecksum(data);
  
  EEPROM.put(EEPROM_ADDRESS, data);
}
