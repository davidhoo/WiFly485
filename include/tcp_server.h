#ifndef TCP_SERVER_H
#define TCP_SERVER_H

#include <Arduino.h>
#include <WiFiServer.h>
#include <WiFiClient.h>
#include "config.h"
#include "device.h"
#include "rs485.h"

// TCP连接状态枚举
enum TCPConnectionStatus {
  TCP_DISCONNECTED = 0,
  TCP_LISTENING = 1,
  TCP_CONNECTED = 2,
  TCP_ERROR = 3
};

// TCP客户端信息结构
struct TCPClientInfo {
  WiFiClient client;
  IPAddress remoteIP;
  uint16_t remotePort;
  unsigned long connectTime;
  bool isConnected;
};

class TCPServer {
public:
  TCPServer();
  ~TCPServer();

  // 初始化TCP服务器
  bool begin(Device* device, RS485* rs485);

  // 启动TCP服务器
  bool start();

  // 停止TCP服务器
  void stop();

  // 处理TCP连接和数据传输
  void handle();

  // 获取TCP服务器状态
  TCPConnectionStatus getStatus();

  // 获取状态字符串
  String getStatusString();

  // 获取连接的客户端数量
  uint8_t getConnectedClientCount();

  // 获取服务器端口
  uint16_t getPort();

  // 发送数据到所有连接的客户端
  bool sendToAllClients(const uint8_t* data, size_t length);

  // 设置连接状态回调函数
  typedef void (*ConnectionStatusCallback)(TCPConnectionStatus status, IPAddress clientIP);
  void setConnectionStatusCallback(ConnectionStatusCallback callback);

private:
  Device* device;
  RS485* rs485;
  WiFiServer* server;
  
  TCPConnectionStatus status;
  uint16_t port;
  
  // 客户端管理
  static const uint8_t MAX_CLIENTS = 4;
  TCPClientInfo clients[MAX_CLIENTS];
  uint8_t connectedClientCount;
  
  // 数据缓冲区
  uint8_t receiveBuffer[TCP_RECEIVE_BUFFER_SIZE];
  
  ConnectionStatusCallback statusCallback;
  
  // 内部辅助函数
  void handleNewConnections();
  void handleClientData();
  void handleClientDisconnections();
  void addClient(WiFiClient& client);
  void removeClient(uint8_t index);
  int8_t findClientIndex(WiFiClient& client);
  void updateConnectionStatus(TCPConnectionStatus newStatus, IPAddress clientIP = IPAddress(0,0,0,0));
  
  // 数据处理
  void processReceivedData(const uint8_t* data, size_t length, uint8_t clientIndex);
  void forwardRS485DataToClients();
};

#endif // TCP_SERVER_H