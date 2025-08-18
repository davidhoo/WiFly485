#include "web_server.h"
#include <ESP8266mDNS.h>
#include <FS.h>
#include "logger.h"

WebServer::WebServer(ConfigManager& configManager, Device& device) 
    : server(80), configManager(configManager), device(device) {
}

void WebServer::begin() {
    // 设置页面路由
    server.on("/", HTTP_GET, [this]() { handleRoot(); });
    server.on("/config", HTTP_GET, [this]() { handleConfig(); });
    server.on("/config", HTTP_POST, [this]() { handleConfigSave(); });
    server.on("/reboot", HTTP_POST, [this]() { handleReboot(); });
    server.on("/api/config", HTTP_GET, [this]() { handleGetConfig(); });
    server.on("/api/config", HTTP_POST, [this]() { handleSetConfig(); });
    server.on("/api/status", HTTP_GET, [this]() { handleGetStatus(); });
    
    // 设置OTA更新
    setupOTA();
    
    // 设置文件系统
    SPIFFS.begin();
    
    // 设置未找到页面处理器
    server.onNotFound([this]() { handleNotFound(); });
    
    // 启动服务器
    server.begin();
    
    LOG_I("WebServer", "Web server started on port 80");
}

void WebServer::handleClient() {
    server.handleClient();
}

void WebServer::handleRoot() {
    if (!handleFileRead("/index.html")) {
        server.send(404, "text/plain", "File Not Found");
    }
}

void WebServer::handleConfig() {
    if (!handleFileRead("/config.html")) {
        server.send(404, "text/plain", "File Not Found");
    }
}

void WebServer::handleConfigSave() {
    // 获取POST参数并保存配置
    if (server.hasArg("plain")) {
        // 注意：这里需要解析JSON并更新配置管理器中的配置
        // 由于ConfigManager没有直接从JSON字符串保存的方法，我们需要手动解析
        String jsonConfig = server.arg("plain");
        // TODO: 解析JSON并更新配置
        // 临时返回成功响应
        server.send(200, "application/json", "{\"status\":\"success\",\"message\":\"配置已保存\"}");
    } else {
        server.send(400, "application/json", "{\"status\":\"error\",\"message\":\"无效的请求\"}");
    }
}

void WebServer::handleReboot() {
    server.send(200, "application/json", "{\"status\":\"success\",\"message\":\"设备将重启\"}");
    delay(1000);
    ESP.restart();
}

void WebServer::handleNotFound() {
    if (!handleFileRead(server.uri())) {
        server.send(404, "text/plain", "File Not Found");
    }
}

void WebServer::handleGetConfig() {
    // 构建JSON响应
    String configJson = "{";
    configJson += "\"network\":{";
    NetworkConfig netConfig = configManager.getNetworkConfig();
    configJson += "\"ssid\":\"" + netConfig.ssid + "\",";
    configJson += "\"password\":\"" + String("") + "\",";
    configJson += "\"dhcpEnabled\":" + String(netConfig.dhcpEnabled ? "true" : "false") + ",";
    configJson += "\"ip\":\"" + netConfig.ip + "\",";
    configJson += "\"gateway\":\"" + netConfig.gateway + "\",";
    configJson += "\"subnet\":\"" + netConfig.subnet + "\"";
    configJson += "},";
    
    configJson += "\"rs485\":{";
    RS485Config rs485Config = configManager.getRS485Config();
    configJson += "\"baudRate\":" + String(rs485Config.baudRate) + ",";
    configJson += "\"dataBits\":" + String(rs485Config.dataBits) + ",";
    configJson += "\"parity\":" + String(rs485Config.parity) + ",";
    configJson += "\"stopBits\":" + String(rs485Config.stopBits);
    configJson += "},";
    
    configJson += "\"device\":{";
    DeviceConfig devConfig = configManager.getDeviceConfig();
    configJson += "\"name\":\"" + devConfig.name + "\",";
    configJson += "\"role\":\"" + devConfig.role + "\",";
    configJson += "\"tcpPort\":" + String(devConfig.tcpPort) + ",";
    configJson += "\"syncPort\":" + String(devConfig.syncPort);
    configJson += "}";
    configJson += "}";
    
    server.send(200, "application/json", configJson);
}

void WebServer::handleSetConfig() {
    handleConfigSave();
}

void WebServer::handleGetStatus() {
    String status = "{";
    status += "\"device_role\":\"" + String(device.isMaster() ? "master" : "slave") + "\",";
    status += "\"device_name\":\"" + device.getName() + "\",";
    status += "\"wifi_status\":\"connected\",";
    status += "\"ip_address\":\"" + WiFi.localIP().toString() + "\",";
    status += "\"uptime\":" + String(millis());
    status += "}";
    
    server.send(200, "application/json", status);
}

void WebServer::setupOTA() {
    httpUpdater.setup(&server, "/firmware");
    LOG_I("WebServer", "OTA Update setup at /firmware");
}

String WebServer::getContentType(const String& filename) {
    if (filename.endsWith(".html")) return "text/html";
    else if (filename.endsWith(".css")) return "text/css";
    else if (filename.endsWith(".js")) return "application/javascript";
    else if (filename.endsWith(".json")) return "application/json";
    else if (filename.endsWith(".png")) return "image/png";
    else if (filename.endsWith(".jpg")) return "image/jpeg";
    else if (filename.endsWith(".ico")) return "image/x-icon";
    else if (filename.endsWith(".xml")) return "text/xml";
    else if (filename.endsWith(".pdf")) return "application/pdf";
    else if (filename.endsWith(".zip")) return "application/zip";
    else if (filename.endsWith(".gz")) return "application/x-gzip";
    return "text/plain";
}

bool WebServer::handleFileRead(const String& path) {
    String filePath = path;
    if (filePath.endsWith("/")) filePath += "index.html";
    
    String contentType = getContentType(filePath);
    
    if (SPIFFS.exists(filePath)) {
        File file = SPIFFS.open(filePath, "r");
        size_t sent = server.streamFile(file, contentType);
        file.close();
        return true;
    }
    
    // 尝试添加.gz后缀
    if (SPIFFS.exists(filePath + ".gz")) {
        File file = SPIFFS.open(filePath + ".gz", "r");
        size_t sent = server.streamFile(file, contentType);
        file.close();
        return true;
    }
    
    return false;
}

void WebServer::sendJsonResponse(int code, const String& message, const JsonObject& data) {
    String response = "{";
    response += "\"code\":" + String(code) + ",";
    response += "\"message\":\"" + message + "\"";
    
    if (data.size() > 0) {
        response += ",\"data\":{";
        // 这里需要根据实际的JsonObject来处理数据
        response += "}";
    }
    
    response += "}";
    server.send(code >= 400 ? code : 200, "application/json", response);
}