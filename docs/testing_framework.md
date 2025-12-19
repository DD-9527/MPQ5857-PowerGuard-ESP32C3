# MPQ5857 测试框架与验证计划

## 概述

本文档定义了重构后的MPQ5857项目的完整测试框架，包括单元测试、集成测试和系统测试，确保重构过程中功能保持完整。

## 测试策略

### 1.1 测试层次
- **单元测试**：验证单个组件的功能
- **集成测试**：验证组件间的交互
- **系统测试**：验证整个系统的端到端功能
- **性能测试**：验证系统性能指标

### 1.2 测试原则
- **自动化优先**：尽可能自动化测试执行
- **持续集成**：每次代码变更都运行测试
- **覆盖率目标**：代码覆盖率达到80%以上
- **缺陷预防**：测试驱动开发，预防缺陷产生

## 单元测试框架

### 2.1 测试环境搭建

**测试目录结构**：
```
tests/
├── unit/                    # 单元测试
│   ├── test_app_controller/
│   ├── test_data_manager/
│   ├── test_protection_controller/
│   └── test_web_server/
├── integration/             # 集成测试
│   ├── test_component_interaction/
│   └── test_event_system/
├── system/                  # 系统测试
│   ├── test_end_to_end/
│   └── test_performance/
├── mocks/                   # 模拟对象
│   ├── mock_adc/
│   ├── mock_mpq5857/
│   └── mock_wifi/
└── utils/                   # 测试工具
    ├── test_helpers.h
    └── test_fixtures.h
```

### 2.2 单元测试组件

#### 2.2.1 应用控制器测试

**tests/unit/test_app_controller/test_app_controller.c**：
```c
#include "unity.h"
#include "app_controller.h"
#include "system_interfaces.h"
#include "../mocks/mock_data_manager.h"
#include "../mocks/mock_protection_controller.h"

static const app_controller_interface_t* app_ctrl = NULL;
static bool event_received = false;
static system_event_t last_event = {0};

void setUp(void) {
    // 重置模拟对象
    mock_data_manager_reset();
    mock_protection_controller_reset();
    
    // 获取应用控制器接口
    app_ctrl = app_controller_get_interface();
    TEST_ASSERT_NOT_NULL(app_ctrl);
    
    // 初始化应用控制器
    TEST_ASSERT_EQUAL(ESP_OK, app_ctrl->init());
    
    event_received = false;
    memset(&last_event, 0, sizeof(last_event));
}

void tearDown(void) {
    // 清理资源
    if (app_ctrl && app_ctrl->is_initialized()) {
        app_ctrl->stop();
    }
}

// 测试初始化功能
void test_app_controller_init_should_succeed(void) {
    TEST_ASSERT_TRUE(app_ctrl->is_initialized());
}

// 测试输出控制功能
void test_set_output_should_update_state(void) {
    // 测试开启输出
    TEST_ASSERT_EQUAL(ESP_OK, app_ctrl->set_output(true));
    TEST_ASSERT_TRUE(app_ctrl->get_output_state());
    
    // 测试关闭输出
    TEST_ASSERT_EQUAL(ESP_OK, app_ctrl->set_output(false));
    TEST_ASSERT_FALSE(app_ctrl->get_output_state());
}

// 测试事件系统
void test_event_handler_should_receive_events(void) {
    // 注册事件处理函数
    TEST_ASSERT_EQUAL(ESP_OK, 
        app_ctrl->register_event_handler(SYSTEM_EVENT_OUTPUT_STATE_CHANGED, 
                                        test_event_handler));
    
    // 触发事件
    TEST_ASSERT_EQUAL(ESP_OK, app_ctrl->set_output(true));
    
    // 验证事件接收
    TEST_ASSERT_TRUE(event_received);
    TEST_ASSERT_EQUAL(SYSTEM_EVENT_OUTPUT_STATE_CHANGED, last_event.type);
}

// 事件处理测试函数
void test_event_handler(const system_event_t* event) {
    event_received = true;
    last_event = *event;
}

// 测试保护配置
void test_protection_config_should_be_stored(void) {
    protection_config_t config = {
        .channel = PROTECTION_CHANNEL_OCP,
        .threshold = 5.0f,
        .action = PROTECTION_ACTION_IMMEDIATE_OFF,
        .enabled = true
    };
    
    // 设置保护配置
    TEST_ASSERT_EQUAL(ESP_OK, app_ctrl->set_protection_config(&config));
    
    // 验证配置已存储
    protection_config_t retrieved_config = {0};
    TEST_ASSERT_EQUAL(ESP_OK, 
        app_ctrl->get_protection_config(PROTECTION_CHANNEL_OCP, &retrieved_config));
    
    TEST_ASSERT_EQUAL(config.channel, retrieved_config.channel);
    TEST_ASSERT_EQUAL(config.threshold, retrieved_config.threshold);
    TEST_ASSERT_EQUAL(config.action, retrieved_config.action);
    TEST_ASSERT_EQUAL(config.enabled, retrieved_config.enabled);
}

// 测试系统状态获取
void test_get_system_state_should_return_valid_data(void) {
    system_state_t state = {0};
    
    // 设置一些状态
    app_ctrl->set_output(true);
    
    // 获取系统状态
    TEST_ASSERT_EQUAL(ESP_OK, app_ctrl->get_system_state(&state));
    
    // 验证状态数据
    TEST_ASSERT_TRUE(state.output_enabled);
    TEST_ASSERT_GREATER_OR_EQUAL(0, state.voltage);
    TEST_ASSERT_GREATER_OR_EQUAL(0, state.current);
    TEST_ASSERT_GREATER_OR_EQUAL(0, state.power);
}

// 测试错误处理
void test_invalid_arguments_should_return_error(void) {
    // 测试空指针
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, app_ctrl->get_system_state(NULL));
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, app_ctrl->set_protection_config(NULL));
    
    // 测试无效事件类型
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, 
        app_ctrl->register_event_handler(SYSTEM_EVENT_MAX, test_event_handler));
}

// 测试重复初始化
void test_double_init_should_succeed(void) {
    TEST_ASSERT_EQUAL(ESP_OK, app_ctrl->init());
    TEST_ASSERT_EQUAL(ESP_OK, app_ctrl->init()); // 第二次初始化应该成功
}

// 测试未初始化调用
void test_uninitialized_calls_should_fail(void) {
    // 创建新的未初始化的实例进行测试
    app_controller_interface_t* test_ctrl = app_controller_get_interface();
    
    // 停止当前实例
    if (app_ctrl->is_initialized()) {
        app_ctrl->stop();
    }
    
    // 未初始化时调用应该失败
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_STATE, test_ctrl->start());
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_STATE, test_ctrl->set_output(true));
}
```

#### 2.2.2 数据管理器测试

**tests/unit/test_data_manager/test_data_manager.c**：
```c
#include "unity.h"
#include "data_manager.h"
#include "system_interfaces.h"
#include "../mocks/mock_adc.h"

static const data_manager_interface_t* data_mgr = NULL;
static adc_data_t last_callback_data = {0};
static bool callback_received = false;

void setUp(void) {
    mock_adc_reset();
    data_mgr = data_manager_get_interface();
    TEST_ASSERT_NOT_NULL(data_mgr);
    
    TEST_ASSERT_EQUAL(ESP_OK, data_mgr->init());
    callback_received = false;
    memset(&last_callback_data, 0, sizeof(last_callback_data));
}

void tearDown(void) {
    if (data_mgr->is_initialized()) {
        data_mgr->stop();
    }
}

void on_data_callback(const adc_data_t* data) {
    callback_received = true;
    last_callback_data = *data;
}

void test_data_manager_init_should_succeed(void) {
    TEST_ASSERT_TRUE(data_mgr->is_initialized());
}

void test_get_latest_data_should_return_valid_adc_data(void) {
    adc_data_t data = {0};
    
    // 设置模拟ADC数据
    mock_adc_set_reading(3.3f, 1.5f); // 3.3V, 1.5A
    
    // 获取最新数据
    TEST_ASSERT_EQUAL(ESP_OK, data_mgr->get_latest_data(&data));
    
    // 验证数据
    TEST_ASSERT_EQUAL(3.3f, data.voltage);
    TEST_ASSERT_EQUAL(1.5f, data.current);
    TEST_ASSERT_EQUAL(4.95f, data.power); // 3.3V * 1.5A
    TEST_ASSERT_GREATER_THAN(0, data.timestamp);
}

void test_data_callback_should_be_called(void) {
    // 注册数据回调
    TEST_ASSERT_EQUAL(ESP_OK, data_mgr->register_data_callback(on_data_callback));
    
    // 启动数据管理器
    TEST_ASSERT_EQUAL(ESP_OK, data_mgr->start());
    
    // 等待一段时间让采样任务运行
    vTaskDelay(pdMS_TO_TICKS(200));
    
    // 验证回调被调用
    TEST_ASSERT_TRUE(callback_received);
    TEST_ASSERT_GREATER_THAN(0, last_callback_data.voltage);
    TEST_ASSERT_GREATER_THAN(0, last_callback_data.current);
}

void test_sample_rate_configuration(void) {
    uint32_t test_rate = 50; // 50Hz
    
    // 设置采样率
    TEST_ASSERT_EQUAL(ESP_OK, data_mgr->set_sample_rate(test_rate));
    
    // 验证采样率
    uint32_t retrieved_rate = 0;
    TEST_ASSERT_EQUAL(ESP_OK, data_mgr->get_sample_rate(&retrieved_rate));
    TEST_ASSERT_EQUAL(test_rate, retrieved_rate);
}

void test_calibration_configuration(void) {
    float gain = 1.1f;
    float offset = -0.05f;
    
    // 设置校准参数
    TEST_ASSERT_EQUAL(ESP_OK, data_mgr->set_calibration(ADC_CHANNEL_0, gain, offset));
    
    // 验证校准参数
    float retrieved_gain = 0, retrieved_offset = 0;
    TEST_ASSERT_EQUAL(ESP_OK, 
        data_mgr->get_calibration(ADC_CHANNEL_0, &retrieved_gain, &retrieved_offset));
    
    TEST_ASSERT_EQUAL(gain, retrieved_gain);
    TEST_ASSERT_EQUAL(offset, retrieved_offset);
}

void test_data_history_should_store_multiple_readings(void) {
    // 启动数据管理器
    TEST_ASSERT_EQUAL(ESP_OK, data_mgr->start());
    
    // 等待多个采样周期
    vTaskDelay(pdMS_TO_TICKS(500));
    
    // 获取历史数据
    adc_data_t history[10];
    size_t actual_count = 0;
    
    TEST_ASSERT_EQUAL(ESP_OK, 
        data_mgr->get_data_history(history, 10, &actual_count));
    
    // 验证获取到多个数据点
    TEST_ASSERT_GREATER_THAN(1, actual_count);
    TEST_ASSERT_LESS_OR_EQUAL(10, actual_count);
    
    // 验证数据有效性
    for (size_t i = 0; i < actual_count; i++) {
        TEST_ASSERT_GREATER_OR_EQUAL(0, history[i].voltage);
        TEST_ASSERT_GREATER_OR_EQUAL(0, history[i].current);
        TEST_ASSERT_GREATER_THAN(0, history[i].timestamp);
    }
}
```

### 2.3 模拟对象实现

#### 2.3.1 ADC模拟

**tests/mocks/mock_adc/mock_adc.c**：
```c
#include "mock_adc.h"
#include "adc.h"

static adc_reading_t g_mock_reading = {0};
static bool g_adc_initialized = false;

void mock_adc_reset(void) {
    memset(&g_mock_reading, 0, sizeof(g_mock_reading));
    g_adc_initialized = false;
}

void mock_adc_set_reading(float voltage, float current) {
    g_mock_reading.voltage = voltage;
    g_mock_reading.current = current;
    g_mock_reading.raw_vsns = (int)(voltage * 1000); // 模拟原始值
    g_mock_reading.raw_isns = (int)(current * 1000); // 模拟原始值
}

// 重载原始的adc_read_once函数
adc_reading_t adc_read_once(void) {
    return g_mock_reading;
}

// 重载原始的adc_init函数
void adc_init(void) {
    g_adc_initialized = true;
}
```

## 集成测试

### 3.1 组件交互测试

**tests/integration/test_component_interaction/test_component_interaction.c**：
```c
#include "unity.h"
#include "app_controller.h"
#include "data_manager.h"
#include "protection_controller.h"
#include "system_interfaces.h"

void setUp(void) {
    // 初始化所有组件
    TEST_ASSERT_EQUAL(ESP_OK, data_manager_init());
    TEST_ASSERT_EQUAL(ESP_OK, protection_controller_init());
    TEST_ASSERT_EQUAL(ESP_OK, app_controller_init());
}

void tearDown(void) {
    // 清理资源
}

void test_adc_data_should_trigger_protection(void) {
    const app_controller_interface_t* app_ctrl = app_controller_get_interface();
    const protection_controller_interface_t* prot_ctrl = protection_controller_get_interface();
    
    // 配置过流保护：阈值2.0A
    protection_config_t config = {
        .channel = PROTECTION_CHANNEL_OCP,
        .threshold = 2.0f,
        .action = PROTECTION_ACTION_IMMEDIATE_OFF,
        .enabled = true
    };
    
    TEST_ASSERT_EQUAL(ESP_OK, app_ctrl->set_protection_config(&config));
    
    // 模拟过流情况
    // 这里需要协调数据管理器和保护控制器
    
    // 验证保护被触发
    TEST_ASSERT_TRUE(prot_ctrl->is_triggered(PROTECTION_CHANNEL_OCP));
}

void test_event_system_should_connect_components(void) {
    const app_controller_interface_t* app_ctrl = app_controller_get_interface();
    
    bool event_received = false;
    system_event_handler_t test_handler = (system_event_handler_t)&event_received;
    
    // 注册事件处理
    TEST_ASSERT_EQUAL(ESP_OK, 
        app_ctrl->register_event_handler(SYSTEM_EVENT_ADC_DATA_UPDATED, test_handler));
    
    // 启动系统
    TEST_ASSERT_EQUAL(ESP_OK, app_ctrl->start());
    
    // 等待事件
    vTaskDelay(pdMS_TO_TICKS(200));
    
    // 验证事件系统工作
    // 这里需要模拟数据更新事件
}
```

### 3.2 Web服务器集成测试

**tests/integration/test_web_server_integration/test_web_server_integration.c**：
```c
#include "unity.h"
#include "http_server.h"
#include "system_interfaces.h"
#include "esp_http_client.h"

static const char* TEST_SERVER_URL = "http://localhost:80";

void setUp(void) {
    // 初始化系统组件
    app_controller_init();
    app_controller_start();
    
    // 启动Web服务器
    start_webserver();
}

void tearDown(void) {
    // 停止Web服务器
    // 清理系统组件
}

void test_root_endpoint_should_return_index_html(void) {
    esp_http_client_config_t config = {
        .url = TEST_SERVER_URL,
        .method = HTTP_METHOD_GET
    };
    
    esp_http_client_handle_t client = esp_http_client_init(&config);
    TEST_ASSERT_NOT_NULL(client);
    
    // 执行请求
    esp_err_t err = esp_http_client_perform(client);
    TEST_ASSERT_EQUAL(ESP_OK, err);
    
    // 验证响应
    int status_code = esp_http_client_get_status_code(client);
    TEST_ASSERT_EQUAL(200, status_code);
    
    // 验证内容类型
    char content_type[64] = {0};
    esp_http_client_get_header(client, "Content-Type", content_type, sizeof(content_type));
    TEST_ASSERT_NOT_NULL(strstr(content_type, "text/html"));
    
    esp_http_client_cleanup(client);
}

void test_api_output_should_control_system(void) {
    // 测试关闭输出
    const char* post_data = "0";
    
    esp_http_client_config_t config = {
        .url = TEST_SERVER_URL "/api/output",
        .method = HTTP_METHOD_POST,
        .post_data = post_data,
        .post_len = strlen(post_data)
    };
    
    esp_http_client_handle_t client = esp_http_client_init(&config);
    TEST_ASSERT_NOT_NULL(client);
    
    esp_err_t err = esp_http_client_perform(client);
    TEST_ASSERT_EQUAL(ESP_OK, err);
    
    int status_code = esp_http_client_get_status_code(client);
    TEST_ASSERT_EQUAL(200, status_code);
    
    // 验证系统状态
    const app_controller_interface_t* app_ctrl = app_controller_get_interface();
    TEST_ASSERT_FALSE(app_ctrl->get_output_state());
    
    esp_http_client_cleanup(client);
}

void test_websocket_should_broadcast_data(void) {
    // 这里需要实现WebSocket客户端测试
    // 验证WebSocket连接和数据接收
}
```

## 系统测试

### 4.1 端到端功能测试

**tests/system/test_end_to_end/test_end_to_end.c**：
```c
#include "unity.h"
#include "system_interfaces.h"
#include "esp_log.h"

static const char *TAG = "system_test";

void setUp(void) {
    ESP_LOGI(TAG, "Setting up system test...");
    
    // 初始化整个系统
    const app_controller_interface_t* app_ctrl = app_controller_get_interface();
    
    TEST_ASSERT_EQUAL(ESP_OK, app_ctrl->init());
    TEST_ASSERT_EQUAL(ESP_OK, app_ctrl->start());
    
    ESP_LOGI(TAG, "System initialized and started");
}

void tearDown(void) {
    ESP_LOGI(TAG, "Tearing down system test...");
    
    const app_controller_interface_t* app_ctrl = app_controller_get_interface();
    if (app_ctrl->is_initialized()) {
        app_ctrl->stop();
    }
}

void test_complete_system_workflow(void) {
    const app_controller_interface_t* app_ctrl = app_controller_get_interface();
    
    ESP_LOGI(TAG, "Starting complete system workflow test...");
    
    // 步骤1: 验证初始状态
    system_state_t initial_state = {0};
    TEST_ASSERT_EQUAL(ESP_OK, app_ctrl->get_system_state(&initial_state));
    TEST_ASSERT_FALSE(initial_state.output_enabled);
    TEST_ASSERT_FALSE(initial_state.is_fault);
    
    // 步骤2: 配置保护参数
    protection_config_t ocp_config = {
        .channel = PROTECTION_CHANNEL_OCP,
        .threshold = 3.0f,  // 3A
        .action = PROTECTION_ACTION_IMMEDIATE_OFF,
        .enabled = true
    };
    
    TEST_ASSERT_EQUAL(ESP_OK, app_ctrl->set_protection_config(&ocp_config));
    
    protection_config_t ovp_config = {
        .channel = PROTECTION_CHANNEL_OVP,
        .threshold = 15.0f,  // 15V
        .action = PROTECTION_ACTION_IMMEDIATE_OFF,
        .enabled = true
    };
    
    TEST_ASSERT_EQUAL(ESP_OK, app_ctrl->set_protection_config(&ovp_config));
    
    ESP_LOGI(TAG, "Protection configured");
    
    // 步骤3: 开启输出
    TEST_ASSERT_EQUAL(ESP_OK, app_ctrl->set_output(true));
    TEST_ASSERT_TRUE(app_ctrl->get_output_state());
    
    // 等待系统稳定
    vTaskDelay(pdMS_TO_TICKS(1000));
    
    // 步骤4: 验证运行状态
    system_state_t running_state = {0};
    TEST_ASSERT_EQUAL(ESP_OK, app_ctrl->get_system_state(&running_state));
    TEST_ASSERT_TRUE(running_state.output_enabled);
    
    ESP_LOGI(TAG, "System running - V:%.2fV, I:%.2fA, P:%.2fW", 
             running_state.voltage, running_state.current, running_state.power);
    
    // 步骤5: 模拟保护触发（需要模拟硬件条件）
    // 这里需要特殊的测试模式来模拟过流/过压
    
    // 步骤6: 关闭输出
    TEST_ASSERT_EQUAL(ESP_OK, app_ctrl->set_output(false));
    TEST_ASSERT_FALSE(app_ctrl->get_output_state());
    
    // 步骤7: 验证最终状态
    system_state_t final_state = {0};
    TEST_ASSERT_EQUAL(ESP_OK, app_ctrl->get_system_state(&final_state));
    TEST_ASSERT_FALSE(final_state.output_enabled);
    
    ESP_LOGI(TAG, "Complete system workflow test passed");
}

void test_system_recovery_from_faults(void) {
    const app_controller_interface_t* app_ctrl = app_controller_get_interface();
    const protection_controller_interface_t* prot_ctrl = protection_controller_get_interface();
    
    ESP_LOGI(TAG, "Testing system recovery from faults...");
    
    // 配置保护
    protection_config_t config = {
        .channel = PROTECTION_CHANNEL_OCP,
        .threshold = 1.0f,  // 1A (容易触发)
        .action = PROTECTION_ACTION_IMMEDIATE_OFF,
        .enabled = true
    };
    
    TEST_ASSERT_EQUAL(ESP_OK, app_ctrl->set_protection_config(&config));
    
    // 开启输出
    TEST_ASSERT_EQUAL(ESP_OK, app_ctrl->set_output(true));
    
    // 等待保护触发（需要模拟条件）
    vTaskDelay(pdMS_TO_TICKS(2000));
    
    // 验证保护状态
    TEST_ASSERT_TRUE(prot_ctrl->is_triggered(PROTECTION_CHANNEL_OCP));
    
    // 复位保护
    TEST_ASSERT_EQUAL(ESP_OK, prot_ctrl->reset_protection(PROTECTION_CHANNEL_OCP));
    TEST_ASSERT_FALSE(prot_ctrl->is_triggered(PROTECTION_CHANNEL_OCP));
    
    ESP_LOGI(TAG, "System recovery test passed");
}

void test_long_term_stability(void) {
    const app_controller_interface_t* app_ctrl = app_controller_get_interface();
    
    ESP_LOGI(TAG, "Testing long term stability (30 seconds)...");
    
    // 运行系统一段时间
    for (int i = 0; i < 30; i++) {
        system_state_t state = {0};
        TEST_ASSERT_EQUAL(ESP_OK, app_ctrl->get_system_state(&state));
        
        ESP_LOGI(TAG, "Second %d - V:%.2fV, I:%.2fA, P:%.2fW, Output:%s", 
                 i+1, state.voltage, state.current, state.power,
                 state.output_enabled ? "ON" : "OFF");
        
        // 随机切换输出状态
        if (i % 5 == 0) {
            bool new_state = !state.output_enabled;
            TEST_ASSERT_EQUAL(ESP_OK, app_ctrl->set_output(new_state));
        }
        
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
    
    ESP_LOGI(TAG, "Long term stability test passed");
}
```

## 性能测试

### 5.1 性能基准测试

**tests/system/test_performance/test_performance.c**：
```c
#include "unity.h"
#include "system_interfaces.h"
#include "esp_timer.h"
#include "esp_log.h"

static const char *TAG = "performance_test";

void setUp(void) {
    const app_controller_interface_t* app_ctrl = app_controller_get_interface();
    app_ctrl->init();
    app_ctrl->start();
}

void tearDown(void) {
    const app_controller_interface_t* app_ctrl = app_controller_get_interface();
    if (app_ctrl->is_initialized()) {
        app_ctrl->stop();
    }
}

void test_api_response_time(void) {
    const app_controller_interface_t* app_ctrl = app_controller_get_interface();
    
    ESP_LOGI(TAG, "Testing API response time...");
    
    // 测试获取系统状态的响应时间
    int64_t start_time = esp_timer_get_time();
    
    system_state_t state = {0};
    for (int i = 0; i < 100; i++) {
        TEST_ASSERT_EQUAL(ESP_OK, app_ctrl->get_system_state(&state));
    }
    
    int64_t end_time = esp_timer_get_time();
    int64_t avg_time = (end_time - start_time) / 100;
    
    ESP_LOGI(TAG, "Average get_system_state time: %lld us", avg_time);
    
    // 验证响应时间在合理范围内 (< 1ms)
    TEST_ASSERT_LESS_THAN(1000, avg_time);
}

void test_data_update_frequency(void) {
    const data_manager_interface_t* data_mgr = data_manager_get_interface();
    
    ESP_LOGI(TAG, "Testing data update frequency...");
    
    uint32_t update_count = 0;
    int64_t start_time = esp_timer_get_time();
    
    // 注册数据回调
    data_mgr->register_data_callback((void*)&update_count);
    
    // 启动数据管理器
    data_mgr->start();
    
    // 运行一段时间
    vTaskDelay(pdMS_TO_TICKS(2000));
    
    int64_t end_time = esp_timer_get_time();
    int64_t duration_ms = (end_time - start_time) / 1000;
    
    float frequency = (float)update_count / (duration_ms / 1000.0f);
    
    ESP_LOGI(TAG, "Data update frequency: %.2f Hz", frequency);
    ESP_LOGI(TAG, "Total updates: %lu in %lld ms", update_count, duration_ms);
    
    // 验证更新频率在预期范围内 (10Hz ± 2Hz)
    TEST_ASSERT_GREATER_THAN(8.0f, frequency);
    TEST_ASSERT_LESS_THAN(12.0f, frequency);
}

void test_memory_usage(void) {
    ESP_LOGI(TAG, "Testing memory usage...");
    
    // 获取初始内存使用
    size_t initial_free = esp_get_free_heap_size();
    ESP_LOGI(TAG, "Initial free memory: %u bytes", initial_free);
    
    // 创建额外的系统负载
    const app_controller_interface_t* app_ctrl = app_controller_get_interface();
    
    // 多次调用API
    for (int i = 0; i < 100; i++) {
        system_state_t state = {0};
        app_ctrl->get_system_state(&state);
        
        protection_config_t config = {0};
        app_ctrl->get_protection_config(PROTECTION_CHANNEL_OCP, &config);
    }
    
    // 获取最终内存使用
    size_t final_free = esp_get_free_heap_size();
    ESP_LOGI(TAG, "Final free memory: %u bytes", final_free);
    
    size_t memory_used = initial_free - final_free;
    ESP_LOGI(TAG, "Memory used: %u bytes", memory_used);
    
    // 验证内存使用在合理范围内 (< 10KB)
    TEST_ASSERT_LESS_THAN(10240, memory_used);
    
    // 验证没有内存泄漏
    TEST_ASSERT_GREATER_THAN(initial_free * 0.8, final_free);
}

void test_web_socket_throughput(void) {
    const web_server_interface_t* web_server = web_server_get_interface();
    
    ESP_LOGI(TAG, "Testing WebSocket throughput...");
    
    int64_t start_time = esp_timer_get_time();
    int message_count = 0;
    
    // 发送多个WebSocket消息
    for (int i = 0; i < 100; i++) {
        ws_message_t msg = {
            .type = WS_MSG_TYPE_ADC_DATA,
            .timestamp_ms = xTaskGetTickCount() * portTICK_PERIOD_MS
        };
        snprintf(msg.json_data, sizeof(msg.json_data), 
                 "{\"v\":%.2f,\"i\":%.2f,\"p\":%.2f}", 12.0f + i*0.1f, 1.5f, 18.0f + i*0.15f);
        
        if (web_server->broadcast_ws_message(&msg) == ESP_OK) {
            message_count++;
        }
    }
    
    int64_t end_time = esp_timer_get_time();
    int64_t total_time_ms = (end_time - start_time) / 1000;
    
    float throughput = (float)message_count / (total_time_ms / 1000.0f);
    
    ESP_LOGI(TAG, "WebSocket throughput: %.2f messages/sec", throughput);
    ESP_LOGI(TAG, "Total messages: %d in %lld ms", message_count, total_time_ms);
    
    // 验证吞吐量 (> 50 messages/sec)
    TEST_ASSERT_GREATER_THAN(50.0f, throughput);
}
```

## 测试执行计划

### 6.1 本地测试执行

```bash
# 编译测试
idf.py build

# 运行单元测试
idf.py -C tests/unit test

# 运行集成测试
idf.py -C tests/integration test

# 运行系统测试
idf.py -C tests/system test

# 运行所有测试
idf.py test
```

### 6.2 持续集成配置

**.github/workflows/test.yml**：
```yaml
name: Test Suite

on: [push, pull_request]

jobs:
  test:
    runs-on: ubuntu-latest
    
    steps:
    - uses: actions/checkout@v2
    
    - name: Setup ESP-IDF
      uses: espressif/esp-idf-ci-action@v1
      with:
        esp_idf_version: v5.0.1
    
    - name: Build project
      run: |
        idf.py build
    
    - name: Run unit tests
      run: |
        idf.py -C tests/unit test
    
    - name: Run integration tests
      run: |
        idf.py -C tests/integration test
    
    - name: Generate test report
      run: |
        idf.py -C tests test-report
    
    - name: Upload test results
      uses: actions/upload-artifact@v2
      with:
        name: test-results
        path: tests/test_report.xml
```

## 测试覆盖率目标

### 7.1 覆盖率要求
- **语句覆盖率**: ≥ 80%
- **分支覆盖率**: ≥ 75%
- **函数覆盖率**: ≥ 90%
- **行覆盖率**: ≥ 80%

### 7.2 覆盖率检查

```bash
# 生成覆盖率报告
gcov -o build/test_results/ *.c
lcov --capture --directory . --output-file coverage.info
genhtml coverage.info --output-directory coverage_report

# 查看覆盖率摘要
lcov --summary coverage.info
```

## 缺陷跟踪和修复

### 8.1 缺陷分类
- **严重**: 系统崩溃、数据丢失
- **重要**: 功能失效、性能严重下降
- **一般**: 功能异常、界面问题
- **轻微**: 代码风格、文档问题

### 8.2 缺陷修复流程
1. 缺陷报告和分类
2. 根因分析
3. 修复方案设计
4. 代码实现和测试
5. 回归测试验证
6. 代码审查和合并

这个测试框架确保重构后的系统具有高质量和可靠性，为后续的功能扩展和维护提供坚实基础。