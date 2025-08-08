#include "wifi_controller.h"
#include "debug_utils.h"

// 静态成员初始化
WiFiController* WiFiController::instance = nullptr;

WiFiController::WiFiController() 
    : initialized(false), currentMode(WIFI_OFF), lastConnectionCheck(0) {
    instance = this;
}

bool WiFiController::initialize() {
    DEBUG_INFO_PRINT("Initializing WiFi Controller...");
    
    // 设置WiFi事件回调
    WiFi.onEvent(onWiFiEvent);
    
    // 设置WiFi模式为关闭，准备重新配置
    WiFi.mode(WIFI_OFF);
    delay(100);
    
    initialized = true;
    DEBUG_INFO_PRINT("WiFi Controller initialized successfully");
    return true;
}

bool WiFiController::startAP(const String& ssid, const String& password) {
    if (!initialized) {
        DEBUG_ERROR_PRINT("WiFi Controller not initialized");
        return false;
    }
    
    DEBUG_INFO_PRINT("Starting WiFi AP mode...");
    DEBUG_INFO_PRINT("SSID: %s", ssid.c_str());
    
    // 断开现有连接
    WiFi.disconnect();
    delay(100);
    
    // 设置AP模式
    WiFi.mode(WIFI_AP);
    delay(100);
    
    // 启动AP
    bool success = WiFi.softAP(ssid.c_str(), password.c_str());
    
    if (success) {
        currentMode = WIFI_AP;
        currentSSID = ssid;
        currentPassword = password;
        
        DEBUG_INFO_PRINT("WiFi AP started successfully");
        DEBUG_INFO_PRINT("AP IP: %s", WiFi.softAPIP().toString().c_str());
        DEBUG_INFO_PRINT("AP MAC: %s", WiFi.softAPmacAddress().c_str());
        
        if (connectionCallback) {
            connectionCallback(true);
        }
    } else {
        DEBUG_ERROR_PRINT("Failed to start WiFi AP");
    }
    
    return success;
}

bool WiFiController::connectSTA(const String& ssid, const String& password) {
    if (!initialized) {
        DEBUG_ERROR_PRINT("WiFi Controller not initialized");
        return false;
    }
    
    DEBUG_INFO_PRINT("Connecting to WiFi STA mode...");
    DEBUG_INFO_PRINT("SSID: %s", ssid.c_str());
    
    // 断开现有连接
    WiFi.disconnect();
    delay(100);
    
    // 设置STA模式
    WiFi.mode(WIFI_STA);
    delay(100);
    
    // 开始连接
    WiFi.begin(ssid.c_str(), password.c_str());
    
    currentSSID = ssid;
    currentPassword = password;
    currentMode = WIFI_STA;
    
    // 等待连接，最多等待15秒
    int attempts = 0;
    const int maxAttempts = 30; // 30 * 500ms = 15秒
    
    while (WiFi.status() != WL_CONNECTED && attempts < maxAttempts) {
        delay(500);
        attempts++;
        DEBUG_VERBOSE_PRINT("Connecting... attempt %d/%d", attempts, maxAttempts);
    }
    
    bool connected = (WiFi.status() == WL_CONNECTED);
    
    if (connected) {
        DEBUG_INFO_PRINT("WiFi connected successfully");
        DEBUG_INFO_PRINT("IP Address: %s", WiFi.localIP().toString().c_str());
        DEBUG_INFO_PRINT("Gateway: %s", WiFi.gatewayIP().toString().c_str());
        DEBUG_INFO_PRINT("Subnet: %s", WiFi.subnetMask().toString().c_str());
        DEBUG_INFO_PRINT("DNS: %s", WiFi.dnsIP().toString().c_str());
        DEBUG_INFO_PRINT("RSSI: %d dBm", WiFi.RSSI());
        
        if (connectionCallback) {
            connectionCallback(true);
        }
    } else {
        DEBUG_ERROR_PRINT("Failed to connect to WiFi");
        DEBUG_ERROR_PRINT("WiFi status: %d", WiFi.status());
        
        if (connectionCallback) {
            connectionCallback(false);
        }
    }
    
    return connected;
}

bool WiFiController::isConnected() {
    if (!initialized) {
        return false;
    }
    
    if (currentMode == WIFI_STA) {
        return WiFi.status() == WL_CONNECTED;
    } else if (currentMode == WIFI_AP) {
        return WiFi.softAPgetStationNum() > 0; // AP模式下有客户端连接
    }
    
    return false;
}

String WiFiController::getLocalIP() {
    if (!initialized) {
        return "0.0.0.0";
    }
    
    if (currentMode == WIFI_STA && WiFi.status() == WL_CONNECTED) {
        return WiFi.localIP().toString();
    } else if (currentMode == WIFI_AP) {
        return WiFi.softAPIP().toString();
    }
    
    return "0.0.0.0";
}

String WiFiController::getMacAddress() {
    if (!initialized) {
        return "00:00:00:00:00:00";
    }
    
    if (currentMode == WIFI_STA) {
        return WiFi.macAddress();
    } else if (currentMode == WIFI_AP) {
        return WiFi.softAPmacAddress();
    }
    
    return WiFi.macAddress();
}

int WiFiController::getRSSI() {
    if (!initialized || currentMode != WIFI_STA || WiFi.status() != WL_CONNECTED) {
        return -100; // 表示无信号
    }
    
    return WiFi.RSSI();
}

void WiFiController::disconnect() {
    if (!initialized) {
        DEBUG_WARNING_PRINT("WiFi Controller not initialized");
        return;
    }
    
    DEBUG_INFO_PRINT("Disconnecting WiFi...");
    
    if (currentMode == WIFI_STA) {
        WiFi.disconnect();
    } else if (currentMode == WIFI_AP) {
        WiFi.softAPdisconnect();
    }
    
    WiFi.mode(WIFI_OFF);
    currentMode = WIFI_OFF;
    currentSSID = "";
    currentPassword = "";
    
    if (connectionCallback) {
        connectionCallback(false);
    }
    
    DEBUG_INFO_PRINT("WiFi disconnected");
}

void WiFiController::onConnectionChanged(ConnectionCallback callback) {
    connectionCallback = callback;
}

void WiFiController::update() {
    if (!initialized) {
        return;
    }
    
    unsigned long now = millis();
    if (now - lastConnectionCheck >= CONNECTION_CHECK_INTERVAL) {
        checkConnectionStatus();
        lastConnectionCheck = now;
    }
}

void WiFiController::checkConnectionStatus() {
    static bool lastConnectionState = false;
    bool currentConnectionState = isConnected();
    
    if (currentConnectionState != lastConnectionState) {
        DEBUG_INFO_PRINT("WiFi connection state changed: %s", 
                         currentConnectionState ? "Connected" : "Disconnected");
        
        if (connectionCallback) {
            connectionCallback(currentConnectionState);
        }
        
        lastConnectionState = currentConnectionState;
    }
}

void WiFiController::onWiFiEvent(WiFiEvent_t event) {
    if (!instance) return;
    
    switch (event) {
        case WIFI_EVENT_STAMODE_CONNECTED:
            DEBUG_INFO_PRINT("WiFi Event: STA Connected");
            break;
        case WIFI_EVENT_STAMODE_DISCONNECTED:
            DEBUG_WARNING_PRINT("WiFi Event: STA Disconnected");
            break;
        case WIFI_EVENT_STAMODE_GOT_IP:
            DEBUG_INFO_PRINT("WiFi Event: STA Got IP");
            break;
        case WIFI_EVENT_SOFTAPMODE_STACONNECTED:
            DEBUG_INFO_PRINT("WiFi Event: AP Station Connected");
            break;
        case WIFI_EVENT_SOFTAPMODE_STADISCONNECTED:
            DEBUG_INFO_PRINT("WiFi Event: AP Station Disconnected");
            break;
        default:
            DEBUG_VERBOSE_PRINT("WiFi Event: %d", event);
            break;
    }
}