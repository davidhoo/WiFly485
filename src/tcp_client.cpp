#include "tcp_client.h"
#include "logger.h"
#include "error_handler.h"

TCPClient::TCPClient() :
  device(nullptr),
  rs485(nullptr),
  status(TCP_CLIENT_DISCONNECTED),
  serverPort(0),
  reconnectInterval(5000),
  maxReconnectRetries(10),
  currentRetryCount(0),
  lastReconnectAttempt(0),
  statusCallback(nullptr) {
}

TCPClient::~TCPClient() {
  disconnect();
}

bool TCPClient::begin(Device* device, RS485* rs485) {
  this->device = device;
  this->rs485 = rs485;
  
  if (!this->device) {
    REPORT_ERROR(ERROR_INVALID_PARAMETER, "TCPClient", "Invalid device pointer");
    return false;
  }
  
  if (!this->rs485) {
    REPORT_ERROR(ERROR_INVALID_PARAMETER, "TCPClient", "Invalid RS485 pointer");
    return false;
  }
  
  // 只有从设备才能启动TCP客户端
  if (!this->device->isSlave()) {
    LOG_W("TCPClient", "Only slave device can start TCP client");
    return false;
  }
  
  LOG_I("TCPClient", "TCP client initialized");
  return true;
}

bool TCPClient::connect(const String& serverIP, uint16_t serverPort) {
  if (serverIP.length() == 0 || serverPort == 0) {
    REPORT_ERROR(ERROR_INVALID_PARAMETER, "TCPClient", "Invalid server address");
    return false;
  }
  
  // 如果已经连接到相同的服务器，直接返回成功
  if (status == TCP_CLIENT_CONNECTED && 
      this->serverIP == serverIP && 
      this->serverPort == serverPort &&
      client.connected()) {
    LOG_I("TCPClient", "Already connected to %s:%d", serverIP.c_str(), serverPort);
    return true;
  }
  
  // 断开现有连接
  if (client.connected()) {
    client.stop();
  }
  
  this->serverIP = serverIP;
  this->serverPort = serverPort;
  currentRetryCount = 0;
  
  updateClientStatus(TCP_CLIENT_CONNECTING);
  
  LOG_I("TCPClient", "Attempting to connect to %s:%d", serverIP.c_str(), serverPort);
  
  // 尝试连接
  if (client.connect(serverIP.c_str(), serverPort)) {
    client.setNoDelay(true); // 禁用Nagle算法，减少延迟
    updateClientStatus(TCP_CLIENT_CONNECTED);
    currentRetryCount = 0;
    LOG_I("TCPClient", "Successfully connected to %s:%d", serverIP.c_str(), serverPort);
    return true;
  } else {
    updateClientStatus(TCP_CLIENT_ERROR);
    LOG_E("TCPClient", "Failed to connect to %s:%d", serverIP.c_str(), serverPort);
    return false;
  }
}

void TCPClient::disconnect() {
  if (client.connected()) {
    client.stop();
  }
  
  updateClientStatus(TCP_CLIENT_DISCONNECTED);
  currentRetryCount = 0;
  
  LOG_I("TCPClient", "Disconnected from server");
}

void TCPClient::handle() {
  // 检查连接状态
  if (status == TCP_CLIENT_CONNECTED) {
    if (!client.connected()) {
      LOG_W("TCPClient", "Connection lost to %s:%d", serverIP.c_str(), serverPort);
      updateClientStatus(TCP_CLIENT_ERROR);
    } else {
      // 处理服务器数据
      handleServerData();
      
      // 转发RS485数据到服务器
      forwardRS485DataToServer();
    }
  }
  
  // 处理重连逻辑
  if (status == TCP_CLIENT_ERROR && serverIP.length() > 0 && serverPort > 0) {
    unsigned long currentTime = millis();
    if (currentTime - lastReconnectAttempt >= reconnectInterval) {
      if (currentRetryCount < maxReconnectRetries) {
        lastReconnectAttempt = currentTime;
        if (attemptReconnect()) {
          // 重连成功，重置重试计数
          currentRetryCount = 0;
        } else {
          currentRetryCount++;
          LOG_W("TCPClient", "Reconnect attempt %d/%d failed", 
                currentRetryCount, maxReconnectRetries);
        }
      } else {
        LOG_E("TCPClient", "Max reconnect attempts reached, giving up");
        updateClientStatus(TCP_CLIENT_DISCONNECTED);
      }
    }
  }
}

TCPClientStatus TCPClient::getStatus() {
  return status;
}

String TCPClient::getStatusString() {
  switch (status) {
    case TCP_CLIENT_DISCONNECTED:
      return "Disconnected";
    case TCP_CLIENT_CONNECTING:
      return "Connecting";
    case TCP_CLIENT_CONNECTED:
      return "Connected";
    case TCP_CLIENT_ERROR:
      return "Error";
    default:
      return "Unknown";
  }
}

bool TCPClient::isConnected() {
  return status == TCP_CLIENT_CONNECTED && client.connected();
}

bool TCPClient::send(const uint8_t* data, size_t length) {
  if (!isConnected() || !data || length == 0) {
    return false;
  }
  
  size_t written = client.write(data, length);
  if (written == length) {
    LOG_V("TCPClient", "Sent %d bytes to server %s:%d", 
          length, serverIP.c_str(), serverPort);
    return true;
  } else {
    LOG_W("TCPClient", "Failed to send complete data to server, sent %d/%d bytes",
          written, length);
    return false;
  }
}

void TCPClient::setClientStatusCallback(ClientStatusCallback callback) {
  statusCallback = callback;
}

void TCPClient::setReconnectParams(unsigned long interval, uint8_t maxRetries) {
  reconnectInterval = interval;
  maxReconnectRetries = maxRetries;
}

String TCPClient::getServerIP() {
  return serverIP;
}

uint16_t TCPClient::getServerPort() {
  return serverPort;
}

void TCPClient::updateClientStatus(TCPClientStatus newStatus) {
  if (status != newStatus) {
    TCPClientStatus oldStatus = status;
    status = newStatus;
    
    LOG_I("TCPClient", "Status changed from %s to %s", 
          (oldStatus == TCP_CLIENT_DISCONNECTED ? "Disconnected" :
           oldStatus == TCP_CLIENT_CONNECTING ? "Connecting" :
           oldStatus == TCP_CLIENT_CONNECTED ? "Connected" :
           oldStatus == TCP_CLIENT_ERROR ? "Error" : "Unknown"),
          getStatusString().c_str());
    
    // 调用回调函数
    if (statusCallback) {
      statusCallback(newStatus, serverIP, serverPort);
    }
  }
}

void TCPClient::handleServerData() {
  int available = client.available();
  if (available > 0) {
    // 限制读取的数据量
    size_t toRead = min(available, (int)TCP_RECEIVE_BUFFER_SIZE);
    size_t bytesRead = client.read(receiveBuffer, toRead);
    
    if (bytesRead > 0) {
      LOG_V("TCPClient", "Received %d bytes from server %s:%d", 
            bytesRead, serverIP.c_str(), serverPort);
      processReceivedData(receiveBuffer, bytesRead);
    }
  }
}

void TCPClient::forwardRS485DataToServer() {
  if (!rs485 || !isConnected()) {
    return;
  }
  
  // 检查RS485是否有数据可读
  if (rs485->available()) {
    // 读取RS485数据
    int bytesRead = rs485->receive(receiveBuffer, TCP_RECEIVE_BUFFER_SIZE);
    
    if (bytesRead > 0) {
      LOG_V("TCPClient", "Received %d bytes from RS485, forwarding to server",
            bytesRead);
      
      // 转发到服务器
      send(receiveBuffer, bytesRead);
    }
  }
}

void TCPClient::processReceivedData(const uint8_t* data, size_t length) {
  if (!data || length == 0 || !rs485) {
    return;
  }
  
  // 将TCP接收到的数据转发到RS485
  if (rs485->send(data, length)) {
    LOG_V("TCPClient", "Forwarded %d bytes from TCP to RS485", length);
  } else {
    LOG_E("TCPClient", "Failed to forward data to RS485");
  }
}

bool TCPClient::attemptReconnect() {
  LOG_I("TCPClient", "Attempting to reconnect to %s:%d (attempt %d/%d)", 
        serverIP.c_str(), serverPort, currentRetryCount + 1, maxReconnectRetries);
  
  updateClientStatus(TCP_CLIENT_CONNECTING);
  
  if (client.connect(serverIP.c_str(), serverPort)) {
    client.setNoDelay(true);
    updateClientStatus(TCP_CLIENT_CONNECTED);
    LOG_I("TCPClient", "Reconnected successfully to %s:%d", 
          serverIP.c_str(), serverPort);
    return true;
  } else {
    updateClientStatus(TCP_CLIENT_ERROR);
    return false;
  }
}