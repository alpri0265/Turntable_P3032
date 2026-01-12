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
  
  // Показуємо початковий екран (сплеш-екран)
  uint16_t initialEncoderAngle = absoluteEncoder.readAngleInt();
  display.showSplashScreen(initialEncoderAngle, menu.getTargetAngle(), false);
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
  // Оновлюємо на сплеш-екрані та в інших меню (крім режиму редагування)
  // updateTargetAngle сам перевіряє прапорець _manualAngleSet
  if (menu.getCurrentMenu() == MENU_SPLASH || !menu.isEditingAngle()) {
    uint16_t absoluteAngle = absoluteEncoder.readAngleInt();
    menu.updateTargetAngle(absoluteAngle);
  }
  
  // Перевіряємо кнопки
  // Для інших меню використовуємо isPressed() з debounce
  // Але перевіряємо кнопку безпосередньо перед використанням
  bool digitButtonPressed = digitModeButton.isPressed();
  
  // Відстежуємо натискання кнопки енкодера для обнулення позиції (тільки на сплеш-екрані)
  static unsigned long buttonPressStartTime = 0;
  static bool buttonWasPressed = false;
  static bool longPressDetected = false;
  static bool wasOnSplash = false;
  
  bool isOnSplash = (menu.getCurrentMenu() == MENU_SPLASH);
  
  // Обробка сплеш-екрану
  if (isOnSplash) {
    // Скидаємо стан при виході з сплеш-екрану і поверненні
    if (!wasOnSplash) {
      buttonWasPressed = false;
      longPressDetected = false;
      wasOnSplash = true;
    }
    
    bool buttonCurrentlyPressed = button.isCurrentlyPressed();
    
    if (buttonCurrentlyPressed && !buttonWasPressed) {
      // Кнопка тільки що натиснута
      buttonPressStartTime = millis();
      buttonWasPressed = true;
      longPressDetected = false;
    } else if (buttonCurrentlyPressed && buttonWasPressed) {
      // Кнопка все ще натиснута - перевіряємо час
      unsigned long pressDuration = millis() - buttonPressStartTime;
      if (pressDuration >= 2000 && !longPressDetected) {
        // Довге натискання виявлено - обнуляємо позицію
        longPressDetected = true;
        stepper.setPosition(0);
        memory.save(0);
        display.showMessage("Position", "reset to 0");
      }
    } else if (!buttonCurrentlyPressed && buttonWasPressed) {
      // Кнопка відпущена
      unsigned long pressDuration = millis() - buttonPressStartTime;
      buttonWasPressed = false;
      
      // Якщо було довге натискання - не переходимо в меню
      if (longPressDetected) {
        longPressDetected = false;
      } else if (pressDuration < LONG_PRESS_THRESHOLD_MS && pressDuration > BUTTON_DEBOUNCE_MS) {
        // Коротке натискання (більше 50мс для debounce, менше 2 секунд) - переходимо в меню
        menu.handleSplashMenu(true, false); // Перехід в меню
      }
    }
    
    // Обробка кнопки старт-стоп на сплеш-екрані
    bool startStopPressed = startStop.isPressed();
    if (startStopPressed) {
      startStop.setState(true);
    }
    
    // Обробка сплеш-екрану (тільки для старт-стоп, кнопка енкодера обробляється вище)
    menu.handleSplashMenu(false, startStopPressed);
  } else {
    // Не на сплеш-екрані - скидаємо прапорець
    wasOnSplash = false;
    // Оновлюємо режим редагування розрядів (тільки в меню Set Angle)
    if (menu.isEditingAngle()) {
      menu.updateDigitMode(digitButtonPressed);
    }
    
    // Зберігаємо попереднє меню для перевірки зміни
    static MenuType lastMenu = MENU_SPLASH;
    MenuType currentMenuBefore = menu.getCurrentMenu();
    
    // ========== ОБРОБКА КНОПКИ ЕНКОДЕРА ДЛЯ МЕНЮ ==========
    // Використовуємо окрему логіку з прямим читанням піну для стабільності
    static unsigned long buttonPressStartTimeMenu = 0;
    static bool buttonWasPressedMenu = false;
    static bool longPressDetectedMenu = false;
    static unsigned long lastDebounceTime = 0;
    static bool lastRawState = HIGH;
    static bool debouncedState = HIGH;
    
    unsigned long currentTime = millis();
    bool buttonPressedForMenu = false;  // Прапорець короткого натискання для меню
    
    // Пряме читання піну (INPUT_PULLUP - LOW = натиснуто)
    bool rawState = digitalRead(ENC_BTN);
    
    // Простий debounce для визначення стабільного стану
    if (rawState != lastRawState) {
      lastDebounceTime = currentTime;
    }
    lastRawState = rawState;
    
    // Оновлюємо debounced стан якщо пройшло достатньо часу
    if (currentTime - lastDebounceTime > BUTTON_DEBOUNCE_MS) {
      debouncedState = rawState;
    }
    
    // Для INPUT_PULLUP: LOW = натиснуто, HIGH = відпущено
    bool buttonCurrentlyPressed = (debouncedState == LOW);
    
    // Відстежуємо початок натискання кнопки (перехід з false в true)
    if (buttonCurrentlyPressed && !buttonWasPressedMenu) {
      // Початок натискання - фіксуємо час
      buttonPressStartTimeMenu = currentTime;
      buttonWasPressedMenu = true;
      longPressDetectedMenu = false;
    }
    
    // Перевіряємо довге натискання КОЖНУ ітерацію loop, поки кнопка натиснута
    if (buttonCurrentlyPressed && buttonWasPressedMenu) {
      unsigned long pressDuration = currentTime - buttonPressStartTimeMenu;
      
      // Перевірка довгого натискання (>= 2 секунди)
      if (pressDuration >= LONG_PRESS_THRESHOLD_MS && !longPressDetectedMenu) {
        // ДОВГЕ НАТИСКАННЯ - повернення на сплеш-екран
        longPressDetectedMenu = true;
        menu.handleLongPress();
        display.resetSplashScreen();
        menu.clearResetSplashFlag();
        lastDisplayUpdate = 0;
      }
    }
    
    // Обробка відпускання кнопки (перехід з true в false)
    if (!buttonCurrentlyPressed && buttonWasPressedMenu) {
      unsigned long pressDuration = currentTime - buttonPressStartTimeMenu;
      
      // Коротке натискання: більше debounce, але менше довгого, і НЕ було виявлено довгого
      if (!longPressDetectedMenu && pressDuration < LONG_PRESS_THRESHOLD_MS && pressDuration >= BUTTON_DEBOUNCE_MS) {
        buttonPressedForMenu = true;
      }
      
      // Скидаємо всі прапорці після відпускання
      buttonWasPressedMenu = false;
      longPressDetectedMenu = false;
    }
    
    // Оновлюємо навігацію по меню
    menu.updateNavigation(encoderDelta, buttonPressedForMenu);
    
    // Перевіряємо, чи змінилося меню на сплеш-екран
    MenuType currentMenuAfter = menu.getCurrentMenu();
    
    // Перевірка 1: Якщо меню змінилося на сплеш-екран
    if (currentMenuBefore != MENU_SPLASH && currentMenuAfter == MENU_SPLASH) {
      // Повернулися на сплеш-екран - скидаємо екран одразу
      display.resetSplashScreen();
      menu.clearResetSplashFlag(); // Скидаємо прапорець
      // Примусово оновлюємо дисплей для відображення сплеш-екрану
      lastDisplayUpdate = 0; // Скидаємо таймер для негайного оновлення
    }
    
    // Перевірка 2: Якщо прапорець встановлений і ми на сплеш-екрані (додаткова перевірка)
    if (menu.shouldResetSplash() && currentMenuAfter == MENU_SPLASH) {
      display.resetSplashScreen();
      menu.clearResetSplashFlag();
      lastDisplayUpdate = 0; // Примусово оновлюємо дисплей
    }
    
    lastMenu = currentMenuAfter;
    
    // Обробка кнопки старт-стоп (тільки якщо не на сплеш-екрані)
    startStop.toggle();
  }
  
  startStop.updateLED();
  
  // Використовуємо напрямок з меню Settings (замість фізичного перемикача)
  RotationDirection currentDirection = menu.getDirection();
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
      // Відстежуємо зміну меню для скидання стану відображення
      static MenuType lastMenuType = MENU_SPLASH;
      MenuType currentMenuType = menu.getCurrentMenu();
      bool menuChanged = (lastMenuType != currentMenuType);
      
      // Додаткова перевірка: якщо меню змінилося на сплеш-екран
      if (lastMenuType != MENU_SPLASH && currentMenuType == MENU_SPLASH) {
        // Примусово скидаємо сплеш-екран при переході
        display.resetSplashScreen();
        menu.clearResetSplashFlag();
        lastDisplayUpdate = 0; // Примусово оновлюємо дисплей
      }
      
      lastMenuType = currentMenuType;
      
      switch (currentMenuType) {
        case MENU_SPLASH:
          {
            // Перевіряємо, чи потрібно скинути сплеш-екран (при поверненні з меню)
            // Це додаткова перевірка на випадок, якщо перехід не був виявлений раніше
            if (menu.shouldResetSplash()) {
              display.resetSplashScreen();
              menu.clearResetSplashFlag();
              lastDisplayUpdate = 0; // Примусово оновлюємо дисплей
            }
            
            // Показуємо кут з абсолютного енкодера та цільовий кут
            uint16_t encoderAngle = absoluteEncoder.readAngleInt();
            display.showSplashScreen(
              encoderAngle,
              menu.getTargetAngle(),
              startStop.getState()
            );
          }
          break;
          
        case MENU_MAIN:
          display.showMainMenu(menu.getCurrentItem());
          break;
          
        case MENU_SET_ANGLE:
          display.showSetAngleMenu(menu.getTargetAngle(), menu.getDigitMode());
          break;
          
        case MENU_SETTINGS:
          // Очищаємо екран при переході в Settings меню
          if (menuChanged) {
            display.clear();
          }
          display.showSettingsMenu(menu.getDirection() == DIR_CCW ? 1 : 0);
          break;
          
        case MENU_SAVE:
          // Очищаємо екран при переході в Save меню
          if (menuChanged) {
            display.clear();
          }
          display.showSaveMenu();
          break;
      }
      lastDisplayUpdate = now;
    }
  }
}
