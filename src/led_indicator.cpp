#include "led_indicator.h"

/**
 * @file led_indicator.cpp
 * @brief LED状态指示系统实现
 */

// 状态更新间隔（毫秒）
const unsigned long BLINK_SLOW_INTERVAL = 1000;  // 慢闪烁间隔 (1Hz)
const unsigned long BLINK_FAST_INTERVAL = 100;   // 快闪烁间隔 (5Hz)
const unsigned long BREATHING_INTERVAL = 250;    // 呼吸效果更新间隔 (2Hz)
const unsigned long ERROR_INTERVAL1 = 200;       // 错误状态短闪间隔
const unsigned long ERROR_INTERVAL2 = 500;       // 错误状态长闪间隔
const unsigned long ERROR_INTERVAL3 = 1100;      // 错误状态总周期
const unsigned long CONNECTING_WIFI_INTERVAL = 250;   // WiFi连接中状态闪烁间隔 (2Hz)
const unsigned long CONNECTING_MASTER_INTERVAL = 167; // 主从连接中状态闪烁间隔 (3Hz)


LEDIndicator::LEDIndicator(int pin)
    : ledPin(pin),
      currentState(LEDState::OFF),
      previousState(LEDState::OFF),
      currentPriority(LEDPriority::PRIORITY_LOW),
      previousPriority(LEDPriority::PRIORITY_LOW),
      brightness(0),
      fadeDirection(1),
      lastUpdate(0),
      stateStartTime(0) {
}

void LEDIndicator::begin() {
    pinMode(ledPin, OUTPUT);
    turnOff();
}

void LEDIndicator::setState(LEDState state, LEDPriority priority) {
    // 检查优先级，只有优先级相等或更高时才允许更新状态
    if (priority >= currentPriority) {
        previousState = currentState;
        previousPriority = currentPriority;
        currentState = state;
        currentPriority = priority;
        stateStartTime = millis();
        
        // 根据状态重置相关参数
        switch (state) {
            case LEDState::OFF:
                turnOff();
                break;
            case LEDState::ON:
                turnOn();
                break;
            case LEDState::BREATHING:
                brightness = 0;
                fadeDirection = 1;
                break;
            default:
                break;
        }
    }
}

void LEDIndicator::update() {
    unsigned long currentTime = millis();
    
    // 防止millis()溢出导致的问题
    if (currentTime < lastUpdate) {
        lastUpdate = currentTime;
        stateStartTime = currentTime;
    }
    
    lastUpdate = currentTime;
    
    // 根据当前状态调用相应的更新函数
    switch (currentState) {
        case LEDState::OFF:
            updateOff();
            break;
        case LEDState::ON:
            updateOn();
            break;
        case LEDState::BLINK_SLOW:
            updateBlinkSlow();
            break;
        case LEDState::BLINK_FAST:
            updateBlinkFast();
            break;
        case LEDState::BREATHING:
            updateBreathing();
            break;
        case LEDState::ERROR:
            updateError();
            break;
        case LEDState::CONNECTING:
            updateConnecting();
            break;
        case LEDState::CONNECTED:
            updateConnected();
            break;
    }
}

LEDState LEDIndicator::getCurrentState() const {
    return currentState;
}

LEDPriority LEDIndicator::getCurrentPriority() const {
    return currentPriority;
}

void LEDIndicator::forceOff() {
    previousState = currentState;
    previousPriority = currentPriority;
    currentState = LEDState::OFF;
    currentPriority = LEDPriority::PRIORITY_CRITICAL; // 强制关闭具有最高优先级
    turnOff();
}

void LEDIndicator::restorePreviousState() {
    if (previousState != LEDState::OFF || previousPriority != LEDPriority::PRIORITY_CRITICAL) {
        currentState = previousState;
        currentPriority = previousPriority;
        stateStartTime = millis();
    }
}

// 状态更新函数实现
void LEDIndicator::updateOff() {
    turnOff();
}

void LEDIndicator::updateOn() {
    turnOn();
}

void LEDIndicator::updateBlinkSlow() {
    unsigned long elapsedTime = millis() - stateStartTime;
    if (elapsedTime % BLINK_SLOW_INTERVAL < BLINK_SLOW_INTERVAL / 2) {
        turnOn();
    } else {
        turnOff();
    }
}

void LEDIndicator::updateBlinkFast() {
    unsigned long elapsedTime = millis() - stateStartTime;
    if (elapsedTime % BLINK_FAST_INTERVAL < BLINK_FAST_INTERVAL / 2) {
        turnOn();
    } else {
        turnOff();
    }
}

void LEDIndicator::updateBreathing() {
    unsigned long currentTime = millis();
    if (currentTime - lastUpdate > BREATHING_INTERVAL) {
        brightness += fadeDirection * 10;
        if (brightness <= 0) {
            brightness = 0;
            fadeDirection = 1;
        } else if (brightness >= 1023) {
            brightness = 1023;
            fadeDirection = -1;
        }
        setBrightness(brightness);
        lastUpdate = currentTime;
    }
}

void LEDIndicator::updateError() {
    unsigned long elapsedTime = millis() - stateStartTime;
    unsigned long cycleTime = elapsedTime % ERROR_INTERVAL3;
    
    if (cycleTime < ERROR_INTERVAL1) {
        // 第一阶段：短闪200ms
        turnOn();
    } else if (cycleTime < ERROR_INTERVAL1 + 200) {
        // 第二阶段：间隔200ms
        turnOff();
    } else if (cycleTime < ERROR_INTERVAL1 + 200 + ERROR_INTERVAL2) {
        // 第三阶段：长闪500ms
        turnOn();
    } else {
        // 第四阶段：间隔到下一个周期
        turnOff();
    }
}

void LEDIndicator::updateConnecting() {
    unsigned long elapsedTime = millis() - stateStartTime;
    
    // 根据优先级确定使用哪个间隔
    // WiFi连接中使用2Hz闪烁频率
    // 主从连接中使用3Hz闪烁频率
    unsigned long interval = CONNECTING_WIFI_INTERVAL; // 默认2Hz
    
    // 根据优先级确定闪烁频率
    if (currentPriority == LEDPriority::PRIORITY_HIGH) {
        // 高优先级表示主从连接中，使用3Hz闪烁频率
        interval = CONNECTING_MASTER_INTERVAL;
    }
    // 低优先级或正常优先级表示WiFi连接中，使用2Hz闪烁频率
    
    if (elapsedTime % interval < interval / 2) {
        turnOn();
    } else {
        turnOff();
    }
}

void LEDIndicator::updateConnected() {
    turnOn();
}

// 辅助函数实现
void LEDIndicator::setBrightness(int brightness) {
    analogWrite(ledPin, brightness);
}

void LEDIndicator::turnOff() {
    digitalWrite(ledPin, LOW);
}

void LEDIndicator::turnOn() {
    digitalWrite(ledPin, HIGH);
}