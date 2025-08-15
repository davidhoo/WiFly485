# WiFly485 系统架构图

## 1. 整体系统架构

```mermaid
graph TB
    subgraph "VRF控制器端"
        VRF[VRF控制器]
        Master[主设备 ESP8266]
        VRF <-->|RS485| Master
    end
    
    subgraph "新风系统端"
        Slave[从设备 ESP8266]
        Fresh[新风系统]
        Slave <-->|RS485| Fresh
    end
    
    subgraph "网络基础设施"
        Router[WiFi路由器]
        Router <-->|WiFi| Master
        Router <-->|WiFi| Slave
    end
    
    Master <-->|TCP 8888<br>数据传输| Slave
    Master <-->|TCP 8889<br>配置同步| Slave
    
    subgraph "管理界面"
        WebUI[Web管理界面]
        Master --> WebUI
        Slave --> WebUI
    end
```

## 2. 软件模块架构

```mermaid
graph TD
    subgraph "应用层"
        Main[主程序 main.cpp]
        WebServer[Web服务器]
        ConfigSync[配置同步]
    end
    
    subgraph "业务逻辑层"
        DeviceManager[设备管理器]
        ProtocolHandler[协议处理器]
        DataRouter[数据路由器]
    end
    
    subgraph "通信层"
        WiFiManager[WiFi管理器]
        TCPProtocol[TCP协议]
        RS485Driver[RS485驱动]
        mDNSService[mDNS服务]
    end
    
    subgraph "系统层"
        ConfigManager[配置管理器]
        Logger[日志系统]
        LEDIndicator[LED指示器]
        ErrorHandler[错误处理器]
        Heartbeat[心跳检测]
    end
    
    Main --> DeviceManager
    Main --> WebServer
    Main --> ConfigSync
    
    DeviceManager --> ProtocolHandler
    DeviceManager --> DataRouter
    
    ProtocolHandler --> TCPProtocol
    DataRouter --> RS485Driver
    
    WiFiManager --> mDNSService
    TCPProtocol --> WiFiManager
    
    ConfigSync --> ConfigManager
    WebServer --> ConfigManager
    
    DeviceManager --> Logger
    DeviceManager --> LEDIndicator
    DeviceManager --> ErrorHandler
    DeviceManager --> Heartbeat
```

## 3. 数据流架构

```mermaid
sequenceDiagram
    participant VRF as VRF控制器
    participant Master as 主设备
    participant Network as 网络
    participant Slave as 从设备
    participant Fresh as 新风系统
    
    Note over VRF, Fresh: 数据传输流程
    
    VRF->>Master: RS485数据
    Master->>Master: 切换到发送模式
    Master->>Network: TCP数据包
    Network->>Slave: TCP数据包
    Slave->>Slave: 切换到发送模式
    Slave->>Fresh: RS485数据
    
    Fresh->>Slave: RS485响应
    Slave->>Slave: 切换到发送模式
    Slave->>Network: TCP响应包
    Network->>Master: TCP响应包
    Master->>Master: 切换到发送模式
    Master->>VRF: RS485响应
```

## 4. 配置同步架构

```mermaid
sequenceDiagram
    participant WebUI as Web界面
    participant Master as 主设备
    participant Slave as 从设备
    
    Note over WebUI, Slave: 配置同步流程
    
    WebUI->>Master: 更新配置
    Master->>Master: 保存配置
    Master->>Slave: 推送配置 (TCP 8889)
    Slave->>Slave: 应用配置
    Slave->>Master: 确认同步
    Master->>WebUI: 同步状态更新
```

## 5. 状态机架构

```mermaid
stateDiagram-v2
    [*] --> PowerOn: 上电启动
    
    PowerOn --> HotspotMode: 启动热点
    HotspotMode --> ConfigMode: 客户端连接
    HotspotMode --> NormalMode: 60秒超时
    
    ConfigMode --> ManagementMode: Web访问
    ManagementMode --> ConfigReload: 保存配置
    ConfigReload --> NormalMode: 重启服务
    
    NormalMode --> MasterMode: 主设备角色
    NormalMode --> SlaveMode: 从设备角色
    
    MasterMode --> Running: 启动服务
    SlaveMode --> Running: 连接主设备
    
    Running --> SyncMode: 配置同步
    SyncMode --> Running: 同步完成
    
    Running --> ErrorMode: 发生错误
    ErrorMode --> Running: 错误恢复
    ErrorMode --> ConfigMode: 严重错误
```

## 6. 测试架构

```mermaid
graph TD
    subgraph "测试框架"
        TestRunner[测试运行器]
        TestFramework[测试框架]
        SerialControl[串口控制]
    end
    
    subgraph "单元测试"
        NetworkTest[网络模块测试]
        RS485Test[RS485模块测试]
        ProtocolTest[协议模块测试]
        ConfigTest[配置同步测试]
        WebTest[Web界面测试]
    end
    
    subgraph "集成测试"
        IntegrationTest[集成测试]
        HardwareTest[硬件测试]
        PerformanceTest[性能测试]
    end
    
    TestRunner --> TestFramework
    TestFramework --> SerialControl
    
    TestRunner --> NetworkTest
    TestRunner --> RS485Test
    TestRunner --> ProtocolTest
    TestRunner --> ConfigTest
    TestRunner --> WebTest
    
    TestRunner --> IntegrationTest
    TestRunner --> HardwareTest
    TestRunner --> PerformanceTest
```

## 7. 内存布局

```mermaid
graph TD
    subgraph "ESP8266内存布局"
        Flash[Flash存储 4MB]
        RAM[RAM 80KB]
        SPIFFS[SPIFFS文件系统]
    end
    
    subgraph "Flash分区"
        Bootloader[引导程序]
        Firmware[固件程序]
        FileSystem[文件系统]
        OTA[OTA分区]
    end
    
    subgraph "RAM使用"
        Stack[程序栈]
        Heap[动态堆]
        Global[全局变量]
        Buffer[缓冲区]
    end
    
    Flash --> Bootloader
    Flash --> Firmware
    Flash --> FileSystem
    Flash --> OTA
    
    RAM --> Stack
    RAM --> Heap
    RAM --> Global
    RAM --> Buffer
    
    SPIFFS --> FileSystem
```

## 8. 网络协议栈

```mermaid
graph TD
    subgraph "应用层协议"
        HTTP[HTTP Web服务]
        ConfigSync[配置同步协议]
        DataTransfer[数据传输协议]
    end
    
    subgraph "传输层"
        TCP8888[TCP 8888<br>数据传输]
        TCP8889[TCP 8889<br>配置同步]
        TCP80[TCP 80<br>Web服务]
    end
    
    subgraph "网络层"
        IP[IP协议]
        DHCP[DHCP客户端]
        mDNS[mDNS服务]
    end
    
    subgraph "数据链路层"
        WiFi[WiFi 802.11]
        AP[AP热点模式]
        STA[STA客户端模式]
    end
    
    HTTP --> TCP80
    ConfigSync --> TCP8889
    DataTransfer --> TCP8888
    
    TCP80 --> IP
    TCP8888 --> IP
    TCP8889 --> IP
    
    IP --> DHCP
    IP --> mDNS
    
    DHCP --> WiFi
    mDNS --> WiFi
    
    WiFi --> AP
    WiFi --> STA
```

## 9. 错误处理架构

```mermaid
graph TD
    subgraph "错误检测"
        NetworkError[网络错误]
        RS485Error[RS485错误]
        ConfigError[配置错误]
        SystemError[系统错误]
    end
    
    subgraph "错误处理器"
        ErrorHandler[错误处理器]
        Logger[日志记录]
        Recovery[恢复机制]
    end
    
    subgraph "恢复策略"
        Retry[重试机制]
        Fallback[降级处理]
        Reset[系统重置]
        SafeMode[安全模式]
    end
    
    NetworkError --> ErrorHandler
    RS485Error --> ErrorHandler
    ConfigError --> ErrorHandler
    SystemError --> ErrorHandler
    
    ErrorHandler --> Logger
    ErrorHandler --> Recovery
    
    Recovery --> Retry
    Recovery --> Fallback
    Recovery --> Reset
    Recovery --> SafeMode
```

## 10. 部署架构

```mermaid
graph TD
    subgraph "开发环境"
        PlatformIO[PlatformIO]
        VSCode[VS Code]
        Git[Git版本控制]
    end
    
    subgraph "编译环境"
        MasterBuild[主设备编译]
        SlaveBuild[从设备编译]
        TestBuild[测试编译]
    end
    
    subgraph "部署目标"
        MasterDevice[主设备硬件]
        SlaveDevice[从设备硬件]
        TestDevice[测试设备]
    end
    
    PlatformIO --> MasterBuild
    PlatformIO --> SlaveBuild
    PlatformIO --> TestBuild
    
    MasterBuild --> MasterDevice
    SlaveBuild --> SlaveDevice
    TestBuild --> TestDevice
    
    VSCode --> PlatformIO
    Git --> PlatformIO
```

这个系统架构文档提供了WiFly485项目的完整技术视图，包括：

1. **整体架构**：展示了VRF控制器、主从设备、网络基础设施的关系
2. **软件模块**：详细的分层架构设计
3. **数据流**：RS485数据如何通过WiFi网络传输
4. **配置同步**：主从设备配置同步机制
5. **状态机**：设备启动和运行状态转换
6. **测试架构**：完整的测试框架设计
7. **内存布局**：ESP8266的内存使用规划
8. **网络协议**：完整的网络协议栈
9. **错误处理**：系统错误检测和恢复机制
10. **部署架构**：从开发到部署的完整流程

这些图表将帮助您在开发过程中更好地理解系统的各个组件如何协同工作。