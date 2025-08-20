#include "heartbeat.h"
#include "tcp_protocol.h"
#include "config.h"
#include "logger.h"

Heartbeat::Heartbeat(Device* device, TCPProtocol* tcpProtocol)
    : _device(device), _tcpProtocol(tcpProtocol), _status(HEARTBEAT_DISCONNECTED),
      _lastHeartbeatTime(0), _lastReceivedTime(0) {
}

Heartbeat::~Heartbeat() {
    // 清理资源
}

void Heartbeat::begin() {
    LOG_I("Heartbeat", "Initializing heartbeat detection");
    _lastHeartbeatTime = millis();
    _lastReceivedTime = millis();
    
    // 不再需要初始化独立的客户端或服务器
    // 心跳将复用业务通讯的TCP连接
}

void Heartbeat::handle() {
    // 检查是否超时
    if (isTimeout()) {
        LOG_W("Heartbeat", "Heartbeat timeout detected");
        _status = HEARTBEAT_STATUS_TIMEOUT;
    }
    
    // 从设备发送心跳包
    if (!_device->isMaster()) {
        unsigned long currentTime = millis();
        if (currentTime - _lastHeartbeatTime > HEARTBEAT_INTERVAL) {
            if (sendHeartbeat()) {
                _lastHeartbeatTime = currentTime;
                LOG_D("Heartbeat", "Heartbeat sent successfully");
            } else {
                LOG_W("Heartbeat", "Failed to send heartbeat");
            }
        }
    }
}

bool Heartbeat::sendHeartbeat() {
    if (_device->isMaster()) {
        // 主设备不需要发送心跳包
        return true;
    }
    
    // 检查TCP连接状态
    if (!_tcpProtocol || _tcpProtocol->getConnectionStatus() != TCP_CONNECTED) {
        LOG_E("Heartbeat", "TCP connection not available");
        _status = HEARTBEAT_DISCONNECTED;
        return false;
    }
    
    // 构造心跳包数据
    // 使用特殊的数据包类型标识心跳包
    uint8_t heartbeatData[4];
    heartbeatData[0] = (HEARTBEAT_PACKET_TYPE >> 8) & 0xFF;  // 高字节
    heartbeatData[1] = HEARTBEAT_PACKET_TYPE & 0xFF;        // 低字节
    heartbeatData[2] = 'P';
    heartbeatData[3] = 'I';
    
    // 通过TCP协议发送心跳包
    if (_tcpProtocol->sendData(heartbeatData, sizeof(heartbeatData))) {
        _lastReceivedTime = millis();
        _status = HEARTBEAT_CONNECTED;
        return true;
    } else {
        LOG_E("Heartbeat", "Failed to send heartbeat packet");
        return false;
    }
}

void Heartbeat::handleHeartbeat() {
    // 更新最后接收时间
    _lastReceivedTime = millis();
    _status = HEARTBEAT_CONNECTED;
}

HeartbeatStatus Heartbeat::getStatus() {
    return _status;
}

void Heartbeat::setStatus(HeartbeatStatus status) {
    _status = status;
}

bool Heartbeat::isTimeout() {
    return (millis() - _lastReceivedTime) > HEARTBEAT_TIMEOUT;
}

bool Heartbeat::reconnect() {
    // 心跳复用业务通讯连接，不再需要独立的重连逻辑
    // 业务通讯模块会处理连接管理
    if (_tcpProtocol && _tcpProtocol->getConnectionStatus() == TCP_CONNECTED) {
        _status = HEARTBEAT_CONNECTED;
        return true;
    } else {
        _status = HEARTBEAT_DISCONNECTED;
        return false;
    }
}