#include "config.h"
#include "encoder.h"
#include "absolute_encoder.h"
#include "button.h"
#include "display.h"
#include "memory.h"
#include "stepper.h"
#include "menu.h"

/* ================== ОБʼЄКТИ ================== */
Encoder encoder(ENC_A, ENC_B);
AbsoluteEncoder absoluteEncoder(ABS_ENC_PIN, 5.0, 360.0);
Button button(ENC_BTN, BUTTON_DEBOUNCE_MS);

#if LCD_MODE == 0
  // 4-bit режим
  Display display(LCD_RS, LCD_E, LCD_D4, LCD_D5, LCD_D6, LCD_D7);
#else
  // I2C режим
  Display display(LCD_I2C_ADDRESS, (LCD_TYPE == 1) ? 16 : 20, (LCD_TYPE == 1) ? 2 : 4);
#endif

Memory memory(MIN_POS, MAX_POS);
Stepper stepper(STEP_PIN, DIR_PIN);
Menu menu;

/* ================== ЗМІННІ ================== */
int32_t targetPosition = 0;

/* ================== SETUP ================== */
void setup() {
  encoder.begin();
  absoluteEncoder.begin();
  button.begin();
  display.begin();
  stepper.begin();
  
  // Завантажуємо позицію з пам'яті
  memory.load(targetPosition);
  stepper.setPosition(targetPosition);
  
  // Встановлюємо початковий цільовий кут з абсолютного енкодера
  uint16_t initialAngle = absoluteEncoder.readAngleInt();
  menu.updateWithAbsoluteEncoder(initialAngle, false, targetPosition, 
                                  stepper.getPosition(), stepper.getRemaining());
  
  // Показуємо початковий екран
  display.updateWithTarget(stepper.getPosition(), STEPS_360, menu.getTargetAngle());
}

/* ================== LOOP ================== */
void loop() {
  // Читаємо інкрементальний енкодер (для точного позиціонування)
  int16_t encoderDelta = encoder.read();
  
  // Читаємо абсолютний енкодер P3022-CW360 (для встановлення цільового кута)
  uint16_t absoluteAngle = absoluteEncoder.readAngleInt();
  
  // Перевіряємо кнопку
  bool buttonPressed = button.isPressed();
  
  // Оновлюємо меню з абсолютним енкодером (встановлює цільовий кут)
  menu.updateWithAbsoluteEncoder(absoluteAngle, buttonPressed, targetPosition, 
                                  stepper.getPosition(), stepper.getRemaining());
  
  // Дозволяємо також використовувати інкрементальний енкодер для точного налаштування
  if (encoderDelta != 0) {
    int32_t newTarget = targetPosition + encoderDelta;
    if (newTarget < MIN_POS) newTarget = MIN_POS;
    else if (newTarget > MAX_POS) newTarget = MAX_POS;
    targetPosition = newTarget;
    // Синхронізуємо кут в меню з нової позиції
    menu.syncAngleFromPosition(targetPosition);
  }
  
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
  
  // Оновлюємо дисплей з поточним та цільовим кутом
  display.updateWithTarget(stepper.getPosition(), STEPS_360, menu.getTargetAngle());
}
