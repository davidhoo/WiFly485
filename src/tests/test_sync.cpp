#include "test_framework.h"
#include "config_sync.h"
#include "device.h"
#include "config_manager.h"

// 测试用的设备和配置管理器
static Device testDevice;
static ConfigManager testConfigManager;
static ConfigSync configSync;

void test_config_sync_initialization() {
  Serial.println("Test: ConfigSync Initialization");
  
  // 初始化设备
  testDevice.begin();
  
  // 初始化配置管理器
  testConfigManager.begin();
  
  // 初始化配置同步
  bool result = configSync.begin(&testDevice, &testConfigManager);
  ASSERT_TRUE(result);
  
  // 检查初始状态
  ASSERT_EQUAL(CONFIG_SYNC_DISCONNECTED, configSync.getSyncStatus());
}

void test_config_sync_status_transitions() {
  Serial.println("Test: ConfigSync Status Transitions");
  
  // 测试状态更新
  // 注意：这些是私有方法，无法在测试中直接调用
  // 我们只能测试公共接口
  // 这里我们只是演示测试的结构
  Serial.println("  [SKIP] Status transitions (private methods)");
}

void test_config_sync_request_sync() {
  Serial.println("Test: ConfigSync Request Sync");
  
  // 设置设备为从设备
  testDevice.setRole(DEVICE_ROLE_SLAVE_ENUM);
  
  // 尝试请求同步（应该失败，因为没有连接到主设备）
  bool result = configSync.requestSync();
  // 注意：在实际测试中，这个结果取决于网络连接状态
  // 我们只是演示测试的结构
  Serial.println("  [SKIP] Request sync (network dependency)");
}

void test_config_sync_force_sync() {
  Serial.println("Test: ConfigSync Force Sync");
  
  // 设置设备为主设备
  testDevice.setRole(DEVICE_ROLE_MASTER_ENUM);
  
  // 尝试强制同步（应该失败，因为没有从设备连接）
  bool result = configSync.forceSync();
  // 注意：在实际测试中，这个结果取决于是否有从设备连接
  // 我们只是演示测试的结构
  Serial.println("  [SKIP] Force sync (network dependency)");
}

void run_config_sync_tests() {
  Serial.println("Running Config Sync Tests...");
  Serial.println("============================");
  
  test_config_sync_initialization();
  test_config_sync_status_transitions();
  test_config_sync_request_sync();
  test_config_sync_force_sync();
  
  Serial.println("============================");
}