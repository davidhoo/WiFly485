#ifndef TCP_SERVER_H
#define TCP_SERVER_H

#include <Arduino.h>
#include <WiFiServer.h>
#include <WiFiClient.h>
#include "device.h"
#include "config.h"

// TCP服务端状态枚举
enum TCPServerStatus {
  TCP_SERVER_STOPPED = 0,
  TCP_SERVER_STARTING = 1,
  TCP_SERVER_RUNNING = 2,
  TCP_SERVER_ERROR = 3
};

// 客户端连接信息结构
struct ClientConnection {
  WiFiClient client;
  String clientIP;
  uint16_t clientPort;
  unsigned long connectTime;
  unsigned long lastActivity;
  bool isActive;
  
  ClientConnection() : clientPort(0), connectTime(0), lastActivity(0), isActive(false) {}
};

class TCPServer {
public:
  TCPServer();
  ~TCPServer();

  // 初始化TCP服务端
  bool begin(Device* device, uint16_t port = DEFAULT_MASTER_TCP_PORT);

  // 启动TCP服务端（仅主设备使用）
  bool start();

  // 停止TCP服务端
  void stop();

  // 处理TCP服务端和客户端连接
  void handle();

  // 获取服务端状态
  TCPServerStatus getServerStatus();

  // 获取服务端状态字符串
  String getServerStatusString();

  // 发送数据到所有连接的客户端
  bool broadcastData(const uint8_t* data, size_t length);

  // 发送数据到指定客户端
  bool sendDataToClient(int clientIndex, const uint8_t* data, size_t length);

  // 获取连接的客户端数量
  int getConnectedClientCount();

  // 获取客户端信息
  ClientConnection* getClientConnection(int index);

  // 获取服务端端口
  uint16_t getServerPort();

  // 设置服务端状态回调函数
  typedef void (*ServerStatusCallback)(TCPServerStatus status);
  void setServerStatusCallback(ServerStatusCallback callback);

  // 设置客户端连接回调函数
  typedef void (*ClientConnectionCallback)(int clientIndex, bool connected);
  void setClientConnectionCallback(ClientConnectionCallback callback);

  // 设置数据接收回调函数
  typedef void (*DataReceivedCallback)(int clientIndex, const uint8_t* data, size_t length);
  void setDataReceivedCallback(DataReceivedCallback callback);

private:
  Device* device;
  WiFiServer* wifiServer;
  
  TCPServerStatus serverStatus;
  uint16_t serverPort;
  
  // 客户端连接管理
  static const int MAX_CLIENTS = 4;  // 最大客户端连接数
  ClientConnection clients[MAX_CLIENTS];
  int connectedClientCount;
  
  // 回调函数
  ServerStatusCallback statusCallback;
  ClientConnectionCallback connectionCallback;
  DataReceivedCallback dataCallback;
  
  // 数据缓冲区
  uint8_t receiveBuffer[TCP_RECEIVE_BUFFER_SIZE];
  
  // 内部辅助函数
  void updateServerStatus(TCPServerStatus status);
  void handleNewConnections();
  void handleClientData();
  void handleClientDisconnections();
  int findAvailableClientSlot();
  void removeClient(int index);
  bool isClientConnected(int index);
  void updateClientActivity(int index);
};

#endif // TCP_SERVER_H