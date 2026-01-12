#ifndef MEMORY_H
#define MEMORY_H

#include <EEPROM.h>
#include <Arduino.h>

// Структура для збереження налаштувань
struct SettingsData {
  int32_t position;        // Позиція двигуна
  uint8_t direction;       // Напрямок руху (0 = CW, 1 = CCW)
  int32_t stepperZero;     // Нульова позиція двигуна
  uint8_t checksum;        // Контрольна сума для перевірки цілісності
};

class Memory {
public:
  Memory(int32_t minPos, int32_t maxPos);
  void load(int32_t& position);
  void save(int32_t position);
  
  // Нові методи для збереження налаштувань
  void loadSettings(int32_t& position, uint8_t& direction, int32_t& stepperZero);
  void saveSettings(int32_t position, uint8_t direction, int32_t stepperZero);
  
private:
  int32_t _minPos;
  int32_t _maxPos;
  static const int EEPROM_ADDRESS = 0;
  
  // Допоміжний метод для обчислення checksum
  uint8_t calculateChecksum(const SettingsData& data);
};

#endif
