#include "led_indicator.h"
#include "test_framework.h"
#include "config.h"

/**
 * @file test_led.cpp
 * @brief LED状态指示系统测试
 */

// 测试LED指示系统的基本功能
TEST(led_basic) {
    LEDIndicator led(LED_PIN); // 使用GPIO2进行测试
    led.begin();
    
    // 测试初始状态
    ASSERT_TRUE(led.getCurrentState() == LEDState::OFF);
    ASSERT_TRUE(led.getCurrentPriority() == LEDPriority::PRIORITY_LOW);
    
    // 测试设置状态
    led.setState(LEDState::ON, LEDPriority::PRIORITY_NORMAL);
    ASSERT_TRUE(led.getCurrentState() == LEDState::ON);
    ASSERT_TRUE(led.getCurrentPriority() == LEDPriority::PRIORITY_NORMAL);
    
    // 测试更新函数
    led.update();
    
    // 测试强制关闭
    led.forceOff();
    ASSERT_TRUE(led.getCurrentState() == LEDState::OFF);
    ASSERT_TRUE(led.getCurrentPriority() == LEDPriority::PRIORITY_CRITICAL);
    
    // 测试恢复之前状态
    led.restorePreviousState();
    ASSERT_TRUE(led.getCurrentState() == LEDState::ON);
    ASSERT_TRUE(led.getCurrentPriority() == LEDPriority::PRIORITY_NORMAL);
}

// 测试LED状态模式
TEST(led_states) {
    LEDIndicator led(LED_PIN); // 使用GPIO2进行测试
    led.begin();
    
    // 测试各种状态，每种状态保持一段时间以便观察
    led.setState(LEDState::ON, LEDPriority::PRIORITY_NORMAL);
    ASSERT_TRUE(led.getCurrentState() == LEDState::ON);
    for (int i = 0; i < 10; i++) {
        led.update();
        delay(100); // 1秒延迟
    }
    
    led.setState(LEDState::OFF, LEDPriority::PRIORITY_NORMAL);
    ASSERT_TRUE(led.getCurrentState() == LEDState::OFF);
    for (int i = 0; i < 10; i++) {
        led.update();
        delay(100); // 1秒延迟
    }
    
    led.setState(LEDState::BLINK_SLOW, LEDPriority::PRIORITY_NORMAL);
    ASSERT_TRUE(led.getCurrentState() == LEDState::BLINK_SLOW);
    for (int i = 0; i < 20; i++) {
        led.update();
        delay(100); // 2秒延迟
    }
    
    led.setState(LEDState::BLINK_FAST, LEDPriority::PRIORITY_NORMAL);
    ASSERT_TRUE(led.getCurrentState() == LEDState::BLINK_FAST);
    for (int i = 0; i < 20; i++) {
        led.update();
        delay(100); // 2秒延迟
    }
    
    led.setState(LEDState::BREATHING, LEDPriority::PRIORITY_NORMAL);
    ASSERT_TRUE(led.getCurrentState() == LEDState::BREATHING);
    for (int i = 0; i < 50; i++) {
        led.update();
        delay(100); // 5秒延迟
    }
    
    led.setState(LEDState::ERROR, LEDPriority::PRIORITY_NORMAL);
    ASSERT_TRUE(led.getCurrentState() == LEDState::ERROR);
    for (int i = 0; i < 20; i++) {
        led.update();
        delay(100); // 2秒延迟
    }
    
    led.setState(LEDState::CONNECTING, LEDPriority::PRIORITY_NORMAL);
    ASSERT_TRUE(led.getCurrentState() == LEDState::CONNECTING);
    for (int i = 0; i < 20; i++) {
        led.update();
        delay(100); // 2秒延迟
    }
    
    led.setState(LEDState::CONNECTED, LEDPriority::PRIORITY_NORMAL);
    ASSERT_TRUE(led.getCurrentState() == LEDState::CONNECTED);
    for (int i = 0; i < 10; i++) {
        led.update();
        delay(100); // 1秒延迟
    }
}

// 测试优先级管理
TEST(led_priority) {
    LEDIndicator led(LED_PIN); // 使用GPIO2进行测试
    led.begin();
    
    // 设置低优先级状态
    led.setState(LEDState::ON, LEDPriority::PRIORITY_LOW);
    ASSERT_TRUE(led.getCurrentState() == LEDState::ON);
    
    // 尝试用相同优先级更新状态，应该成功
    led.setState(LEDState::OFF, LEDPriority::PRIORITY_LOW);
    ASSERT_TRUE(led.getCurrentState() == LEDState::OFF); // 状态应该改变为OFF
    
    // 再次用相同优先级更新状态，应该成功
    led.setState(LEDState::ON, LEDPriority::PRIORITY_LOW);
    ASSERT_TRUE(led.getCurrentState() == LEDState::ON); // 状态应该改变为ON
    
    // 用更高优先级更新状态，应该成功
    led.setState(LEDState::OFF, LEDPriority::PRIORITY_NORMAL);
    ASSERT_TRUE(led.getCurrentState() == LEDState::OFF);
    
    // 再次用低优先级更新状态，应该失败
    led.setState(LEDState::ON, LEDPriority::PRIORITY_LOW);
    ASSERT_TRUE(led.getCurrentState() == LEDState::OFF);
}

// 测试错误状态模式
TEST(led_error_pattern) {
    LEDIndicator led(LED_PIN); // 使用GPIO2进行测试
    led.begin();
    
    // 测试错误状态模式，保持一段时间以便观察
    led.setState(LEDState::ERROR, LEDPriority::PRIORITY_HIGH);
    ASSERT_TRUE(led.getCurrentState() == LEDState::ERROR);
    for (int i = 0; i < 50; i++) {
        led.update();
        delay(100); // 5秒延迟，观察错误模式
    }
}
