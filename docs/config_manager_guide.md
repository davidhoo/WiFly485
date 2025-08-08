# ConfigManager 配置管理系统使用指南

## 概述

ConfigManager 是 WiFly485 项目的核心配置管理系统，负责处理设备配置的加载、保存、验证和同步。它基于 JSON 格式存储配置，支持热重载和主从设备间的配置同步。

## 功能特性

### 核心功能
- ✅ JSON 格式配置文件管理
- ✅ 配置验证和错误处理
- ✅ 默认配置自动生成
- ✅ 配置热重载（延迟保存）
- ✅ 主从设备配置同步
- ✅ 配置备份和恢复
- ✅ 事件驱动的配置变更通知

### 配置结构

配置文件采用分层结构，包含以下主要部分：

#### 1. 设备配置 (device)
```json
{
  "device": {
    "role": "master|slave",           // 设备角色
    "device_id": "unique_id",         // 唯一设备ID（基于MAC地址）
    "device_name": "WiFly485_Master"  // 设备名称
  }
}
```

#### 2. 网络配置 (network)
```json
{
  "network": {
    "ssid": "WiFi名称",               // WiFi网络名称
    "password": "WiFi密码",           // WiFi密码
    "hostname": "wifly485-master-xxx", // 设备主机名
    "ip_mode": "dhcp|static",         // IP获取模式
    "static_ip": "192.168.1.100",     // 静态IP（可选）
    "gateway": "192.168.1.1",         // 网关（可选）
    "subnet": "255.255.255.0",        // 子网掩码（可选）
    "dns1": "8.8.8.8",               // 主DNS（可选）
    "dns2": "8.8.4.4"                // 备DNS（可选）
  }
}
```

#### 3. RS485配置 (rs485)
```json
{
  "rs485": {
    "baud_rate": 9600,                // 波特率
    "data_bits": 8,                   // 数据位
    "stop_bits": 1,                   // 停止位
    "parity": "none|even|odd",        // 校验位
    "direction_pin": 2                // 方向控制引脚
  }
}
```

#### 4. 通信配置 (communication)
```json
{
  "communication": {
    "data_port": 8888,                // 数据传输端口
    "sync_port": 8889,                // 配置同步端口
    "auto_push": true,                // 自动推送配置（主设备）
    "auto_sync": true,                // 自动同步配置（从设备）
    "sync_interval": 300              // 同步间隔（秒）
  }
}
```

#### 5. 心跳配置 (heartbeat)
```json
{
  "heartbeat": {
    "interval": 5000,                 // 心跳间隔（毫秒）
    "timeout": 15000,                 // 超时时间（毫秒）
    "max_retries": 10                 // 最大重试次数
  }
}
```

## API 使用指南

### 基本使用

```cpp
#include "config_manager.h"

// 创建配置管理器实例
FileSystemController fsController;
ConfigManager configManager(&fsController);

// 初始化和启动
configManager.initialize();
configManager.start();

// 获取配置
const SystemConfig& config = configManager.getConfig();
String deviceId = configManager.getDeviceId();
DeviceRole role = configManager.getDeviceRole();
```

### 配置访问

```cpp
// 使用便捷宏
const SystemConfig& config = GET_CONFIG();
DeviceRole role = GET_DEVICE_ROLE();
String deviceId = GET_DEVICE_ID();
String hostname = GET_HOSTNAME();

// 直接访问配置项
uint32_t baudRate = config.rs485.baud_rate;
String ssid = config.network.ssid;
uint16_t dataPort = config.communication.data_port;
```

### 配置修改

```cpp
// 更新网络配置
NetworkConfig newNetwork = config.network;
newNetwork.ssid = "NewWiFiNetwork";
newNetwork.password = "NewPassword";
configManager.updateNetworkConfig(newNetwork);

// 更新RS485配置
RS485Config newRS485 = config.rs485;
newRS485.baud_rate = 19200;
configManager.updateRS485Config(newRS485);

// 更新完整配置
SystemConfig newConfig = config;
// 修改配置项...
configManager.updateFullConfig(newConfig);
```

### 配置同步（主从架构）

```cpp
// 主设备：获取配置用于同步
String configJson = configManager.getConfigForSync();
// 发送给从设备...

// 从设备：同步主设备配置
bool syncResult = configManager.syncFromMaster(masterConfigJson);

// 检查是否需要同步
bool needSync = configManager.isConfigSyncNeeded(masterConfigJson);
```

### 配置管理

```cpp
// 导出配置为JSON字符串
String configJson;
configManager.exportConfig(configJson);

// 从JSON字符串导入配置
configManager.importConfig(configJson);

// 重置为默认配置
configManager.resetToDefault();

// 手动保存配置
configManager.saveConfig();
```

## 事件系统集成

ConfigManager 与事件系统深度集成，支持以下事件：

```cpp
// 监听配置变更事件
LISTEN_EVENT(EVENT_CONFIG_CHANGED, "MyService", [](const EventData& event) {
    DEBUG_INFO_PRINT("Configuration changed: %s", event.message.c_str());
    // 处理配置变更...
});

// 监听状态更新事件
LISTEN_EVENT(EVENT_STATUS_UPDATE, "MyService", [](const EventData& event) {
    if (event.source == "ConfigManager") {
        DEBUG_INFO_PRINT("Config status: %s", event.message.c_str());
    }
});
```

## 配置验证

ConfigManager 提供完整的配置验证功能：

### 网络配置验证
- IP地址格式验证
- 静态IP配置完整性检查
- DNS服务器格式验证

### RS485配置验证
- 波特率范围检查（1200-115200）
- 数据位范围检查（5-8）
- 停止位范围检查（1-2）
- 校验位选项验证
- GPIO引脚范围检查

### 通信配置验证
- 端口号范围检查（1024-65535）
- 端口冲突检查
- 同步间隔范围检查（60-3600秒）

## 错误处理

```cpp
// 检查配置是否加载成功
if (!configManager.isConfigLoaded()) {
    DEBUG_ERROR_PRINT("Configuration not loaded");
    // 处理错误...
}

// 检查配置更新是否成功
if (!configManager.updateNetworkConfig(newNetwork)) {
    DEBUG_ERROR_PRINT("Failed to update network configuration");
    // 处理验证失败...
}
```

## 调试和监控

```cpp
// 打印完整配置信息
configManager.printConfig();

// 打印配置摘要
configManager.printConfigSummary();

// 检查配置是否有变更
if (configManager.hasConfigChanged()) {
    DEBUG_INFO_PRINT("Configuration has pending changes");
}
```

## 最佳实践

### 1. 初始化顺序
```cpp
// 正确的初始化顺序
fsController.initialize();      // 先初始化文件系统
configManager.initialize();     // 再初始化配置管理器
configManager.start();          // 最后启动服务
```

### 2. 配置修改
```cpp
// 推荐：使用专门的更新方法
configManager.updateNetworkConfig(newNetwork);

// 避免：直接修改配置结构体
// config.network.ssid = "new_ssid";  // 不推荐
```

### 3. 错误处理
```cpp
// 始终检查操作结果
if (!configManager.updateRS485Config(newRS485)) {
    DEBUG_ERROR_PRINT("RS485 config validation failed");
    // 恢复到之前的配置或使用默认值
}
```

### 4. 事件监听
```cpp
// 在服务启动时注册事件监听器
LISTEN_EVENT(EVENT_CONFIG_CHANGED, "MyService", [this](const EventData& event) {
    // 重新加载配置相关的设置
    this->reloadConfiguration();
});
```

## 故障排除

### 常见问题

1. **配置文件损坏**
   - ConfigManager 会自动创建备份文件
   - 如果主配置文件损坏，会尝试从备份恢复
   - 最终会回退到默认配置

2. **配置验证失败**
   - 检查配置参数是否在有效范围内
   - 查看调试输出获取详细错误信息
   - 使用 `printConfig()` 查看当前配置状态

3. **配置同步失败**
   - 确保主从设备在同一网络
   - 检查同步端口是否被占用
   - 验证网络连接状态

### 调试技巧

```cpp
// 启用详细调试输出
#define DEBUG_LEVEL 3

// 监控配置变更
LISTEN_EVENT(EVENT_CONFIG_CHANGED, "Debug", [](const EventData& event) {
    DEBUG_INFO_PRINT("Config changed by: %s, reason: %s", 
        event.source.c_str(), event.message.c_str());
});
```

## 性能考虑

- 配置变更采用延迟保存机制（5秒延迟），避免频繁写入文件系统
- JSON序列化使用2KB缓冲区，适合ESP8266内存限制
- 配置验证在内存中进行，不影响文件系统性能
- 事件通知异步处理，不阻塞主线程

---

**文档版本**: 1.0  
**最后更新**: 2025-08-08  
**适用版本**: WiFly485 v1.0+