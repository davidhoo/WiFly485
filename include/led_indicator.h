#ifndef LED_INDICATOR_H
#define LED_INDICATOR_H

#include "hal_interfaces.h"
#include "gpio_controller.h"
#include "event_system.h"

// LED状态枚举
enum LEDState {
    LED_STATE_OFF = 0,              // 熄灭
    LED_STATE_ON = 1,               // 常亮
    LED_STATE_BLINK_SLOW = 2,       // 慢闪烁 (1Hz)
    LED_STATE_BLINK_NORMAL_2HZ = 3, // 中等闪烁 (2Hz)
    LED_STATE_BLINK_NORMAL_3HZ = 4, // 中等闪烁 (3Hz)
    LED_STATE_BLINK_FAST = 5,       // 快闪烁 (5Hz)
    LED_STATE_BLINK_ULTRA_FAST = 6, // 超快速闪烁 (10Hz)
    LED_STATE_BREATHE = 7,          // 呼吸效果 (2Hz渐变)
    LED_STATE_ERROR = 8,            // 错误状态（双闪模式）
    LED_STATE_BLINK_VERY_SLOW = 9   // 超慢闪烁 (0.3Hz)
};

// LED优先级枚举
enum LEDPriority {
    LED_PRIORITY_LOW = 0,     // 低优先级
    LED_PRIORITY_NORMAL = 1,  // 正常优先级
    LED_PRIORITY_HIGH = 2,    // 高优先级
    LED_PRIORITY_CRITICAL = 3 // 关键优先级
};

// LED状态信息结构
struct LEDStateInfo {
    LEDState state;
    LEDPriority priority;
    unsigned long timestamp;
    
    LEDStateInfo() : state(LED_STATE_OFF), priority(LED_PRIORITY_LOW), timestamp(0) {}
    LEDStateInfo(LEDState s, LEDPriority p) : state(s), priority(p), timestamp(millis()) {}
};

class LEDIndicator : public ILEDIndicator {
private:
    GPIOController* gpioController;
    uint8_t ledPin;
    bool initialized;
    
    // 当前状态和请求状态
    LEDStateInfo currentState;
    LEDStateInfo requestedState;
    
    // 闪烁控制
    unsigned long lastBlinkTime;
    bool blinkState;
    
    // 呼吸效果控制
    unsigned long lastBreathTime;
    int breathValue;
    int breathDirection;
    
    // 私有方法
    void applyState(LEDState state);
    void updateBlinkState();
    void updateBreathState();
    void updateErrorState();
    bool shouldOverrideCurrentState(const LEDStateInfo& newState) const;

public:
    LEDIndicator(GPIOController* gpio);
    virtual ~LEDIndicator() = default;
    
    bool initialize() override;
    bool configure(uint8_t pin = 2); // 配置LED引脚
    void setState(uint8_t state) override;
    void setStateWithPriority(LEDState state, LEDPriority priority);
    uint8_t getState() const override { return static_cast<uint8_t>(currentState.state); }
    void update() override;
    
    // 便捷方法
    void turnOn();
    void turnOff();
    void blinkSlow();
    void blinkFast();
    void setErrorState();
    void blinkHotspotWait();
    void blinkWiFiConnecting();
    void blinkMasterSlaveConnecting();
    void blinkConfigSync();
    void blinkConfigMode();
    
    bool isInitialized() const { return initialized; }
};

#endif