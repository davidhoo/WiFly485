// 全局变量
let deviceRole = 'unknown';

// 页面加载完成后执行
document.addEventListener('DOMContentLoaded', function() {
    // 初始化页面
    initializePage();
    
    // 绑定事件监听器
    bindEventListeners();
});

// 初始化页面
function initializePage() {
    // 加载设备状态
    loadDeviceStatus();
    
    // 如果在配置页面，加载配置
    if (window.location.pathname.includes('config')) {
        loadConfig();
    }
}

// 绑定事件监听器
function bindEventListeners() {
    // 主页面按钮事件
    const configBtn = document.getElementById('config-btn');
    if (configBtn) {
        configBtn.addEventListener('click', function() {
            window.location.href = '/config';
        });
    }
    
    const rebootBtn = document.getElementById('reboot-btn');
    if (rebootBtn) {
        rebootBtn.addEventListener('click', function() {
            if (confirm('确定要重启设备吗？')) {
                rebootDevice();
            }
        });
    }
    
    // 配置页面事件
    const dhcpCheckbox = document.getElementById('dhcp-enabled');
    if (dhcpCheckbox) {
        dhcpCheckbox.addEventListener('change', function() {
            toggleStaticIPFields();
        });
    }
    
    const saveConfigBtn = document.getElementById('save-config-btn');
    if (saveConfigBtn) {
        saveConfigBtn.addEventListener('click', function() {
            saveConfig();
        });
    }
    
    const resetConfigBtn = document.getElementById('reset-config-btn');
    if (resetConfigBtn) {
        resetConfigBtn.addEventListener('click', function() {
            if (confirm('确定要重置配置吗？这将恢复为默认配置。')) {
                resetConfig();
            }
        });
    }
}

// 加载设备状态
function loadDeviceStatus() {
    fetch('/api/status')
        .then(response => response.json())
        .then(data => {
            document.getElementById('device-role').textContent = data.device_role;
            document.getElementById('device-name').textContent = data.device_name;
            document.getElementById('wifi-status').textContent = data.wifi_status;
            document.getElementById('ip-address').textContent = data.ip_address;
            
            // 格式化运行时间
            const uptime = formatUptime(data.uptime);
            document.getElementById('uptime').textContent = uptime;
            
            // 保存设备角色
            deviceRole = data.device_role;
            
            // 根据设备角色调整界面
            adjustUIForDeviceRole();
        })
        .catch(error => {
            console.error('加载设备状态失败:', error);
            showMessage('加载设备状态失败', 'error');
        });
}

// 格式化运行时间
function formatUptime(milliseconds) {
    const seconds = Math.floor(milliseconds / 1000);
    const minutes = Math.floor(seconds / 60);
    const hours = Math.floor(minutes / 60);
    const days = Math.floor(hours / 24);
    
    if (days > 0) {
        return `${days}天 ${hours % 24}小时 ${minutes % 60}分钟`;
    } else if (hours > 0) {
        return `${hours}小时 ${minutes % 60}分钟`;
    } else if (minutes > 0) {
        return `${minutes}分钟 ${seconds % 60}秒`;
    } else {
        return `${seconds}秒`;
    }
}

// 根据设备角色调整界面
function adjustUIForDeviceRole() {
    // 如果是从设备，隐藏某些配置选项
    if (deviceRole === 'slave') {
        const deviceConfigSection = document.getElementById('device-config-section');
        if (deviceConfigSection) {
            deviceConfigSection.style.display = 'none';
        }
        
        const rs485ConfigSection = document.getElementById('rs485-config-section');
        if (rs485ConfigSection) {
            rs485ConfigSection.style.display = 'none';
        }
    }
}

// 切换静态IP字段显示
function toggleStaticIPFields() {
    const dhcpEnabled = document.getElementById('dhcp-enabled').checked;
    const staticIPGroup = document.getElementById('static-ip-group');
    const gatewayGroup = document.getElementById('gateway-group');
    const subnetGroup = document.getElementById('subnet-group');
    
    if (dhcpEnabled) {
        staticIPGroup.classList.add('hidden');
        gatewayGroup.classList.add('hidden');
        subnetGroup.classList.add('hidden');
    } else {
        staticIPGroup.classList.remove('hidden');
        gatewayGroup.classList.remove('hidden');
        subnetGroup.classList.remove('hidden');
    }
}

// 加载配置
function loadConfig() {
    fetch('/api/config')
        .then(response => response.json())
        .then(data => {
            // 填充网络配置
            document.getElementById('ssid').value = data.network.ssid || '';
            document.getElementById('password').value = data.network.password || '';
            document.getElementById('dhcp-enabled').checked = data.network.dhcpEnabled || false;
            document.getElementById('ip-address').value = data.network.ip || '';
            document.getElementById('gateway').value = data.network.gateway || '';
            document.getElementById('subnet').value = data.network.subnet || '';
            
            // 填充RS485配置
            document.getElementById('baud-rate').value = data.rs485.baudRate || 9600;
            document.getElementById('data-bits').value = data.rs485.dataBits || 8;
            document.getElementById('parity').value = data.rs485.parity || 0;
            document.getElementById('stop-bits').value = data.rs485.stopBits || 1;
            
            // 填充设备配置
            document.getElementById('device-name').value = data.device.name || '';
            document.getElementById('device-role').value = data.device.role || 'slave';
            document.getElementById('tcp-port').value = data.device.tcpPort || 8888;
            document.getElementById('sync-port').value = data.device.syncPort || 8889;
            
            // 切换静态IP字段显示
            toggleStaticIPFields();
        })
        .catch(error => {
            console.error('加载配置失败:', error);
            showMessage('加载配置失败', 'error');
        });
}

// 保存配置
function saveConfig() {
    // 构建配置对象
    const config = {
        network: {
            ssid: document.getElementById('ssid').value,
            password: document.getElementById('password').value,
            dhcpEnabled: document.getElementById('dhcp-enabled').checked,
            ip: document.getElementById('ip-address').value,
            gateway: document.getElementById('gateway').value,
            subnet: document.getElementById('subnet').value
        },
        rs485: {
            baudRate: parseInt(document.getElementById('baud-rate').value),
            dataBits: parseInt(document.getElementById('data-bits').value),
            parity: parseInt(document.getElementById('parity').value),
            stopBits: parseInt(document.getElementById('stop-bits').value)
        },
        device: {
            name: document.getElementById('device-name').value,
            role: document.getElementById('device-role').value,
            tcpPort: parseInt(document.getElementById('tcp-port').value),
            syncPort: parseInt(document.getElementById('sync-port').value)
        }
    };
    
    // 发送配置到服务器
    fetch('/api/config', {
        method: 'POST',
        headers: {
            'Content-Type': 'application/json'
        },
        body: JSON.stringify(config)
    })
    .then(response => response.json())
    .then(data => {
        if (data.status === 'success') {
            showMessage('配置保存成功', 'success');
        } else {
            showMessage('配置保存失败: ' + data.message, 'error');
        }
    })
    .catch(error => {
        console.error('保存配置失败:', error);
        showMessage('保存配置失败', 'error');
    });
}

// 重置配置
function resetConfig() {
    // 这里应该发送请求到服务器重置配置
    // 暂时显示提示信息
    showMessage('配置重置功能尚未实现', 'info');
}

// 重启设备
function rebootDevice() {
    fetch('/reboot', {
        method: 'POST'
    })
    .then(response => response.json())
    .then(data => {
        showMessage('设备正在重启...', 'info');
    })
    .catch(error => {
        console.error('重启设备失败:', error);
        showMessage('重启设备失败', 'error');
    });
}

// 显示消息
function showMessage(message, type) {
    // 创建消息元素
    const messageElement = document.createElement('div');
    messageElement.className = `message message-${type}`;
    messageElement.textContent = message;
    
    // 添加样式
    messageElement.style.position = 'fixed';
    messageElement.style.top = '20px';
    messageElement.style.right = '20px';
    messageElement.style.padding = '15px';
    messageElement.style.borderRadius = '4px';
    messageElement.style.color = 'white';
    messageElement.style.fontWeight = 'bold';
    messageElement.style.zIndex = '1000';
    messageElement.style.boxShadow = '0 2px 10px rgba(0,0,0,0.2)';
    
    // 根据消息类型设置背景色
    switch (type) {
        case 'success':
            messageElement.style.backgroundColor = '#27ae60';
            break;
        case 'error':
            messageElement.style.backgroundColor = '#e74c3c';
            break;
        case 'info':
            messageElement.style.backgroundColor = '#3498db';
            break;
        default:
            messageElement.style.backgroundColor = '#95a5a6';
    }
    
    // 添加到页面
    document.body.appendChild(messageElement);
    
    // 3秒后移除消息
    setTimeout(() => {
        if (messageElement.parentNode) {
            messageElement.parentNode.removeChild(messageElement);
        }
    }, 3000);
}