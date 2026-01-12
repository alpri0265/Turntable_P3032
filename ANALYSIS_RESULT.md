# АНАЛІЗ ПРОБЛЕМИ ДОВГОГО НАТИСКАННЯ НА СПЛЕШ-ЕКРАНІ

## ЗНАЙДЕНІ ПРОБЛЕМИ

### ПРОБЛЕМА №1: ДЕБАУНС ЗАПІЗНЮЄ ФІКСАЦІЮ ПОЧАТКУ НАТИСКАННЯ

**Рядки 118-130:**
```cpp
// Простий debounce для визначення стабільного стану
if (rawState != lastRawStateSplash) {
  lastDebounceTimeSplash = currentTime;
}
lastRawStateSplash = rawState;

// Оновлюємо debounced стан якщо пройшло достатньо часу
if (currentTime - lastDebounceTimeSplash > BUTTON_DEBOUNCE_MS) {
  debouncedStateSplash = rawState;
}

bool buttonCurrentlyPressed = (debouncedStateSplash == LOW);

// Відстежуємо початок натискання кнопки (перехід з false в true)
if (buttonCurrentlyPressed && !buttonWasPressed) {
  buttonPressStartTime = currentTime;  // ❌ ФІКСУЄТЬСЯ З ЗАПІЗНЕННЯМ!
  buttonWasPressed = true;
  longPressDetected = false;
}
```

**ЧОМУ ЦЕ ПРОБЛЕМА:**
1. Коли користувач натискає кнопку, `rawState` стає `LOW` НЕГАЙНО
2. Але `debouncedStateSplash` оновлюється тільки через 50мс (BUTTON_DEBOUNCE_MS)
3. Тому `buttonCurrentlyPressed` стає `true` тільки через 50мс
4. І `buttonPressStartTime` фіксується через 50мс після реального натискання
5. **НАСЛІДОК**: Таймер довгого натискання запускається з запізненням на 50мс

**ПРИКЛАД:**
- Реальне натискання: t=0мс
- `buttonPressStartTime` фіксується: t=50мс (запізнення!)
- Довге натискання визначається: t=2050мс (2000мс + 50мс запізнення)
- Користувач натискає 2 секунди, але система визначає як коротке натискання!

---

### ПРОБЛЕМА №2: ВИКОРИСТАННЯ ГЛОБАЛЬНИХ СТАТИЧНИХ ЗМІННИХ

**Рядки 93-95 (ГЛОБАЛЬНІ змінні для сплеш-екрану):**
```cpp
static unsigned long buttonPressStartTime = 0;
static bool buttonWasPressed = false;
static bool longPressDetected = false;
```

**Рядки 190-192 (ОКРЕМІ змінні для меню):**
```cpp
static unsigned long buttonPressStartTimeMenu = 0;
static bool buttonWasPressedMenu = false;
static bool longPressDetectedMenu = false;
```

**ПРОБЛЕМА:**
- На сплеш-екрані використовуються ГЛОБАЛЬНІ змінні
- В меню використовуються ОКРЕМІ змінні
- Це створює конфлікт між двома логіками
- Якщо користувач швидко переключається між сплеш-екраном і меню, стани можуть змішатися

---

### ПРОБЛЕМА №3: СКИДАННЯ СТАНУ ВІДБУВАЄТЬСЯ ТІЛЬКИ ПРИ ПЕРШОМУ ВХОДІ

**Рядки 102-108:**
```cpp
if (!wasOnSplash) {
  buttonWasPressed = false;
  longPressDetected = false;
  wasOnSplash = true;
  buttonPressStartTime = 0;
}
```

**ПРОБЛЕМА:**
- Скидання відбувається тільки якщо `!wasOnSplash` (тобто при першому вході)
- Якщо користувач вже був на сплеш-екрані, а потім перейшов в меню і повернувся, то `wasOnSplash` вже `true`
- Тому скидання НЕ відбувається, і старих значень залишаються!

---

### ПРОБЛЕМА №4: ДЕБАУНС МОЖЕ "ВТРАТИТИ" НАТИСКАННЯ

**Рядки 140-152:**
```cpp
// Перевіряємо довге натискання КОЖНУ ітерацію loop, поки кнопка натиснута
if (buttonCurrentlyPressed && buttonWasPressed) {
  unsigned long pressDuration = currentTime - buttonPressStartTime;
  
  if (pressDuration >= LONG_PRESS_THRESHOLD_MS && !longPressDetected) {
    longPressDetected = true;
    // ...
  }
}
```

**ПРОБЛЕМА:**
- Перевірка виконується тільки якщо `buttonCurrentlyPressed == true`
- Якщо через дрижання кнопки `debouncedStateSplash` тимчасово стане `HIGH`, то `buttonCurrentlyPressed` стане `false`
- Тоді перевірка довгого натискання перестане виконуватися, навіть якщо кнопка все ще натиснута!

---

## ПОРІВНЯННЯ З РОБОЧОЮ ЛОГІКОЮ В МЕНЮ

Логіка в меню (рядки 188-252) **ІДЕНТИЧНА** за алгоритмом, але:

1. ✅ Використовує **ОКРЕМІ** статичні змінні (`buttonPressStartTimeMenu`, `buttonWasPressedMenu`, `longPressDetectedMenu`)
2. ✅ Не має конфлікту з іншими логіками
3. ❌ Має ту саму проблему з запізненням через debounce

**ВИСНОВОК:** Проблема НЕ в алгоритмі (він правильний), а в:
- Використанні глобальних змінних на сплеш-екрані
- Запізненні через debounce

---

## РЕКОМЕНДОВАНЕ ВИПРАВЛЕННЯ

### ВАРІАНТ 1: Фіксувати час натискання НЕГАЙНО (без чекання debounce)

```cpp
// Відстежуємо початок натискання НЕГАЙНО (raw state)
if (rawState == LOW && lastRawStateSplash == HIGH) {
  // Початок натискання - фіксуємо час НЕГАЙНО (без чекання debounce)
  buttonPressStartTime = currentTime;
  buttonWasPressed = true;
  longPressDetected = false;
}

// Для перевірки довгого натискання використовуємо raw state
if (rawState == LOW && buttonWasPressed) {
  unsigned long pressDuration = currentTime - buttonPressStartTime;
  if (pressDuration >= LONG_PRESS_THRESHOLD_MS && !longPressDetected) {
    longPressDetected = true;
    // ...
  }
}

// Для відпускання використовуємо debounced state
if (debouncedStateSplash == HIGH && buttonWasPressed) {
  // Обробка відпускання
}
```

### ВАРІАНТ 2: Використовувати окремі змінні для сплеш-екрану

```cpp
// Всередині блоку if (isOnSplash):
static unsigned long buttonPressStartTimeSplash = 0;
static bool buttonWasPressedSplash = false;
static bool longPressDetectedSplash = false;
```

### ВАРІАНТ 3: Комбінація обох варіантів (НАЙКРАЩЕ)

Використати окремі змінні + фіксувати час натискання негайно (raw state) + debounce тільки для відпускання.

---

## ДЕТАЛЬНИЙ КРОК-ЗА-КРОКОМ АНАЛІЗ ПОТОКУ ВИКОНАННЯ

### КОРИСТУВАЧ НАТИСКАЄ КНОПКУ НА 2 СЕКУНДИ:

**t=0мс:**
- `rawState = LOW` (кнопка натиснута)
- `lastDebounceTimeSplash = 0`
- `debouncedStateSplash = HIGH` (ще не оновився)
- `buttonCurrentlyPressed = false` (бо `debouncedStateSplash == HIGH`)
- ❌ `buttonPressStartTime` НЕ фіксується!

**t=50мс:**
- `rawState = LOW` (кнопка все ще натиснута)
- `currentTime - lastDebounceTimeSplash = 50мс >= 50мс` ✅
- `debouncedStateSplash = LOW` (оновлюється!)
- `buttonCurrentlyPressed = true` (бо `debouncedStateSplash == LOW`)
- ✅ `buttonPressStartTime = 50мс` (ФІКСУЄТЬСЯ З ЗАПІЗНЕННЯМ!)

**t=2050мс:**
- `pressDuration = 2050мс - 50мс = 2000мс`
- `pressDuration >= 2000мс` ✅
- `longPressDetected = true`
- ✅ Довге натискання визначається через 2050мс (а не 2000мс!)

**ПРОБЛЕМА:** Якщо користувач натискає кнопку рівно 2000мс, то реальний час натискання = 2000мс, але `buttonPressStartTime` фіксується через 50мс, тому `pressDuration = 1950мс`, і довге натискання НЕ визначається!

---

## ВИСНОВОК

Основна проблема - **ЗАПІЗНЕННЯ ФІКСАЦІЇ ЧАСУ НАТИСКАННЯ** через debounce.

Рішення: Фіксувати `buttonPressStartTime` **НЕГАЙНО** при зміні `rawState` (без чекання debounce), а debounce використовувати тільки для визначення стабільного стану для обробки відпускання.