#ifndef ENCODER_H
#define ENCODER_H

#include <Arduino.h>

class Encoder {
public:
  Encoder(uint8_t pinA, uint8_t pinB);
  void begin();
  int16_t read();  // Читає та скидає дельту
  int16_t getDelta();  // Читає дельту без скидання
  
private:
  uint8_t _pinA;
  uint8_t _pinB;
  volatile int16_t _delta;
  
  static Encoder* _instance;
  static void isrA();
  static void isrB();
  void handleA();
  void handleB();
};

#endif
