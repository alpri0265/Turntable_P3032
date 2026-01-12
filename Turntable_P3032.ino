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
void stepper_step(int8_t dir)
{
  digitalWrite(DIR_PIN, dir > 0 ? HIGH : LOW);
  digitalWrite(STEP_PIN, HIGH);
  delayMicroseconds(4);
  digitalWrite(STEP_PIN, LOW);
  delayMicroseconds(600);   // швидкість
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
    int32_t next = current_position + d;

    if (next >= MIN_POS && next <= MAX_POS)
    {
      stepper_step(d);
      current_position = next;
    }
  }

  // Кнопка — зберегти позицію
  if (!digitalRead(ENC_BTN) && millis() - last_btn > 500)
  {
    eeprom_save();
    last_btn = millis();

    lcd.setCursor(0, 1);
    lcd.print("Saved to EEPROM");
    delay(400);
  }

  if (millis() - last_lcd > 100)
  {
    lcd_draw();
    last_lcd = millis();
  }
}
