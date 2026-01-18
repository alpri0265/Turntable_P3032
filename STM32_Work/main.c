#include "main.h"

// Місце для оголошення змінних (аналог глобальних змінних Arduino)

int main(void) {
    // 1. Ініціалізація периферії (HAL, Clock, GPIO)
    HAL_Init();
    SystemClock_Config(); 
    
    // Тут буде ініціалізація ваших пінів (аналог setup() в Arduino)

    while (1) {
        // Головний цикл (аналог loop() в Arduino)
        
        // Ваша логіка тут
    }
}