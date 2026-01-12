#include "config.h"
#include "encoder.h"
#include "absolute_encoder.h"
#include "button.h"
#include "display.h"
#include "memory.h"
#include "stepper.h"
#include "menu.h"
#include "start_stop.h"

/* ================== ОБʼЄКТИ ================== */
Encoder encoder(ENC_A, ENC_B);
AbsoluteEncoder absoluteEncoder(ABS_ENC_PIN, 5.0, 360.0);
Button button(ENC_BTN, BUTTON_DEBOUNCE_MS);
Button digitModeButton(DIGIT_MODE_BUTTON_PIN, BUTTON_DEBOUNCE_MS);
Button encoderZeroButton(ENCODER_ZERO_BUTTON_PIN, BUTTON_DEBOUNCE_MS);  // Кнопка встановлення нуля енкодера
Button stepFineAdjustButton(STEP_FINE_ADJUST_BUTTON_PIN, BUTTON_DEBOUNCE_MS);  // Кнопка руху на один крок

#if LCD_MODE == 0
  // 4-bit режим
  Display display(LCD_RS, LCD_E, LCD_D4, LCD_D5, LCD_D6, LCD_D7);
#else
  // I2C режим
  Display display(LCD_I2C_ADDRESS, (LCD_TYPE == 1) ? 16 : 20, (LCD_TYPE == 1) ? 2 : 4);
#endif

Memory memory(MIN_POS, MAX_POS);
Stepper stepper(STEP_PIN, DIR_PIN, ENABLE_PIN);
Menu menu;
StartStop startStop(START_STOP_BUTTON_PIN, START_STOP_LED_PIN, BUTTON_DEBOUNCE_MS);

/* ================== ЗМІННІ ================== */
unsigned long lastDisplayUpdate = 0;

/* ================== SETUP ================== */
void setup() {
  encoder.begin();
  absoluteEncoder.begin();
  button.begin();
  digitModeButton.begin();
  encoderZeroButton.begin();  // Кнопка встановлення нуля енкодера
  stepFineAdjustButton.begin();  // Кнопка руху на один крок
  display.begin();
  stepper.begin();
  startStop.begin();
  
  // Завантажуємо налаштування з пам'яті (позиція, напрямок, нуль енкодера)
  int32_t savedPosition = 0;
  uint8_t savedDirection = 0;  // DIR_CW = 0
  int32_t savedStepperZero = 0;
  memory.loadSettings(savedPosition, savedDirection, savedStepperZero);
  
  // Нормалізуємо позицію до діапазону 0-360 градусів перед встановленням
  while (savedPosition < 0) {
    savedPosition += STEPS_360;
  }
  while (savedPosition >= STEPS_360) {
    savedPosition -= STEPS_360;
  }
  stepper.setPosition(savedPosition);
  
  // Встановлюємо напрямок руху
  menu.setDirection((RotationDirection)savedDirection);
  
  // Встановлюємо напрямок для stepper (інверсія для CCW)
  stepper.setDirectionInvert(savedDirection == DIR_CCW);
  
  // Встановлюємо нульову позицію двигуна
  menu.setStepperZeroPosition(savedStepperZero);
  
  // Встановлюємо початковий цільовий кут з абсолютного енкодера
  uint16_t initialAngle = absoluteEncoder.readAngleInt();
  menu.updateTargetAngle(initialAngle);
  
  // Показуємо початковий екран (сплеш-екран)
  float initialEncoderAngle = absoluteEncoder.readAngle();
  display.showSplashScreen(initialEncoderAngle, menu.getTargetAngle(), false, stepper.isEnabled());
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
  // НЕ оновлюємо цільовий кут коли двигун рухається (startStop.getState() == true)
  // updateTargetAngle сам перевіряє прапорець _manualAngleSet
  if ((menu.getCurrentMenu() == MENU_SPLASH || !menu.isEditingAngle()) && !startStop.getState()) {
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
  
  // Обробка сплеш-екрану - ВИКОРИСТОВУЄМО ТОЧНО ТУ САМУ ЛОГІКУ, ЩО І В МЕНЮ
  if (isOnSplash) {
    // Скидаємо стан при виході з сплеш-екрану і поверненні
    if (!wasOnSplash) {
      buttonWasPressed = false;
      longPressDetected = false;
      wasOnSplash = true;
      buttonPressStartTime = 0;
    }
    
    // ========== ОБРОБКА КНОПКИ ЕНКОДЕРА ДЛЯ СПЛЕШ-ЕКРАНУ (ТОЧНА КОПІЯ ЛОГІКИ З МЕНЮ) ==========
    static unsigned long lastDebounceTimeSplash = 0;
    static bool lastRawStateSplash = HIGH;
    static bool debouncedStateSplash = HIGH;
    
    unsigned long currentTime = millis();
    bool rawState = digitalRead(ENC_BTN);  // Пряме читання піну (INPUT_PULLUP - LOW = натиснуто)
    
    // Простий debounce для визначення стабільного стану
    if (rawState != lastRawStateSplash) {
      lastDebounceTimeSplash = currentTime;
    }
    lastRawStateSplash = rawState;
    
    // Оновлюємо debounced стан якщо пройшло достатньо часу
    if (currentTime - lastDebounceTimeSplash > BUTTON_DEBOUNCE_MS) {
      debouncedStateSplash = rawState;
    }
    
    // Для INPUT_PULLUP: LOW = натиснуто, HIGH = відпущено
    bool buttonCurrentlyPressed = (debouncedStateSplash == LOW);
    
    // Відстежуємо початок натискання кнопки (перехід з false в true)
    if (buttonCurrentlyPressed && !buttonWasPressed) {
      // Початок натискання - фіксуємо час
      buttonPressStartTime = currentTime;
      buttonWasPressed = true;
      longPressDetected = false;
    }
    
    // Перевіряємо довге натискання КОЖНУ ітерацію loop, поки кнопка натиснута
    if (buttonCurrentlyPressed && buttonWasPressed) {
      unsigned long pressDuration = currentTime - buttonPressStartTime;
      
      // Перевірка довгого натискання (>= 2 секунди)
      if (pressDuration >= LONG_PRESS_THRESHOLD_MS && !longPressDetected) {
        // ДОВГЕ НАТИСКАННЯ - перемикаємо утримання двигуна
        longPressDetected = true;
        bool currentState = stepper.isEnabled();
        stepper.setEnabled(!currentState);
        display.resetSplashScreen();  // Оновлюємо екран
      }
    }
    
    // Обробка відпускання кнопки (перехід з true в false)
    if (!buttonCurrentlyPressed && buttonWasPressed) {
      unsigned long pressDuration = currentTime - buttonPressStartTime;
      buttonWasPressed = false;
      
      // Якщо було довге натискання - не переходимо в меню
      if (longPressDetected) {
        longPressDetected = false;
      } else if (pressDuration < LONG_PRESS_THRESHOLD_MS && pressDuration >= BUTTON_DEBOUNCE_MS) {
        // Коротке натискання - переходимо в меню
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
  
  // Обробка кнопки встановлення нуля абсолютного енкодера (працює на всіх екранах)
  if (encoderZeroButton.isPressed()) {
    // Зберігаємо поточний цільовий кут (наприклад 100°) перед обнуленням
    uint16_t savedTargetAngle = menu.getTargetAngle();
    
    // Зберігаємо поточну позицію двигуна як нульову
    int32_t currentPosition = stepper.getPosition();
    menu.setStepperZeroPosition(currentPosition);
    
    // Встановлюємо нуль енкодера (тепер кут 0°)
    absoluteEncoder.setZero();
    
    // Зберігаємо поточну позицію, напрямок та нуль в пам'ять
    uint8_t currentDirection = (uint8_t)menu.getDirection();
    int32_t currentStepperZero = menu.getStepperZeroPosition();
    memory.saveSettings(currentPosition, currentDirection, currentStepperZero);
    
    // Відновлюємо збережений цільовий кут (100°) - він залишається незмінним
    menu.setTargetAngle(savedTargetAngle);
    
    display.showMessage("Encoder", "");
  }
  
  // Обробка кнопки руху на один крок (для точного регулювання кута)
  static unsigned long stepButtonPressStartTime = 0;
  static bool stepButtonWasPressed = false;
  static unsigned long lastStepTime = 0;
  unsigned long currentTime = millis();
  bool stepButtonCurrentlyPressed = stepFineAdjustButton.isCurrentlyPressed();
  
  if (stepButtonCurrentlyPressed && !stepButtonWasPressed) {
    // Початок натискання - виконуємо один крок та фіксуємо час
    stepper.move(1);
    stepButtonPressStartTime = currentTime;
    stepButtonWasPressed = true;
    lastStepTime = currentTime;
  } else if (stepButtonCurrentlyPressed && stepButtonWasPressed) {
    // Кнопка утримується - перевіряємо чи настав час швидкого повторення
    unsigned long pressDuration = currentTime - stepButtonPressStartTime;
    if (pressDuration >= STEP_BUTTON_LONG_PRESS_MS) {
      // Довге натискання - швидке повторення кроків
      if (currentTime - lastStepTime >= STEP_BUTTON_REPEAT_DELAY_MS) {
        stepper.move(1);
        lastStepTime = currentTime;
      }
    }
  } else if (!stepButtonCurrentlyPressed && stepButtonWasPressed) {
    // Кнопка відпущена
    stepButtonWasPressed = false;
  }
  
  // Використовуємо напрямок з меню Settings (замість фізичного перемикача)
  RotationDirection currentDirection = menu.getDirection();
  stepper.setDirectionInvert(currentDirection == DIR_CCW);
  
  // Отримуємо цільову позицію з меню
  int32_t targetPosition = menu.getTargetPosition();
  
  // Оновлюємо кроковий двигун (неблокуюче) - завжди викликаємо для виконання кроків
  stepper.update();
  
  // Виконуємо рух до цільової позиції (тільки якщо старт активний)
  if (startStop.getState()) {
    // Обчислюємо ефективну поточну позицію (включаючи кроки в процесі виконання)
    int32_t currentEffectivePosition = stepper.getPosition() + stepper.getRemaining();
    
    // Нормалізуємо поточну позицію до діапазону 0-360 градусів
    while (currentEffectivePosition < 0) {
      currentEffectivePosition += STEPS_360;
    }
    while (currentEffectivePosition >= STEPS_360) {
      currentEffectivePosition -= STEPS_360;
    }
    
    // Нормалізуємо цільову позицію
    int32_t normalizedTarget = targetPosition;
    while (normalizedTarget < 0) {
      normalizedTarget += STEPS_360;
    }
    while (normalizedTarget >= STEPS_360) {
      normalizedTarget -= STEPS_360;
    }
    
    // Обчислюємо різницю з урахуванням кругового діапазону (найкоротший шлях)
    int32_t stepsNeeded = normalizedTarget - currentEffectivePosition;
    
    // Якщо різниця більше половини кола, рухаємося в іншому напрямку (коротший шлях)
    if (stepsNeeded > STEPS_360 / 2) {
      stepsNeeded -= STEPS_360;
    } else if (stepsNeeded < -STEPS_360 / 2) {
      stepsNeeded += STEPS_360;
    }
    
    // Статична змінна для відстеження попередньої цільової позиції
    static int32_t lastTargetPosition = 0;
    static bool targetChanged = false;
    
    // Відстежуємо зміну цільової позиції
    if (targetPosition != lastTargetPosition) {
      targetChanged = true;
      lastTargetPosition = targetPosition;
    }
    
    // Перевіряємо, чи досягнуто цільовий кут за допомогою енкодера (для зупинки)
    uint16_t currentEncoderAngle = absoluteEncoder.readAngleInt();
    uint16_t targetAngle = menu.getTargetAngle();
    
    // Обчислюємо різницю між поточним та цільовим кутом (враховуючи круговий діапазон)
    int16_t angleDiff = currentEncoderAngle - targetAngle;
    if (angleDiff > 180) {
      angleDiff -= 360;
    } else if (angleDiff < -180) {
      angleDiff += 360;
    }
    
    // Гістерезис для стабільності: якщо вже рухаємося і наближаємося до цілі - не зупиняємося рано
    static bool wasMoving = false;
    static int16_t lastAngleDiff = 999;
    
    // Якщо досягнуто цільовий кут (допуск ±2 градуси) І (не рухалися або дуже близько) - вимикаємо двигун
    bool shouldStop = (abs(angleDiff) <= 2) && (!wasMoving || abs(angleDiff) <= 1);
    
    if (shouldStop) {
      startStop.setState(false);
      targetChanged = false;
      wasMoving = false;
      lastAngleDiff = 999;
    } else {
      // Використовуємо позицію двигуна для обчислення кроків (стабільніше)
      // Енкодер використовується тільки для зупинки
      stepper.setDistanceToTarget(abs(stepsNeeded));
      
      if (stepper.getRemaining() == 0) {
        // Використовуємо обчислені stepsNeeded (на основі позиції двигуна)
        if (abs(stepsNeeded) > 10) {  // Мінімальний поріг 10 кроків (≈1.1°) для стабільності
          // Обмежуємо максимальну швидкість руху (не більше 180° за раз)
          int32_t stepsToMove = stepsNeeded;
          if (stepsToMove > STEPS_360 / 2) {
            stepsToMove = STEPS_360 / 2;
          } else if (stepsToMove < -STEPS_360 / 2) {
            stepsToMove = -STEPS_360 / 2;
          }
          
          stepper.move(stepsToMove);
          targetChanged = false;
          wasMoving = true;
        } else {
          wasMoving = false;
        }
      } else {
        wasMoving = true;
      }
    }
  } else {
    // Якщо стоп - зупиняємо рух
    // (stepper.update() не викликається, тому рух зупиняється)
  }
  
  // Обробка збереження
  static unsigned long saveMessageTime = 0;
  if (menu.shouldSave()) {
    int32_t currentPosition = stepper.getPosition();
    uint8_t currentDirection = (uint8_t)menu.getDirection();
    int32_t currentStepperZero = menu.getStepperZeroPosition();
    memory.saveSettings(currentPosition, currentDirection, currentStepperZero);
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
            float encoderAngle = absoluteEncoder.readAngle();
            display.showSplashScreen(
              encoderAngle,
              menu.getTargetAngle(),
              startStop.getState(),
              stepper.isEnabled()
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
