#ifndef HEARTBEAT_H
#define HEARTBEAT_H

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include "device.h"
#include "logger.h"

// 前向声明
class TCPProtocol;
class WiFiManager;

// 心跳包类型标识
#define HEARTBEAT_PACKET_TYPE 0xFF01

// 心跳检测状态
enum HeartbeatStatus {
    HEARTBEAT_DISCONNECTED,     // 未连接
    HEARTBEAT_CONNECTED,        // 已连接
    HEARTBEAT_STATUS_TIMEOUT    // 连接超时
};
class Heartbeat {
public:
    Heartbeat(Device* device, TCPProtocol* tcpProtocol, WiFiManager* wifiManager);
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
    
    // 更新最后接收时间（用于通知连接活跃）
    void updateLastReceivedTime();

private:
    Device* _device;
    TCPProtocol* _tcpProtocol;
    WiFiManager* _wifiManager;
    HeartbeatStatus _status;
    unsigned long _lastHeartbeatTime;
    unsigned long _lastReceivedTime;
    bool _canStart;  // 标记是否可以开始心跳检测
};

#endif // HEARTBEAT_H