#include "config_manager.h"
#include "debug_utils.h"
#include <ESP8266WiFi.h>

// 全局配置管理器实例
ConfigManager* g_configManager = nullptr;

ConfigManager::ConfigManager(FileSystemController* fs) 
    : BaseService("ConfigManager", 1000), fsController(fs), configLoaded(false), 
      configChanged(false), lastSaveTime(0) {
    g_configManager = this;
}

bool ConfigManager::onInitialize() {
    DEBUG_INFO_PRINT("ConfigManager: Initializing...");
    
    if (!fsController || !fsController->isInitialized()) {
        DEBUG_ERROR_PRINT("ConfigManager: FileSystem not initialized");
        return false;
    }
    
    // 设置默认配置
    setDefaultConfig();
    
    DEBUG_INFO_PRINT("ConfigManager: Initialized successfully");
    return true;
}

bool ConfigManager::onStart() {
    DEBUG_INFO_PRINT("ConfigManager: Starting...");
    
    // 尝试加载配置文件
    if (!loadConfig()) {
        DEBUG_WARNING_PRINT("ConfigManager: Failed to load config, using defaults");
        // 保存默认配置
        if (!saveConfig()) {
            DEBUG_ERROR_PRINT("ConfigManager: Failed to save default config");
            return false;
        }
    }
    
    // 发布配置加载事件
    PUBLISH_EVENT(EVENT_STATUS_UPDATE, "ConfigManager", "Configuration loaded successfully");
    
    DEBUG_INFO_PRINT("ConfigManager: Started successfully");
    return true;
}

void ConfigManager::onUpdate() {
    // 检查是否需要延迟保存配置
    if (configChanged && (millis() - lastSaveTime >= SAVE_DELAY)) {
        DEBUG_INFO_PRINT("ConfigManager: Auto-saving changed configuration");
        if (saveConfig()) {
            configChanged = false;
            PUBLISH_EVENT(EVENT_STATUS_UPDATE, "ConfigManager", "Configuration auto-saved");
        } else {
            DEBUG_ERROR_PRINT("ConfigManager: Failed to auto-save configuration");
        }
    }
}

bool ConfigManager::onStop() {
    DEBUG_INFO_PRINT("ConfigManager: Stopping...");
    
    // 保存任何未保存的配置更改
    if (configChanged) {
        DEBUG_INFO_PRINT("ConfigManager: Saving pending changes before stop");
        saveConfig();
    }
    
    DEBUG_INFO_PRINT("ConfigManager: Stopped successfully");
    return true;
}

void ConfigManager::setDefaultConfig() {
    DEBUG_INFO_PRINT("ConfigManager: Setting default configuration");
    
    // 设备配置
#ifdef DEVICE_ROLE_MASTER
    currentConfig.device.role = ROLE_MASTER;
    currentConfig.device.device_name = "WiFly485_Master";
#else
    currentConfig.device.role = ROLE_SLAVE;
    currentConfig.device.device_name = "WiFly485_Slave";
#endif
    
    currentConfig.device.device_id = generateDeviceId();
    
    // 网络配置
    currentConfig.network.ssid = "";
    currentConfig.network.password = "";
    currentConfig.network.hostname = generateHostname(currentConfig.device.role, currentConfig.device.device_id);
    currentConfig.network.ip_mode = "dhcp";
    currentConfig.network.static_ip = "";
    currentConfig.network.gateway = "";
    currentConfig.network.subnet = "";
    currentConfig.network.dns1 = "";
    currentConfig.network.dns2 = "";
    
    // RS485配置
    currentConfig.rs485.baud_rate = 9600;
    currentConfig.rs485.data_bits = 8;
    currentConfig.rs485.stop_bits = 1;
    currentConfig.rs485.parity = "none";
    currentConfig.rs485.direction_pin = 2; // GPIO2
    
    // 通信配置
    currentConfig.communication.data_port = 8888;
    currentConfig.communication.sync_port = 8889;
    currentConfig.communication.auto_push = (currentConfig.device.role == ROLE_MASTER);
    currentConfig.communication.auto_sync = (currentConfig.device.role == ROLE_SLAVE);
    currentConfig.communication.sync_interval = 300; // 5分钟
    
    // 心跳配置
    currentConfig.heartbeat.interval = 5000;   // 5秒
    currentConfig.heartbeat.timeout = 15000;   // 15秒
    currentConfig.heartbeat.max_retries = 10;
    
    // 系统信息
    currentConfig.config_version = "1.0";
    currentConfig.last_modified = String(millis());
    
    configLoaded = true;
    DEBUG_INFO_PRINT("ConfigManager: Default configuration set");
}

String ConfigManager::generateDeviceId() {
    // 使用MAC地址生成唯一设备ID
    String mac = WiFi.macAddress();
    mac.replace(":", "");
    mac.toLowerCase();
    return mac.substring(6); // 取后6位
}

String ConfigManager::generateHostname(DeviceRole role, const String& deviceId) {
    String hostname = "wifly485-";
    hostname += (role == ROLE_MASTER) ? "master-" : "slave-";
    hostname += deviceId;
    return hostname;
}

bool ConfigManager::loadConfig() {
    DEBUG_INFO_PRINT("ConfigManager: Loading configuration from file");
    
    if (!fsController->exists(CONFIG_FILE_PATH)) {
        DEBUG_WARNING_PRINT("ConfigManager: Config file does not exist");
        return false;
    }
    
    String configJson = fsController->readFile(CONFIG_FILE_PATH);
    if (configJson.isEmpty()) {
        DEBUG_ERROR_PRINT("ConfigManager: Failed to read config file");
        return false;
    }
    
    return importConfig(configJson);
}

bool ConfigManager::saveConfig() {
    DEBUG_INFO_PRINT("ConfigManager: Saving configuration to file");
    
    // 创建备份
    if (fsController->exists(CONFIG_FILE_PATH)) {
        if (!createBackup()) {
            DEBUG_WARNING_PRINT("ConfigManager: Failed to create backup");
        }
    }
    
    // 导出配置为JSON
    String configJson;
    if (!exportConfig(configJson)) {
        DEBUG_ERROR_PRINT("ConfigManager: Failed to export config to JSON");
        return false;
    }
    
    // 写入文件
    if (!fsController->writeFile(CONFIG_FILE_PATH, configJson)) {
        DEBUG_ERROR_PRINT("ConfigManager: Failed to write config file");
        // 尝试恢复备份
        if (!restoreFromBackup()) {
            DEBUG_ERROR_PRINT("ConfigManager: Failed to restore from backup");
        }
        return false;
    }
    
    // 更新最后修改时间
    currentConfig.last_modified = String(millis());
    configChanged = false;
    
    DEBUG_INFO_PRINT("ConfigManager: Configuration saved successfully");
    return true;
}

bool ConfigManager::createBackup() {
    if (!fsController->exists(CONFIG_FILE_PATH)) {
        return true; // 没有原文件，不需要备份
    }
    
    String configContent = fsController->readFile(CONFIG_FILE_PATH);
    if (configContent.isEmpty()) {
        return false;
    }
    
    return fsController->writeFile(CONFIG_BACKUP_PATH, configContent);
}

bool ConfigManager::restoreFromBackup() {
    if (!fsController->exists(CONFIG_BACKUP_PATH)) {
        return false;
    }
    
    String backupContent = fsController->readFile(CONFIG_BACKUP_PATH);
    if (backupContent.isEmpty()) {
        return false;
    }
    
    return fsController->writeFile(CONFIG_FILE_PATH, backupContent);
}

bool ConfigManager::exportConfig(String& jsonString) {
    DynamicJsonDocument doc(2048);
    
    if (!configToJson(currentConfig, doc)) {
        DEBUG_ERROR_PRINT("ConfigManager: Failed to convert config to JSON");
        return false;
    }
    
    if (serializeJson(doc, jsonString) == 0) {
        DEBUG_ERROR_PRINT("ConfigManager: Failed to serialize JSON");
        return false;
    }
    
    return true;
}

bool ConfigManager::importConfig(const String& jsonString) {
    DynamicJsonDocument doc(2048);
    
    DeserializationError error = deserializeJson(doc, jsonString);
    if (error) {
        DEBUG_ERROR_PRINT("ConfigManager: JSON deserialization failed: %s", error.c_str());
        return false;
    }
    
    SystemConfig newConfig;
    if (!jsonToConfig(doc, newConfig)) {
        DEBUG_ERROR_PRINT("ConfigManager: Failed to convert JSON to config");
        return false;
    }
    
    if (!validateConfig(newConfig)) {
        DEBUG_ERROR_PRINT("ConfigManager: Configuration validation failed");
        return false;
    }
    
    currentConfig = newConfig;
    configLoaded = true;
    configChanged = true;
    lastSaveTime = millis();
    
    DEBUG_INFO_PRINT("ConfigManager: Configuration imported successfully");
    PUBLISH_EVENT(EVENT_CONFIG_CHANGED, "ConfigManager", "Configuration imported");
    
    return true;
}

bool ConfigManager::configToJson(const SystemConfig& config, JsonDocument& doc) {
    // 设备配置
    JsonObject device = doc.createNestedObject("device");
    device["role"] = (config.device.role == ROLE_MASTER) ? "master" : "slave";
    device["device_id"] = config.device.device_id;
    device["device_name"] = config.device.device_name;
    
    // 网络配置
    JsonObject network = doc.createNestedObject("network");
    network["ssid"] = config.network.ssid;
    network["password"] = config.network.password;
    network["hostname"] = config.network.hostname;
    network["ip_mode"] = config.network.ip_mode;
    if (!config.network.static_ip.isEmpty()) {
        network["static_ip"] = config.network.static_ip;
        network["gateway"] = config.network.gateway;
        network["subnet"] = config.network.subnet;
        network["dns1"] = config.network.dns1;
        network["dns2"] = config.network.dns2;
    }
    
    // RS485配置
    JsonObject rs485 = doc.createNestedObject("rs485");
    rs485["baud_rate"] = config.rs485.baud_rate;
    rs485["data_bits"] = config.rs485.data_bits;
    rs485["stop_bits"] = config.rs485.stop_bits;
    rs485["parity"] = config.rs485.parity;
    rs485["direction_pin"] = config.rs485.direction_pin;
    
    // 通信配置
    JsonObject communication = doc.createNestedObject("communication");
    communication["data_port"] = config.communication.data_port;
    communication["sync_port"] = config.communication.sync_port;
    communication["auto_push"] = config.communication.auto_push;
    communication["auto_sync"] = config.communication.auto_sync;
    communication["sync_interval"] = config.communication.sync_interval;
    
    // 心跳配置
    JsonObject heartbeat = doc.createNestedObject("heartbeat");
    heartbeat["interval"] = config.heartbeat.interval;
    heartbeat["timeout"] = config.heartbeat.timeout;
    heartbeat["max_retries"] = config.heartbeat.max_retries;
    
    // 系统信息
    doc["config_version"] = config.config_version;
    doc["last_modified"] = config.last_modified;
    
    return true;
}

bool ConfigManager::jsonToConfig(const JsonDocument& doc, SystemConfig& config) {
    // 设备配置
    if (doc.containsKey("device")) {
        JsonObject device = doc["device"];
        String role = device["role"] | "";
        config.device.role = (role == "master") ? ROLE_MASTER : ROLE_SLAVE;
        config.device.device_id = device["device_id"] | generateDeviceId();
        config.device.device_name = device["device_name"] | "WiFly485_Unknown";
    }
    
    // 网络配置
    if (doc.containsKey("network")) {
        JsonObject network = doc["network"];
        config.network.ssid = network["ssid"] | "";
        config.network.password = network["password"] | "";
        config.network.hostname = network["hostname"] | generateHostname(config.device.role, config.device.device_id);
        config.network.ip_mode = network["ip_mode"] | "dhcp";
        config.network.static_ip = network["static_ip"] | "";
        config.network.gateway = network["gateway"] | "";
        config.network.subnet = network["subnet"] | "";
        config.network.dns1 = network["dns1"] | "";
        config.network.dns2 = network["dns2"] | "";
    }
    
    // RS485配置
    if (doc.containsKey("rs485")) {
        JsonObject rs485 = doc["rs485"];
        config.rs485.baud_rate = rs485["baud_rate"] | 9600;
        config.rs485.data_bits = rs485["data_bits"] | 8;
        config.rs485.stop_bits = rs485["stop_bits"] | 1;
        config.rs485.parity = rs485["parity"] | "none";
        config.rs485.direction_pin = rs485["direction_pin"] | 2;
    }
    
    // 通信配置
    if (doc.containsKey("communication")) {
        JsonObject communication = doc["communication"];
        config.communication.data_port = communication["data_port"] | 8888;
        config.communication.sync_port = communication["sync_port"] | 8889;
        config.communication.auto_push = communication["auto_push"] | (config.device.role == ROLE_MASTER);
        config.communication.auto_sync = communication["auto_sync"] | (config.device.role == ROLE_SLAVE);
        config.communication.sync_interval = communication["sync_interval"] | 300;
    }
    
    // 心跳配置
    if (doc.containsKey("heartbeat")) {
        JsonObject heartbeat = doc["heartbeat"];
        config.heartbeat.interval = heartbeat["interval"] | 5000;
        config.heartbeat.timeout = heartbeat["timeout"] | 15000;
        config.heartbeat.max_retries = heartbeat["max_retries"] | 10;
    }
    
    // 系统信息
    config.config_version = doc["config_version"] | "1.0";
    config.last_modified = doc["last_modified"] | String(millis());
    
    return true;
}

bool ConfigManager::validateConfig(const SystemConfig& config) {
    DEBUG_VERBOSE_PRINT("ConfigManager: Validating configuration");
    
    // 验证设备配置
    if (config.device.device_id.isEmpty() || config.device.device_name.isEmpty()) {
        DEBUG_ERROR_PRINT("ConfigManager: Invalid device configuration");
        return false;
    }
    
    // 验证网络配置
    if (!validateNetworkConfig(config.network)) {
        return false;
    }
    
    // 验证RS485配置
    if (!validateRS485Config(config.rs485)) {
        return false;
    }
    
    // 验证通信配置
    if (!validateCommunicationConfig(config.communication)) {
        return false;
    }
    
    DEBUG_VERBOSE_PRINT("ConfigManager: Configuration validation passed");
    return true;
}

bool ConfigManager::validateNetworkConfig(const NetworkConfig& config) {
    // 如果使用静态IP，验证IP地址格式
    if (config.ip_mode == "static") {
        if (config.static_ip.isEmpty() || config.gateway.isEmpty() || config.subnet.isEmpty()) {
            DEBUG_ERROR_PRINT("ConfigManager: Static IP configuration incomplete");
            return false;
        }
        
        // 简单的IP地址格式验证
        IPAddress ip, gateway, subnet;
        if (!ip.fromString(config.static_ip) || 
            !gateway.fromString(config.gateway) || 
            !subnet.fromString(config.subnet)) {
            DEBUG_ERROR_PRINT("ConfigManager: Invalid IP address format");
            return false;
        }
    }
    
    return true;
}

bool ConfigManager::validateRS485Config(const RS485Config& config) {
    // 验证波特率
    if (config.baud_rate < 1200 || config.baud_rate > 115200) {
        DEBUG_ERROR_PRINT("ConfigManager: Invalid baud rate: %d", config.baud_rate);
        return false;
    }
    
    // 验证数据位
    if (config.data_bits < 5 || config.data_bits > 8) {
        DEBUG_ERROR_PRINT("ConfigManager: Invalid data bits: %d", config.data_bits);
        return false;
    }
    
    // 验证停止位
    if (config.stop_bits < 1 || config.stop_bits > 2) {
        DEBUG_ERROR_PRINT("ConfigManager: Invalid stop bits: %d", config.stop_bits);
        return false;
    }
    
    // 验证校验位
    if (config.parity != "none" && config.parity != "even" && config.parity != "odd") {
        DEBUG_ERROR_PRINT("ConfigManager: Invalid parity: %s", config.parity.c_str());
        return false;
    }
    
    // 验证方向控制引脚
    if (config.direction_pin > 16) {
        DEBUG_ERROR_PRINT("ConfigManager: Invalid direction pin: %d", config.direction_pin);
        return false;
    }
    
    return true;
}

bool ConfigManager::validateCommunicationConfig(const CommunicationConfig& config) {
    // 验证端口号
    if (config.data_port < 1024 || config.data_port > 65535) {
        DEBUG_ERROR_PRINT("ConfigManager: Invalid data port: %d", config.data_port);
        return false;
    }
    
    if (config.sync_port < 1024 || config.sync_port > 65535) {
        DEBUG_ERROR_PRINT("ConfigManager: Invalid sync port: %d", config.sync_port);
        return false;
    }
    
    if (config.data_port == config.sync_port) {
        DEBUG_ERROR_PRINT("ConfigManager: Data port and sync port cannot be the same");
        return false;
    }
    
    // 验证同步间隔
    if (config.sync_interval < 60 || config.sync_interval > 3600) {
        DEBUG_ERROR_PRINT("ConfigManager: Invalid sync interval: %d", config.sync_interval);
        return false;
    }
    
    return true;
}

bool ConfigManager::updateFullConfig(const SystemConfig& config) {
    if (!validateConfig(config)) {
        DEBUG_ERROR_PRINT("ConfigManager: Configuration validation failed");
        return false;
    }
    
    currentConfig = config;
    currentConfig.last_modified = String(millis());
    configChanged = true;
    lastSaveTime = millis();
    
    PUBLISH_EVENT(EVENT_CONFIG_CHANGED, "ConfigManager", "Full configuration updated");
    DEBUG_INFO_PRINT("ConfigManager: Full configuration updated");
    
    return true;
}

bool ConfigManager::resetToDefault() {
    DEBUG_INFO_PRINT("ConfigManager: Resetting to default configuration");
    
    setDefaultConfig();
    configChanged = true;
    lastSaveTime = millis();
    
    if (!saveConfig()) {
        DEBUG_ERROR_PRINT("ConfigManager: Failed to save default configuration");
        return false;
    }
    
    PUBLISH_EVENT(EVENT_CONFIG_CHANGED, "ConfigManager", "Configuration reset to default");
    DEBUG_INFO_PRINT("ConfigManager: Configuration reset to default successfully");
    
    return true;
}

void ConfigManager::printConfig() {
    DEBUG_INFO_PRINT("=== Current Configuration ===");
    DEBUG_INFO_PRINT("Device:");
    DEBUG_INFO_PRINT("  Role: %s", (currentConfig.device.role == ROLE_MASTER) ? "Master" : "Slave");
    DEBUG_INFO_PRINT("  ID: %s", currentConfig.device.device_id.c_str());
    DEBUG_INFO_PRINT("  Name: %s", currentConfig.device.device_name.c_str());
    
    DEBUG_INFO_PRINT("Network:");
    DEBUG_INFO_PRINT("  SSID: %s", currentConfig.network.ssid.c_str());
    DEBUG_INFO_PRINT("  Hostname: %s", currentConfig.network.hostname.c_str());
    DEBUG_INFO_PRINT("  IP Mode: %s", currentConfig.network.ip_mode.c_str());
    
    DEBUG_INFO_PRINT("RS485:");
    DEBUG_INFO_PRINT("  Baud Rate: %d", currentConfig.rs485.baud_rate);
    DEBUG_INFO_PRINT("  Data Bits: %d", currentConfig.rs485.data_bits);
    DEBUG_INFO_PRINT("  Stop Bits: %d", currentConfig.rs485.stop_bits);
    DEBUG_INFO_PRINT("  Parity: %s", currentConfig.rs485.parity.c_str());
    DEBUG_INFO_PRINT("  Direction Pin: %d", currentConfig.rs485.direction_pin);
    
    DEBUG_INFO_PRINT("Communication:");
    DEBUG_INFO_PRINT("  Data Port: %d", currentConfig.communication.data_port);
    DEBUG_INFO_PRINT("  Sync Port: %d", currentConfig.communication.sync_port);
    DEBUG_INFO_PRINT("  Auto Push: %s", currentConfig.communication.auto_push ? "Yes" : "No");
    DEBUG_INFO_PRINT("  Auto Sync: %s", currentConfig.communication.auto_sync ? "Yes" : "No");
    
    DEBUG_INFO_PRINT("Version: %s", currentConfig.config_version.c_str());
    DEBUG_INFO_PRINT("Last Modified: %s", currentConfig.last_modified.c_str());
    DEBUG_INFO_PRINT("=============================");
}

void ConfigManager::printConfigSummary() {
    DEBUG_INFO_PRINT("Config: %s [%s] - %s:%d",
        currentConfig.device.device_name.c_str(),
        currentConfig.device.device_id.c_str(),
        currentConfig.network.hostname.c_str(),
        currentConfig.communication.data_port);
}

// 配置同步相关方法实现
bool ConfigManager::syncFromMaster(const String& masterConfigJson) {
    DEBUG_INFO_PRINT("ConfigManager: Syncing configuration from master");
    
    if (currentConfig.device.role != ROLE_SLAVE) {
        DEBUG_ERROR_PRINT("ConfigManager: Only slave devices can sync from master");
        return false;
    }
    
    // 解析主设备配置
    DynamicJsonDocument doc(2048);
    DeserializationError error = deserializeJson(doc, masterConfigJson);
    if (error) {
        DEBUG_ERROR_PRINT("ConfigManager: Failed to parse master config JSON: %s", error.c_str());
        return false;
    }
    
    SystemConfig masterConfig;
    if (!jsonToConfig(doc, masterConfig)) {
        DEBUG_ERROR_PRINT("ConfigManager: Failed to convert master config JSON");
        return false;
    }
    
    // 保留从设备的网络配置
    NetworkConfig slaveNetworkConfig = currentConfig.network;
    
    // 应用主设备配置
    currentConfig = masterConfig;
    
    // 恢复从设备的网络配置和角色
    currentConfig.network = slaveNetworkConfig;
    currentConfig.device.role = ROLE_SLAVE;
    currentConfig.device.device_name = "WiFly485_Slave";
    currentConfig.communication.auto_push = false;
    currentConfig.communication.auto_sync = true;
    
    // 更新修改时间
    currentConfig.last_modified = String(millis());
    configChanged = true;
    lastSaveTime = millis();
    
    PUBLISH_EVENT(EVENT_CONFIG_CHANGED, "ConfigManager", "Configuration synced from master");
    DEBUG_INFO_PRINT("ConfigManager: Configuration synced from master successfully");
    
    return true;
}

String ConfigManager::getConfigForSync() {
    DEBUG_VERBOSE_PRINT("ConfigManager: Preparing configuration for sync");
    
    String configJson;
    if (!exportConfig(configJson)) {
        DEBUG_ERROR_PRINT("ConfigManager: Failed to export config for sync");
        return "";
    }
    
    return configJson;
}

bool ConfigManager::isConfigSyncNeeded(const String& masterConfigJson) {
    if (masterConfigJson.isEmpty()) {
        return false;
    }
    
    // 解析主设备配置
    DynamicJsonDocument masterDoc(2048);
    DeserializationError error = deserializeJson(masterDoc, masterConfigJson);
    if (error) {
        DEBUG_ERROR_PRINT("ConfigManager: Failed to parse master config for comparison");
        return false;
    }
    
    // 导出当前配置
    DynamicJsonDocument currentDoc(2048);
    if (!configToJson(currentConfig, currentDoc)) {
        DEBUG_ERROR_PRINT("ConfigManager: Failed to export current config for comparison");
        return false;
    }
    
    // 比较关键配置项
    bool needSync = false;
    
    // 比较RS485配置
    if (masterDoc["rs485"]["baud_rate"] != currentDoc["rs485"]["baud_rate"] ||
        masterDoc["rs485"]["data_bits"] != currentDoc["rs485"]["data_bits"] ||
        masterDoc["rs485"]["stop_bits"] != currentDoc["rs485"]["stop_bits"] ||
        masterDoc["rs485"]["parity"] != currentDoc["rs485"]["parity"] ||
        masterDoc["rs485"]["direction_pin"] != currentDoc["rs485"]["direction_pin"]) {
        needSync = true;
        DEBUG_VERBOSE_PRINT("ConfigManager: RS485 config differs from master");
    }
    
    // 比较通信配置
    if (masterDoc["communication"]["data_port"] != currentDoc["communication"]["data_port"] ||
        masterDoc["communication"]["sync_port"] != currentDoc["communication"]["sync_port"] ||
        masterDoc["communication"]["sync_interval"] != currentDoc["communication"]["sync_interval"]) {
        needSync = true;
        DEBUG_VERBOSE_PRINT("ConfigManager: Communication config differs from master");
    }
    
    // 比较心跳配置
    if (masterDoc["heartbeat"]["interval"] != currentDoc["heartbeat"]["interval"] ||
        masterDoc["heartbeat"]["timeout"] != currentDoc["heartbeat"]["timeout"] ||
        masterDoc["heartbeat"]["max_retries"] != currentDoc["heartbeat"]["max_retries"]) {
        needSync = true;
        DEBUG_VERBOSE_PRINT("ConfigManager: Heartbeat config differs from master");
    }
    
    return needSync;
}

// 配置更新方法实现
bool ConfigManager::updateDeviceConfig(const DeviceConfig& config) {
    DEBUG_INFO_PRINT("ConfigManager: Updating device configuration");
    
    currentConfig.device = config;
    currentConfig.last_modified = String(millis());
    configChanged = true;
    lastSaveTime = millis();
    
    PUBLISH_EVENT(EVENT_CONFIG_CHANGED, "ConfigManager", "Device configuration updated");
    return true;
}

bool ConfigManager::updateNetworkConfig(const NetworkConfig& config) {
    DEBUG_INFO_PRINT("ConfigManager: Updating network configuration");
    
    if (!validateNetworkConfig(config)) {
        DEBUG_ERROR_PRINT("ConfigManager: Network configuration validation failed");
        return false;
    }
    
    currentConfig.network = config;
    currentConfig.last_modified = String(millis());
    configChanged = true;
    lastSaveTime = millis();
    
    PUBLISH_EVENT(EVENT_CONFIG_CHANGED, "ConfigManager", "Network configuration updated");
    return true;
}

bool ConfigManager::updateRS485Config(const RS485Config& config) {
    DEBUG_INFO_PRINT("ConfigManager: Updating RS485 configuration");
    
    if (!validateRS485Config(config)) {
        DEBUG_ERROR_PRINT("ConfigManager: RS485 configuration validation failed");
        return false;
    }
    
    currentConfig.rs485 = config;
    currentConfig.last_modified = String(millis());
    configChanged = true;
    lastSaveTime = millis();
    
    PUBLISH_EVENT(EVENT_CONFIG_CHANGED, "ConfigManager", "RS485 configuration updated");
    return true;
}

bool ConfigManager::updateCommunicationConfig(const CommunicationConfig& config) {
    DEBUG_INFO_PRINT("ConfigManager: Updating communication configuration");
    
    if (!validateCommunicationConfig(config)) {
        DEBUG_ERROR_PRINT("ConfigManager: Communication configuration validation failed");
        return false;
    }
    
    currentConfig.communication = config;
    currentConfig.last_modified = String(millis());
    configChanged = true;
    lastSaveTime = millis();
    
    PUBLISH_EVENT(EVENT_CONFIG_CHANGED, "ConfigManager", "Communication configuration updated");
    return true;
}

bool ConfigManager::updateHeartbeatConfig(const HeartbeatConfig& config) {
    DEBUG_INFO_PRINT("ConfigManager: Updating heartbeat configuration");
    
    // 验证心跳配置
    if (config.interval < 1000 || config.interval > 60000) {
        DEBUG_ERROR_PRINT("ConfigManager: Invalid heartbeat interval: %d", config.interval);
        return false;
    }
    
    if (config.timeout < config.interval || config.timeout > 300000) {
        DEBUG_ERROR_PRINT("ConfigManager: Invalid heartbeat timeout: %d", config.timeout);
        return false;
    }
    
    if (config.max_retries < 1 || config.max_retries > 100) {
        DEBUG_ERROR_PRINT("ConfigManager: Invalid max retries: %d", config.max_retries);
        return false;
    }
    
    currentConfig.heartbeat = config;
    currentConfig.last_modified = String(millis());
    configChanged = true;
    lastSaveTime = millis();
    
    PUBLISH_EVENT(EVENT_CONFIG_CHANGED, "ConfigManager", "Heartbeat configuration updated");
    return true;
}