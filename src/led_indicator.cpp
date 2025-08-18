#include "led_indicator.h"

/**
 * @file led_indicator.cpp
 * @brief LED状态指示系统实现
 */

// 状态更新间隔（毫秒）
const unsigned long BLINK_SLOW_INTERVAL = 1000;  // 慢闪烁间隔
const unsigned long BLINK_FAST_INTERVAL = 200;   // 快闪烁间隔
const unsigned long BREATHING_INTERVAL = 20;     // 呼吸效果更新间隔
const unsigned long HEARTBEAT_INTERVAL1 = 200;   // 心跳效果第一阶段
const unsigned long HEARTBEAT_INTERVAL2 = 2000;  // 心跳效果第二阶段
const unsigned long ERROR_INTERVAL = 500;        // 错误状态闪烁间隔
const unsigned long CONNECTING_INTERVAL = 500;   // 连接中状态闪烁间隔


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
            case LEDState::HEARTBEAT:
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
        case LEDState::HEARTBEAT:
            updateHeartbeat();
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
        brightness += fadeDirection * 5;
        if (brightness <= 0) {
            brightness = 0;
            fadeDirection = 1;
        } else if (brightness >= 255) {
            brightness = 255;
            fadeDirection = -1;
        }
        setBrightness(brightness);
        lastUpdate = currentTime;
    }
}

void LEDIndicator::updateHeartbeat() {
    unsigned long elapsedTime = millis() - stateStartTime;
    unsigned long cycleTime = elapsedTime % (HEARTBEAT_INTERVAL1 + HEARTBEAT_INTERVAL1 + HEARTBEAT_INTERVAL2);
    
    if (cycleTime < HEARTBEAT_INTERVAL1) {
        // 第一阶段：快速变亮
        brightness = map(cycleTime, 0, HEARTBEAT_INTERVAL1, 0, 255);
    } else if (cycleTime < HEARTBEAT_INTERVAL1 + HEARTBEAT_INTERVAL1) {
        // 第二阶段：快速变暗
        brightness = map(cycleTime - HEARTBEAT_INTERVAL1, 0, HEARTBEAT_INTERVAL1, 255, 0);
    } else {
        // 第三阶段：保持熄灭
        brightness = 0;
    }
    setBrightness(brightness);
}

void LEDIndicator::updateError() {
    unsigned long elapsedTime = millis() - stateStartTime;
    if (elapsedTime % ERROR_INTERVAL < ERROR_INTERVAL / 2) {
        turnOn();
    } else {
        turnOff();
    }
}

void LEDIndicator::updateConnecting() {
    unsigned long elapsedTime = millis() - stateStartTime;
    if (elapsedTime % CONNECTING_INTERVAL < CONNECTING_INTERVAL / 2) {
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