#ifndef TCP_CLIENT_H
#define TCP_CLIENT_H

#include <Arduino.h>
#include <WiFiClient.h>
#include "device.h"
#include "mdns_service.h"
#include "config.h"

// TCP连接状态枚举
enum TCPConnectionStatus {
  TCP_DISCONNECTED = 0,
  TCP_DISCOVERING = 1,
  TCP_CONNECTING = 2,
  TCP_CONNECTED = 3,
  TCP_ERROR = 4
};

class TCPClient {
public:
  TCPClient();
  ~TCPClient();

  // 初始化TCP客户端
  bool begin(Device* device, MDNSService* mdnsService);

  // 启动TCP连接（仅从设备使用）
  bool startConnection();

  // 停止TCP连接
  void stopConnection();

  // 处理TCP连接和数据传输
  void handle();

  // 获取连接状态
  TCPConnectionStatus getConnectionStatus();

  // 获取连接状态字符串
  String getConnectionStatusString();

  // 发送数据到主设备
  bool sendData(const uint8_t* data, size_t length);

  // 检查是否有可用数据
  int available();

  // 读取数据
  int read();
  int read(uint8_t* buffer, size_t size);

  // 获取连接的主设备信息
  String getMasterIP();
  uint16_t getMasterPort();

  // 设置连接状态回调函数
  typedef void (*ConnectionStatusCallback)(TCPConnectionStatus status);
  void setConnectionStatusCallback(ConnectionStatusCallback callback);

  // 设置数据接收回调函数
  typedef void (*DataReceivedCallback)(const uint8_t* data, size_t length);
  void setDataReceivedCallback(DataReceivedCallback callback);

private:
  Device* device;
  MDNSService* mdnsService;
  WiFiClient wifiClient;
  
  TCPConnectionStatus connectionStatus;
  String masterIP;
  uint16_t masterPort;
  
  // 回调函数
  ConnectionStatusCallback statusCallback;
  DataReceivedCallback dataCallback;
  
  // 连接管理
  unsigned long lastDiscoveryTime;
  unsigned long lastConnectionAttempt;
  unsigned long lastHeartbeat;
  int connectionRetryCount;
  
  // 数据缓冲区
  uint8_t receiveBuffer[TCP_RECEIVE_BUFFER_SIZE];
  
  // 内部辅助函数
  void updateConnectionStatus(TCPConnectionStatus status);
  bool discoverMaster();
  bool connectToMaster();
  void handleDataReceive();
  void handleHeartbeat();
  bool isConnectionAlive();
  void resetConnection();
};

#endif // TCP_CLIENT_H