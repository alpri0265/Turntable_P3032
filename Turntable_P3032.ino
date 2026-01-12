#include "config.h"
#include "encoder.h"
#include "absolute_encoder.h"
#include "button.h"
#include "display.h"
#include "memory.h"
#include "stepper.h"
#include "menu.h"
#include "direction_switch.h"
#include "start_stop.h"

/* ================== ОБʼЄКТИ ================== */
Encoder encoder(ENC_A, ENC_B);
AbsoluteEncoder absoluteEncoder(ABS_ENC_PIN, 5.0, 360.0);
Button button(ENC_BTN, BUTTON_DEBOUNCE_MS);
Button digitModeButton(DIGIT_MODE_BUTTON_PIN, BUTTON_DEBOUNCE_MS);

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
DirectionSwitch directionSwitch(DIRECTION_SWITCH_PIN);
StartStop startStop(START_STOP_BUTTON_PIN, START_STOP_LED_PIN, BUTTON_DEBOUNCE_MS);

/* ================== ЗМІННІ ================== */
unsigned long lastDisplayUpdate = 0;

/* ================== SETUP ================== */
void setup() {
  encoder.begin();
  absoluteEncoder.begin();
  button.begin();
  digitModeButton.begin();
  display.begin();
  stepper.begin();
  directionSwitch.begin();
  startStop.begin();
  
  // Завантажуємо позицію з пам'яті
  int32_t savedPosition = 0;
  memory.load(savedPosition);
  stepper.setPosition(savedPosition);
  
  // Встановлюємо початковий цільовий кут з абсолютного енкодера
  uint16_t initialAngle = absoluteEncoder.readAngleInt();
  menu.updateTargetAngle(initialAngle);
  
  // Показуємо початковий екран (головне меню)
  display.showMainMenu(menu.getCurrentItem());
}

/* ================== LOOP ================== */
void loop() {
  // Читаємо інкрементальний енкодер (для навігації по меню)
  // Обмежуємо значення для плавної навігації (тільки ±1 за раз)
  int16_t rawDelta = encoder.read();
  int16_t encoderDelta = 0;
  if (rawDelta > 0) {
    encoderDelta = 1;  // Тільки один крок вгору
  } else if (rawDelta < 0) {
    encoderDelta = -1; // Тільки один крок вниз
  }
  
  // Читаємо абсолютний енкодер P3022-CW360 (встановлює цільовий кут)
  // Тільки якщо не активний режим редагування через меню
  // updateTargetAngle сам перевіряє прапорець _manualAngleSet
  if (!menu.isEditingAngle()) {
    uint16_t absoluteAngle = absoluteEncoder.readAngleInt();
    menu.updateTargetAngle(absoluteAngle);
  }
  
  // Перевіряємо кнопки
  bool buttonPressed = button.isPressed();
  bool digitButtonPressed = digitModeButton.isPressed();
  
  // Оновлюємо режим редагування розрядів (тільки в меню Set Angle)
  if (menu.isEditingAngle()) {
    menu.updateDigitMode(digitButtonPressed);
  }
  
  // Оновлюємо навігацію по меню (інкрементальний енкодер + кнопка)
  menu.updateNavigation(encoderDelta, buttonPressed);
  
  // Обробка кнопки старт-стоп
  startStop.toggle();
  startStop.updateLED();
  
  // Читаємо перемикач напрямку та встановлюємо інверсію
  static RotationDirection currentDirection = DIR_CW;
  currentDirection = directionSwitch.read();
  stepper.setDirectionInvert(currentDirection == DIR_CCW);
  
  // Отримуємо цільову позицію з меню
  int32_t targetPosition = menu.getTargetPosition();
  
  // Виконуємо рух до цільової позиції (тільки якщо старт активний)
  if (startStop.getState()) {
    int32_t stepsNeeded = targetPosition - stepper.getPosition() - stepper.getRemaining();
    if (stepsNeeded != 0) {
      stepper.move(stepsNeeded);
    }
    
    // Оновлюємо кроковий двигун (неблокуюче)
    stepper.update();
    
    // Перевіряємо, чи досягнуто цільову позицію (тільки після того, як двигун рухався)
    // Вимикаємо тільки якщо немає залишкових кроків (двигун завершив рух)
    if (stepper.getRemaining() == 0) {
      static bool positionReached = false;
      positionReached = menu.isPositionReached(
        stepper.getPosition(), 
        stepper.getRemaining()
      );
      
      // Якщо позиція досягнута і немає залишкових кроків - автоматично вимикаємо двигун
      if (positionReached) {
        startStop.setState(false);
      }
    }
  } else {
    // Якщо стоп - зупиняємо рух
    // (stepper.update() не викликається, тому рух зупиняється)
  }
  
  // Обробка збереження
  static unsigned long saveMessageTime = 0;
  if (menu.shouldSave()) {
    memory.save(stepper.getPosition());
    menu.clearSaveFlag();
    display.showMessage("Position saved", "to EEPROM");
    saveMessageTime = millis();
  }
  
  // Оновлюємо дисплей залежно від поточного меню
  unsigned long now = millis();
  
  // Перевіряємо, чи показується повідомлення про збереження
  if (saveMessageTime > 0 && (now - saveMessageTime < 1000)) {
    // Повідомлення вже відображається
  } else {
    if (saveMessageTime > 0) {
      saveMessageTime = 0; // Повідомлення приховано
    }
    
    // Оновлюємо меню
    if (now - lastDisplayUpdate > LCD_UPDATE_MS) {
      switch (menu.getCurrentMenu()) {
        case MENU_MAIN:
          display.showMainMenu(menu.getCurrentItem());
          break;
          
        case MENU_STATUS:
          {
            // Перевіряємо досягнення позиції
            bool positionReached = menu.isPositionReached(
              stepper.getPosition(), 
              stepper.getRemaining()
            );
            bool directionCCW = (currentDirection == DIR_CCW);
            display.showStatusMenu(
              stepper.getPosition(), 
              STEPS_360, 
              menu.getTargetAngle(),
              positionReached,
              directionCCW
            );
          }
          break;
          
                case MENU_SET_ANGLE:
                  display.showSetAngleMenu(menu.getTargetAngle(), menu.getDigitMode());
                  break;
          
        case MENU_SETTINGS:
          display.showSettingsMenu();
          break;
          
        case MENU_SAVE:
          display.showSaveMenu();
          break;
      }
      lastDisplayUpdate = now;
    }
  }
}
