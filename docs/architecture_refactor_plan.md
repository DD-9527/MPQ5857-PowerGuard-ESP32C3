# MPQ5857 项目架构重构方案

## 当前问题分析

### 1.1 耦合问题

- `web_server` 组件直接调用硬件相关函数（如 `mpq5857_set_output()`、`gpio_set_level()`）
- WebSocket定时器直接访问硬件数据，违反单一职责原则
- API处理函数包含业务逻辑，不利于测试和维护

### 1.2 架构缺陷

- 组件间直接函数调用，缺乏抽象层
- 数据流混乱，难以追踪和调试
- 扩展性差，添加新功能需要修改多个组件

## 2. 重构目标

### 2.1 解耦原则

- **单一职责**：每个组件只负责特定功能
- **依赖倒置**：依赖抽象而非具体实现
- **接口隔离**：定义清晰的组件边界
- **开闭原则**：对扩展开放，对修改关闭

### 2.2 性能要求

- 保持实时性（WebSocket 200ms更新频率）
- 最小化内存开销
- 支持异步操作

## 3. 建议架构设计

### 3.1 分层架构

```md
┌─────────────────────────────────────┐
│           前端界面层                 │
├─────────────────────────────────────┤
│           Web服务器层                │
├─────────────────────────────────────┤
│         应用控制器层                 │
├─────────────────────────────────────┤
│    数据管理器  │  保护控制器          │
├─────────────────────────────────────┤
│    ADC驱动    │  MPQ5857驱动         │
├─────────────────────────────────────┤
│         硬件抽象层                   │
└─────────────────────────────────────┘
```

### 3.2 组件职责定义

#### 3.2.1 应用控制器 (app_controller)

- 协调各组件工作
- 处理业务逻辑
- 管理系统状态
- 提供统一API接口

#### 3.2.2 数据管理器 (data_manager)

- 管理ADC数据采样
- 处理数据校准和转换
- 维护数据缓存
- 触发数据更新事件

#### 3.2.3 保护控制器 (protection_controller)

- 管理过压/欠压/过流保护
- 处理保护阈值配置
- 触发保护动作
- 记录保护事件

#### 3.2.4 Web服务器 (web_server)

- 处理HTTP请求
- 管理WebSocket连接
- 仅与app_controller交互
- 提供RESTful API

### 3.3 通信机制

#### 3.3.1 事件驱动架构

```c
typedef enum {
    EVENT_ADC_DATA_UPDATED,      // ADC数据更新
    EVENT_PROTECTION_TRIGGERED,  // 保护触发
    EVENT_OUTPUT_STATE_CHANGED,  // 输出状态变化
    EVENT_PROTECTION_CONFIG_CHANGED, // 保护配置变更
    EVENT_SYSTEM_ERROR          // 系统错误
} system_event_t;
```

#### 3.3.2 消息队列通信

- 使用FreeRTOS队列进行异步通信
- 定义标准消息格式
- 支持消息优先级

#### 3.3.3 回调机制

- 组件可注册事件回调
- 支持多监听器模式
- 线程安全的回调调用

## 4. 接口设计

### 4.1 应用控制器接口

```c
typedef struct {
    // 系统控制
    esp_err_t (*init)(void);
    esp_err_t (*start)(void);
    esp_err_t (*stop)(void);
    
    // 输出控制
    esp_err_t (*set_output)(bool enable);
    bool (*get_output_state)(void);
    
    // 保护配置
    esp_err_t (*set_protection)(uint8_t channel, uint8_t threshold, uint8_t action);
    esp_err_t (*get_protection_config)(uint8_t channel, protection_config_t *config);
    
    // 数据获取
    system_state_t (*get_system_state)(void);
    adc_reading_t (*get_adc_data)(void);
    
    // 事件注册
    esp_err_t (*register_event_handler)(system_event_t event, event_handler_t handler);
    esp_err_t (*unregister_event_handler)(system_event_t event, event_handler_t handler);
} app_controller_interface_t;
```

### 4.2 数据管理器接口

```c
typedef struct {
    // 初始化
    esp_err_t (*init)(void);
    
    // 数据获取
    adc_reading_t (*get_latest_data)(void);
    esp_err_t (*get_data_history)(adc_reading_t *buffer, size_t count);
    
    // 配置
    esp_err_t (*set_sample_rate)(uint32_t rate_hz);
    esp_err_t (*set_calibration)(adc_channel_t channel, float gain, float offset);
    
    // 事件
    esp_err_t (*register_data_callback)(void (*callback)(adc_reading_t *data));
} data_manager_interface_t;
```

### 4.3 保护控制器接口

```c
typedef struct {
    // 保护配置
    esp_err_t (*set_threshold)(uint8_t channel, float threshold);
    esp_err_t (*set_action)(uint8_t channel, protection_action_t action);
    esp_err_t (*enable_protection)(uint8_t channel, bool enable);
    
    // 状态查询
    bool (*is_protection_active)(uint8_t channel);
    protection_status_t (*get_protection_status)(uint8_t channel);
    
    // 事件
    esp_err_t (*register_protection_callback)(void (*callback)(uint8_t channel, bool triggered));
} protection_controller_interface_t;
```

## 5. 重构步骤

### 5.1 第一阶段：接口定义

1. 创建抽象接口头文件
2. 定义数据结构和枚举
3. 建立事件机制基础
4. 保持现有功能不变

### 5.2 第二阶段：组件重构

1. 实现app_controller组件
2. 重构data_manager组件
3. 重构protection_controller组件
4. 添加单元测试

### 5.3 第三阶段：Web服务器重构

1. 移除直接硬件调用
2. 使用app_controller接口
3. 添加请求验证
4. 优化错误处理

### 5.4 第四阶段：集成测试

1. 验证所有功能正常
2. 性能测试
3. 稳定性测试
4. 文档更新

## 6. 数据流设计

### 6.1 ADC数据流

```md
硬件ADC → ADC驱动 → 数据管理器 → 应用控制器 → Web服务器 → 前端
```

### 6.2 控制命令流

```md
前端 → Web服务器 → 应用控制器 → 保护控制器 → MPQ5857驱动 → 硬件
```

### 6.3 事件流

```md
硬件事件 → 驱动层 → 控制器层 → 事件总线 → 监听器 → 响应动作
```

## 7. 错误处理策略

### 7.1 分层错误处理

- 驱动层：硬件相关错误
- 控制器层：业务逻辑错误
- 应用层：用户操作错误

### 7.2 错误码定义

```c
typedef enum {
    ERR_APP_CONTROLLER_BASE = 0x1000,
    ERR_DATA_MANAGER_BASE = 0x2000,
    ERR_PROTECTION_BASE = 0x3000,
    ERR_WEB_SERVER_BASE = 0x4000,
} error_code_base_t;
```

### 7.3 错误恢复机制

- 自动重试策略
- 降级处理模式
- 用户通知机制

## 8. 性能优化

### 8.1 内存管理

- 使用静态分配减少碎片
- 实现内存池管理
- 避免动态分配在中断上下文

### 8.2 实时性保证

- 优先级队列管理
- 关键路径优化
- 异步处理非关键任务

### 8.3 资源监控

- CPU使用率监控
- 内存使用统计
- 任务堆栈监控

## 9. 测试策略

### 9.1 单元测试

- 为每个组件编写测试用例
- 使用mock对象模拟依赖
- 自动化测试执行

### 9.2 集成测试

- 验证组件间交互
- 测试边界条件
- 性能基准测试

### 9.3 系统测试

- 端到端功能验证
- 长时间稳定性测试
- 异常情况处理测试

## 10. 实施建议

### 10.1 渐进式重构

- 保持现有功能正常运行
- 逐步迁移到新架构
- 每个阶段都要充分测试

### 10.2 版本控制

- 使用feature branch进行重构
- 频繁提交小的变更
- 保持主分支稳定

### 10.3 文档维护

- 同步更新接口文档
- 记录设计决策
- 维护变更日志

这个重构方案将帮助你建立一个清晰、可维护、可扩展的架构，为后续的功能开发打下良好基础。
