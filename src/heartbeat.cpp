#include "heartbeat.h"
#include "tcp_protocol.h"
#include "config.h"
#include "logger.h"

Heartbeat::Heartbeat(Device* device) 
    : _device(device), _status(HEARTBEAT_DISCONNECTED), 
      _lastHeartbeatTime(0), _lastReceivedTime(0),
      _client(nullptr), _server(nullptr) {
}

Heartbeat::~Heartbeat() {
    // 清理资源
}

void Heartbeat::begin() {
    LOG_I("Heartbeat", "Initializing heartbeat detection");
    _lastHeartbeatTime = millis();
    _lastReceivedTime = millis();
    
    // 根据设备角色初始化客户端或服务器
    if (_device->isMaster()) {
        _server = new WiFiServer(HEARTBEAT_PORT);
        _server->begin();
        LOG_I("Heartbeat", "Heartbeat server started on port %d", HEARTBEAT_PORT);
    } else {
        _client = new WiFiClient();
    }
}

void Heartbeat::handle() {
    // 检查是否超时
    if (isTimeout()) {
        LOG_W("Heartbeat", "Heartbeat timeout detected");
        _status = HEARTBEAT_STATUS_TIMEOUT;
        
        // 尝试重新连接
        if (reconnect()) {
            LOG_I("Heartbeat", "Reconnected successfully");
            _status = HEARTBEAT_CONNECTED;
        } else {
            LOG_E("Heartbeat", "Failed to reconnect");
            _status = HEARTBEAT_DISCONNECTED;
        }
    }
    
    // 主设备处理客户端连接
    if (_device->isMaster() && _server) {
        WiFiClient client = _server->available();
        if (client) {
            // 读取心跳包
            if (client.available()) {
                char buffer[10];
                int len = client.readBytes(buffer, sizeof(buffer) - 1);
                buffer[len] = '\0';
                
                if (strcmp(buffer, "PING") == 0) {
                    client.print("PONG");
                    _lastReceivedTime = millis();
                    _status = HEARTBEAT_CONNECTED;
                    LOG_D("Heartbeat", "Received PING from slave, sent PONG");
                }
            }
            client.stop();
        }
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
    
    // 确保已连接到主设备
    // 确保已连接到主设备
    // 确保已连接到主设备
    if (!_client || !_client->connected()) {
        // 尝试连接到主设备
        String masterIP = _device->getMasterIP();
        
        if (!_client->connect(masterIP.c_str(), HEARTBEAT_PORT)) {
            LOG_E("Heartbeat", "Failed to connect to master at %s", masterIP.c_str());
            return false;
        }
    }
    _client->print("PING");
    
    // 等待响应
    unsigned long startTime = millis();
    while (!_client->available() && (millis() - startTime) < 3000) {
        delay(10);
    }
    
    if (_client->available()) {
        String response = _client->readStringUntil('\n');
        if (response == "PONG") {
            _lastReceivedTime = millis();
            _status = HEARTBEAT_CONNECTED;
            return true;
        }
    }
    
    return false;
}

void Heartbeat::handleHeartbeat() {
    // 这个方法在handle()中已经处理了
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
    if (_device->isMaster()) {
        // 主设备不需要重新连接
        return true;
    }
    
    // 从设备重新连接到主设备
    if (_client) {
        _client->stop();
        delete _client;
    }
    _client = new WiFiClient();
    
    String masterIP = _device->getMasterIP();
    
    if (_client->connect(masterIP.c_str(), HEARTBEAT_PORT)) {
        LOG_I("Heartbeat", "Reconnected to master at %s", masterIP.c_str());
        _lastReceivedTime = millis();
        return true;
    } else {
        LOG_E("Heartbeat", "Failed to reconnect to master at %s", masterIP.c_str());
        return false;
    }
    }