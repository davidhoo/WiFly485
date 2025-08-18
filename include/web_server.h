#ifndef WEB_SERVER_H
#define WEB_SERVER_H

#include <ESP8266WebServer.h>
#include <ESP8266HTTPUpdateServer.h>
#include "config_manager.h"
#include "device.h"

class WebServer {
private:
    ESP8266WebServer server;
    ESP8266HTTPUpdateServer httpUpdater;
    ConfigManager& configManager;
    Device& device;
    
    // 页面处理函数
    void handleRoot();
    void handleConfig();
    void handleConfigSave();
    void handleReboot();
    void handleNotFound();
    
    // API处理函数
    void handleGetConfig();
    void handleSetConfig();
    void handleGetStatus();
    void handleFirmwareUpdate();
    
    // 工具函数
    String getContentType(const String& filename);
    bool handleFileRead(const String& path);
    void sendJsonResponse(int code, const String& message, const JsonObject& data = JsonObject());

public:
    WebServer(ConfigManager& configManager, Device& device);
    void begin();
    void handleClient();
    
    // OTA更新相关
    void setupOTA();
};

#endif // WEB_SERVER_H