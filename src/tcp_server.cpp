#include "tcp_server.h"
#include "logger.h"
#include "error_handler.h"
#include "config.h"

// 服务端配置常量
#define CLIENT_TIMEOUT 60000         // 客户端超时时间(毫秒)
#define CLIENT_ACTIVITY_CHECK 5000   // 客户端活动检查间隔(毫秒)

TCPServer::TCPServer() : device(nullptr),
                         wifiServer(nullptr),
                         serverStatus(TCP_SERVER_STOPPED),
                         serverPort(DEFAULT_MASTER_TCP_PORT),
                         connectedClientCount(0),
                         statusCallback(nullptr),
                         connectionCallback(nullptr),
                         dataCallback(nullptr)
{
  // 构造函数
}

TCPServer::~TCPServer()
{
  // 析构函数
  stop();
  if (wifiServer)
  {
    delete wifiServer;
    wifiServer = nullptr;
  }
}

bool TCPServer::begin(Device* device, uint16_t port)
{
  this->device = device;
  this->serverPort = port;
  
  if (!this->device)
  {
    REPORT_ERROR(ERROR_INVALID_PARAMETER, "TCPServer", "Invalid device");
    return false;
  }
  
  // 只有主设备才需要TCP服务端功能
  if (!device->isMaster())
  {
    LOG_W("TCPServer", "TCP server is only for master devices");
    return false;
  }
  
  // 创建WiFi服务端实例
  if (wifiServer)
  {
    delete wifiServer;
  }
  wifiServer = new WiFiServer(serverPort);
  
  if (!wifiServer)
  {
    REPORT_ERROR(ERROR_OUT_OF_MEMORY, "TCPServer", "Failed to create WiFi server");
    return false;
  }
  
  LOG_I("TCPServer", "TCP server initialized for master device on port %d", serverPort);
  return true;
}

bool TCPServer::start()
{
  if (!device || !wifiServer)
  {
    REPORT_ERROR(ERROR_INVALID_PARAMETER, "TCPServer", "Not initialized");
    return false;
  }
  
  if (!device->isMaster())
  {
    LOG_W("TCPServer", "TCP server is only for master devices");
    return false;
  }
  
  LOG_I("TCPServer", "Starting TCP server on port %d", serverPort);
  updateServerStatus(TCP_SERVER_STARTING);
  
  // 启动WiFi服务端
  wifiServer->begin();
  
  // 初始化客户端连接数组
  for (int i = 0; i < MAX_CLIENTS; i++)
  {
    clients[i].isActive = false;
    clients[i].client.stop();
  }
  connectedClientCount = 0;
  
  updateServerStatus(TCP_SERVER_RUNNING);
  LOG_I("TCPServer", "TCP server started successfully");
  
  return true;
}

void TCPServer::stop()
{
  if (wifiServer)
  {
    // 断开所有客户端连接
    for (int i = 0; i < MAX_CLIENTS; i++)
    {
      if (clients[i].isActive)
      {
        clients[i].client.stop();
        clients[i].isActive = false;
      }
    }
    
    wifiServer->stop();
    connectedClientCount = 0;
  }
  
  updateServerStatus(TCP_SERVER_STOPPED);
  LOG_I("TCPServer", "TCP server stopped");
}

void TCPServer::handle()
{
  if (!device || !device->isMaster() || serverStatus != TCP_SERVER_RUNNING)
  {
    return;
  }
  
  // 处理新的客户端连接
  handleNewConnections();
  
  // 处理客户端数据
  handleClientData();
  
  // 处理客户端断开连接
  handleClientDisconnections();
}

TCPServerStatus TCPServer::getServerStatus()
{
  return serverStatus;
}

String TCPServer::getServerStatusString()
{
  switch (serverStatus)
  {
    case TCP_SERVER_STOPPED:
      return "Stopped";
    case TCP_SERVER_STARTING:
      return "Starting";
    case TCP_SERVER_RUNNING:
      return "Running";
    case TCP_SERVER_ERROR:
      return "Error";
    default:
      return "Unknown";
  }
}

bool TCPServer::broadcastData(const uint8_t* data, size_t length)
{
  if (!data || length == 0)
  {
    LOG_W("TCPServer", "Invalid data parameters");
    return false;
  }
  
  bool success = true;
  int sentCount = 0;
  
  for (int i = 0; i < MAX_CLIENTS; i++)
  {
    if (clients[i].isActive && clients[i].client.connected())
    {
      if (sendDataToClient(i, data, length))
      {
        sentCount++;
      }
      else
      {
        success = false;
      }
    }
  }
  
  LOG_D("TCPServer", "Broadcast data to %d clients (%d bytes)", sentCount, length);
  return success;
}

bool TCPServer::sendDataToClient(int clientIndex, const uint8_t* data, size_t length)
{
  if (clientIndex < 0 || clientIndex >= MAX_CLIENTS)
  {
    LOG_W("TCPServer", "Invalid client index: %d", clientIndex);
    return false;
  }
  
  if (!clients[clientIndex].isActive || !clients[clientIndex].client.connected())
  {
    LOG_W("TCPServer", "Client %d is not connected", clientIndex);
    return false;
  }
  
  if (!data || length == 0)
  {
    LOG_W("TCPServer", "Invalid data parameters");
    return false;
  }
  
  size_t bytesWritten = clients[clientIndex].client.write(data, length);
  if (bytesWritten != length)
  {
    LOG_E("TCPServer", "Failed to send all data to client %d: %d/%d bytes sent", 
          clientIndex, bytesWritten, length);
    return false;
  }
  
  updateClientActivity(clientIndex);
  LOG_D("TCPServer", "Sent %d bytes to client %d", length, clientIndex);
  return true;
}

int TCPServer::getConnectedClientCount()
{
  return connectedClientCount;
}

ClientConnection* TCPServer::getClientConnection(int index)
{
  if (index < 0 || index >= MAX_CLIENTS)
  {
    return nullptr;
  }
  
  return &clients[index];
}

uint16_t TCPServer::getServerPort()
{
  return serverPort;
}

void TCPServer::setServerStatusCallback(ServerStatusCallback callback)
{
  statusCallback = callback;
}

void TCPServer::setClientConnectionCallback(ClientConnectionCallback callback)
{
  connectionCallback = callback;
}

void TCPServer::setDataReceivedCallback(DataReceivedCallback callback)
{
  dataCallback = callback;
}

void TCPServer::updateServerStatus(TCPServerStatus status)
{
  if (serverStatus != status)
  {
    serverStatus = status;
    
    // 调用回调函数
    if (statusCallback)
    {
      statusCallback(status);
    }
    
    LOG_I("TCPServer", "Server status changed to %s", getServerStatusString().c_str());
  }
}

void TCPServer::handleNewConnections()
{
  if (!wifiServer->hasClient())
  {
    return;
  }
  
  WiFiClient newClient = wifiServer->available();
  if (!newClient)
  {
    return;
  }
  
  // 查找可用的客户端槽位
  int clientIndex = findAvailableClientSlot();
  if (clientIndex == -1)
  {
    // 没有可用槽位，拒绝连接
    LOG_W("TCPServer", "Maximum clients reached, rejecting new connection");
    newClient.stop();
    return;
  }
  
  // 接受新连接
  clients[clientIndex].client = newClient;
  clients[clientIndex].clientIP = newClient.remoteIP().toString();
  clients[clientIndex].clientPort = newClient.remotePort();
  clients[clientIndex].connectTime = millis();
  clients[clientIndex].lastActivity = millis();
  clients[clientIndex].isActive = true;
  
  connectedClientCount++;
  
  LOG_I("TCPServer", "New client connected: %s:%d (slot %d, total: %d)", 
        clients[clientIndex].clientIP.c_str(), 
        clients[clientIndex].clientPort, 
        clientIndex, 
        connectedClientCount);
  
  // 调用连接回调函数
  if (connectionCallback)
  {
    connectionCallback(clientIndex, true);
  }
}

void TCPServer::handleClientData()
{
  for (int i = 0; i < MAX_CLIENTS; i++)
  {
    if (!clients[i].isActive || !clients[i].client.connected())
    {
      continue;
    }
    
    int availableBytes = clients[i].client.available();
    if (availableBytes > 0)
    {
      // 限制读取的数据量
      int bytesToRead = min(availableBytes, (int)sizeof(receiveBuffer));
      int bytesRead = clients[i].client.read(receiveBuffer, bytesToRead);
      
      if (bytesRead > 0)
      {
        updateClientActivity(i);
        LOG_D("TCPServer", "Received %d bytes from client %d", bytesRead, i);
        
        // 调用数据接收回调函数
        if (dataCallback)
        {
          dataCallback(i, receiveBuffer, bytesRead);
        }
      }
    }
  }
}

void TCPServer::handleClientDisconnections()
{
  unsigned long currentTime = millis();
  
  for (int i = 0; i < MAX_CLIENTS; i++)
  {
    if (!clients[i].isActive)
    {
      continue;
    }
    
    // 检查客户端连接状态
    if (!isClientConnected(i))
    {
      LOG_I("TCPServer", "Client %d disconnected: %s:%d", 
            i, clients[i].clientIP.c_str(), clients[i].clientPort);
      removeClient(i);
    }
    // 检查客户端超时
    else if (currentTime - clients[i].lastActivity > CLIENT_TIMEOUT)
    {
      LOG_W("TCPServer", "Client %d timeout: %s:%d", 
            i, clients[i].clientIP.c_str(), clients[i].clientPort);
      removeClient(i);
    }
  }
}

int TCPServer::findAvailableClientSlot()
{
  for (int i = 0; i < MAX_CLIENTS; i++)
  {
    if (!clients[i].isActive)
    {
      return i;
    }
  }
  return -1;
}

void TCPServer::removeClient(int index)
{
  if (index < 0 || index >= MAX_CLIENTS || !clients[index].isActive)
  {
    return;
  }
  
  clients[index].client.stop();
  clients[index].isActive = false;
  clients[index].clientIP = "";
  clients[index].clientPort = 0;
  clients[index].connectTime = 0;
  clients[index].lastActivity = 0;
  
  connectedClientCount--;
  
  // 调用连接回调函数
  if (connectionCallback)
  {
    connectionCallback(index, false);
  }
}

bool TCPServer::isClientConnected(int index)
{
  if (index < 0 || index >= MAX_CLIENTS || !clients[index].isActive)
  {
    return false;
  }
  
  return clients[index].client.connected();
}

void TCPServer::updateClientActivity(int index)
{
  if (index >= 0 && index < MAX_CLIENTS && clients[index].isActive)
  {
    clients[index].lastActivity = millis();
  }
}