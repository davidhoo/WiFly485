#include "gpio_controller.h"
#include "debug_utils.h"

GPIOController::GPIOController() : initialized(false) {
}

bool GPIOController::initialize() {
    DEBUG_INFO_PRINT("Initializing GPIO Controller...");
    
    // ESP8266的GPIO初始化通常不需要特殊操作
    // 只需要标记为已初始化
    initialized = true;
    
    DEBUG_INFO_PRINT("GPIO Controller initialized successfully");
    return true;
}

void GPIOController::pinMode(uint8_t pin, uint8_t mode) {
    if (!initialized) {
        DEBUG_ERROR_PRINT("GPIO Controller not initialized");
        return;
    }
    
    ::pinMode(pin, mode);
    DEBUG_VERBOSE_PRINT("Set pin %d to mode %d", pin, mode);
}

void GPIOController::digitalWrite(uint8_t pin, uint8_t value) {
    if (!initialized) {
        DEBUG_ERROR_PRINT("GPIO Controller not initialized");
        return;
    }
    
    ::digitalWrite(pin, value);
    DEBUG_VERBOSE_PRINT("Set pin %d to %s", pin, value ? "HIGH" : "LOW");
}

int GPIOController::digitalRead(uint8_t pin) {
    if (!initialized) {
        DEBUG_ERROR_PRINT("GPIO Controller not initialized");
        return LOW;
    }
    
    int value = ::digitalRead(pin);
    DEBUG_VERBOSE_PRINT("Read pin %d: %s", pin, value ? "HIGH" : "LOW");
    return value;
}

int GPIOController::analogRead(uint8_t pin) {
    if (!initialized) {
        DEBUG_ERROR_PRINT("GPIO Controller not initialized");
        return 0;
    }
    
    int value = ::analogRead(pin);
    DEBUG_VERBOSE_PRINT("Analog read pin %d: %d", pin, value);
    return value;
}

void GPIOController::analogWrite(uint8_t pin, int value) {
    if (!initialized) {
        DEBUG_ERROR_PRINT("GPIO Controller not initialized");
        return;
    }
    
    ::analogWrite(pin, value);
    DEBUG_VERBOSE_PRINT("Analog write pin %d: %d", pin, value);
}