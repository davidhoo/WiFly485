# LED指示器使用指南

## 概述

LED指示器模块为WiFly485系统提供了直观的设备状态指示功能。通过不同颜色、闪烁频率和模式的LED状态，用户可以快速了解设备的运行状态。

## LED状态定义

### 基本状态
### 基本状态

| 状态 | 枚举值 | 描述 | 显示模式 |
|------|--------|------|----------|
| LED_STATE_OFF | 0 | LED关闭 | 熄灭 |
| LED_STATE_ON | 1 | LED常亮 | 持续点亮 |
| LED_STATE_BLINK_SLOW | 2 | 慢闪烁 | 500毫秒间隔闪烁（1Hz） |
| LED_STATE_BLINK_NORMAL_2HZ | 3 | 中等闪烁 | 250毫秒间隔闪烁（2Hz） |
| LED_STATE_BLINK_NORMAL_3HZ | 4 | 中等闪烁 | 167毫秒间隔闪烁（3Hz） |
| LED_STATE_BLINK_FAST | 5 | 快闪烁 | 100毫秒间隔闪烁（5Hz） |
| LED_STATE_BLINK_ULTRA_FAST | 6 | 超快速闪烁 | 50毫秒间隔闪烁（10Hz） |
| LED_STATE_BREATHE | 7 | 呼吸效果 | 渐变亮灭（淡入淡出，2Hz） |
| LED_STATE_ERROR | 8 | 错误状态 | 双闪模式（短闪200ms+间隔200ms+长闪500ms+间隔1100ms） |
| LED_STATE_BLINK_VERY_SLOW | 9 | 超慢闪烁 | 1667毫秒间隔闪烁（0.3Hz） |
### 状态优先级

LED指示器支持状态优先级机制，确保重要的状态信息能够及时显示：

| 优先级 | 枚举值 | 描述 |
|--------|--------|------|
| LED_PRIORITY_LOW | 0 | 低优先级 |
| LED_PRIORITY_NORMAL | 1 | 正常优先级 |
| LED_PRIORITY_HIGH | 2 | 高优先级 |
| LED_PRIORITY_CRITICAL | 3 | 关键优先级（错误状态） |

## 使用方法

### 初始化

```cpp
#include "led_indicator.h"
#include "gpio_controller.h"

// 创建GPIO控制器实例
GPIOController gpioController;

// 创建LED指示器实例
LEDIndicator ledIndicator(&gpioController);

// 初始化
if (!ledIndicator.initialize()) {
    // 处理初始化失败
}
```

### 基本控制方法

```cpp
// 设置LED状态（使用默认优先级）
ledIndicator.setState(LED_STATE_ON);

// 设置LED状态（指定优先级）
ledIndicator.setStateWithPriority(LED_STATE_BLINK_FAST, LED_PRIORITY_HIGH);

// 便捷方法
ledIndicator.turnOn();                   // 常亮
ledIndicator.turnOff();                  // 熄灭
ledIndicator.blinkSlow();                // 慢闪烁 (1Hz)
ledIndicator.blinkFast();                // 快闪烁 (5Hz)
ledIndicator.setErrorState();            // 错误状态 (双闪模式)
ledIndicator.blinkHotspotWait();         // 热点等待 (10Hz超快速闪烁)
ledIndicator.blinkWiFiConnecting();      // WiFi连接中 (2Hz中等闪烁)
ledIndicator.blinkMasterSlaveConnecting(); // 主从连接中 (3Hz中等闪烁)
ledIndicator.blinkConfigSync();          // 配置同步 (1Hz慢速闪烁)
ledIndicator.blinkConfigMode();          // 配置模式 (0.3Hz超慢闪烁)
```

### 在主循环中更新

```cpp
void loop() {
    // 定期更新LED状态
    ledIndicator.update();
    
    // 其他代码...
}
```

## 系统集成状态映射

在WiFly485系统中，LED状态与设备运行状态的映射关系如下：

| 系统状态 | LED状态 | 描述 |
|----------|---------|------|
| 系统启动中 | 快速闪烁 (5Hz) | 设备正在初始化 |
| 热点等待 | 超快速闪烁 (10Hz) | WiFi热点已启动，等待客户端连接 |
| WiFi连接中 | 中等闪烁 (2Hz) | 正在连接WiFi网络 |
| 主从连接中 | 中等闪烁 (3Hz) | 从设备正在寻找主设备 |
| WiFi连接成功 | 常亮 | 网络连接正常 |
| WiFi连接断开 | 慢闪烁 (1Hz) | 网络连接异常 |
| 配置同步中 | 慢闪烁 (1Hz) | 正在与对端设备同步配置参数 |
| 数据传输中 | 呼吸模式 (2Hz) | RS485有数据收发 |
| 配置模式 | 超慢闪烁 (0.3Hz) | 处于Web配置模式 |
| 发生错误 | 双闪模式 | 设备出现错误 |
| 正常运行 | 常亮 | 设备运行正常 |

## 优先级规则

1. **错误状态优先**：LED_STATE_ERROR状态具有最高优先级，会覆盖所有其他状态
2. **高优先级覆盖低优先级**：高优先级状态会覆盖低优先级状态
3. **同优先级可更新**：相同优先级的状态可以相互替换
4. **错误状态持续时间**：错误状态会持续显示5秒钟，防止被意外覆盖

## 配置选项

LED指示器可以通过`configure()`方法配置使用的GPIO引脚：

```cpp
// 配置使用GPIO2（默认）
ledIndicator.configure(2);

// 配置使用其他GPIO引脚
ledIndicator.configure(0);  // GPIO0
ledIndicator.configure(4);  // GPIO4
```

## 调试信息

LED指示器模块使用调试系统输出相关信息：

- 初始化成功/失败信息
- 状态变更信息
- 优先级覆盖信息

确保在编译时启用了适当的调试级别以查看这些信息。

## 注意事项

1. **引脚冲突**：确保配置的LED引脚没有与其他硬件功能冲突
2. **电源限制**：注意LED的电流消耗，避免超过ESP8266的GPIO驱动能力
3. **PWM兼容性**：呼吸效果使用PWM输出，确保所选引脚支持PWM功能
4. **状态更新频率**：在主循环中定期调用`update()`方法以确保LED状态正确更新