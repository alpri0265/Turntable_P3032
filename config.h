#ifndef CONFIG_H
#define CONFIG_H

/* ================== ПІНИ ================== */
// LCD 4-bit mode (для LCD1602 або LCD2004)
#define LCD_RS 8
#define LCD_E  9
#define LCD_D4 10
#define LCD_D5 11
#define LCD_D6 12
#define LCD_D7 13

// LCD I2C mode (для LCD1602 або LCD2004 з I2C модулем)
#define LCD_I2C_ADDRESS 0x27  // Стандартна адреса I2C (може бути 0x3F)

// Автоматичне визначення пінів I2C залежно від платформи
#if defined(ARDUINO_AVR_NANO) || defined(ARDUINO_AVR_UNO)
  #define LCD_I2C_SDA A4       // SDA для Arduino Nano/Uno
  #define LCD_I2C_SCL A5       // SCL для Arduino Nano/Uno
#else
  #define LCD_I2C_SDA 20       // SDA для Arduino Mega
  #define LCD_I2C_SCL 21       // SCL для Arduino Mega
#endif

/* ================== LCD НАЛАШТУВАННЯ ================== */
// Виберіть тип дисплея: 1 = LCD1602 (16x2), 2 = LCD2004 (20x4)
#define LCD_TYPE 2

// Виберіть режим: 0 = 4-bit, 1 = I2C
#define LCD_MODE 1  // I2C (працює на Mega та Nano)

// Encoder (інкрементальний)
#define ENC_A   2      // INT0
#define ENC_B   3      // INT1
#define ENC_BTN 4

// Абсолютний енкодер P3022-CW360 (аналоговий)
#define ABS_ENC_PIN A0  // Аналоговий пін для абсолютного енкодера

// DM556 Stepper Driver
#define STEP_PIN 6
#define DIR_PIN  7
#define ENABLE_PIN 9  // Пін для ENABLE (зняття з утримання)

// Кнопка встановлення нуля абсолютного енкодера
#define ENCODER_ZERO_BUTTON_PIN 5  // Пін для кнопки обнулення енкодера

// Кнопка перемикання розрядів (для введення кута вручну)
// Автоматичне визначення пінів залежно від платформи
#if defined(ARDUINO_AVR_NANO) || defined(ARDUINO_AVR_UNO)
  #define DIGIT_MODE_BUTTON_PIN 8  // Пін для кнопки перемикання розрядів (Nano/Uno)
#else
  #define DIGIT_MODE_BUTTON_PIN 8  // Пін для кнопки перемикання розрядів (Mega)
#endif

// Кнопка старт-стоп зі світлодіодом
// Автоматичне визначення пінів залежно від платформи
#if defined(ARDUINO_AVR_NANO) || defined(ARDUINO_AVR_UNO)
  #define START_STOP_BUTTON_PIN 12  // Пін для кнопки старт-стоп (Nano/Uno)
  #define START_STOP_LED_PIN 11     // Пін для світлодіода старт-стоп (Nano/Uno)
#else
  #define START_STOP_BUTTON_PIN 22  // Пін для кнопки старт-стоп (Mega)
  #define START_STOP_LED_PIN 23     // Пін для світлодіода старт-стоп (Mega)
#endif

/* ================== МЕХАНІКА ================== */
#define STEPS_PER_REV 200
#define MICROSTEP     16
#define STEPS_360 (STEPS_PER_REV * MICROSTEP)

#define MIN_POS 0
#define MAX_POS STEPS_360

/* ================== ПАРАМЕТРИ ================== */
#define STEP_DELAY_US 600  // затримка між кроками в мікросекундах
#define STEP_PULSE_US 4    // тривалість імпульсу STEP
#define LCD_UPDATE_MS 100  // інтервал оновлення LCD
#define BUTTON_DEBOUNCE_MS 50  // затримка для кнопки (зменшено для кращої відповіді)
#define SAVE_MESSAGE_MS 400     // час показу повідомлення про збереження
#define LONG_PRESS_THRESHOLD_MS 2000 // Час для довгого натискання кнопки енкодера (мс) - 2 секунди

#endif
