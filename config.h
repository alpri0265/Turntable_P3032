#ifndef CONFIG_H
#define CONFIG_H

/* ================== ПІНИ ================== */
// LCD1602 (4-bit)
#define LCD_RS 8
#define LCD_E  9
#define LCD_D4 10
#define LCD_D5 11
#define LCD_D6 12
#define LCD_D7 13

// Encoder
#define ENC_A   2      // INT0
#define ENC_B   3      // INT1
#define ENC_BTN 4

// DM556 Stepper Driver
#define STEP_PIN 6
#define DIR_PIN  7

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
#define BUTTON_DEBOUNCE_MS 500  // затримка для кнопки
#define SAVE_MESSAGE_MS 400     // час показу повідомлення про збереження

#endif
