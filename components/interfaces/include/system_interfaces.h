/**
 * @file system_interfaces.h
 * @brief 系统组件接口定义
 * 
 * 该文件定义了所有系统组件的标准接口，用于解耦各组件之间的直接依赖
 */

#ifndef SYSTEM_INTERFACES_H
#define SYSTEM_INTERFACES_H

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ========================== 错误码定义 ========================== */
typedef enum {
    // 基础错误码
    ERR_SYSTEM_OK = ESP_OK,
    ERR_SYSTEM_FAIL = ESP_FAIL,
    ERR_SYSTEM_INVALID_ARG = ESP_ERR_INVALID_ARG,
    ERR_SYSTEM_NO_MEM = ESP_ERR_NO_MEM,
    ERR_SYSTEM_TIMEOUT = ESP_ERR_TIMEOUT,
    
    // 应用控制器错误码 (0x1000-0x1FFF)
    ERR_APP_CONTROLLER_BASE = 0x1000,
    ERR_APP_CONTROLLER_NOT_INIT,
    ERR_APP_CONTROLLER_ALREADY_INIT,
    ERR_APP_CONTROLLER_INVALID_STATE,
    
    // 数据管理器错误码 (0x2000-0x2FFF)
    ERR_DATA_MANAGER_BASE = 0x2000,
    ERR_DATA_MANAGER_NOT_INIT,
    ERR_DATA_MANAGER_INVALID_CHANNEL,
    ERR_DATA_MANAGER_CALIBRATION_FAILED,
    
    // 保护控制器错误码 (0x3000-0x3FFF)
    ERR_PROTECTION_BASE = 0x3000,
    ERR_PROTECTION_NOT_INIT,
    ERR_PROTECTION_INVALID_CHANNEL,
    ERR_PROTECTION_INVALID_THRESHOLD,
    
    // Web服务器错误码 (0x4000-0x4FFF)
    ERR_WEB_SERVER_BASE = 0x4000,
    ERR_WEB_SERVER_NOT_INIT,
    ERR_WEB_SERVER_REQUEST_FAILED,
    ERR_WEB_SERVER_WS_SEND_FAILED,
} system_error_t;

/* ========================== 数据类型定义 ========================== */

/**
 * @brief ADC读取数据结构
 */
typedef struct {
    int raw_voltage;        // 原始电压ADC值
    int raw_current;        // 原始电流ADC值
    float voltage;          // 校准后的电压值(V)
    float current;          // 校准后的电流值(A)
    float power;            // 计算功率(W)
    uint32_t timestamp;     // 时间戳(ms)
} adc_data_t;

/**
 * @brief 系统状态结构
 */
typedef struct {
    bool output_enabled;            // 输出状态
    float voltage;                  // 当前电压(V)
    float current;                  // 当前电流(A)
    float power;                    // 当前功率(W)
    uint8_t protection_status;      // 保护状态位图
    bool is_fault;                  // 是否有故障
    uint32_t uptime_ms;             // 系统运行时间(ms)
} system_state_t;

/**
 * @brief 保护通道定义
 */
typedef enum {
    PROTECTION_CHANNEL_OCP = 1,     // 过流保护
    PROTECTION_CHANNEL_UVP = 2,     // 欠压保护
    PROTECTION_CHANNEL_OVP = 3,     // 过压保护
} protection_channel_t;

/**
 * @brief 保护动作类型
 */
typedef enum {
    PROTECTION_ACTION_INDICATE = 0,     // 仅指示
    PROTECTION_ACTION_IMMEDIATE_OFF = 1, // 立即断开
    PROTECTION_ACTION_DEGLITCH_OFF = 2,  // 去抖断开
} protection_action_t;

/**
 * @brief 保护配置结构
 */
typedef struct {
    protection_channel_t channel;       // 保护通道
    float threshold;                    // 保护阈值
    protection_action_t action;         // 保护动作
    bool enabled;                       // 是否启用
} protection_config_t;

/**
 * @brief 保护状态结构
 */
typedef struct {
    protection_channel_t channel;
    bool is_triggered;
    float current_value;
    float threshold;
    uint32_t trigger_time_ms;
} protection_status_t;

/**
 * @brief 系统事件类型
 */
typedef enum {
    SYSTEM_EVENT_ADC_DATA_UPDATED,              // ADC数据更新
    SYSTEM_EVENT_OUTPUT_STATE_CHANGED,          // 输出状态变化
    SYSTEM_EVENT_PROTECTION_TRIGGERED,          // 保护触发
    SYSTEM_EVENT_PROTECTION_CONFIG_CHANGED,     // 保护配置变更
    SYSTEM_EVENT_SYSTEM_ERROR,                  // 系统错误
    SYSTEM_EVENT_SYSTEM_INITIALIZED,            // 系统初始化完成
    SYSTEM_EVENT_SYSTEM_SHUTDOWN,               // 系统关闭
} system_event_type_t;

/**
 * @brief 系统事件数据结构
 */
typedef struct {
    system_event_type_t type;
    void* data;
    size_t data_size;
    uint32_t timestamp_ms;
} system_event_t;

/**
 * @brief 事件处理函数原型
 */
typedef void (*system_event_handler_t)(const system_event_t* event);

/**
 * @brief WebSocket消息类型
 */
typedef enum {
    WS_MSG_TYPE_STATUS = 0,         // 状态信息
    WS_MSG_TYPE_ADC_DATA = 1,       // ADC数据
    WS_MSG_TYPE_PROTECTION = 2,     // 保护状态
    WS_MSG_TYPE_ERROR = 3,          // 错误信息
} ws_message_type_t;

/**
 * @brief WebSocket消息结构
 */
typedef struct {
    ws_message_type_t type;
    char json_data[256];
    uint32_t timestamp_ms;
} ws_message_t;

/* ========================== 应用控制器接口 ========================== */

/**
 * @brief 应用控制器接口结构
 */
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

/* ========================== 数据管理器接口 ========================== */

/**
 * @brief 数据管理器接口结构
 */
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

/* ========================== 保护控制器接口 ========================== */

/**
 * @brief 保护控制器接口结构
 */
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

/* ========================== Web服务器接口 ========================== */

/**
 * @brief Web服务器接口结构
 */
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

/* ========================== 全局接口实例 ========================== */

/**
 * @brief 获取应用控制器接口实例
 */
const app_controller_interface_t* app_controller_get_interface(void);

/**
 * @brief 获取数据管理器接口实例
 */
const data_manager_interface_t* data_manager_get_interface(void);

/**
 * @brief 获取保护控制器接口实例
 */
const protection_controller_interface_t* protection_controller_get_interface(void);

/**
 * @brief 获取Web服务器接口实例
 */
const web_server_interface_t* web_server_get_interface(void);

/* ========================== 工具函数 ========================== */

/**
 * @brief 错误码转字符串
 */
const char* system_error_to_string(esp_err_t error);

/**
 * @brief 事件类型转字符串
 */
const char* system_event_type_to_string(system_event_type_t event);

/**
 * @brief 保护通道转字符串
 */
const char* protection_channel_to_string(protection_channel_t channel);

/**
 * @brief 保护动作转字符串
 */
const char* protection_action_to_string(protection_action_t action);

#ifdef __cplusplus
}
#endif


