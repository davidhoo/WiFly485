#include <Arduino.h>
#include "debug_utils.h"
#include "gpio_controller.h"
#include "uart_controller.h"
#include "wifi_controller.h"
#include "filesystem_controller.h"
#include "service_manager.h"
#include "event_system.h"
#include "config_manager.h"

// 硬件抽象层控制器实例
GPIOController gpioController;
UARTController uartController;
WiFiController wifiController;
FileSystemController fsController;

// 服务管理器实例
ServiceManager serviceManager;

// 配置管理器实例
ConfigManager configManager(&fsController);

// 测试服务类
class TestService : public BaseService {
private:
    int counter;
    
public:
    TestService() : BaseService("TestService", 2000), counter(0) {}
    
protected:
    bool onInitialize() override {
        DEBUG_INFO_PRINT("TestService: Initializing...");
        counter = 0;
        return true;
    }
    
    bool onStart() override {
        DEBUG_INFO_PRINT("TestService: Starting...");
        
        // 监听系统事件
        LISTEN_EVENT(EVENT_SYSTEM_STARTUP, "TestService", [this](const EventData& event) {
            DEBUG_INFO_PRINT("TestService: Received system startup event");
        });
        
        return true;
    }
    
    void onUpdate() override {
        counter++;
        DEBUG_VERBOSE_PRINT("TestService: Update #%d", counter);
        
        // 每10次更新发布一个状态事件
        if (counter % 10 == 0) {
            PUBLISH_EVENT(EVENT_STATUS_UPDATE, "TestService", "Counter: " + String(counter));
        }
    }
    
    bool onStop() override {
        DEBUG_INFO_PRINT("TestService: Stopping...");
        return true;
    }
};

TestService testService;

void setup() {
    Serial.begin(115200);
    delay(1000);
    
    DEBUG_INFO_PRINT("WiFly485 Starting...");
    printSystemInfo();
    
    // 初始化事件系统
    DEBUG_INFO_PRINT("Initializing Event System...");
    if (!EVENT_SYSTEM->initialize()) {
        DEBUG_ERROR_PRINT("Failed to initialize Event System");
    }
    
    // 初始化服务管理器
    DEBUG_INFO_PRINT("Initializing Service Manager...");
    if (!serviceManager.initialize()) {
        DEBUG_ERROR_PRINT("Failed to initialize Service Manager");
    }
    
    // 初始化硬件抽象层
    DEBUG_INFO_PRINT("Initializing Hardware Abstraction Layer...");
    
    if (!gpioController.initialize()) {
        DEBUG_ERROR_PRINT("Failed to initialize GPIO Controller");
    }
    
    if (!uartController.initialize(9600, SERIAL_8N1)) {
        DEBUG_ERROR_PRINT("Failed to initialize UART Controller");
    }
    
    if (!wifiController.initialize()) {
        DEBUG_ERROR_PRINT("Failed to initialize WiFi Controller");
    }
    
    if (!fsController.initialize()) {
        DEBUG_ERROR_PRINT("Failed to initialize File System Controller");
    }
    
    // 注册配置管理器服务
    DEBUG_INFO_PRINT("Registering services...");
    if (!serviceManager.registerService(&configManager)) {
        DEBUG_ERROR_PRINT("Failed to register ConfigManager");
    }
    
    // 注册测试服务
    if (!serviceManager.registerService(&testService)) {
        DEBUG_ERROR_PRINT("Failed to register TestService");
    }
    
    // 启动所有服务
    DEBUG_INFO_PRINT("Starting all services...");
    if (!serviceManager.startAllServices()) {
        DEBUG_ERROR_PRINT("Failed to start all services");
    }
    
    // 发布系统启动事件
    // 发布系统启动事件
    PUBLISH_EVENT(EVENT_SYSTEM_STARTUP, "Main", "System started successfully");
    
    // 打印配置信息
    if (configManager.isConfigLoaded()) {
        configManager.printConfigSummary();
        DEBUG_INFO_PRINT("Device Role: %s",
            (GET_DEVICE_ROLE() == ROLE_MASTER) ? "Master" : "Slave");
        DEBUG_INFO_PRINT("Device ID: %s", GET_DEVICE_ID().c_str());
        DEBUG_INFO_PRINT("Hostname: %s", GET_HOSTNAME().c_str());
    }
    
    DEBUG_INFO_PRINT("Hardware Abstraction Layer initialized");
    DEBUG_INFO_PRINT("WiFly485 Started Successfully");
}

void loop() {
    // 更新服务管理器
    serviceManager.update();
    
    // 处理事件
    EVENT_SYSTEM->processEvents();
    
    // 更新WiFi控制器状态
    wifiController.update();
    
    // 定期打印系统状态
    static unsigned long lastStatusPrint = 0;
    if (millis() - lastStatusPrint >= 30000) { // 每30秒打印一次
        printMemoryInfo();
        serviceManager.printServiceStatus();
        EVENT_SYSTEM->printListeners();
        lastStatusPrint = millis();
    }
    
    delay(50);
}
