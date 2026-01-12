# Налаштування LCD дисплея

## Підтримувані дисплеї
- **LCD1602** (16x2 символи)
- **LCD2004** (20x4 символи)

## Підтримувані інтерфейси
- **4-bit режим** (паралельний)
- **I2C режим** (через I2C модуль)

## Налаштування в config.h

### Вибір типу дисплея
```cpp
#define LCD_TYPE 1  // 1 = LCD1602 (16x2), 2 = LCD2004 (20x4)
```

### Вибір режиму підключення
```cpp
#define LCD_MODE 0  // 0 = 4-bit, 1 = I2C
```

## 4-bit режим (LCD_MODE = 0)

### Підключення для Arduino Mega:
- **LCD_RS** → пін 8
- **LCD_E** → пін 9
- **LCD_D4** → пін 10
- **LCD_D5** → пін 11
- **LCD_D6** → пін 12
- **LCD_D7** → пін 13
- **VSS** → GND
- **VDD** → 5V
- **V0** → контраст (потенціометр 10kΩ)
- **A** → 5V (підсвітка)
- **K** → GND (підсвітка)

### Підключення LCD2004:
Те саме, що і для LCD1602, але дисплей має 4 рядки замість 2.

## I2C режим (LCD_MODE = 1)

### Підключення для Arduino Mega:
- **SDA** → пін 20 (SDA)
- **SCL** → пін 21 (SCL)
- **VCC** → 5V
- **GND** → GND

### Налаштування адреси I2C:
```cpp
#define LCD_I2C_ADDRESS 0x27  // Стандартна адреса (може бути 0x3F)
```

### Визначення адреси I2C:
Якщо не впевнені в адресі, використайте I2C сканер:
```cpp
#include <Wire.h>

void setup() {
  Wire.begin();
  Serial.begin(9600);
  while (!Serial);
  Serial.println("I2C Scanner");
}

void loop() {
  byte error, address;
  int nDevices;
  Serial.println("Scanning...");
  nDevices = 0;
  for(address = 1; address < 127; address++ ) {
    Wire.beginTransmission(address);
    error = Wire.endTransmission();
    if (error == 0) {
      Serial.print("I2C device found at address 0x");
      if (address<16) Serial.print("0");
      Serial.print(address,HEX);
      Serial.println("  !");
      nDevices++;
    }
  }
  if (nDevices == 0) Serial.println("No I2C devices found\n");
  else Serial.println("done\n");
  delay(5000);
}
```

## Необхідні бібліотеки

### Для 4-bit режиму:
- `LiquidCrystal` (входить в стандартний набір Arduino IDE)

### Для I2C режиму:
- `Wire` (входить в стандартний набір Arduino IDE)
- `LiquidCrystal_I2C` (потрібно встановити через Library Manager)

Встановлення LiquidCrystal_I2C:
1. Відкрийте Arduino IDE
2. Скetch → Include Library → Manage Libraries
3. Пошук "LiquidCrystal_I2C"
4. Встановіть бібліотеку від Frank de Brabander або іншу сумісну

## Приклади налаштувань

### LCD1602 з 4-bit:
```cpp
#define LCD_TYPE 1
#define LCD_MODE 0
```

### LCD2004 з 4-bit:
```cpp
#define LCD_TYPE 2
#define LCD_MODE 0
```

### LCD1602 з I2C:
```cpp
#define LCD_TYPE 1
#define LCD_MODE 1
#define LCD_I2C_ADDRESS 0x27
```

### LCD2004 з I2C:
```cpp
#define LCD_TYPE 2
#define LCD_MODE 1
#define LCD_I2C_ADDRESS 0x27
```

## Відображення інформації

### LCD1602 (16x2):
- Рядок 0: Поточний кут (Cur: XXX°)
- Рядок 1: Цільовий кут (Tgt: XXX°)

### LCD2004 (20x4):
- Рядок 0: Заголовок (Turntable Control)
- Рядок 1: Поточний кут (Current: XXX°)
- Рядок 2: Цільовий кут (Target: XXX°)
- Рядок 3: Позиція в кроках (Steps: XXXX / 3200)
