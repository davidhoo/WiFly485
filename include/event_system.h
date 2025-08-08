#ifndef EVENT_SYSTEM_H
#define EVENT_SYSTEM_H

#include <Arduino.h>
#include <vector>
#include <functional>
#include <map>
#include "debug_utils.h"

// 事件类型枚举
enum EventType {
    EVENT_SYSTEM_STARTUP,
    EVENT_SYSTEM_SHUTDOWN,
    EVENT_WIFI_CONNECTED,
    EVENT_WIFI_DISCONNECTED,
    EVENT_CONFIG_CHANGED,
    EVENT_DATA_RECEIVED,
    EVENT_DATA_SENT,
    EVENT_ERROR_OCCURRED,
    EVENT_STATUS_UPDATE,
    EVENT_CUSTOM_BASE = 1000  // 自定义事件从1000开始
};

// 事件数据结构
struct EventData {
    EventType type;
    String source;
    String message;
    void* data;
    size_t dataSize;
    unsigned long timestamp;
    
    EventData(EventType t, const String& src = "", const String& msg = "", void* d = nullptr, size_t size = 0)
        : type(t), source(src), message(msg), data(d), dataSize(size), timestamp(millis()) {}
};

// 事件处理器类型定义
typedef std::function<void(const EventData&)> EventHandler;

// 事件监听器结构
struct EventListener {
    String name;
    EventHandler handler;
    bool oneTime;  // 是否只执行一次
    
    EventListener(const String& n, EventHandler h, bool once = false)
        : name(n), handler(h), oneTime(once) {}
};

// 事件系统类
class EventSystem {
private:
    std::map<EventType, std::vector<EventListener>> listeners;
    std::vector<EventData> eventQueue;
    bool initialized;
    size_t maxQueueSize;
    static const size_t DEFAULT_MAX_QUEUE_SIZE = 50;
    
    // 单例模式
    static EventSystem* instance;
    EventSystem();
    
public:
    virtual ~EventSystem();
    
    // 单例访问
    static EventSystem* getInstance();
    static void destroyInstance();
    
    // 初始化和清理
    bool initialize(size_t maxQueue = DEFAULT_MAX_QUEUE_SIZE);
    void cleanup();
    
    // 事件监听
    bool addEventListener(EventType type, const String& listenerName, EventHandler handler, bool oneTime = false);
    bool removeEventListener(EventType type, const String& listenerName);
    void removeAllListeners(EventType type);
    void removeAllListeners();
    
    // 事件发布
    bool publishEvent(EventType type, const String& source = "", const String& message = "", void* data = nullptr, size_t dataSize = 0);
    bool publishEvent(const EventData& event);
    
    // 事件处理
    void processEvents();
    void processEvent(const EventData& event);
    
    // 查询方法
    size_t getListenerCount(EventType type) const;
    size_t getTotalListenerCount() const;
    size_t getQueueSize() const { return eventQueue.size(); }
    bool isInitialized() const { return initialized; }
    
    // 调试方法
    void printListeners() const;
    void printEventQueue() const;
    String getEventTypeName(EventType type) const;
};

// 便利宏定义
#define EVENT_SYSTEM EventSystem::getInstance()
#define PUBLISH_EVENT(type, source, message) EVENT_SYSTEM->publishEvent(type, source, message)
#define LISTEN_EVENT(type, name, handler) EVENT_SYSTEM->addEventListener(type, name, handler)
#define LISTEN_EVENT_ONCE(type, name, handler) EVENT_SYSTEM->addEventListener(type, name, handler, true)

#endif