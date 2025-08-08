#ifndef SERVICE_MANAGER_H
#define SERVICE_MANAGER_H

#include <Arduino.h>
#include <vector>
#include <functional>
#include "debug_utils.h"

// 服务状态枚举
enum ServiceStatus {
    SERVICE_STOPPED,
    SERVICE_STARTING,
    SERVICE_RUNNING,
    SERVICE_STOPPING,
    SERVICE_ERROR
};

// 服务接口基类
class IService {
public:
    virtual ~IService() = default;
    virtual bool initialize() = 0;
    virtual bool start() = 0;
    virtual void update() = 0;
    virtual bool stop() = 0;
    virtual ServiceStatus getStatus() const = 0;
    virtual String getName() const = 0;
    virtual String getStatusString() const = 0;
};

// 服务管理器类
class ServiceManager {
private:
    std::vector<IService*> services;
    bool initialized;
    unsigned long lastUpdateTime;
    static const unsigned long UPDATE_INTERVAL = 10; // 10ms更新间隔
    
public:
    ServiceManager();
    virtual ~ServiceManager();
    
    // 服务管理方法
    bool initialize();
    bool registerService(IService* service);
    bool unregisterService(IService* service);
    bool startAllServices();
    bool stopAllServices();
    void updateAllServices();
    
    // 查询方法
    IService* getService(const String& name);
    std::vector<IService*> getServices() const { return services; }
    size_t getServiceCount() const { return services.size(); }
    bool isInitialized() const { return initialized; }
    
    // 状态报告
    void printServiceStatus();
    bool areAllServicesRunning();
    bool hasErrorServices();
    
    // 主循环更新方法
    void update();
};

// 基础服务实现类
class BaseService : public IService {
protected:
    String serviceName;
    ServiceStatus status;
    unsigned long lastUpdateTime;
    unsigned long updateInterval;
    
public:
    BaseService(const String& name, unsigned long interval = 1000);
    virtual ~BaseService() = default;
    
    // IService接口实现
    bool initialize() override;
    bool start() override;
    void update() override;
    bool stop() override;
    ServiceStatus getStatus() const override { return status; }
    String getName() const override { return serviceName; }
    String getStatusString() const override;
    
    // 子类需要实现的虚方法
    virtual bool onInitialize() { return true; }
    virtual bool onStart() { return true; }
    virtual void onUpdate() {}
    virtual bool onStop() { return true; }
    
    // 工具方法
    void setUpdateInterval(unsigned long interval) { updateInterval = interval; }
    unsigned long getUpdateInterval() const { return updateInterval; }
    bool shouldUpdate();
};

#endif