#include "led_indicator.h"
#include "debug_utils.h"

LEDIndicator::LEDIndicator(GPIOController* gpio) 
    : gpioController(gpio), ledPin(2), initialized(false),
      currentState(), requestedState(),
      lastBlinkTime(0), blinkState(false),
      lastBreathTime(0), breathValue(0), breathDirection(1) {
}

bool LEDIndicator::initialize() {
    DEBUG_INFO_PRINT("LEDIndicator: Initializing...");
    
    if (!gpioController || !gpioController->isInitialized()) {
        DEBUG_ERROR_PRINT("LEDIndicator: GPIO Controller not initialized");
        return false;
    }
    
    // 配置默认引脚
    if (!configure()) {
        DEBUG_ERROR_PRINT("LEDIndicator: Failed to configure LED pin");
        return false;
    }
    
    // 初始化为关闭状态
    turnOff();
    
    DEBUG_INFO_PRINT("LEDIndicator: Initialized successfully on pin %d", ledPin);
    return true;
}

bool LEDIndicator::configure(uint8_t pin) {
    ledPin = pin;
    
    // 设置引脚模式为输出
    gpioController->pinMode(ledPin, OUTPUT);
    
    // 初始化为关闭状态
    gpioController->digitalWrite(ledPin, LOW);
    
    DEBUG_INFO_PRINT("LEDIndicator: Configured LED on pin %d", ledPin);
    return true;
}

void LEDIndicator::setState(uint8_t state) {
    setStateWithPriority(static_cast<LEDState>(state), LED_PRIORITY_NORMAL);
}

void LEDIndicator::setStateWithPriority(LEDState state, LEDPriority priority) {
    LEDStateInfo newState(state, priority);
    
    // 检查是否应该覆盖当前状态
    if (shouldOverrideCurrentState(newState)) {
        requestedState = newState;
        DEBUG_VERBOSE_PRINT("LEDIndicator: State changed to %d with priority %d", 
                           state, priority);
    } else {
        DEBUG_VERBOSE_PRINT("LEDIndicator: State change ignored due to lower priority");
    }
}

bool LEDIndicator::shouldOverrideCurrentState(const LEDStateInfo& newState) const {
    // 如果当前状态是错误状态且新状态不是错误状态，则不允许覆盖
    if (currentState.state == LED_STATE_ERROR && newState.state != LED_STATE_ERROR) {
        // 除非时间超过5秒，允许从错误状态恢复
        if (millis() - currentState.timestamp < 5000) {
            return false;
        }
    }
    
    // 如果新状态优先级更高，则允许覆盖
    if (newState.priority > currentState.priority) {
        return true;
    }
    
    // 如果优先级相同，允许更新
    if (newState.priority == currentState.priority) {
        return true;
    }
    
    // 其他情况不允许覆盖
    return false;
}

void LEDIndicator::update() {
    if (!initialized) {
        return;
    }
    
    // 如果有请求的状态，更新当前状态
    if (requestedState.state != currentState.state || 
        requestedState.priority != currentState.priority) {
        currentState = requestedState;
        applyState(currentState.state);
    }
    
    // 根据当前状态更新LED
    switch (currentState.state) {
        case LED_STATE_OFF:
        case LED_STATE_ON:
            // 这些状态已经在applyState中处理
            break;
            
        case LED_STATE_BLINK_SLOW:
        case LED_STATE_BLINK_NORMAL_2HZ:
        case LED_STATE_BLINK_NORMAL_3HZ:
        case LED_STATE_BLINK_FAST:
        case LED_STATE_BLINK_ULTRA_FAST:
        case LED_STATE_BLINK_VERY_SLOW:
        case LED_STATE_ERROR:
            updateBlinkState();
            break;
            
        case LED_STATE_BREATHE:
            updateBreathState();
            break;
            
        default:
            break;
    }
}

void LEDIndicator::applyState(LEDState state) {
    switch (state) {
        case LED_STATE_OFF:
            gpioController->digitalWrite(ledPin, LOW);
            blinkState = false;
            break;
            
        case LED_STATE_ON:
            gpioController->digitalWrite(ledPin, HIGH);
            blinkState = true;
            break;
            
        case LED_STATE_BLINK_SLOW:
        case LED_STATE_BLINK_NORMAL_2HZ:
        case LED_STATE_BLINK_NORMAL_3HZ:
        case LED_STATE_BLINK_FAST:
        case LED_STATE_BLINK_ULTRA_FAST:
        case LED_STATE_BLINK_VERY_SLOW:
        case LED_STATE_ERROR:
            // 初始化闪烁状态
            lastBlinkTime = millis();
            blinkState = false;
            gpioController->digitalWrite(ledPin, LOW);
            break;
            
        case LED_STATE_BREATHE:
            // 初始化呼吸状态
            lastBreathTime = millis();
            breathValue = 0;
            breathDirection = 1;
            break;
            
        default:
            gpioController->digitalWrite(ledPin, LOW);
            break;
    }
}

void LEDIndicator::updateBlinkState() {
    unsigned long currentTime = millis();
    
    // 根据状态确定闪烁间隔
    switch (currentState.state) {
        case LED_STATE_BLINK_SLOW:
            // 慢速闪烁 (1Hz)
            if (currentTime - lastBlinkTime >= 500) {
                blinkState = !blinkState;
                gpioController->digitalWrite(ledPin, blinkState ? HIGH : LOW);
                lastBlinkTime = currentTime;
            }
            break;
            
        case LED_STATE_BLINK_NORMAL_2HZ:
            // 中等闪烁 (2Hz)
            if (currentTime - lastBlinkTime >= 250) {
                blinkState = !blinkState;
                gpioController->digitalWrite(ledPin, blinkState ? HIGH : LOW);
                lastBlinkTime = currentTime;
            }
            break;
            
        case LED_STATE_BLINK_NORMAL_3HZ:
            // 中等闪烁 (3Hz)
            if (currentTime - lastBlinkTime >= 167) {
                blinkState = !blinkState;
                gpioController->digitalWrite(ledPin, blinkState ? HIGH : LOW);
                lastBlinkTime = currentTime;
            }
            break;
            
        case LED_STATE_BLINK_FAST:
            // 快速闪烁 (5Hz)
            if (currentTime - lastBlinkTime >= 100) {
                blinkState = !blinkState;
                gpioController->digitalWrite(ledPin, blinkState ? HIGH : LOW);
                lastBlinkTime = currentTime;
            }
            break;
            
        case LED_STATE_BLINK_ULTRA_FAST:
            // 超快速闪烁 (10Hz)
            if (currentTime - lastBlinkTime >= 50) {
                blinkState = !blinkState;
                gpioController->digitalWrite(ledPin, blinkState ? HIGH : LOW);
                lastBlinkTime = currentTime;
            }
            break;
            
        case LED_STATE_ERROR:
            // 错误状态（双闪模式）
            updateErrorState();
            break;
            
        case LED_STATE_BLINK_VERY_SLOW:
            // 超慢闪烁 (0.3Hz)
            if (currentTime - lastBlinkTime >= 1667) {
                blinkState = !blinkState;
                gpioController->digitalWrite(ledPin, blinkState ? HIGH : LOW);
                lastBlinkTime = currentTime;
            }
            break;
            
        default:
            // 默认处理
            if (currentTime - lastBlinkTime >= 500) {
                blinkState = !blinkState;
                gpioController->digitalWrite(ledPin, blinkState ? HIGH : LOW);
                lastBlinkTime = currentTime;
            }
            break;
    }
}

void LEDIndicator::updateBreathState() {
    unsigned long currentTime = millis();
    
    // 每20毫秒更新一次呼吸效果
    if (currentTime - lastBreathTime >= 20) {
        // 更新呼吸值
        breathValue += breathDirection * 5;
        
        // 检查边界
        if (breathValue >= 1023) {
            breathValue = 1023;
            breathDirection = -1;
        } else if (breathValue <= 0) {
            breathValue = 0;
            breathDirection = 1;
        }
        
        // 注意：ESP8266的analogWrite范围是0-1023
        gpioController->analogWrite(ledPin, breathValue);
        lastBreathTime = currentTime;
    }
}
void LEDIndicator::updateErrorState() {
    unsigned long currentTime = millis();
    static unsigned long errorStateStartTime = 0;
    static int errorSubState = 0; // 0: 短闪, 1: 间隔, 2: 长闪, 3: 间隔
    static unsigned long errorSubStateStartTime = 0;
    
    // 初始化错误状态
    if (errorStateStartTime == 0) {
        errorStateStartTime = currentTime;
        errorSubState = 0;
        errorSubStateStartTime = currentTime;
        this->gpioController->digitalWrite(this->ledPin, HIGH); // 开始短闪
    }
    
    switch (errorSubState) {
        case 0: // 短闪200ms
            if (currentTime - errorSubStateStartTime >= 200) {
                this->gpioController->digitalWrite(this->ledPin, LOW);
                errorSubState = 1;
                errorSubStateStartTime = currentTime;
            }
            break;
            
        case 1: // 间隔200ms
            if (currentTime - errorSubStateStartTime >= 200) {
                errorSubState = 2;
                errorSubStateStartTime = currentTime;
            }
            break;
            
        case 2: // 长闪500ms
            if (errorSubStateStartTime == currentTime) {
                this->gpioController->digitalWrite(this->ledPin, HIGH); // 开始长闪
            }
            if (currentTime - errorSubStateStartTime >= 500) {
                this->gpioController->digitalWrite(this->ledPin, LOW);
                errorSubState = 3;
                errorSubStateStartTime = currentTime;
            }
            break;
            
        case 3: // 间隔1100ms
            if (currentTime - errorSubStateStartTime >= 1100) {
                errorSubState = 0; // 重新开始循环
                errorSubStateStartTime = currentTime;
                errorStateStartTime = 0; // 重置状态机
            }
            break;
    }
}

// 便捷方法实现
// 便捷方法实现
void LEDIndicator::turnOn() {
    setStateWithPriority(LED_STATE_ON, LED_PRIORITY_NORMAL);
}

void LEDIndicator::turnOff() {
    setStateWithPriority(LED_STATE_OFF, LED_PRIORITY_LOW);
}
void LEDIndicator::blinkSlow() {
    setStateWithPriority(LED_STATE_BLINK_SLOW, LED_PRIORITY_NORMAL);
}

void LEDIndicator::blinkFast() {
    setStateWithPriority(LED_STATE_BLINK_FAST, LED_PRIORITY_HIGH);
}

void LEDIndicator::setErrorState() {
    setStateWithPriority(LED_STATE_ERROR, LED_PRIORITY_CRITICAL);
}

void LEDIndicator::blinkHotspotWait() {
    this->setStateWithPriority(LED_STATE_BLINK_ULTRA_FAST, LED_PRIORITY_HIGH);
}

void LEDIndicator::blinkWiFiConnecting() {
    this->setStateWithPriority(LED_STATE_BLINK_NORMAL_2HZ, LED_PRIORITY_NORMAL);
}

void LEDIndicator::blinkMasterSlaveConnecting() {
    this->setStateWithPriority(LED_STATE_BLINK_NORMAL_3HZ, LED_PRIORITY_NORMAL);
}

void LEDIndicator::blinkConfigSync() {
    this->setStateWithPriority(LED_STATE_BLINK_SLOW, LED_PRIORITY_NORMAL);
}

void LEDIndicator::blinkConfigMode() {
    this->setStateWithPriority(LED_STATE_BLINK_VERY_SLOW, LED_PRIORITY_LOW);
}