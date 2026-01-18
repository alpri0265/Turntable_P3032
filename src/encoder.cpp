#include "encoder.h"

Encoder* Encoder::_instance = nullptr;

Encoder::Encoder(uint8_t pinA, uint8_t pinB) 
  : _pinA(pinA), _pinB(pinB), _delta(0) {
  _instance = this;
}

void Encoder::begin() {
  pinMode(_pinA, INPUT_PULLUP);
  pinMode(_pinB, INPUT_PULLUP);
  
#if defined(ARDUINO_ARCH_STM32) || defined(STM32F1xx) || defined(ARDUINO_ARCH_STM32F1)
  // На STM32 всі піни підтримують переривання через EXTI
  // attachInterrupt() може працювати з піном безпосередньо
  attachInterrupt(_pinA, isrA, CHANGE);
  attachInterrupt(_pinB, isrB, CHANGE);
#else
  // Arduino (AVR) - потрібно використовувати digitalPinToInterrupt()
  attachInterrupt(digitalPinToInterrupt(_pinA), isrA, CHANGE);
  attachInterrupt(digitalPinToInterrupt(_pinB), isrB, CHANGE);
#endif
}

int16_t Encoder::read() {
  noInterrupts();
  int16_t d = _delta;
  _delta = 0;
  interrupts();
  return d;
}

int16_t Encoder::getDelta() {
  noInterrupts();
  int16_t d = _delta;
  interrupts();
  return d;
}

void Encoder::isrA() {
  if (_instance) {
    _instance->handleA();
  }
}

void Encoder::isrB() {
  if (_instance) {
    _instance->handleB();
  }
}

void Encoder::handleA() {
  if (digitalRead(_pinA) == digitalRead(_pinB))
    _delta++;
  else
    _delta--;
}

void Encoder::handleB() {
  if (digitalRead(_pinA) != digitalRead(_pinB))
    _delta++;
  else
    _delta--;
}
