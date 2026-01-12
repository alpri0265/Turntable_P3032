#include <LiquidCrystal.h>
#include <EEPROM.h>

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

// DM556
#define STEP_PIN 6
#define DIR_PIN  7

/* ================== МЕХАНІКА ================== */
#define STEPS_PER_REV 200
#define MICROSTEP     16
#define STEPS_360 (STEPS_PER_REV * MICROSTEP)

#define MIN_POS 0
#define MAX_POS STEPS_360

/* ================== ОБʼЄКТИ ================== */
LiquidCrystal lcd(LCD_RS, LCD_E, LCD_D4, LCD_D5, LCD_D6, LCD_D7);

/* ================== ЗМІННІ ================== */
volatile int16_t enc_delta = 0;
int32_t current_position = 0;

unsigned long last_lcd = 0;
unsigned long last_btn = 0;

// Неблокуючий рух крокового двигуна
int32_t stepper_remaining = 0;
unsigned long last_step_time = 0;
#define STEP_DELAY_US 600  // затримка між кроками в мікросекундах
#define STEP_PULSE_US 4    // тривалість імпульсу STEP

/* ================== EEPROM ================== */
void eeprom_load()
{
  EEPROM.get(0, current_position);
  if (current_position < MIN_POS || current_position > MAX_POS)
    current_position = 0;
}

void eeprom_save()
{
  EEPROM.put(0, current_position);
}

/* ================== ENCODER ISR ================== */
void isr_encA()
{
  if (digitalRead(ENC_A) == digitalRead(ENC_B))
    enc_delta++;
  else
    enc_delta--;
}

void isr_encB()
{
  if (digitalRead(ENC_A) != digitalRead(ENC_B))
    enc_delta++;
  else
    enc_delta--;
}

/* ================== STEPPER ================== */
void stepper_update()
{
  if (stepper_remaining == 0) return;
  
  unsigned long now = micros();
  
  // Перевіряємо, чи минуло достатньо часу для наступного кроку
  if (now - last_step_time >= STEP_DELAY_US)
  {
    // Встановлюємо напрямок (тільки якщо ще не встановлено)
    static int8_t current_dir = 0;
    int8_t new_dir = (stepper_remaining > 0) ? 1 : -1;
    if (current_dir != new_dir)
    {
      digitalWrite(DIR_PIN, new_dir > 0 ? HIGH : LOW);
      current_dir = new_dir;
      // Невелика затримка для стабілізації напрямку
      unsigned long dir_set_time = micros();
      while (micros() - dir_set_time < 2) {} // ~2 мкс
    }
    
    // Формуємо імпульс STEP
    digitalWrite(STEP_PIN, HIGH);
    unsigned long pulse_start = micros();
    while (micros() - pulse_start < STEP_PULSE_US) {} // чекаємо 4 мкс
    digitalWrite(STEP_PIN, LOW);
    
    // Оновлюємо позицію
    if (stepper_remaining > 0)
    {
      stepper_remaining--;
      current_position++;
    }
    else
    {
      stepper_remaining++;
      current_position--;
    }
    
    last_step_time = now;
  }
}

void stepper_move(int32_t steps)
{
  if (steps == 0) return;
  // Додаємо кроки до черги (не замінюємо)
  stepper_remaining += steps;
  // Якщо це перший крок, встановлюємо час
  if (last_step_time == 0)
    last_step_time = micros();
}

/* ================== LCD ================== */
void lcd_draw()
{
  uint16_t deg = (uint32_t)current_position * 360 / STEPS_360;

  lcd.setCursor(0, 0);
  lcd.print("Manual mode   ");

  lcd.setCursor(0, 1);
  lcd.print("Angle: ");
  if (deg < 100) lcd.print(' ');
  if (deg < 10)  lcd.print(' ');
  lcd.print(deg);
  lcd.print((char)223); // °
  lcd.print("   ");
}

void lcd_draw_angle_only()
{
  uint16_t deg = (uint32_t)current_position * 360 / STEPS_360;

  lcd.setCursor(7, 1);
  if (deg < 100) lcd.print(' ');
  if (deg < 10)  lcd.print(' ');
  lcd.print(deg);
  lcd.print((char)223); // °
  lcd.print("   ");
}

/* ================== SETUP ================== */
void setup()
{
  pinMode(STEP_PIN, OUTPUT);
  pinMode(DIR_PIN, OUTPUT);

  pinMode(ENC_A, INPUT_PULLUP);
  pinMode(ENC_B, INPUT_PULLUP);
  pinMode(ENC_BTN, INPUT_PULLUP);

  attachInterrupt(digitalPinToInterrupt(ENC_A), isr_encA, CHANGE);
  attachInterrupt(digitalPinToInterrupt(ENC_B), isr_encB, CHANGE);

  lcd.begin(16, 2);
  lcd.clear();

  eeprom_load();
  lcd_draw();
}

/* ================== LOOP ================== */
void loop()
{
  noInterrupts();
  int16_t d = enc_delta;
  enc_delta = 0;
  interrupts();

  if (d != 0)
  {
    // Обчислюємо цільову позицію з урахуванням поточної позиції та черги кроків
    int32_t target_pos = current_position + stepper_remaining + d;

    // Обмежуємо позицію в межах дозволеного діапазону
    if (target_pos < MIN_POS)
      target_pos = MIN_POS;
    else if (target_pos > MAX_POS)
      target_pos = MAX_POS;

    // Обчислюємо скільки кроків потрібно додати до черги
    int32_t steps_to_add = target_pos - current_position - stepper_remaining;
    
    if (steps_to_add != 0)
    {
      // Додаємо кроки до черги (неблокуюче)
      stepper_remaining += steps_to_add;
      if (last_step_time == 0)
        last_step_time = micros();
    }
  }
  
  // Оновлюємо кроковий двигун (неблокуюче)
  stepper_update();

  // Кнопка — зберегти позицію
  static bool saved_message_shown = false;
  if (!digitalRead(ENC_BTN) && millis() - last_btn > 500)
  {
    eeprom_save();
    last_btn = millis();
    saved_message_shown = true;

    lcd.setCursor(0, 1);
    lcd.print("Saved to EEPROM");
  }

  // Оновлюємо LCD
  static uint16_t last_deg = 999;
  uint16_t deg = (uint32_t)current_position * 360 / STEPS_360;
  
  if (millis() - last_lcd > 100)
  {
    // Якщо показували повідомлення про збереження, повертаємося до відображення кута
    if (saved_message_shown && millis() - last_btn > 400)
    {
      saved_message_shown = false;
      lcd_draw_angle_only();
      last_deg = deg;
    }
    // Оновлюємо кут тільки якщо він змінився
    else if (!saved_message_shown && last_deg != deg)
    {
      lcd_draw_angle_only();
      last_deg = deg;
    }
    last_lcd = millis();
  }
}
