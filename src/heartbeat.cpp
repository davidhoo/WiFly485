#include "heartbeat.h"
#include "tcp_protocol.h"
#include "wifi_manager.h"
#include "config.h"
#include "logger.h"

Heartbeat::Heartbeat(Device* device, TCPProtocol* tcpProtocol, WiFiManager* wifiManager)
    : _device(device), _tcpProtocol(tcpProtocol), _wifiManager(wifiManager), _status(HEARTBEAT_DISCONNECTED),
      _lastHeartbeatTime(0), _lastReceivedTime(0), _canStart(false) {
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
    // 检查WiFi和TCP连接状态，确定是否可以开始心跳检测
    if (!_canStart) {
        if (_wifiManager && _wifiManager->getConnectionStatus() == WIFI_CONNECTED &&
            _tcpProtocol && _tcpProtocol->getConnectionStatus() == TCP_CONNECTED) {
            _canStart = true;
            _lastReceivedTime = millis(); // 重置最后接收时间
            LOG_I("Heartbeat", "WiFi and TCP connected, starting heartbeat detection");
        } else {
            // 如果连接状态不满足要求，保持心跳状态为断开
            _status = HEARTBEAT_DISCONNECTED;
            return;
        }
    }
    
    // 检查连接状态是否发生变化
    if (_wifiManager && _wifiManager->getConnectionStatus() != WIFI_CONNECTED) {
        _canStart = false;
        _status = HEARTBEAT_DISCONNECTED;
        LOG_W("Heartbeat", "WiFi disconnected, stopping heartbeat detection");
        return;
    }
    
    if (_tcpProtocol && _tcpProtocol->getConnectionStatus() != TCP_CONNECTED) {
        _canStart = false;
        _status = HEARTBEAT_DISCONNECTED;
        LOG_W("Heartbeat", "TCP disconnected, stopping heartbeat detection");
        return;
    }
    
    // 检查是否超时
    if (isTimeout()) {
      if (_status != HEARTBEAT_STATUS_TIMEOUT) {
        LOG_W("Heartbeat", "Heartbeat timeout detected, last received time: %lu, current time: %lu, timeout: %lu",
              _lastReceivedTime, millis(), HEARTBEAT_RECEIVE_TIMEOUT);
        _status = HEARTBEAT_STATUS_TIMEOUT;
      }
    } else {
      // 如果没有超时，且之前是超时状态，则恢复连接状态
      if (_status == HEARTBEAT_STATUS_TIMEOUT) {
        _status = HEARTBEAT_CONNECTED;
        LOG_I("Heartbeat", "Heartbeat recovered from timeout");
      }
      // 对于主设备，如果没有超时且有最近的接收时间，保持连接状态
      else if (_device && _device->isMaster() && _status != HEARTBEAT_CONNECTED) {
        unsigned long timeSinceLastReceived = millis() - _lastReceivedTime;
        if (timeSinceLastReceived < HEARTBEAT_RECEIVE_TIMEOUT) {
          _status = HEARTBEAT_CONNECTED;
          LOG_D("Heartbeat", "Master device heartbeat status updated to Connected");
        }
      }
    }
    
    // 从设备发送心跳包
    if (!_device->isMaster()) {
      unsigned long currentTime = millis();
      if (currentTime - _lastHeartbeatTime > HEARTBEAT_SEND_INTERVAL) {
        LOG_D("Heartbeat", "Sending heartbeat packet");
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
        LOG_D("Heartbeat", "Master device does not send heartbeat packets");
        return true;
    }
    
    // 检查TCP连接状态
    if (!_tcpProtocol || _tcpProtocol->getConnectionStatus() != TCP_CONNECTED) {
        LOG_E("Heartbeat", "TCP connection not available for sending heartbeat");
        _status = HEARTBEAT_DISCONNECTED;
        return false;
    }
    
    // 构造心跳包数据
    // 使用特殊的数据包类型标识心跳包
    uint8_t heartbeatData[4];
    heartbeatData[0] = (HEARTBEAT_PACKET_TYPE >> 8) & 0xFF;  // 高字节 (0xFF)
    heartbeatData[1] = HEARTBEAT_PACKET_TYPE & 0xFF;        // 低字节 (0x01)
    heartbeatData[2] = 'P';
    heartbeatData[3] = 'I';
    
    LOG_D("Heartbeat", "Sending heartbeat packet with type: 0x%04X", HEARTBEAT_PACKET_TYPE);
    
    // 通过TCP协议发送心跳包
    if (_tcpProtocol->sendData(heartbeatData, sizeof(heartbeatData))) {
        _lastHeartbeatTime = millis();  // 更新发送时间而不是接收时间
        _status = HEARTBEAT_CONNECTED;
        LOG_D("Heartbeat", "Heartbeat packet sent successfully");
        return true;
    } else {
        LOG_E("Heartbeat", "Failed to send heartbeat packet");
        _status = HEARTBEAT_STATUS_TIMEOUT; // 发送失败也标记为超时
        return false;
    }
}

void Heartbeat::updateLastReceivedTime() {
    unsigned long currentTime = millis();
    // 只有在当前时间比最后接收时间晚的情况下才更新
    // 这样可以避免在短时间内多次更新
    if (currentTime > _lastReceivedTime) {
        LOG_D("Heartbeat", "Updating last received time from %lu to %lu", _lastReceivedTime, currentTime);
        _lastReceivedTime = currentTime;
    }
}

void Heartbeat::handleHeartbeat() {
  LOG_D("Heartbeat", "Handling received heartbeat packet");
  // 更新最后接收时间
  unsigned long previousReceivedTime = _lastReceivedTime;
  _lastReceivedTime = millis();
  
  // 对于主设备，接收到心跳包就表示连接正常
  if (_device && _device->isMaster()) {
    _status = HEARTBEAT_CONNECTED;
    LOG_I("Heartbeat", "Master device received heartbeat, status updated to Connected");
  }
  
  LOG_D("Heartbeat", "Heartbeat packet handled, last received time updated from %lu to %lu, interval: %lu ms",
        previousReceivedTime, _lastReceivedTime, _lastReceivedTime - previousReceivedTime);
}

HeartbeatStatus Heartbeat::getStatus() {
    return _status;
}

void Heartbeat::setStatus(HeartbeatStatus status) {
    _status = status;
}

bool Heartbeat::isTimeout() {
    return (millis() - _lastReceivedTime) > HEARTBEAT_RECEIVE_TIMEOUT;
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