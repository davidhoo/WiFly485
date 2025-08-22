#ifndef TCP_CLIENT_H
#define TCP_CLIENT_H

#include <Arduino.h>
#include <WiFiClient.h>
#include "config.h"
#include "device.h"
#include "rs485.h"

// TCP客户端连接状态枚举
enum TCPClientStatus {
  TCP_CLIENT_DISCONNECTED = 0,
  TCP_CLIENT_CONNECTING = 1,
  TCP_CLIENT_CONNECTED = 2,
  TCP_CLIENT_ERROR = 3
};

class TCPClient {
public:
  TCPClient();
  ~TCPClient();

  // 初始化TCP客户端
  bool begin(Device* device, RS485* rs485);

  // 连接到服务器
  bool connect(const String& serverIP, uint16_t serverPort);

  // 断开连接
  void disconnect();

  // 处理TCP连接和数据传输
  void handle();

  // 获取连接状态
  TCPClientStatus getStatus();

  // 获取状态字符串
  String getStatusString();

  // 检查是否已连接
  bool isConnected();

  // 发送数据到服务器
  bool send(const uint8_t* data, size_t length);

  // 设置连接状态回调函数
  typedef void (*ClientStatusCallback)(TCPClientStatus status, const String& serverIP, uint16_t serverPort);
  void setClientStatusCallback(ClientStatusCallback callback);

  // 设置重连参数
  void setReconnectParams(unsigned long interval = 5000, uint8_t maxRetries = 10);

  // 获取服务器信息
  String getServerIP();
  uint16_t getServerPort();

private:
  Device* device;
  RS485* rs485;
  WiFiClient client;
  
  TCPClientStatus status;
  String serverIP;
  uint16_t serverPort;
  
  // 重连参数
  unsigned long reconnectInterval;
  uint8_t maxReconnectRetries;
  uint8_t currentRetryCount;
  unsigned long lastReconnectAttempt;
  
  // 数据缓冲区
  uint8_t receiveBuffer[TCP_RECEIVE_BUFFER_SIZE];
  
  ClientStatusCallback statusCallback;
  
  // 内部辅助函数
  void updateClientStatus(TCPClientStatus newStatus);
  void handleServerData();
  void forwardRS485DataToServer();
  void processReceivedData(const uint8_t* data, size_t length);
  bool attemptReconnect();
};

#endif // TCP_CLIENT_H