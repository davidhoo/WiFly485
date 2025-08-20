#ifndef LED_INDICATOR_H
#define LED_INDICATOR_H

#include <Arduino.h>
#include <functional>

/**
 * @file led_indicator.h
 * @brief LED状态指示系统
 * 
 * 提供LED状态指示功能，支持多种状态模式和优先级管理
 */

// LED状态枚举
enum class LEDState {
    OFF,           // 关闭
    ON,            // 常亮
    BLINK_SLOW,    // 慢闪烁
    BLINK_FAST,    // 快闪烁
    BREATHING,     // 呼吸效果
    ERROR,         // 错误状态
    CONNECTING,    // 连接中
    CONNECTED      // 已连接
};

// 状态优先级枚举
enum class LEDPriority {
    PRIORITY_LOW = 0,
    PRIORITY_NORMAL = 1,
    PRIORITY_HIGH = 2,
    PRIORITY_CRITICAL = 3
};

class LEDIndicator {
public:
    /**
     * @brief 构造函数
     * @param pin LED连接的GPIO引脚
     */
    LEDIndicator(int pin);
    
    /**
     * @brief 初始化LED
     */
    void begin();
    
    /**
     * @brief 设置LED状态
     * @param state 状态类型
     * @param priority 状态优先级
     */
    void setState(LEDState state, LEDPriority priority = LEDPriority::PRIORITY_NORMAL);
    
    /**
     * @brief 更新LED状态（需要在loop中定期调用）
     */
    void update();
    
    /**
     * @brief 获取当前状态
     * @return 当前LED状态
     */
    LEDState getCurrentState() const;
    
    /**
     * @brief 获取当前优先级
     * @return 当前状态优先级
     */
    LEDPriority getCurrentPriority() const;
    
    /**
     * @brief 强制关闭LED
     */
    void forceOff();
    
    /**
     * @brief 恢复到之前的状态
     */
    void restorePreviousState();

private:
    int ledPin;
    LEDState currentState;
    LEDState previousState;
    LEDPriority currentPriority;
    LEDPriority previousPriority;
    
    // PWM相关
    int brightness;
    int fadeDirection;
    unsigned long lastUpdate;
    unsigned long stateStartTime;
    
    // 状态更新函数
    void updateOff();
    void updateOn();
    void updateBlinkSlow();
    void updateBlinkFast();
    void updateBreathing();
    void updateHeartbeat();
    void updateError();
    void updateConnecting();
    void updateConnected();
    
    // 辅助函数
    void setBrightness(int brightness);
    void turnOff();
    void turnOn();
};

#endif // LED_INDICATOR_H