#ifndef CONFIG_MANAGER_H
#define CONFIG_MANAGER_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include "service_manager.h"
#include "filesystem_controller.h"
#include "event_system.h"

// 配置文件路径
#define CONFIG_FILE_PATH "/config.json"
#define CONFIG_BACKUP_PATH "/config.backup.json"

// 配置事件类型 (使用现有的事件系统枚举)
// EVENT_CONFIG_CHANGED 已在 event_system.h 中定义
// 其他配置相关事件将使用 EVENT_STATUS_UPDATE

// 设备角色枚举
enum DeviceRole {
    ROLE_MASTER,
    ROLE_SLAVE
};

// 配置结构体
struct DeviceConfig {
    DeviceRole role;
    String device_id;
    String device_name;
};

struct NetworkConfig {
    String ssid;
    String password;
    String hostname;
    String ip_mode;  // "dhcp" or "static"
    String static_ip;
    String gateway;
    String subnet;
    String dns1;
    String dns2;
};

struct RS485Config {
    uint32_t baud_rate;
    uint8_t data_bits;
    uint8_t stop_bits;
    String parity;  // "none", "even", "odd"
    uint8_t direction_pin;
};

struct CommunicationConfig {
    uint16_t data_port;
    uint16_t sync_port;
    bool auto_push;      // 主设备自动推送配置
    bool auto_sync;      // 从设备自动同步配置
    uint32_t sync_interval;  // 同步间隔(秒)
};

struct HeartbeatConfig {
    uint32_t interval;      // 心跳间隔(毫秒)
    uint32_t timeout;       // 超时时间(毫秒)
    uint8_t max_retries;    // 最大重试次数
};

struct SystemConfig {
    DeviceConfig device;
    NetworkConfig network;
    RS485Config rs485;
    CommunicationConfig communication;
    HeartbeatConfig heartbeat;
    String config_version;
    String last_modified;
};

// 配置管理器类
class ConfigManager : public BaseService {
private:
    FileSystemController* fsController;
    SystemConfig currentConfig;
    bool configLoaded;
    bool configChanged;
    unsigned long lastSaveTime;
    static const unsigned long SAVE_DELAY = 5000; // 5秒延迟保存
    
    // 内部方法
    bool loadConfigFromFile();
    bool saveConfigToFile();
    bool validateConfig(const SystemConfig& config);
    void setDefaultConfig();
    String generateDeviceId();
    String generateHostname(DeviceRole role, const String& deviceId);
    bool createBackup();
    bool restoreFromBackup();
    
    // JSON序列化/反序列化
    bool configToJson(const SystemConfig& config, JsonDocument& doc);
    bool jsonToConfig(const JsonDocument& doc, SystemConfig& config);
    
public:
    ConfigManager(FileSystemController* fs);
    virtual ~ConfigManager() = default;
    
    // BaseService接口实现
    bool onInitialize() override;
    bool onStart() override;
    void onUpdate() override;
    bool onStop() override;
    
    // 配置访问方法
    const SystemConfig& getConfig() const { return currentConfig; }
    bool isConfigLoaded() const { return configLoaded; }
    bool hasConfigChanged() const { return configChanged; }
    
    // 配置修改方法
    bool updateDeviceConfig(const DeviceConfig& config);
    bool updateNetworkConfig(const NetworkConfig& config);
    bool updateRS485Config(const RS485Config& config);
    bool updateCommunicationConfig(const CommunicationConfig& config);
    bool updateHeartbeatConfig(const HeartbeatConfig& config);
    bool updateFullConfig(const SystemConfig& config);
    
    // 配置管理方法
    bool loadConfig();
    bool saveConfig();
    bool resetToDefault();
    bool exportConfig(String& jsonString);
    bool importConfig(const String& jsonString);
    
    // 配置同步方法
    bool syncFromMaster(const String& masterConfigJson);
    String getConfigForSync();
    bool isConfigSyncNeeded(const String& masterConfigJson);
    
    // 工具方法
    String getDeviceId() const { return currentConfig.device.device_id; }
    DeviceRole getDeviceRole() const { return currentConfig.device.role; }
    String getDeviceName() const { return currentConfig.device.device_name; }
    String getHostname() const { return currentConfig.network.hostname; }
    
    // 配置验证方法
    bool validateNetworkConfig(const NetworkConfig& config);
    bool validateRS485Config(const RS485Config& config);
    bool validateCommunicationConfig(const CommunicationConfig& config);
    
    // 调试方法
    void printConfig();
    void printConfigSummary();
};

// 全局配置管理器实例声明
extern ConfigManager* g_configManager;

// 便捷宏定义
#define CONFIG_MANAGER g_configManager
#define GET_CONFIG() (g_configManager->getConfig())
#define GET_DEVICE_ROLE() (g_configManager->getDeviceRole())
#define GET_DEVICE_ID() (g_configManager->getDeviceId())
#define GET_HOSTNAME() (g_configManager->getHostname())

#endif