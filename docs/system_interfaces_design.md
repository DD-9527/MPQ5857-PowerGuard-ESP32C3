# 系统接口设计文档

## 概述

本文档定义了MPQ5857项目中各组件的标准接口，用于解耦组件之间的直接依赖关系，提高系统的可维护性和可扩展性。

## 接口设计原则

### 1. 单一职责原则
每个接口只负责特定的功能领域，避免功能交叉。

### 2. 依赖倒置原则
高层组件依赖抽象接口，而非具体实现。

### 3. 接口隔离原则
接口应该小而专一，避免臃肿的万能接口。

### 4. 开闭原则
对扩展开放，对修改关闭。

## 核心接口定义

### 3.1 应用控制器接口 (app_controller_interface_t)

**职责**：系统核心业务逻辑协调器

```c
typedef struct {
    /* 生命周期管理 */
    esp_err_t (*init)(void);
    esp_err_t (*start)(void);
    esp_err_t (*stop)(void);
    bool (*is_initialized)(void);
    
    /* 输出控制 */
    esp_err_t (*set_output)(bool enable);
    bool (*get_output_state)(void);
    
    /* 保护配置 */
    esp_err_t (*set_protection_config)(const protection_config_t* config);
    esp_err_t (*get_protection_config)(protection_channel_t channel, protection_config_t* config);
    esp_err_t (*get_protection_status)(protection_channel_t channel, protection_status_t* status);
    
    /* 数据获取 */
    esp_err_t (*get_system_state)(system_state_t* state);
    esp_err_t (*get_adc_data)(adc_data_t* data);
    
    /* 事件管理 */
    esp_err_t (*register_event_handler)(system_event_type_t event, system_event_handler_t handler);
    esp_err_t (*unregister_event_handler)(system_event_type_t event, system_event_handler_t handler);
    
    /* 系统信息 */
    const char* (*get_version)(void);
    uint32_t (*get_uptime_ms)(void);
    
} app_controller_interface_t;
```

**设计说明**：
- 作为系统的主要入口点，协调各组件工作
- 提供统一的事件注册机制
- 管理系统的整体状态和生命周期
- 隐藏底层组件的复杂性

### 3.2 数据管理器接口 (data_manager_interface_t)

**职责**：ADC数据采样和管理

```c
typedef struct {
    /* 生命周期管理 */
    esp_err_t (*init)(void);
    esp_err_t (*start)(void);
    esp_err_t (*stop)(void);
    bool (*is_initialized)(void);
    
    /* 数据获取 */
    esp_err_t (*get_latest_data)(adc_data_t* data);
    esp_err_t (*get_data_history)(adc_data_t* buffer, size_t count, size_t* actual_count);
    
    /* 配置管理 */
    esp_err_t (*set_sample_rate)(uint32_t rate_hz);
    esp_err_t (*get_sample_rate)(uint32_t* rate_hz);
    esp_err_t (*set_calibration)(adc_channel_t channel, float gain, float offset);
    esp_err_t (*get_calibration)(adc_channel_t channel, float* gain, float* offset);
    
    /* 事件注册 */
    esp_err_t (*register_data_callback)(void (*callback)(const adc_data_t* data));
    esp_err_t (*unregister_data_callback)(void (*callback)(const adc_data_t* data));
    
} data_manager_interface_t;
```

**设计说明**：
- 负责ADC数据的采样、校准和缓存
- 支持数据历史记录功能
- 提供灵活的数据回调机制
- 支持动态采样率调整

### 3.3 保护控制器接口 (protection_controller_interface_t)

**职责**：过压/欠压/过流保护管理

```c
typedef struct {
    /* 生命周期管理 */
    esp_err_t (*init)(void);
    esp_err_t (*start)(void);
    esp_err_t (*stop)(void);
    bool (*is_initialized)(void);
    
    /* 保护配置 */
    esp_err_t (*set_config)(const protection_config_t* config);
    esp_err_t (*get_config)(protection_channel_t channel, protection_config_t* config);
    esp_err_t (*enable_channel)(protection_channel_t channel, bool enable);
    
    /* 状态查询 */
    bool (*is_triggered)(protection_channel_t channel);
    esp_err_t (*get_status)(protection_channel_t channel, protection_status_t* status);
    esp_err_t (*get_all_status)(protection_status_t* status_array, size_t max_count, size_t* actual_count);
    
    /* 手动复位 */
    esp_err_t (*reset_protection)(protection_channel_t channel);
    esp_err_t (*reset_all_protections)(void);
    
    /* 事件注册 */
    esp_err_t (*register_callback)(void (*callback)(protection_channel_t channel, bool triggered));
    esp_err_t (*unregister_callback)(void (*callback)(protection_channel_t channel, bool triggered));
    
} protection_controller_interface_t;
```

**设计说明**：
- 统一管理所有保护功能
- 支持动态配置和启用/禁用
- 提供详细的保护状态信息
- 支持手动复位功能

### 3.4 Web服务器接口 (web_server_interface_t)

**职责**：HTTP服务和WebSocket通信

```c
typedef struct {
    /* 生命周期管理 */
    esp_err_t (*init)(void);
    esp_err_t (*start)(void);
    esp_err_t (*stop)(void);
    bool (*is_initialized)(void);
    
    /* WebSocket通信 */
    esp_err_t (*send_ws_message)(const ws_message_t* message);
    esp_err_t (*broadcast_ws_message)(const ws_message_t* message);
    
    /* HTTP API */
    esp_err_t (*register_api_handler)(const char* path, httpd_method_t method, 
                                     esp_err_t (*handler)(httpd_req_t* req));
    esp_err_t (*unregister_api_handler)(const char* path, httpd_method_t method);
    
    /* 客户端管理 */
    size_t (*get_connected_client_count)(void);
    bool (*is_client_connected)(int client_fd);
    
} web_server_interface_t;
```

**设计说明**：
- 专注于HTTP请求处理和WebSocket通信
- 提供灵活的API注册机制
- 支持消息广播和点对点发送
- 提供客户端连接管理功能

## 数据结构设计

### 4.1 系统状态结构 (system_state_t)

```c
typedef struct {
    bool output_enabled;            // 输出状态
    float voltage;                  // 当前电压(V)
    float current;                  // 当前电流(A)
    float power;                    // 当前功率(W)
    uint8_t protection_status;      // 保护状态位图
    bool is_fault;                  // 是否有故障
    uint32_t uptime_ms;             // 系统运行时间(ms)
} system_state_t;
```

### 4.2 ADC数据结构 (adc_data_t)

```c
typedef struct {
    int raw_voltage;        // 原始电压ADC值
    int raw_current;        // 原始电流ADC值
    float voltage;          // 校准后的电压值(V)
    float current;          // 校准后的电流值(A)
    float power;            // 计算功率(W)
    uint32_t timestamp;     // 时间戳(ms)
} adc_data_t;
```

### 4.3 保护配置结构 (protection_config_t)

```c
typedef struct {
    protection_channel_t channel;       // 保护通道
    float threshold;                    // 保护阈值
    protection_action_t action;         // 保护动作
    bool enabled;                       // 是否启用
} protection_config_t;
```

## 事件系统设计

### 5.1 事件类型定义

```c
typedef enum {
    SYSTEM_EVENT_ADC_DATA_UPDATED,              // ADC数据更新
    SYSTEM_EVENT_OUTPUT_STATE_CHANGED,          // 输出状态变化
    SYSTEM_EVENT_PROTECTION_TRIGGERED,          // 保护触发
    SYSTEM_EVENT_PROTECTION_CONFIG_CHANGED,     // 保护配置变更
    SYSTEM_EVENT_SYSTEM_ERROR,                  // 系统错误
    SYSTEM_EVENT_SYSTEM_INITIALIZED,            // 系统初始化完成
    SYSTEM_EVENT_SYSTEM_SHUTDOWN,               // 系统关闭
} system_event_type_t;
```

### 5.2 事件数据结构

```c
typedef struct {
    system_event_type_t type;
    void* data;
    size_t data_size;
    uint32_t timestamp_ms;
} system_event_t;
```

### 5.3 事件处理机制

- 支持多监听器注册
- 线程安全的事件分发
- 优先级事件处理
- 异步事件处理

## 错误处理设计

### 6.1 错误码分类

```c
typedef enum {
    // 基础错误码
    ERR_SYSTEM_OK = ESP_OK,
    ERR_SYSTEM_FAIL = ESP_FAIL,
    ERR_SYSTEM_INVALID_ARG = ESP_ERR_INVALID_ARG,
    
    // 应用控制器错误码 (0x1000-0x1FFF)
    ERR_APP_CONTROLLER_BASE = 0x1000,
    
    // 数据管理器错误码 (0x2000-0x2FFF)
    ERR_DATA_MANAGER_BASE = 0x2000,
    
    // 保护控制器错误码 (0x3000-0x3FFF)
    ERR_PROTECTION_BASE = 0x3000,
    
    // Web服务器错误码 (0x4000-0x4FFF)
    ERR_WEB_SERVER_BASE = 0x4000,
} system_error_t;
```

### 6.2 错误处理策略

- 分层错误处理
- 错误恢复机制
- 错误日志记录
- 用户友好的错误提示

## 接口使用示例

### 7.1 初始化流程

```c
// 获取接口实例
const app_controller_interface_t* app_ctrl = app_controller_get_interface();

// 初始化系统
esp_err_t err = app_ctrl->init();
if (err != ESP_OK) {
    ESP_LOGE(TAG, "App controller init failed: %s", system_error_to_string(err));
    return err;
}

// 注册事件处理
app_ctrl->register_event_handler(SYSTEM_EVENT_ADC_DATA_UPDATED, adc_data_handler);
app_ctrl->register_event_handler(SYSTEM_EVENT_PROTECTION_TRIGGERED, protection_handler);

// 启动系统
err = app_ctrl->start();
```

### 7.2 控制输出

```c
const app_controller_interface_t* app_ctrl = app_controller_get_interface();

// 设置输出状态
esp_err_t err = app_ctrl->set_output(true);
if (err != ESP_OK) {
    ESP_LOGE(TAG, "Failed to set output: %s", system_error_to_string(err));
}

// 获取当前状态
bool is_enabled = app_ctrl->get_output_state();
```

### 7.3 配置保护

```c
const app_controller_interface_t* app_ctrl = app_controller_get_interface();

protection_config_t config = {
    .channel = PROTECTION_CHANNEL_OCP,
    .threshold = 5.0f,  // 5A
    .action = PROTECTION_ACTION_IMMEDIATE_OFF,
    .enabled = true
};

esp_err_t err = app_ctrl->set_protection_config(&config);
```

## 性能考虑

### 8.1 内存管理
- 优先使用静态分配
- 实现内存池管理
- 避免动态分配在中断上下文

### 8.2 实时性
- 关键路径优化
- 异步处理非关键任务
- 优先级队列管理

### 8.3 资源监控
- CPU使用率监控
- 内存使用统计
- 任务堆栈监控

## 扩展性设计

### 9.1 插件机制
- 支持动态加载新组件
- 标准化的插件接口
- 组件生命周期管理

### 9.2 配置管理
- 统一的配置系统
- 运行时配置更新
- 配置验证机制

### 9.3 多实例支持
- 支持多个数据源
- 支持多种保护模式
- 支持多种通信协议

这个接口设计为系统提供了清晰的架构边界，使得各组件可以独立开发、测试和维护，同时保持良好的扩展性和灵活性。