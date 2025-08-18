#ifndef HEARTBEAT_H
#define HEARTBEAT_H

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include "device.h"
#include "logger.h"

// 心跳包发送间隔（毫秒）
#define HEARTBEAT_INTERVAL 5000

// 连接超时时间（毫秒）
#define HEARTBEAT_TIMEOUT 15000

// 心跳检测状态
// 心跳检测状态
enum HeartbeatStatus {
    HEARTBEAT_DISCONNECTED,     // 未连接
    HEARTBEAT_CONNECTED,        // 已连接
    HEARTBEAT_STATUS_TIMEOUT    // 连接超时
};
class Heartbeat {
public:
    Heartbeat(Device* device);
    ~Heartbeat();

    // 初始化心跳检测
    void begin();
    
    // 处理心跳检测逻辑
    void handle();
    
    // 发送心跳包
    bool sendHeartbeat();
    
    // 处理接收到的心跳包
    void handleHeartbeat();
    
    // 获取当前心跳状态
    HeartbeatStatus getStatus();
    
    // 设置心跳状态
    void setStatus(HeartbeatStatus status);
    
    // 检查是否超时
    bool isTimeout();
    
    // 尝试重新连接
    bool reconnect();

private:
    Device* _device;
    HeartbeatStatus _status;
    unsigned long _lastHeartbeatTime;
    unsigned long _lastReceivedTime;
    WiFiClient* _client;
    WiFiServer* _server;
};

#endif // HEARTBEAT_H