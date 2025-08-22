#include "tcp_server.h"
#include "logger.h"
#include "error_handler.h"

TCPServer::TCPServer() :
  device(nullptr),
  rs485(nullptr),
  server(nullptr),
  status(TCP_DISCONNECTED),
  port(DEFAULT_MASTER_TCP_PORT),
  connectedClientCount(0),
  statusCallback(nullptr) {
  
  // 初始化客户端数组
  for (uint8_t i = 0; i < MAX_CLIENTS; i++) {
    clients[i].isConnected = false;
    clients[i].connectTime = 0;
  }
}

TCPServer::~TCPServer() {
  stop();
  if (server) {
    delete server;
    server = nullptr;
  }
}

bool TCPServer::begin(Device* device, RS485* rs485) {
  this->device = device;
  this->rs485 = rs485;
  
  if (!this->device) {
    REPORT_ERROR(ERROR_INVALID_PARAMETER, "TCPServer", "Invalid device pointer");
    return false;
  }
  
  if (!this->rs485) {
    REPORT_ERROR(ERROR_INVALID_PARAMETER, "TCPServer", "Invalid RS485 pointer");
    return false;
  }
  
  // 只有主设备才能启动TCP服务器
  if (!this->device->isMaster()) {
    LOG_W("TCPServer", "Only master device can start TCP server");
    return false;
  }
  
  // 创建WiFi服务器实例
  if (server) {
    delete server;
  }
  server = new WiFiServer(port);
  
  if (!server) {
    REPORT_ERROR(ERROR_OUT_OF_MEMORY, "TCPServer", "Failed to create WiFi server");
    return false;
  }
  
  LOG_I("TCPServer", "TCP server initialized on port %d", port);
  return true;
}

bool TCPServer::start() {
  if (!server) {
    REPORT_ERROR(ERROR_SYSTEM_INIT_FAILED, "TCPServer", "Server not initialized");
    return false;
  }
  
  if (status == TCP_LISTENING || status == TCP_CONNECTED) {
    LOG_W("TCPServer", "Server already started");
    return true;
  }
  
  // 启动服务器
  server->begin();
  server->setNoDelay(true); // 禁用Nagle算法，减少延迟
  
  updateConnectionStatus(TCP_LISTENING);
  
  LOG_I("TCPServer", "TCP server started, listening on port %d", port);
  return true;
}

void TCPServer::stop() {
  if (!server) {
    return;
  }
  
  // 断开所有客户端连接
  for (uint8_t i = 0; i < MAX_CLIENTS; i++) {
    if (clients[i].isConnected) {
      clients[i].client.stop();
      clients[i].isConnected = false;
    }
  }
  connectedClientCount = 0;
  
  // 停止服务器
  server->stop();
  updateConnectionStatus(TCP_DISCONNECTED);
  
  LOG_I("TCPServer", "TCP server stopped");
}

void TCPServer::handle() {
  if (!server || status == TCP_DISCONNECTED) {
    return;
  }
  
  // 处理新连接
  handleNewConnections();
  
  // 处理客户端数据
  handleClientData();
  
  // 处理客户端断开连接
  handleClientDisconnections();
  
  // 转发RS485数据到客户端
  forwardRS485DataToClients();
}

TCPConnectionStatus TCPServer::getStatus() {
  return status;
}

String TCPServer::getStatusString() {
  switch (status) {
    case TCP_DISCONNECTED:
      return "Disconnected";
    case TCP_LISTENING:
      return "Listening";
    case TCP_CONNECTED:
      return "Connected";
    case TCP_ERROR:
      return "Error";
    default:
      return "Unknown";
  }
}

uint8_t TCPServer::getConnectedClientCount() {
  return connectedClientCount;
}

uint16_t TCPServer::getPort() {
  return port;
}

bool TCPServer::sendToAllClients(const uint8_t* data, size_t length) {
  if (!data || length == 0) {
    return false;
  }
  
  bool success = false;
  for (uint8_t i = 0; i < MAX_CLIENTS; i++) {
    if (clients[i].isConnected && clients[i].client.connected()) {
      size_t written = clients[i].client.write(data, length);
      if (written == length) {
        success = true;
        LOG_V("TCPServer", "Sent %d bytes to client %s:%d", 
              length, clients[i].remoteIP.toString().c_str(), clients[i].remotePort);
      } else {
        LOG_W("TCPServer", "Failed to send complete data to client %s:%d, sent %d/%d bytes",
              clients[i].remoteIP.toString().c_str(), clients[i].remotePort, written, length);
      }
    }
  }
  
  return success;
}

void TCPServer::setConnectionStatusCallback(ConnectionStatusCallback callback) {
  statusCallback = callback;
}

void TCPServer::handleNewConnections() {
  if (!server->hasClient()) {
    return;
  }
  
  WiFiClient newClient = server->accept();
  if (!newClient) {
    return;
  }
  
  // 检查是否还有空闲的客户端槽位
  if (connectedClientCount >= MAX_CLIENTS) {
    LOG_W("TCPServer", "Maximum clients reached, rejecting new connection from %s", 
          newClient.remoteIP().toString().c_str());
    newClient.stop();
    return;
  }
  
  addClient(newClient);
}

void TCPServer::handleClientData() {
  for (uint8_t i = 0; i < MAX_CLIENTS; i++) {
    if (!clients[i].isConnected || !clients[i].client.connected()) {
      continue;
    }
    
    int available = clients[i].client.available();
    if (available > 0) {
      // 限制读取的数据量
      size_t toRead = min(available, (int)TCP_RECEIVE_BUFFER_SIZE);
      size_t bytesRead = clients[i].client.read(receiveBuffer, toRead);
      
      if (bytesRead > 0) {
        LOG_V("TCPServer", "Received %d bytes from client %s:%d", 
              bytesRead, clients[i].remoteIP.toString().c_str(), clients[i].remotePort);
        processReceivedData(receiveBuffer, bytesRead, i);
      }
    }
  }
}

void TCPServer::handleClientDisconnections() {
  for (uint8_t i = 0; i < MAX_CLIENTS; i++) {
    if (clients[i].isConnected && !clients[i].client.connected()) {
      LOG_I("TCPServer", "Client %s:%d disconnected", 
            clients[i].remoteIP.toString().c_str(), clients[i].remotePort);
      removeClient(i);
    }
  }
}

void TCPServer::addClient(WiFiClient& client) {
  // 找到空闲的客户端槽位
  for (uint8_t i = 0; i < MAX_CLIENTS; i++) {
    if (!clients[i].isConnected) {
      clients[i].client = client;
      clients[i].remoteIP = client.remoteIP();
      clients[i].remotePort = client.remotePort();
      clients[i].connectTime = millis();
      clients[i].isConnected = true;
      
      connectedClientCount++;
      
      LOG_I("TCPServer", "New client connected: %s:%d (total: %d)", 
            clients[i].remoteIP.toString().c_str(), clients[i].remotePort, connectedClientCount);
      
      // 更新状态
      if (status == TCP_LISTENING) {
        updateConnectionStatus(TCP_CONNECTED, clients[i].remoteIP);
      }
      
      break;
    }
  }
}

void TCPServer::removeClient(uint8_t index) {
  if (index >= MAX_CLIENTS || !clients[index].isConnected) {
    return;
  }
  
  IPAddress clientIP = clients[index].remoteIP;
  clients[index].client.stop();
  clients[index].isConnected = false;
  clients[index].connectTime = 0;
  
  connectedClientCount--;
  
  LOG_I("TCPServer", "Client removed: %s (total: %d)", 
        clientIP.toString().c_str(), connectedClientCount);
  
  // 如果没有客户端连接，更新状态为监听
  if (connectedClientCount == 0 && status == TCP_CONNECTED) {
    updateConnectionStatus(TCP_LISTENING);
  }
}

int8_t TCPServer::findClientIndex(WiFiClient& client) {
  for (uint8_t i = 0; i < MAX_CLIENTS; i++) {
    if (clients[i].isConnected && clients[i].client == client) {
      return i;
    }
  }
  return -1;
}

void TCPServer::updateConnectionStatus(TCPConnectionStatus newStatus, IPAddress clientIP) {
  if (status != newStatus) {
    TCPConnectionStatus oldStatus = status;
    status = newStatus;
    
    LOG_I("TCPServer", "Status changed from %s to %s", 
          (oldStatus == TCP_DISCONNECTED ? "Disconnected" :
           oldStatus == TCP_LISTENING ? "Listening" :
           oldStatus == TCP_CONNECTED ? "Connected" :
           oldStatus == TCP_ERROR ? "Error" : "Unknown"),
          getStatusString().c_str());
    
    // 调用回调函数
    if (statusCallback) {
      statusCallback(newStatus, clientIP);
    }
  }
}

void TCPServer::processReceivedData(const uint8_t* data, size_t length, uint8_t clientIndex) {
  if (!data || length == 0 || !rs485) {
    return;
  }
  
  // 将TCP接收到的数据转发到RS485
  if (rs485->send(data, length)) {
    LOG_V("TCPServer", "Forwarded %d bytes from TCP to RS485", length);
  } else {
    LOG_E("TCPServer", "Failed to forward data to RS485");
  }
}

void TCPServer::forwardRS485DataToClients() {
  if (!rs485 || connectedClientCount == 0) {
    return;
  }
  
  // 检查RS485是否有数据可读
  if (rs485->available()) {
    // 读取RS485数据
    int bytesRead = rs485->receive(receiveBuffer, TCP_RECEIVE_BUFFER_SIZE);
    
    if (bytesRead > 0) {
      LOG_V("TCPServer", "Received %d bytes from RS485, forwarding to %d clients",
            bytesRead, connectedClientCount);
      
      // 转发到所有连接的客户端
      sendToAllClients(receiveBuffer, bytesRead);
    }
  }
}