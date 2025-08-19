#include "tcp_protocol.h"
#include <ESP8266WiFi.h>
#include "error_handler.h"

TCPProtocol::TCPProtocol() :
  device(nullptr),
  rs485(nullptr),
  server(nullptr),
  connectionStatus(TCP_DISCONNECTED),
  lastConnectionAttempt(0),
  connectionStartTime(0),
  statusCallback(nullptr),
  receiveBufferIndex(0) {
}

TCPProtocol::~TCPProtocol() {
  if (server) {
    delete server;
  }
  
  if (client.connected()) {
    client.stop();
  }
}

bool TCPProtocol::begin(Device* device, RS485* rs485) {
  this->device = device;
  this->rs485 = rs485;
  if (!this->device || !this->rs485) {
    REPORT_ERROR(ERROR_INVALID_PARAMETER, "TCPProtocol", "Invalid device or rs485");
    return false;
  }
  
  // 根据设备角色初始化服务器或客户端
  if (this->device->isMaster()) {
    // 主设备创建服务器，监听8888端口
    server = new WiFiServer(8888);
    server->begin();
    Serial.println("TCPProtocol: Server started on port 8888");
  } else {
    // 从设备不需要创建服务器
    server = nullptr;
    Serial.println("TCPProtocol: Client mode initialized");
  }
  
  return true;
}

void TCPProtocol::handle() {
  // 处理TCP连接和数据传输
  if (device->isMaster()) {
    // 主设备处理服务器连接
    handleServer();
  } else {
    // 从设备处理客户端连接
    handleClient();
  }
  
  // 处理连接状态
  if (connectionStatus == TCP_CONNECTING) {
    // 检查连接是否超时
    if (isConnectionTimedOut()) {
      Serial.println("TCPProtocol: Connection timeout");
      updateConnectionStatus(TCP_CONNECTION_FAILED);
      if (client.connected()) {
        client.stop();
      }
    }
  } else if (connectionStatus == TCP_CONNECTION_FAILED) {
    // 尝试重连（仅从设备）
    if (!device->isMaster()) {
      unsigned long currentTime = millis();
      if (currentTime - lastConnectionAttempt > RECONNECT_INTERVAL) {
        lastConnectionAttempt = currentTime;
        connectToMaster();
      }
    }
  }
}

bool TCPProtocol::sendData(const uint8_t* data, size_t length) {
  if (!data || length == 0) {
    REPORT_ERROR(ERROR_INVALID_PARAMETER, "TCPProtocol", "Invalid data or length");
    return false;
  }
  
  if (length > MAX_PACKET_SIZE) {
    REPORT_ERROR(ERROR_INVALID_PARAMETER, "TCPProtocol", "Data too large");
    return false;
  }
  
  // 发送数据包
  return sendPacket(data, length);
}

bool TCPProtocol::available() {
  if (device->isMaster()) {
    // 主设备检查是否有客户端连接并且有数据
    return client.connected() && client.available();
  } else {
    // 从设备检查客户端是否有数据
    return client.connected() && client.available();
  }
}

int TCPProtocol::receiveData(uint8_t* buffer, size_t bufferSize) {
  if (!buffer || bufferSize == 0) {
    REPORT_ERROR(ERROR_INVALID_PARAMETER, "TCPProtocol", "Invalid buffer or size");
    return -1;
  }
  
  // 接收数据包
  return receivePacket(buffer, bufferSize);
}

TCPConnectionStatus TCPProtocol::getConnectionStatus() {
  return connectionStatus;
}

String TCPProtocol::getConnectionStatusString() {
  switch (connectionStatus) {
    case TCP_DISCONNECTED:
      return "Disconnected";
    case TCP_CONNECTING:
      return "Connecting";
    case TCP_CONNECTED:
      return "Connected";
    case TCP_CONNECTION_FAILED:
      return "Connection Failed";
    default:
      return "Unknown";
  }
}

void TCPProtocol::setConnectionStatusCallback(ConnectionStatusCallback callback) {
  statusCallback = callback;
}

void TCPProtocol::updateConnectionStatus(TCPConnectionStatus status) {
  if (connectionStatus != status) {
    connectionStatus = status;
    
    // 调用回调函数
    if (statusCallback) {
      statusCallback(status);
    }
    
    // 打印状态变化
    Serial.printf("TCPProtocol: Connection status changed to %s\n", getConnectionStatusString().c_str());
  }
}

bool TCPProtocol::isConnectionTimedOut() {
  return (millis() - connectionStartTime) > CONNECTION_TIMEOUT;
}

uint16_t TCPProtocol::calculateChecksum(const uint8_t* data, size_t length) {
  if (!data || length == 0) {
    REPORT_ERROR(ERROR_INVALID_PARAMETER, "TCPProtocol", "Invalid data or length for checksum");
    return 0;
  }
  
  uint16_t checksum = 0;
  for (size_t i = 0; i < length; i++) {
    checksum += data[i];
  }
  
  return checksum;
}

bool TCPProtocol::sendPacket(const uint8_t* data, size_t length) {
  if (!data || length == 0 || length > MAX_PACKET_SIZE) {
    REPORT_ERROR(ERROR_INVALID_PARAMETER, "TCPProtocol", "Invalid data, length or data too large for packet");
    return false;
  }
  
  // 检查连接状态
  if (!client.connected()) {
    REPORT_ERROR(ERROR_TCP_CONNECTION_FAILED, "TCPProtocol", "Client not connected");
    return false;
  }
  
  // 构造数据包头部
  PacketHeader header;
  header.length = length;
  header.checksum = calculateChecksum(data, length);
  
  // 发送头部
  size_t headerSent = client.write((uint8_t*)&header, PACKET_HEADER_SIZE);
  if (headerSent != PACKET_HEADER_SIZE) {
    REPORT_ERROR(ERROR_TCP_SEND_FAILED, "TCPProtocol", "Failed to send packet header");
    return false;
  }
  
  // 发送数据
  size_t dataSent = client.write(data, length);
  if (dataSent != length) {
    REPORT_ERROR(ERROR_TCP_SEND_FAILED, "TCPProtocol", "Failed to send packet data");
    return false;
  }
  
  // 确保数据发送完成
  client.flush();
  
  return true;
}

int TCPProtocol::receivePacket(uint8_t* buffer, size_t bufferSize) {
  if (!buffer || bufferSize == 0) {
    REPORT_ERROR(ERROR_INVALID_PARAMETER, "TCPProtocol", "Invalid buffer or size for packet reception");
    return -1;
  }
  
  // 检查连接状态
  if (!client.connected()) {
    REPORT_ERROR(ERROR_TCP_CONNECTION_FAILED, "TCPProtocol", "Client not connected for packet reception");
    return -1;
  }
  
  // 检查是否有足够的数据可读（至少包含头部）
  if (client.available() < PACKET_HEADER_SIZE) {
    return 0; // 没有完整的数据包
  }
  
  // 读取数据包头部
  PacketHeader header;
  size_t headerRead = client.readBytes((uint8_t*)&header, PACKET_HEADER_SIZE);
  if (headerRead != PACKET_HEADER_SIZE) {
    REPORT_ERROR(ERROR_TCP_RECEIVE_FAILED, "TCPProtocol", "Failed to read packet header");
    return -1;
  }
  
  // 检查数据包长度是否有效
  if (header.length == 0 || header.length > MAX_PACKET_SIZE) {
    REPORT_ERROR(ERROR_TCP_RECEIVE_FAILED, "TCPProtocol", "Invalid packet length");
    return -1;
  }
  
  // 检查缓冲区大小是否足够
  if (header.length > bufferSize) {
    REPORT_ERROR(ERROR_TCP_RECEIVE_FAILED, "TCPProtocol", "Buffer too small for packet data");
    return -1;
  }
  
  // 检查是否有足够的数据可读
  if (client.available() < header.length) {
    // 数据不完整，重新放回头部数据（这里简化处理，实际应用中可能需要更复杂的缓冲区管理）
    Serial.println("TCPProtocol: Incomplete packet data");
    return 0;
  }
  
  // 读取数据
  size_t dataRead = client.readBytes(buffer, header.length);
  if (dataRead != header.length) {
    REPORT_ERROR(ERROR_TCP_RECEIVE_FAILED, "TCPProtocol", "Failed to read packet data");
    return -1;
  }
  
  // 验证校验和
  uint16_t calculatedChecksum = calculateChecksum(buffer, header.length);
  if (calculatedChecksum != header.checksum) {
    REPORT_ERROR(ERROR_TCP_RECEIVE_FAILED, "TCPProtocol", "Checksum mismatch");
    return -1;
  }
  
  return dataRead;
}

void TCPProtocol::handleServer() {
  if (!server) {
    return;
  }
  
  // 检查是否有新的客户端连接
  WiFiClient newClient = server->available();
  if (newClient) {
    if (!client.connected()) {
      // 接受新连接
      client = newClient;
      updateConnectionStatus(TCP_CONNECTED);
      Serial.printf("TCPProtocol: New client connected from %s\n", client.remoteIP().toString().c_str());
    } else {
      // 已经有连接，拒绝新连接
      newClient.stop();
      Serial.println("TCPProtocol: New client rejected, already connected");
    }
  }
  
  // 处理现有连接的数据传输
  if (client.connected()) {
    // 检查是否有来自RS485的数据需要转发到TCP客户端
    if (rs485->available()) {
      uint8_t buffer[256];
      int bytesRead = rs485->receive(buffer, sizeof(buffer));
      if (bytesRead > 0) {
        // 发送到TCP客户端
        sendData(buffer, bytesRead);
      }
    }
    
    // 检查是否有来自TCP客户端的数据需要转发到RS485
    if (client.available()) {
      uint8_t buffer[256];
      int bytesRead = receiveData(buffer, sizeof(buffer));
      if (bytesRead > 0) {
        // 发送到RS485
        rs485->send(buffer, bytesRead);
      }
    }
  } else {
    // 没有连接的客户端
    updateConnectionStatus(TCP_DISCONNECTED);
  }
}

void TCPProtocol::handleClient() {
  // 检查是否需要连接到主设备
  if (!client.connected()) {
    updateConnectionStatus(TCP_DISCONNECTED);
    unsigned long currentTime = millis();
    if (currentTime - lastConnectionAttempt > RECONNECT_INTERVAL) {
      lastConnectionAttempt = currentTime;
      connectToMaster();
    }
  } else {
    // 处理数据传输
    updateConnectionStatus(TCP_CONNECTED);
    
    // 检查是否有来自RS485的数据需要转发到主设备
    if (rs485->available()) {
      uint8_t buffer[256];
      int bytesRead = rs485->receive(buffer, sizeof(buffer));
      if (bytesRead > 0) {
        // 发送到主设备
        sendData(buffer, bytesRead);
      }
    }
    
    // 检查是否有来自主设备的数据需要转发到RS485
    if (client.available()) {
      uint8_t buffer[256];
      int bytesRead = receiveData(buffer, sizeof(buffer));
      if (bytesRead > 0) {
        // 发送到RS485
        rs485->send(buffer, bytesRead);
      }
    }
  }
}

bool TCPProtocol::connectToMaster() {
  // 这里需要实现连接到主设备的逻辑
  // 在实际应用中，可能需要通过mDNS或其他方式发现主设备的IP地址
  // 为了简化，我们假设主设备的IP地址是已知的或通过配置获取的
  
  // 示例代码，实际应用中需要替换为实际的主设备IP地址获取方式
  IPAddress masterIP(192, 168, 1, 100); // 示例IP地址
  
  Serial.printf("TCPProtocol: Connecting to master at %s:8888\n", masterIP.toString().c_str());
  
  updateConnectionStatus(TCP_CONNECTING);
  connectionStartTime = millis();
  
  // 尝试连接到主设备
  if (client.connect(masterIP, 8888)) {
    Serial.println("TCPProtocol: Connected to master");
    updateConnectionStatus(TCP_CONNECTED);
    return true;
  } else {
    Serial.println("TCPProtocol: Failed to connect to master");
    updateConnectionStatus(TCP_CONNECTION_FAILED);
    return false;
  }
}