#include "config.h"
#include "encoder.h"
#include "button.h"
#include "display.h"
#include "memory.h"
#include "stepper.h"
#include "menu.h"

/* ================== ОБʼЄКТИ ================== */
Encoder encoder(ENC_A, ENC_B);
Button button(ENC_BTN, BUTTON_DEBOUNCE_MS);
Display display(LCD_RS, LCD_E, LCD_D4, LCD_D5, LCD_D6, LCD_D7);
Memory memory(MIN_POS, MAX_POS);
Stepper stepper(STEP_PIN, DIR_PIN);
Menu menu;

/* ================== ЗМІННІ ================== */
int32_t targetPosition = 0;

/* ================== SETUP ================== */
void setup() {
  encoder.begin();
  button.begin();
  display.begin();
  stepper.begin();
  
  // Завантажуємо позицію з пам'яті
  memory.load(targetPosition);
  stepper.setPosition(targetPosition);
  
  // Показуємо початковий екран
  display.showAngle(stepper.getPosition(), STEPS_360);
}

/* ================== LOOP ================== */
void loop() {
  // Читаємо енкодер
  int16_t encoderDelta = encoder.read();
  
  // Перевіряємо кнопку
  bool buttonPressed = button.isPressed();
  
  // Оновлюємо меню (обробка вводу)
  menu.update(encoderDelta, buttonPressed, targetPosition, 
              stepper.getPosition(), stepper.getRemaining());
  
  // Виконуємо рух до цільової позиції
  int32_t stepsNeeded = targetPosition - stepper.getPosition() - stepper.getRemaining();
  if (stepsNeeded != 0) {
    stepper.move(stepsNeeded);
  }
  
  // Оновлюємо кроковий двигун (неблокуюче)
  stepper.update();
  
  // Обробка збереження
  if (menu.shouldSave()) {
    memory.save(stepper.getPosition());
    menu.clearSaveFlag();
    display.showMessage("Manual mode   ", "Saved to EEPROM");
  }
  
  // Оновлюємо дисплей
  display.update(stepper.getPosition(), STEPS_360);
}
