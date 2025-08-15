#include <Arduino.h>
#include "service_manager.h"
#include "debug_utils.h"

// 测试服务类
class TestService : public BaseService {
private:
    bool initResult;
    bool startResult;
    bool stopResult;
    int updateCount;
    
public:
    TestService(const String& name, bool initRes = true, bool startRes = true, bool stopRes = true)
        : BaseService(name), initResult(initRes), startResult(startRes), stopResult(stopRes), updateCount(0) {}
    
    bool onInitialize() override {
        DEBUG_VERBOSE_PRINT("TestService %s initialized", serviceName.c_str());
        return initResult;
    }
    
    bool onStart() override {
        DEBUG_VERBOSE_PRINT("TestService %s started", serviceName.c_str());
        return startResult;
    }
    
    void onUpdate() override {
        updateCount++;
        DEBUG_VERBOSE_PRINT("TestService %s updated (count: %d)", serviceName.c_str(), updateCount);
    }
    
    bool onStop() override {
        DEBUG_VERBOSE_PRINT("TestService %s stopped", serviceName.c_str());
        return stopResult;
    }
    
    int getUpdateCount() const { return updateCount; }
};

// 简单的测试框架
class ServiceManagerTest {
private:
    ServiceManager serviceManager;
    int testsPassed;
    int testsTotal;
    
public:
    ServiceManagerTest() : testsPassed(0), testsTotal(0) {}
    
    void runAllTests() {
        DEBUG_INFO_PRINT("=== ServiceManager Test Suite ===");
        
        // 初始化服务管理器
        if (!serviceManager.initialize()) {
            DEBUG_ERROR_PRINT("Failed to initialize ServiceManager for testing");
            return;
        }
        
        // 运行测试
        testInitialization();
        testServiceRegistration();
        testServiceLifecycle();
        testServiceManagement();
        testServiceStatus();
        
        // 输出结果
        DEBUG_INFO_PRINT("=== Test Results ===");
        DEBUG_INFO_PRINT("Tests passed: %d/%d", testsPassed, testsTotal);
        if (testsPassed == testsTotal) {
            DEBUG_INFO_PRINT("All tests PASSED!");
        } else {
            DEBUG_ERROR_PRINT("Some tests FAILED!");
        }
    }
    
private:
    void assertTrue(bool condition, const String& testName) {
        testsTotal++;
        if (condition) {
            testsPassed++;
            DEBUG_INFO_PRINT("✓ %s", testName.c_str());
        } else {
            DEBUG_ERROR_PRINT("✗ %s", testName.c_str());
        }
    }
    
    void testInitialization() {
        DEBUG_INFO_PRINT("--- Testing Initialization ---");
        
        assertTrue(serviceManager.isInitialized(), "ServiceManager is initialized");
    }
    
    void testServiceRegistration() {
        DEBUG_INFO_PRINT("--- Testing Service Registration ---");
        
        TestService* service1 = new TestService("TestService1");
        TestService* service2 = new TestService("TestService2");
        
        // 测试注册服务
        bool registerResult1 = serviceManager.registerService(service1);
        assertTrue(registerResult1, "Registering first service successful");
        assertTrue(serviceManager.getServiceCount() == 1, "Service count is 1 after first registration");
        
        bool registerResult2 = serviceManager.registerService(service2);
        assertTrue(registerResult2, "Registering second service successful");
        assertTrue(serviceManager.getServiceCount() == 2, "Service count is 2 after second registration");
        
        // 测试重复注册同一服务
        bool registerDuplicateResult = serviceManager.registerService(service1);
        assertTrue(registerDuplicateResult, "Registering duplicate service should succeed");
        
        // 测试注册同名服务（应该失败）
        TestService* service3 = new TestService("TestService1"); // 同名服务
        bool registerSameNameResult = serviceManager.registerService(service3);
        assertTrue(!registerSameNameResult, "Registering service with same name should fail");
        
        // 测试获取服务
        IService* retrievedService = serviceManager.getService("TestService1");
        assertTrue(retrievedService == service1, "Retrieved service should match registered service");
        
        IService* nonExistentService = serviceManager.getService("NonExistentService");
        assertTrue(nonExistentService == nullptr, "Retrieving non-existent service should return nullptr");
        
        // 清理
        delete service3; // 删除未注册的服务
    }
    
    void testServiceLifecycle() {
        DEBUG_INFO_PRINT("--- Testing Service Lifecycle ---");
        
        TestService* service = new TestService("LifecycleTestService");
        
        // 测试服务初始化
        bool initResult = service->initialize();
        assertTrue(initResult, "Service initialization successful");
        assertTrue(service->getStatus() == SERVICE_STARTING, "Service status should be STARTING after initialization");
        
        // 测试服务启动
        bool startResult = service->start();
        assertTrue(startResult, "Service start successful");
        assertTrue(service->getStatus() == SERVICE_RUNNING, "Service status should be RUNNING after start");
        
        // 测试服务更新
        int updateCountBefore = service->getUpdateCount();
        service->update();
        int updateCountAfter = service->getUpdateCount();
        assertTrue(updateCountAfter > updateCountBefore, "Service update should increment update count");
        
        // 测试服务停止
        bool stopResult = service->stop();
        assertTrue(stopResult, "Service stop successful");
        assertTrue(service->getStatus() == SERVICE_STOPPED, "Service status should be STOPPED after stop");
        
        // 清理
        delete service;
    }
    
    void testServiceManagement() {
        DEBUG_INFO_PRINT("--- Testing Service Management ---");
        
        TestService* service1 = new TestService("ManagedService1");
        TestService* service2 = new TestService("ManagedService2");
        
        serviceManager.registerService(service1);
        serviceManager.registerService(service2);
        
        // 测试启动所有服务
        bool startAllResult = serviceManager.startAllServices();
        assertTrue(startAllResult, "Starting all services successful");
        assertTrue(service1->getStatus() == SERVICE_RUNNING, "First service should be running");
        assertTrue(service2->getStatus() == SERVICE_RUNNING, "Second service should be running");
        
        // 测试更新所有服务
        int updateCount1Before = service1->getUpdateCount();
        int updateCount2Before = service2->getUpdateCount();
        serviceManager.updateAllServices();
        int updateCount1After = service1->getUpdateCount();
        int updateCount2After = service2->getUpdateCount();
        assertTrue(updateCount1After > updateCount1Before, "First service should be updated");
        assertTrue(updateCount2After > updateCount2Before, "Second service should be updated");
        
        // 测试停止所有服务
        bool stopAllResult = serviceManager.stopAllServices();
        assertTrue(stopAllResult, "Stopping all services successful");
        assertTrue(service1->getStatus() == SERVICE_STOPPED, "First service should be stopped");
        assertTrue(service2->getStatus() == SERVICE_STOPPED, "Second service should be stopped");
    }
    
    void testServiceStatus() {
        DEBUG_INFO_PRINT("--- Testing Service Status ---");
        
        TestService* service1 = new TestService("StatusTestService1");
        TestService* service2 = new TestService("StatusTestService2", true, false, true); // 启动失败的服务
        
        serviceManager.registerService(service1);
        serviceManager.registerService(service2);
        
        // 启动所有服务（其中一个会失败）
        bool startAllResult = serviceManager.startAllServices();
        assertTrue(!startAllResult, "Starting all services should fail when one service fails to start");
        
        // 测试服务状态查询
        assertTrue(!serviceManager.areAllServicesRunning(), "Not all services should be running");
        
        // 测试错误服务查询
        assertTrue(serviceManager.hasErrorServices(), "There should be error services");
        
        // 清理
        serviceManager.stopAllServices();
        delete service1;
        delete service2;
    }
};

// 全局测试实例
ServiceManagerTest* g_test = nullptr;

void setup() {
    Serial.begin(115200);
    delay(2000);
    
    DEBUG_INFO_PRINT("Starting ServiceManager Tests...");
    
    g_test = new ServiceManagerTest();
    g_test->runAllTests();
    
    DEBUG_INFO_PRINT("Tests completed. System will now halt.");
}

void loop() {
    // 测试完成后停止
    delay(1000);
}