#ifndef WIFI_CONTROLLER_H
#define WIFI_CONTROLLER_H

#include "hal_interfaces.h"
#include <ESP8266WiFi.h>

class WiFiController : public IWiFiController {
private:
    bool initialized;
    ConnectionCallback connectionCallback;
    WiFiMode_t currentMode;
    String currentSSID;
    String currentPassword;
    unsigned long lastConnectionCheck;
    static const unsigned long CONNECTION_CHECK_INTERVAL = 5000; // 5秒检查一次连接状态
    
    void checkConnectionStatus();
    static void onWiFiEvent(WiFiEvent_t event);
    static WiFiController* instance; // 用于静态回调

public:
    WiFiController();
    virtual ~WiFiController() = default;
    
    bool initialize() override;
    bool startAP(const String& ssid, const String& password) override;
    bool connectSTA(const String& ssid, const String& password) override;
    bool isConnected() override;
    String getLocalIP() override;
    String getMacAddress() override;
    int getRSSI() override;
    void disconnect() override;
    void onConnectionChanged(ConnectionCallback callback) override;
    
    // 额外的实用方法
    void update(); // 在主循环中调用以检查连接状态
    bool isInitialized() const { return initialized; }
    WiFiMode_t getCurrentMode() const { return currentMode; }
    String getCurrentSSID() const { return currentSSID; }
};

#endif