# MPQ5857 架构重构实施指南

## 概述

本文档提供了详细的重构实施步骤，帮助你将现有的耦合架构逐步迁移到清晰的分层架构。

## 重构策略

### 1.1 渐进式重构原则

- **保持功能完整**：每个阶段都要确保现有功能正常工作
- **小步快跑**：每次只修改一小部分，频繁测试
- **版本控制**：使用Git分支管理重构过程
- **回滚准备**：每个阶段都要有回滚方案

### 1.2 重构顺序

1. 创建接口定义和基础结构
2. 重构数据管理组件
3. 重构保护控制组件
4. 创建应用控制器
5. 重构Web服务器
6. 集成测试和优化

## 第一阶段：创建基础结构

### 2.1 创建新组件目录结构

```md
components/
├── interfaces/          # 新增：接口定义
│   ├── include/
│   │   └── system_interfaces.h
│   └── CMakeLists.txt
├── app_controller/      # 新增：应用控制器
│   ├── app_controller.c
│   ├── include/
│   │   └── app_controller.h
│   └── CMakeLists.txt
├── data_manager/        # 新增：数据管理器
│   ├── data_manager.c
│   ├── include/
│   │   └── data_manager.h
│   └── CMakeLists.txt
├── protection_controller/ # 新增：保护控制器
│   ├── protection_controller.c
│   ├── include/
│   │   └── protection_controller.h
│   └── CMakeLists.txt
└── web_server/          # 重构：移除硬件依赖
    ├── http_server.c
    ├── include/
    │   └── http_server.h
    └── CMakeLists.txt
```

### 2.2 创建接口组件

**components/interfaces/CMakeLists.txt**：

```cmake
idf_component_register(
    SRCS ""  # 只有头文件，没有源文件
    INCLUDE_DIRS "include"
    PRIV_REQUIRES esp_http_server  # 需要httpd_req_t定义
)
```

### 2.3 更新项目依赖

**main/idf_component.yml**：

```yaml
dependencies:
  idf:
    version: ">=4.1.0"
  board_io: 
    path: ../components/board_io
  adc: 
    path: ../components/adc
  mpq5857: 
    path: ../components/mpq5857
  web_server: 
    path: ../components/web_server
  interfaces:           # 新增
    path: ../components/interfaces
  app_controller:       # 新增
    path: ../components/app_controller
  data_manager:         # 新增
    path: ../components/data_manager
  protection_controller: # 新增
    path: ../components/protection_controller
```

## 第二阶段：重构数据管理器

### 3.1 创建数据管理器组件

**components/data_manager/include/data_manager.h**：

```c
#ifndef DATA_MANAGER_H
#define DATA_MANAGER_H

#include "system_interfaces.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 初始化数据管理器
 */
esp_err_t data_manager_init(void);

/**
 * @brief 启动数据管理器
 */
esp_err_t data_manager_start(void);

/**
 * @brief 停止数据管理器
 */
esp_err_t data_manager_stop(void);

/**
 * @brief 获取数据管理器接口
 */
const data_manager_interface_t* data_manager_get_interface(void);

/**
 * @brief 检查是否已初始化
 */
bool data_manager_is_initialized(void);

#ifdef __cplusplus
}
#endif

#endif /* DATA_MANAGER_H */
```

**components/data_manager/data_manager.c**：

```c
#include "data_manager.h"
#include "adc.h"  // 暂时保留，后续重构
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

static const char *TAG = "data_manager";

// 内部数据结构
typedef struct {
    bool initialized;
    bool started;
    adc_data_t latest_data;
    uint32_t sample_rate_hz;
    TaskHandle_t sampling_task;
    QueueHandle_t data_queue;
    void (*data_callback)(const adc_data_t* data);
    SemaphoreHandle_t data_mutex;
} data_manager_t;

static data_manager_t g_data_manager = {0};

// 采样任务
static void sampling_task(void *pvParameters) {
    while (g_data_manager.started) {
        // 获取ADC数据
        adc_reading_t adc_reading = adc_read_once();
        
        // 转换为标准格式
        adc_data_t data = {
            .raw_voltage = adc_reading.raw_vsns,
            .raw_current = adc_reading.raw_isns,
            .voltage = adc_reading.voltage,
            .current = adc_reading.current,
            .power = adc_reading.voltage * adc_reading.current,
            .timestamp = xTaskGetTickCount() * portTICK_PERIOD_MS
        };
        
        // 更新最新数据
        xSemaphoreTake(g_data_manager.data_mutex, portMAX_DELAY);
        g_data_manager.latest_data = data;
        xSemaphoreGive(g_data_manager.data_mutex);
        
        // 调用回调函数
        if (g_data_manager.data_callback) {
            g_data_manager.data_callback(&data);
        }
        
        // 发送事件通知
        system_event_t event = {
            .type = SYSTEM_EVENT_ADC_DATA_UPDATED,
            .data = &data,
            .data_size = sizeof(data),
            .timestamp_ms = data.timestamp
        };
        
        // 延时控制采样率
        vTaskDelay(pdMS_TO_TICKS(1000 / g_data_manager.sample_rate_hz));
    }
    
    vTaskDelete(NULL);
}

// 接口函数实现
static esp_err_t dm_init(void) {
    if (g_data_manager.initialized) {
        return ESP_OK;
    }
    
    // 初始化ADC
    adc_init();
    
    // 创建互斥锁
    g_data_manager.data_mutex = xSemaphoreCreateMutex();
    if (!g_data_manager.data_mutex) {
        return ESP_ERR_NO_MEM;
    }
    
    // 创建数据队列
    g_data_manager.data_queue = xQueueCreate(10, sizeof(adc_data_t));
    if (!g_data_manager.data_queue) {
        vSemaphoreDelete(g_data_manager.data_mutex);
        return ESP_ERR_NO_MEM;
    }
    
    // 初始化默认参数
    g_data_manager.sample_rate_hz = 10; // 10Hz默认采样率
    g_data_manager.initialized = true;
    
    ESP_LOGI(TAG, "Data manager initialized");
    return ESP_OK;
}

static esp_err_t dm_start(void) {
    if (!g_data_manager.initialized) {
        return ESP_ERR_INVALID_STATE;
    }
    
    if (g_data_manager.started) {
        return ESP_OK;
    }
    
    g_data_manager.started = true;
    
    // 创建采样任务
    BaseType_t ret = xTaskCreate(sampling_task, "dm_sampling", 2048, NULL, 5, 
                                 &g_data_manager.sampling_task);
    if (ret != pdPASS) {
        g_data_manager.started = false;
        return ESP_FAIL;
    }
    
    ESP_LOGI(TAG, "Data manager started");
    return ESP_OK;
}

static esp_err_t dm_stop(void) {
    if (!g_data_manager.started) {
        return ESP_OK;
    }
    
    g_data_manager.started = false;
    
    // 等待任务结束
    vTaskDelay(pdMS_TO_TICKS(100));
    
    ESP_LOGI(TAG, "Data manager stopped");
    return ESP_OK;
}

static bool dm_is_initialized(void) {
    return g_data_manager.initialized;
}

static esp_err_t dm_get_latest_data(adc_data_t* data) {
    if (!data || !g_data_manager.initialized) {
        return ESP_ERR_INVALID_ARG;
    }
    
    xSemaphoreTake(g_data_manager.data_mutex, portMAX_DELAY);
    *data = g_data_manager.latest_data;
    xSemaphoreGive(g_data_manager.data_mutex);
    
    return ESP_OK;
}

static esp_err_t dm_set_sample_rate(uint32_t rate_hz) {
    if (rate_hz == 0 || rate_hz > 1000) {
        return ESP_ERR_INVALID_ARG;
    }
    
    g_data_manager.sample_rate_hz = rate_hz;
    return ESP_OK;
}

static esp_err_t dm_register_data_callback(void (*callback)(const adc_data_t* data)) {
    g_data_manager.data_callback = callback;
    return ESP_OK;
}

// 接口实例
static const data_manager_interface_t g_data_manager_interface = {
    .init = dm_init,
    .start = dm_start,
    .stop = dm_stop,
    .is_initialized = dm_is_initialized,
    .get_latest_data = dm_get_latest_data,
    .set_sample_rate = dm_set_sample_rate,
    .register_data_callback = dm_register_data_callback,
    // 其他函数实现...
};

const data_manager_interface_t* data_manager_get_interface(void) {
    return &g_data_manager_interface;
}

// 公共函数
esp_err_t data_manager_init(void) {
    return g_data_manager_interface.init();
}

esp_err_t data_manager_start(void) {
    return g_data_manager_interface.start();
}

esp_err_t data_manager_stop(void) {
    return g_data_manager_interface.stop();
}

bool data_manager_is_initialized(void) {
    return g_data_manager_interface.is_initialized();
}
```

## 第三阶段：创建应用控制器

### 4.1 应用控制器实现

**components/app_controller/app_controller.c**：

```c
#include "app_controller.h"
#include "system_interfaces.h"
#include "esp_log.h"
#include "freertos/semphr.h"

static const char *TAG = "app_controller";

// 应用控制器状态
typedef struct {
    bool initialized;
    bool started;
    system_state_t system_state;
    SemaphoreHandle_t state_mutex;
    
    // 子组件接口
    const data_manager_interface_t* data_mgr;
    const protection_controller_interface_t* protection_mgr;
    
    // 事件处理
    system_event_handler_t event_handlers[SYSTEM_EVENT_MAX];
} app_controller_t;

static app_controller_t g_app_controller = {0};

// 事件分发函数
static void dispatch_event(const system_event_t* event) {
    if (event->type < SYSTEM_EVENT_MAX && g_app_controller.event_handlers[event->type]) {
        g_app_controller.event_handlers[event->type](event);
    }
}

// ADC数据更新事件处理
static void on_adc_data_updated(const adc_data_t* data) {
    xSemaphoreTake(g_app_controller.state_mutex, portMAX_DELAY);
    
    // 更新系统状态
    g_app_controller.system_state.voltage = data->voltage;
    g_app_controller.system_state.current = data->current;
    g_app_controller.system_state.power = data->power;
    
    xSemaphoreGive(g_app_controller.state_mutex);
    
    // 分发事件
    system_event_t event = {
        .type = SYSTEM_EVENT_ADC_DATA_UPDATED,
        .data = (void*)data,
        .data_size = sizeof(adc_data_t),
        .timestamp_ms = data->timestamp
    };
    dispatch_event(&event);
}

// 接口函数实现
static esp_err_t ac_init(void) {
    if (g_app_controller.initialized) {
        return ESP_OK;
    }
    
    // 创建互斥锁
    g_app_controller.state_mutex = xSemaphoreCreateMutex();
    if (!g_app_controller.state_mutex) {
        return ESP_ERR_NO_MEM;
    }
    
    // 获取子组件接口
    g_app_controller.data_mgr = data_manager_get_interface();
    g_app_controller.protection_mgr = protection_controller_get_interface();
    
    // 初始化子组件
    esp_err_t err = g_app_controller.data_mgr->init();
    if (err != ESP_OK) {
        vSemaphoreDelete(g_app_controller.state_mutex);
        return err;
    }
    
    // 注册数据回调
    g_app_controller.data_mgr->register_data_callback(on_adc_data_updated);
    
    // 初始化系统状态
    g_app_controller.system_state.output_enabled = false;
    g_app_controller.system_state.is_fault = false;
    g_app_controller.system_state.uptime_ms = 0;
    
    g_app_controller.initialized = true;
    
    ESP_LOGI(TAG, "App controller initialized");
    return ESP_OK;
}

static esp_err_t ac_start(void) {
    if (!g_app_controller.initialized) {
        return ESP_ERR_INVALID_STATE;
    }
    
    if (g_app_controller.started) {
        return ESP_OK;
    }
    
    // 启动子组件
    esp_err_t err = g_app_controller.data_mgr->start();
    if (err != ESP_OK) {
        return err;
    }
    
    g_app_controller.started = true;
    
    // 发送系统启动事件
    system_event_t event = {
        .type = SYSTEM_EVENT_SYSTEM_INITIALIZED,
        .data = NULL,
        .data_size = 0,
        .timestamp_ms = xTaskGetTickCount() * portTICK_PERIOD_MS
    };
    dispatch_event(&event);
    
    ESP_LOGI(TAG, "App controller started");
    return ESP_OK;
}

static esp_err_t ac_set_output(bool enable) {
    if (!g_app_controller.initialized) {
        return ESP_ERR_INVALID_STATE;
    }
    
    // 这里调用硬件控制，但由app_controller统一管理
    // 实际实现中会调用mpq5857_set_output或其他硬件控制函数
    
    xSemaphoreTake(g_app_controller.state_mutex, portMAX_DELAY);
    g_app_controller.system_state.output_enabled = enable;
    xSemaphoreGive(g_app_controller.state_mutex);
    
    // 发送状态变化事件
    system_event_t event = {
        .type = SYSTEM_EVENT_OUTPUT_STATE_CHANGED,
        .data = &enable,
        .data_size = sizeof(bool),
        .timestamp_ms = xTaskGetTickCount() * portTICK_PERIOD_MS
    };
    dispatch_event(&event);
    
    ESP_LOGI(TAG, "Output %s", enable ? "enabled" : "disabled");
    return ESP_OK;
}

static esp_err_t ac_get_system_state(system_state_t* state) {
    if (!state || !g_app_controller.initialized) {
        return ESP_ERR_INVALID_ARG;
    }
    
    xSemaphoreTake(g_app_controller.state_mutex, portMAX_DELAY);
    *state = g_app_controller.system_state;
    xSemaphoreGive(g_app_controller.state_mutex);
    
    return ESP_OK;
}

static esp_err_t ac_register_event_handler(system_event_type_t event, system_event_handler_t handler) {
    if (event >= SYSTEM_EVENT_MAX || !g_app_controller.initialized) {
        return ESP_ERR_INVALID_ARG;
    }
    
    g_app_controller.event_handlers[event] = handler;
    return ESP_OK;
}

// 接口实例
static const app_controller_interface_t g_app_controller_interface = {
    .init = ac_init,
    .start = ac_start,
    .is_initialized = ac_is_initialized,
    .set_output = ac_set_output,
    .get_output_state = ac_get_output_state,
    .get_system_state = ac_get_system_state,
    .register_event_handler = ac_register_event_handler,
    // 其他函数...
};

const app_controller_interface_t* app_controller_get_interface(void) {
    return &g_app_controller_interface;
}
```

## 第四阶段：重构Web服务器

### 5.1 修改Web服务器以使用新接口

修改 **components/web_server/http_server.c**：

```c
#include "http_server.h"
#include "system_interfaces.h"  // 新增
#include "esp_log.h"

static const char *TAG = "http_server";

// 全局接口指针
static const app_controller_interface_t* g_app_controller = NULL;
static const web_server_interface_t* g_web_server = NULL;

// 修改原有的set_led_handler函数
esp_err_t set_led_handler(httpd_req_t *req) {
    if (!g_app_controller) {
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }
    
    char buf[8] = {0};
    int ret = httpd_req_recv(req, buf, sizeof(buf) - 1);
    if (ret <= 0) {
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }
    
    int cmd = atoi(buf);
    
    // 使用应用控制器接口，而不是直接调用硬件函数
    esp_err_t err = g_app_controller->set_output(cmd ? true : false);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set output: %s", system_error_to_string(err));
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }
    
    const char *resp = cmd ? "{\"on\":1}" : "{\"on\":0}";
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, resp, strlen(resp));
    return ESP_OK;
}

// 修改WebSocket定时器回调
static void IRAM_ATTR ws_push_timer_cb(void *arg) {
    if (!g_app_controller) {
        return;
    }
    
    // 检查是否有WebSocket客户端连接
    size_t client_count = g_web_server->get_connected_client_count();
    if (client_count == 0) {
        return;
    }
    
    // 获取系统状态
    system_state_t state;
    esp_err_t err = g_app_controller->get_system_state(&state);
    if (err != ESP_OK) {
        return;
    }
    
    // 构建JSON消息
    char json[128];
    snprintf(json, sizeof(json), "{\"v\":%.2f,\"i\":%.2f,\"p\":%.2f,\"out\":%d}",
             state.voltage, state.current, state.power, state.output_enabled ? 1 : 0);
    
    // 创建WebSocket消息
    ws_message_t msg = {
        .type = WS_MSG_TYPE_ADC_DATA,
        .timestamp_ms = xTaskGetTickCount() * portTICK_PERIOD_MS
    };
    strncpy(msg.json_data, json, sizeof(msg.json_data) - 1);
    
    // 广播消息
    g_web_server->broadcast_ws_message(&msg);
}

// 在start_webserver函数中添加接口初始化
httpd_handle_t start_webserver(void) {
    // 获取接口实例
    g_app_controller = app_controller_get_interface();
    g_web_server = web_server_get_interface();
    
    if (!g_app_controller || !g_web_server) {
        ESP_LOGE(TAG, "Failed to get system interfaces");
        return NULL;
    }
    
    // 原有的Web服务器启动代码...
    // (保持不变)
}
```

## 第五阶段：更新主函数

### 6.1 修改main.c以使用新架构

```c
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "sdkconfig.h"

#include "board_io.h"
#include "system_interfaces.h"  // 新增
#include "app_controller.h"     // 新增

static const char *TAG = "app_main";

// 系统事件处理函数
static void system_event_handler(const system_event_t* event) {
    switch (event->type) {
        case SYSTEM_EVENT_ADC_DATA_UPDATED:
            // ADC数据更新处理
            break;
            
        case SYSTEM_EVENT_OUTPUT_STATE_CHANGED:
            // 输出状态变化处理
            break;
            
        case SYSTEM_EVENT_PROTECTION_TRIGGERED:
            // 保护触发处理
            break;
            
        case SYSTEM_EVENT_SYSTEM_ERROR:
            // 系统错误处理
            ESP_LOGE(TAG, "System error occurred");
            break;
            
        default:
            break;
    }
}

void app_main(void) {
    ESP_LOGI(TAG, "MPQ5857 System Starting...");
    
    // 基础初始化
    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    
    // 挂载SPIFFS
    esp_vfs_spiffs_conf_t spiffs_conf = {
        .base_path = "/spiffs",
        .partition_label = "spiffs_data",
        .max_files = 5,
        .format_if_mount_failed = true
    };
    ESP_ERROR_CHECK(esp_vfs_spiffs_register(&spiffs_conf));
    
    // 硬件初始化
    configure_pins();
    
    // 获取应用控制器接口
    const app_controller_interface_t* app_ctrl = app_controller_get_interface();
    
    // 初始化系统
    ESP_ERROR_CHECK(app_ctrl->init());
    
    // 注册事件处理
    app_ctrl->register_event_handler(SYSTEM_EVENT_SYSTEM_ERROR, system_event_handler);
    app_ctrl->register_event_handler(SYSTEM_EVENT_ADC_DATA_UPDATED, system_event_handler);
    
    // 启动系统
    ESP_ERROR_CHECK(app_ctrl->start());
    
    // 启动WiFi（这将触发Web服务器启动）
    wifi_init_softap();
    
    // 演示功能
    print_spiffs_free();
    
    // 主循环
    while (1) {
        // 系统运行状态监控
        system_state_t state;
        if (app_ctrl->get_system_state(&state) == ESP_OK) {
            ESP_LOGI(TAG, "System: V=%.2fV, I=%.2fA, P=%.2fW, Output=%s",
                     state.voltage, state.current, state.power,
                     state.output_enabled ? "ON" : "OFF");
        }
        
        // 演示输出切换
        app_ctrl->set_output(0);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        app_ctrl->set_output(1);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}
```

## 测试和验证

### 7.1 编译测试
```bash
idf.py build
```

### 7.2 功能测试清单

- [ ] Web界面正常显示
- [ ] 实时数据更新正常
- [ ] 输出控制功能正常
- [ ] 保护配置功能正常
- [ ] WebSocket通信正常
- [ ] 系统状态监控正常

### 7.3 性能测试

- [ ] 内存使用检查
- [ ] CPU使用率监控
- [ ] 响应时间测试
- [ ] 长时间运行稳定性

## 后续优化建议

### 8.1 代码优化

- 添加更多的错误处理
- 优化内存使用
- 添加性能监控

### 8.2 功能扩展

- 添加数据记录功能
- 支持远程配置
- 添加告警机制

### 8.3 测试完善

- 添加单元测试
- 添加集成测试
- 添加性能基准测试

这个重构实施指南提供了一个完整的迁移路径，帮助你逐步将现有的耦合架构转换为清晰的分层架构，同时保持功能的完整性。
