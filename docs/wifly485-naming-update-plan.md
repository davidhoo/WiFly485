# WiFly485 命名统一更新计划

## 任务概述
将项目中所有产品相关名称从"RS485"统一改为"WiFly485"，包括热点名称、服务发现名称、设备名称等。

## 已完成修改
- ✅ 需求文档.md：标题、设备名称、热点名称、mDNS名称、Web界面标题
- ✅ README.md：WiFi热点名称

## 待修改文件清单

### 1. platformio.ini
需要添加主从设备配置，使用WiFly485命名：

```ini
[env:esp12e_master]
platform = espressif8266
board = esp12e
framework = arduino
lib_deps = 
    ESP8266mDNS
    ESP8266WebServer
build_flags =
    -DDEVICE_ROLE_MASTER
    -DDEVICE_NAME="WiFly485_Master"

[env:esp12e_slave]
platform = espressif8266
board = esp12e
framework = arduino
lib_deps = 
    ESP8266mDNS
    ESP8266WebServer
build_flags =
    -DDEVICE_ROLE_SLAVE
    -DDEVICE_NAME="WiFly485_Slave"
```

### 2. 源代码文件（待创建）
当实际代码开发时，需要在以下位置使用WiFly485命名：

- 热点名称模板：`WiFly485_Master_XXXX` 和 `WiFly485_Slave_XXXX`
- mDNS服务名称：`wifly485-master-XXXX.local` 和 `wifly485-slave-XXXX.local`
- 默认设备名称：`WiFly485_Master` 和 `WiFly485_Slave`
- Web界面标题：使用"WiFly485"而非"RS485"

### 3. 配置文件默认值
当创建配置文件时，确保使用：
- device_name: "WiFly485_Master" | "WiFly485_Slave"
- hostname: "wifly485-master-XXXX" | "wifly485-slave-XXXX"

## 命名规范总结
- **产品名称**: WiFly485
- **热点前缀**: WiFly485_Master_XXXX, WiFly485_Slave_XXXX
- **mDNS域名**: wifly485-master-XXXX.local, wifly485-slave-XXXX.local
- **设备名称**: WiFly485_Master, WiFly485_Slave
- **Web界面标题**: WiFly485主设备管理, WiFly485从设备管理

## 下一步操作
需要切换到Code模式来完成platformio.ini和源代码文件的修改。