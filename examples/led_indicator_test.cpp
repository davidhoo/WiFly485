/*
 * LED指示器测试程序
 * 
 * 本程序演示了LED指示器的各种功能和状态
 */

#include <Arduino.h>
#include "debug_utils.h"
#include "gpio_controller.h"
#include "led_indicator.h"

// 硬件控制器实例
GPIOController gpioController;
LEDIndicator ledIndicator(&gpioController);

void setup() {
    Serial.begin(115200);
    delay(1000);
    
    DEBUG_INFO_PRINT("LED Indicator Test Starting...");
    
    // 初始化GPIO控制器
    if (!gpioController.initialize()) {
        DEBUG_ERROR_PRINT("Failed to initialize GPIO Controller");
        return;
    }
    
    // 初始化LED指示器
    if (!ledIndicator.initialize()) {
        DEBUG_ERROR_PRINT("Failed to initialize LED Indicator");
        return;
    }
    
    DEBUG_INFO_PRINT("LED Indicator Test Started Successfully");
    DEBUG_INFO_PRINT("Testing different LED states...");
}

void loop() {
    // 测试常亮状态
    DEBUG_INFO_PRINT("Testing LED_STATE_ON (Constant ON)");
    ledIndicator.turnOn();
    delay(3000);
    
    // 测试常灭状态
    DEBUG_INFO_PRINT("Testing LED_STATE_OFF (Constant OFF)");
    ledIndicator.turnOff();
    delay(3000);
    
    // 测试慢闪烁状态
    DEBUG_INFO_PRINT("Testing LED_STATE_BLINK_SLOW (Slow Blink)");
    ledIndicator.blinkSlow();
    delay(5000);
    
    // 测试快闪烁状态
    DEBUG_INFO_PRINT("Testing LED_STATE_BLINK_FAST (Fast Blink)");
    ledIndicator.blinkFast();
    delay(5000);
    
    // 测试错误状态
    DEBUG_INFO_PRINT("Testing LED_STATE_ERROR (Error State - Very Fast Blink)");
    ledIndicator.setErrorState();
    delay(5000);
    
    // 测试优先级系统
    DEBUG_INFO_PRINT("Testing Priority System");
    
    // 设置低优先级状态
    DEBUG_INFO_PRINT("Setting low priority state (Slow Blink)");
    ledIndicator.setStateWithPriority(LED_STATE_BLINK_SLOW, LED_PRIORITY_LOW);
    delay(2000);
    
    // 尝试设置更低优先级状态（应该被忽略）
    DEBUG_INFO_PRINT("Trying to set lower priority state (should be ignored)");
    ledIndicator.setStateWithPriority(LED_STATE_OFF, LED_PRIORITY_LOW);
    delay(2000);
    
    // 设置高优先级状态（应该覆盖）
    DEBUG_INFO_PRINT("Setting high priority state (should override)");
    ledIndicator.setStateWithPriority(LED_STATE_BLINK_FAST, LED_PRIORITY_HIGH);
    delay(3000);
    
    // 测试错误状态的优先级（应该覆盖所有其他状态）
    DEBUG_INFO_PRINT("Testing error state priority (should override all)");
    ledIndicator.setErrorState();
    delay(3000);
    
    // 等待一段时间后恢复正常状态
    delay(2000);
    ledIndicator.turnOn();
    delay(2000);
    
    DEBUG_INFO_PRINT("Test cycle completed. Restarting...");
    DEBUG_INFO_PRINT("=====================================");
}