#include "tcp_protocol.h"
#include <ESP8266WiFi.h>
#include "error_handler.h"
#include "heartbeat.h"

TCPProtocol::TCPProtocol() :
  device(nullptr),
  rs485(nullptr),
  mdnsService(nullptr),  // 初始化mDNS服务指针
  server(nullptr),
  connectionStatus(TCP_DISCONNECTED),
  lastConnectionAttempt(0),
  connectionStartTime(0),
  statusCallback(nullptr),
  receiveBufferIndex(0),
  incompletePacketSize(0) {
}

TCPProtocol::~TCPProtocol() {
  if (server) {
    delete server;
  }
  
  if (client.connected()) {
    client.stop();
  }
}
bool TCPProtocol::begin(Device* device, RS485* rs485, MDNSService* mdnsService) {  // 修改函数签名
  this->device = device;
  this->rs485 = rs485;
  this->mdnsService = mdnsService;  // 保存mDNS服务指针
  if (!this->device || !this->rs485 || !this->mdnsService) {  // 添加对mDNS服务的检查
    REPORT_ERROR(ERROR_INVALID_PARAMETER, "TCPProtocol", "Invalid device, rs485 or mdnsService");
    return false;
  }
  
  // 根据设备角色初始化服务器或客户端
  if (this->device->isMaster()) {
    // 主设备创建服务器，监听DEFAULT_MASTER_TCP_PORT端口
    server = new WiFiServer(DEFAULT_MASTER_TCP_PORT);
    server->begin();
    LOG_I("TCPProtocol", "Server started on port %d", DEFAULT_MASTER_TCP_PORT);
  } else {
    // 从设备不需要创建服务器
    server = nullptr;
    LOG_I("TCPProtocol", "Client mode initialized");
  }
  
  LOG_I("TCPProtocol", "TCPProtocol initialized successfully for %s device",
        this->device->isMaster() ? "master" : "slave");
  return true;
}

void TCPProtocol::setHeartbeat(Heartbeat* heartbeat) {
  this->heartbeat = heartbeat;
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
      LOG_E("TCPProtocol", "Connection timeout, connection start time: %lu, current time: %lu, timeout: %lu",
            connectionStartTime, millis(), CONNECTION_TIMEOUT);
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
        LOG_I("TCPProtocol", "Attempting to reconnect to master, last attempt: %lu, current time: %lu, interval: %lu",
              lastConnectionAttempt, currentTime, RECONNECT_INTERVAL);
        lastConnectionAttempt = currentTime;
        connectToMaster();
      }
    }
  }
  
  // 定期记录连接状态（仅在调试级别）
  static unsigned long lastStatusLogTime = 0;
  unsigned long currentTime = millis();
  if (currentTime - lastStatusLogTime > 30000) { // 每30秒记录一次
    LOG_D("TCPProtocol", "Current connection status: %s", getConnectionStatusString().c_str());
    lastStatusLogTime = currentTime;
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
    LOG_I("TCPProtocol", "Connection status changed to %s", getConnectionStatusString().c_str());
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
  uint16_t packetLength = length;
  uint16_t packetChecksum = calculateChecksum(data, length);
  
  LOG_D("TCPProtocol", "Sending packet with length: %d, checksum: %d", packetLength, packetChecksum);
  
  // 手动构造头部字节数组，使用小端序
  uint8_t headerBytes[PACKET_HEADER_SIZE];
  headerBytes[0] = packetLength & 0xFF;        // 长度低字节
  headerBytes[1] = (packetLength >> 8) & 0xFF; // 长度高字节
  headerBytes[2] = packetChecksum & 0xFF;      // 校验和低字节
  headerBytes[3] = (packetChecksum >> 8) & 0xFF; // 校验和高字节
  
  // 发送头部
  size_t headerSent = client.write(headerBytes, PACKET_HEADER_SIZE);
  if (headerSent != PACKET_HEADER_SIZE) {
    REPORT_ERROR(ERROR_TCP_SEND_FAILED, "TCPProtocol", "Failed to send packet header");
    return false;
  }
  
  // 确保头部发送完成
  client.flush();
  LOG_D("TCPProtocol", "Packet header sent and flushed");
  
  // 发送数据
  size_t dataSent = client.write(data, length);
  if (dataSent != length) {
    REPORT_ERROR(ERROR_TCP_SEND_FAILED, "TCPProtocol", "Failed to send packet data");
    return false;
  }
  
  // 确保数据发送完成
  client.flush();
  LOG_D("TCPProtocol", "Packet data sent and flushed");
  
  LOG_D("TCPProtocol", "Packet sent successfully");
  return true;
}
int TCPProtocol::receivePacket(uint8_t* buffer, size_t bufferSize) {
  if (!buffer || bufferSize == 0) {
    REPORT_ERROR(ERROR_INVALID_PARAMETER, "TCPProtocol", "Invalid buffer or size for packet reception");
    return -1;
  }
  
  // 检查连接状态
  LOG_D("TCPProtocol", "Checking connection status for packet reception");
  if (!client.connected()) {
    REPORT_ERROR(ERROR_TCP_CONNECTION_FAILED, "TCPProtocol", "Client not connected for packet reception");
    return -1;
  }
  
  // 检查是否有之前保存的不完整数据包
  if (incompletePacketSize > 0) {
    // 检查是否有足够的数据来完成之前不完整的数据包
    if (client.available() >= (int)(incompletePacketHeader.length - incompletePacketSize)) {
      // 读取剩余的数据
      size_t remainingBytes = incompletePacketHeader.length - incompletePacketSize;
      size_t dataRead = client.readBytes(incompletePacketBuffer + incompletePacketSize, remainingBytes);
      if (dataRead != remainingBytes) {
        REPORT_ERROR(ERROR_TCP_RECEIVE_FAILED, "TCPProtocol", "Failed to read remaining packet data");
        incompletePacketSize = 0; // 重置不完整数据包状态
        return -1;
      }
      
      // 更新不完整数据包大小
      incompletePacketSize += dataRead;
      
      // 验证校验和
      uint16_t calculatedChecksum = calculateChecksum(incompletePacketBuffer, incompletePacketHeader.length);
      if (calculatedChecksum != incompletePacketHeader.checksum) {
        REPORT_ERROR(ERROR_TCP_RECEIVE_FAILED, "TCPProtocol", "Checksum mismatch for incomplete packet");
        incompletePacketSize = 0; // 重置不完整数据包状态
        return -1;
      }
      
      // 检查是否为心跳包
      if (isHeartbeatPacket(incompletePacketBuffer, incompletePacketSize)) {
        handleHeartbeatPacket(incompletePacketBuffer, incompletePacketSize);
        // 重置不完整数据包状态
        incompletePacketSize = 0;
        // 心跳包不需要返回给应用层
        return -2; // 特殊返回值表示处理了心跳包但没有应用数据
      }
      
      // 将数据复制到输出缓冲区
      if (incompletePacketHeader.length > bufferSize) {
        REPORT_ERROR(ERROR_TCP_RECEIVE_FAILED, "TCPProtocol", "Buffer too small for packet data");
        incompletePacketSize = 0; // 重置不完整数据包状态
        return -1;
      }
      
      memcpy(buffer, incompletePacketBuffer, incompletePacketHeader.length);
      size_t packetSize = incompletePacketHeader.length;
      
      // 重置不完整数据包状态
      incompletePacketSize = 0;
      
      return packetSize;
    } else {
      // 仍然没有足够的数据，继续读取
      size_t availableBytes = client.available();
      if (availableBytes > 0) {
        size_t dataRead = client.readBytes(incompletePacketBuffer + incompletePacketSize, availableBytes);
        incompletePacketSize += dataRead;
        
        // 检查新读取的数据中是否包含心跳包
        // 我们需要检查从incompletePacketSize - dataRead到incompletePacketSize的范围
        // 但由于心跳包可能跨越之前的不完整数据和新读取的数据，我们需要检查整个缓冲区
        bool heartbeatProcessed = false;
        if (incompletePacketSize >= 4) {
          // 检查缓冲区中是否包含心跳包
          for (size_t i = 0; i <= incompletePacketSize - 4; i++) {
            if (isHeartbeatPacket(incompletePacketBuffer + i, incompletePacketSize - i)) {
              // 找到心跳包，处理它
              handleHeartbeatPacket(incompletePacketBuffer + i, 4);
              
              // 移除已处理的心跳包数据
              if (i + 4 < incompletePacketSize) {
                // 还有剩余数据，移动到缓冲区开始
                memmove(incompletePacketBuffer, incompletePacketBuffer + i + 4, incompletePacketSize - i - 4);
                incompletePacketSize -= (i + 4);
              } else {
                // 没有剩余数据
                incompletePacketSize = 0;
              }
              
              // 标记已处理心跳包
              heartbeatProcessed = true;
              
              // 心跳包不需要返回给应用层
              // 如果缓冲区中没有其他数据，返回特殊值
              // 如果还有其他数据，可能需要进一步处理
              break;
            }
          }
        }
        
        // 如果处理了心跳包但没有其他数据，返回特殊值
        if (heartbeatProcessed && incompletePacketSize == 0) {
          // 更新最后接收时间
          if (heartbeat) {
            heartbeat->updateLastReceivedTime();
          }
          return -2; // 特殊返回值表示处理了心跳包但没有应用数据
        }
      }
      
      // 仍然没有完整的数据包
      return 0;
    }
  }
  
  // 检查是否有足够的数据可读（至少包含头部）
  if (client.available() < PACKET_HEADER_SIZE) {
    return 0; // 没有完整的数据包
  }
  
  // 读取数据包头部 - 使用字节数组避免结构体对齐问题
  uint8_t headerBytes[PACKET_HEADER_SIZE];
  size_t headerRead = client.readBytes(headerBytes, PACKET_HEADER_SIZE);
  if (headerRead != PACKET_HEADER_SIZE) {
    REPORT_ERROR(ERROR_TCP_RECEIVE_FAILED, "TCPProtocol", "Failed to read packet header");
    return -1;
  }
  
  // 手动解析头部数据，避免字节序和对齐问题
  PacketHeader header;
  header.length = (headerBytes[1] << 8) | headerBytes[0];    // 小端序
  header.checksum = (headerBytes[3] << 8) | headerBytes[2];  // 小端序
  
  LOG_D("TCPProtocol", "Received packet header - length: %d, checksum: %d", header.length, header.checksum);
  
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
    // 数据不完整，保存头部信息
    incompletePacketHeader = header;
    size_t availableBytes = client.available();
    
    LOG_W("TCPProtocol", "Incomplete packet data, header length: %d, available data: %d", header.length, availableBytes);
    
    // 对于小数据包（如心跳包），尝试多次等待数据到达
    if (header.length <= 16) {
      int retryCount = 0;
      const int maxRetries = 10; // 重试次数
      const int retryDelay = 20; // 延迟时间调整为20ms
      
      LOG_D("TCPProtocol", "Small packet detected (length: %d), starting retry mechanism", header.length);
      
      while (retryCount < maxRetries && client.available() < header.length) {
        delay(retryDelay);
        retryCount++;
        availableBytes = client.available();
        LOG_D("TCPProtocol", "Retry %d/%d: available data: %d (need: %d)", retryCount, maxRetries, availableBytes, header.length);
        
        // 如果数据已经足够，跳出循环
        if (availableBytes >= header.length) {
          LOG_I("TCPProtocol", "Complete data arrived after %d retries", retryCount);
          break;
        }
      }
      
      // 重新检查数据可用性
      if (client.available() >= header.length) {
        // 数据已经到达，继续正常处理
        LOG_D("TCPProtocol", "Data arrived after %d retries, proceeding with normal processing", retryCount);
      } else {
        // 仍然没有足够数据，但如果是心跳包长度，特殊处理
        if (header.length == 4 && availableBytes == 0) {
          LOG_W("TCPProtocol", "Heartbeat packet data still missing after %d retries, this may indicate a network issue", retryCount);
        }
        
        // 保存到不完整缓冲区
        if (availableBytes > 0) {
          size_t dataRead = client.readBytes(incompletePacketBuffer, availableBytes);
          incompletePacketSize = dataRead;
          LOG_D("TCPProtocol", "Saved %d bytes to incomplete buffer (expected: %d)", dataRead, header.length);
        } else {
          incompletePacketSize = 0;
        }
        return 0;
      }
    } else {
      // 对于大数据包，直接保存到不完整缓冲区
      if (availableBytes > 0) {
        size_t dataRead = client.readBytes(incompletePacketBuffer, availableBytes);
        incompletePacketSize = dataRead;
        LOG_D("TCPProtocol", "Large packet: saved %d bytes to incomplete buffer", dataRead);
      } else {
        incompletePacketSize = 0;
      }
      return 0;
    }
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
  
  // 检查是否为心跳包
  if (isHeartbeatPacket(buffer, dataRead)) {
    handleHeartbeatPacket(buffer, dataRead);
    // 心跳包不需要返回给应用层
    return -2; // 特殊返回值表示处理了心跳包但没有应用数据
  }
  
  return dataRead;
}

bool TCPProtocol::isHeartbeatPacket(const uint8_t* data, size_t length) {
  // 心跳包格式：前两个字节为心跳包类型标识(0xFF01)
  if (length >= 4 && data[0] == 0xFF && data[1] == 0x01) {
    // 检查是否为"PI"字符串
    if (data[2] == 'P' && data[3] == 'I') {
      return true;
    }
  }
  return false;
}

void TCPProtocol::handleHeartbeatPacket(const uint8_t* data, size_t length) {
  LOG_D("TCPProtocol", "Handling heartbeat packet with length: %d", length);
  // 如果有心跳模块，调用心跳模块的处理函数
  if (heartbeat) {
    heartbeat->handleHeartbeat();
    // 确保更新最后接收时间
    heartbeat->updateLastReceivedTime();
    LOG_D("TCPProtocol", "Heartbeat packet handled successfully");
  } else {
    LOG_W("TCPProtocol", "Heartbeat module not available");
  }
}

void TCPProtocol::handleServer() {
  if (!server) {
    return;
  }
  
  // 检查是否有新的客户端连接
  WiFiClient newClient = server->accept();
  if (newClient) {
    if (!client.connected()) {
      // 接受新连接
      client = newClient;
      updateConnectionStatus(TCP_CONNECTED);
      LOG_I("TCPProtocol", "New client connected from %s", client.remoteIP().toString().c_str());
    } else {
      // 已经有连接，拒绝新连接
      newClient.stop();
      LOG_W("TCPProtocol", "New client rejected, already connected");
    }
  }
  
  // 处理现有连接的数据传输
  if (client.connected()) {
    // 检查是否有来自RS485的数据需要转发到TCP客户端
    if (rs485->available()) {
      uint8_t buffer[1024];
      int bytesRead = rs485->receive(buffer, sizeof(buffer));
      if (bytesRead > 0) {
        // 发送到TCP客户端
        sendData(buffer, bytesRead);
      }
    }
    
    // 检查是否有来自TCP客户端的数据需要转发到RS485
    if (client.available()) {
      uint8_t buffer[1024];
      int bytesRead = receiveData(buffer, sizeof(buffer));
      if (bytesRead > 0) {
        // 发送到RS485
        rs485->send(buffer, bytesRead);
      } else if (bytesRead == 0) {
        // 没有完整数据包，但连接是活跃的
        LOG_D("TCPProtocol", "No complete packet available, but connection is active");
      } else if (bytesRead == -2) {
        // 处理了心跳包但没有应用数据
        LOG_D("TCPProtocol", "Heartbeat packet processed successfully");
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
      uint8_t buffer[1024];
      int bytesRead = rs485->receive(buffer, sizeof(buffer));
      if (bytesRead > 0) {
        // 发送到主设备
        sendData(buffer, bytesRead);
      }
    }
    
    // 检查是否有来自主设备的数据需要转发到RS485
    if (client.available()) {
      uint8_t buffer[1024];
      int bytesRead = receiveData(buffer, sizeof(buffer));
      if (bytesRead > 0) {
        // 发送到RS485
        rs485->send(buffer, bytesRead);
      } else if (bytesRead == 0) {
        // 没有完整数据包，但连接是活跃的
        LOG_D("TCPProtocol", "No complete packet available, but connection is active");
      } else if (bytesRead == -2) {
        // 处理了心跳包但没有应用数据
        LOG_D("TCPProtocol", "Heartbeat packet processed successfully");
      }
    }
  }
}

// 实现通过mDNS发现主设备IP地址的函数
bool TCPProtocol::discoverMasterIP(IPAddress& masterIP, uint16_t& masterPort) {
  if (!mdnsService) {
    REPORT_ERROR(ERROR_INVALID_PARAMETER, "TCPProtocol", "mDNS service not initialized");
    return false;
  }
  
  // 使用mDNS服务查找主设备
  String masterIPStr;
  if (mdnsService->discoverMaster(masterIPStr, masterPort)) {
    // 将String类型的IP地址转换为IPAddress类型
    if (masterIP.fromString(masterIPStr)) {
      LOG_I("TCPProtocol", "Discovered master at %s:%d", masterIPStr.c_str(), masterPort);
      return true;
    } else {
      LOG_E("TCPProtocol", "Failed to parse master IP address");
      return false;
    }
  } else {
    LOG_E("TCPProtocol", "Failed to discover master via mDNS");
    return false;
  }
}

bool TCPProtocol::connectToMaster() {
  // 通过mDNS查找主设备IP地址
  IPAddress masterIP;
  uint16_t masterPort = DEFAULT_MASTER_TCP_PORT; // 默认端口
  
  LOG_I("TCPProtocol", "Discovering master via mDNS...");
  if (!discoverMasterIP(masterIP, masterPort)) {
    LOG_E("TCPProtocol", "Failed to discover master via mDNS");
    // mDNS查找失败，返回false，让上层决定是否重试
    updateConnectionStatus(TCP_CONNECTION_FAILED);
    return false;
  }
  
  LOG_I("TCPProtocol", "Connecting to master at %s:%d", masterIP.toString().c_str(), masterPort);
  
  updateConnectionStatus(TCP_CONNECTING);
  connectionStartTime = millis();
  
  // 尝试连接到主设备
  if (client.connect(masterIP, masterPort)) {
    LOG_I("TCPProtocol", "Connected to master");
    updateConnectionStatus(TCP_CONNECTED);
    return true;
  } else {
    LOG_E("TCPProtocol", "Failed to connect to master");
    updateConnectionStatus(TCP_CONNECTION_FAILED);
    return false;
  }
}
