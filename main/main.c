#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <errno.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_err.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "driver/gpio.h"

#include "board_io.h"
#include "http_server.h"
#include "mpq5857.h"
#include "adc.h"
#include "myFS.h"

static const char *TAG = "app_main";

#define BUFFER_SIZE 512U  /*!< File send buffer size */

/* ========================== Static File Send Utility ========================== */
/**
 * @brief Send file from littleFS as HTTP response
 * @param req HTTP request handle
 * @param path Absolute path in littleFS, e.g., /littlefs/index.html
 * @return ESP_OK on success, ESP_FAIL on failure
 */
static esp_err_t send_file(httpd_req_t *req, const char *path)
{
    /* Open file */
    FILE *file = fopen(path, "r");
    if (!file) {
        ESP_LOGE(TAG, "Failed to open file %s, errno=%d", path, errno);
        httpd_resp_send_404(req);
        return ESP_FAIL;
    }

    /* Get file size and set Content-Length */
    struct stat file_stat;
    if (fstat(fileno(file), &file_stat) != 0) {
        ESP_LOGE(TAG, "Failed to stat file %s", path);
        fclose(file);
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }

    char len_str[16];
    snprintf(len_str, sizeof(len_str), "%ld", (long)file_stat.st_size);
    httpd_resp_set_hdr(req, "Content-Length", len_str);

    /* Send file in chunks */
    char buffer[BUFFER_SIZE];
    size_t bytes_read;
    do {
        bytes_read = fread(buffer, 1, sizeof(buffer), file);
        if (bytes_read > 0) {
            if (httpd_resp_send_chunk(req, buffer, bytes_read) != ESP_OK) {
                ESP_LOGE(TAG, "Failed to send file chunk");
                fclose(file);
                return ESP_FAIL;
            }
        }
    } while (bytes_read > 0);

    /* End response */
    fclose(file);
    httpd_resp_send_chunk(req, NULL, 0);  /* Empty chunk indicates EOF */
    return ESP_OK;
}

/* ========================== HTTP Request Handlers ========================== */
/**
 * @brief GET / -> Return /littlefs/index.html
 */
esp_err_t root_get_handler(httpd_req_t *req)
{
    httpd_resp_set_type(req, "text/html");
    return send_file(req, "/littlefs/index.html");
}

/**
 * @brief Static resources handler, supports css/js/svg/json/etc.
 */
esp_err_t static_get_handler(httpd_req_t *req)
{
    char path[PATH_MAX_LEN];
    snprintf(path, sizeof(path), "/littlefs%s", req->uri);  /* /css/app.css -> /littlefs/css/app.css */

    /* Set MIME type based on file extension */
    if (strstr(path, ".css")) {
        httpd_resp_set_type(req, "text/css");
    } else if (strstr(path, ".js")) {
        httpd_resp_set_type(req, "application/javascript");
    } else if (strstr(path, ".svg")) {
        httpd_resp_set_type(req, "image/svg+xml");
    } else if (strstr(path, ".json")) {
        httpd_resp_set_type(req, "application/json");
    } else {
        httpd_resp_set_type(req, "text/plain");
    }

    return send_file(req, path);
}

/**
 * @brief POST /api/power-control handler
 *        Request body: {"enabled":true}
 *        Response: {"enabled":true,"success":true}
 */
esp_err_t power_control_handler(httpd_req_t *req)
{
    char buf[32] = {0};
    int ret = httpd_req_recv(req, buf, sizeof(buf) - 1);
    if (ret <= 0) {
        ESP_LOGE(TAG, "Failed to receive request data");
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }

    /* Simple JSON parsing for enabled field */
    bool enabled = false;
    if (strstr(buf, "\"enabled\":true")) {
        enabled = true;
    } else if (strstr(buf, "\"enabled\":false")) {
        enabled = false;
    } else {
        ESP_LOGE(TAG, "Invalid request format: %s", buf);
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid JSON format");
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "Set power output to %s", enabled ? "ON" : "OFF");
    mpq5857_set_output(enabled ? 1 : 0);
    
    char resp[64];
    int len = snprintf(resp, sizeof(resp), "{\"enabled\":%s,\"success\":true}",
                       enabled ? "true" : "false");
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, resp, len);
    return ESP_OK;
}

/**
 * @brief POST /api/protection-config handler
 *        Expected: {"channel":1,"threshold":75,"action":1}
 *        Response: {"channel":1,"threshold":75,"action":1,"success":true}
 */
esp_err_t protection_config_handler(httpd_req_t *req)
{
    char buf[128] = {0};
    int ret = httpd_req_recv(req, buf, sizeof(buf) - 1);
    if (ret <= 0) {
        ESP_LOGE(TAG, "Failed to receive request data");
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }

    int channel, threshold, action;
    if (sscanf(buf, "{\"channel\":%d,\"threshold\":%d,\"action\":%d}",
               &channel, &threshold, &action) != 3) {
        ESP_LOGE(TAG, "Failed to parse JSON: %s", buf);
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid JSON format");
        return ESP_FAIL;
    }

    /* Validate parameters */
    if (channel < 0 || channel > 3 || threshold < 0 || action < 0 || action > 2) {
        ESP_LOGE(TAG, "Invalid parameters: ch=%d, thr=%d, act=%d", channel, threshold, action);
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid parameter values");
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "Protection config: channel=%d, threshold=%d, action=%d",
             channel, threshold, action);

    /* Log parameters (TODO: Implement actual register configuration) */
    ESP_LOGI(TAG, "[OCP] ch=%d, thr=%d, act=%s",
             channel, threshold, action ? "disconnect" : "indicate");

    switch (channel) {
        case 0:
            ESP_LOGI(TAG, "%s", action ? "Save Default" : "Load Default");
            if(action){
                params_t cfg = {0};
                load_params(&cfg);
                MPQ5857_Registers_u_t value;
                value = mpq5857_flush();
                for(int i = 0; i<4 ; i++){
                    ((uint8_t *)(&cfg.registers))[i] = value.bytes[i];
                }
                save_params(&cfg);
                load_params(&cfg);
                ESP_LOGI(TAG, "Configuration loaded: ssid=%s, pwd=%s, registers=%x",
                 cfg.wifi_ssid, cfg.wifi_pwd, cfg.registers);
            }else{
                ESP_LOGI(TAG, "正在加载默认配置到硬件...");
                params_t cfg = {0};
                if (load_params(&cfg)) {
                    ESP_LOGI(TAG, "成功加载默认配置: registers=%08x", cfg.registers);
                    
                    mpq5857_write_reg(0x00, (uint8_t)(cfg.registers & 0x000000FF));
                    mpq5857_write_reg(0x01, (uint8_t)((cfg.registers & 0x0000FF00) >> 8));
                    mpq5857_write_reg(0x02, (uint8_t)((cfg.registers & 0x00FF0000) >> 16));
                    mpq5857_write_reg(0x03, (uint8_t)((cfg.registers & 0xFF000000) >> 24));

                    mpq5857_flush();
                    ESP_LOGI(TAG, "默认配置已成功应用到硬件");
                } else {
                    ESP_LOGE(TAG, "加载默认配置失败");
                }
            }
            break;
        case 1:
            ESP_LOGI(TAG, "Configure over-current protection");
            mpq5857_set_OCPLVL(threshold);
            mpq5857_set_OCPM(action);
            break;
        case 2:
            ESP_LOGI(TAG, "Configure under-voltage protection");
            mpq5857_set_UVPLVL(threshold);
            mpq5857_set_UVPM(action);
            break;
        case 3:
            ESP_LOGI(TAG, "Configure over-voltage protection");
            mpq5857_set_OVPLVL(threshold);
            mpq5857_set_OVPM(action);
            break;
        default:
            break;
    }

    char resp[128];
    int len = snprintf(resp, sizeof(resp),
        "{\"channel\":%d,\"threshold\":%d,\"action\":%d,\"success\":true}",
        channel, threshold, action);
    
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, resp, len);
    return ESP_OK;
}

/**
 * @brief GET /api/protection-status handler
 *        Response: {"channels":[{"channel":1,"type":"OCP","threshold":1,"action":1}]}
 */
esp_err_t protection_status_handler(httpd_req_t *req)
{
    MPQ5857_Registers_u_t value;
    value = mpq5857_flush();

    char resp[256];
    int len = snprintf(resp, sizeof(resp),
        "{\"channels\":["
        "{\"channel\":1,\"type\":\"OCP\",\"threshold\":%d,\"action\":%d},"
        "{\"channel\":2,\"type\":\"UVP\",\"threshold\":%d,\"action\":%d},"
        "{\"channel\":3,\"type\":\"OVP\",\"threshold\":%d,\"action\":%d}"
        "]}",
        value.value.reg00.bits.ocplvl, value.value.reg00.bits.ocpm, 
        value.value.reg00.bits.uvplvl, value.value.reg00.bits.uvpm, 
        value.value.reg01.bits.ovplvl, value.value.reg01.bits.ovpm
    );

    ESP_LOGI(TAG, "保护状态API响应: %s", resp);

    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, resp, strlen(resp));
    return ESP_OK;
}

/**
 * @brief POST /api/flttmr-config handler
 *        Expected: {"slope":0}
 *        Response: {"slope":0,"description":"2/ms","success":true}
 */
esp_err_t flttmr_config_handler(httpd_req_t *req)
{
    char buf[32] = {0};
    int ret = httpd_req_recv(req, buf, sizeof(buf) - 1);
    if (ret <= 0) {
        ESP_LOGE(TAG, "Failed to receive request data");
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }

    int slope;
    if (sscanf(buf, "{\"slope\":%d}", &slope) != 1 || slope < 0 || slope > 7) {
        ESP_LOGE(TAG, "Invalid FLTTMR value: %s", buf);
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid slope value");
        return ESP_FAIL;
    }

    const char *descriptions[] = {
        "0ms", "0.2ms", "0.4ms", "1ms",
        "2ms(默认)", "8ms", "32ms", "128ms"
    };

    ESP_LOGI(TAG, "Set FLTTMR slope to %d: %s", slope, descriptions[slope]);

    esp_err_t err = mpq5857_set_FLTTMR(slope);
    
    char resp[128];
    int len = snprintf(resp, sizeof(resp),
        "{\"slope\":%d,\"description\":\"%s\",\"success\":%s}",
        slope, descriptions[slope], err ? "true" : "false");
    
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, resp, len);
    return ESP_OK;
}

/**
 * @brief GET /api/flttmr-status handler
 *        Response: {"slope":0,"description":"2ms","options":[...]}
 */
esp_err_t flttmr_status_handler(httpd_req_t *req)
{
    uint8_t real_value = mpq5857_get_FLTTMR() & 0x07;

    char resp[512];
    int len = snprintf(resp, sizeof(resp),
        "{\"slope\":%d,\"description\":\"0.25V/ms（默认）\","
        "\"options\":["
        "{\"value\":0,\"code\":\"000\",\"desc\":\"0ms\"},"
        "{\"value\":1,\"code\":\"001\",\"desc\":\"0.2ms\"},"
        "{\"value\":2,\"code\":\"010\",\"desc\":\"0.4ms\"},"
        "{\"value\":3,\"code\":\"011\",\"desc\":\"1ms\"},"
        "{\"value\":4,\"code\":\"100\",\"desc\":\"2ms(默认)\"},"
        "{\"value\":5,\"code\":\"101\",\"desc\":\"8ms\"},"
        "{\"value\":6,\"code\":\"110\",\"desc\":\"32ms\"},"
        "{\"value\":7,\"code\":\"111\",\"desc\":\"128ms\"}"
        "]}", 
        real_value
    );

    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, resp, len);
    return ESP_OK;
}
/**
 * @brief POST /api/sssr-config handler
 *        Expected: {"slope":0}
 *        Response: {"slope":0,"description":"0.25V/ms","success":true}
 */
esp_err_t sssr_config_handler(httpd_req_t *req)
{
    char buf[32] = {0};
    int ret = httpd_req_recv(req, buf, sizeof(buf) - 1);
    if (ret <= 0) {
        ESP_LOGE(TAG, "Failed to receive request data");
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }

    int slope;
    if (sscanf(buf, "{\"slope\":%d}", &slope) != 1 || slope < 0 || slope > 7) {
        ESP_LOGE(TAG, "Invalid SSSR value: %s", buf);
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid slope value");
        return ESP_FAIL;
    }

    const char *descriptions[] = {
        "0.25V/ms（默认）", "0.5V/ms", "1V/ms", "4V/ms",
        "16V/ms", "32V/ms", "64V/ms", "无软启动（直接开启）"
    };

    ESP_LOGI(TAG, "Set SSSR slope to %d: %s", slope, descriptions[slope]);

    esp_err_t err = mpq5857_set_SSSR(slope);

    char resp[128];
    int len = snprintf(resp, sizeof(resp),
        "{\"slope\":%d,\"description\":\"%s\",\"success\":%s}",
        slope, descriptions[slope], err ? "true" : "false");
    
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, resp, len);
    return ESP_OK;
}

/**
 * @brief GET /api/sssr-status handler
 *        Response: {"slope":0,"description":"0.25V/ms","options":[...]}
 */
esp_err_t sssr_status_handler(httpd_req_t *req)
{
    uint8_t real_value = mpq5857_get_SSSR() & 0x07;

    char resp[512];
    int len = snprintf(resp, sizeof(resp),
        "{\"slope\":%d,\"description\":\"0.25V/ms（默认）\","
        "\"options\":["
        "{\"value\":0,\"code\":\"000\",\"desc\":\"0.25V/ms（默认）\"},"
        "{\"value\":1,\"code\":\"001\",\"desc\":\"0.5V/ms\"},"
        "{\"value\":2,\"code\":\"010\",\"desc\":\"1V/ms\"},"
        "{\"value\":3,\"code\":\"011\",\"desc\":\"4V/ms\"},"
        "{\"value\":4,\"code\":\"100\",\"desc\":\"16V/ms\"},"
        "{\"value\":5,\"code\":\"101\",\"desc\":\"32V/ms\"},"
        "{\"value\":6,\"code\":\"110\",\"desc\":\"64V/ms\"},"
        "{\"value\":7,\"code\":\"111\",\"desc\":\"无软启动（直接开启）\"}"
        "]}",
        real_value
    );

    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, resp, len);
    return ESP_OK;
}

/**
 * @brief GET /api/system-status handler
 *        Response: Complete system status
 */
esp_err_t system_status_handler(httpd_req_t *req)
{
    bool output_enabled = gpio_get_level(CONFIG_GPIO_EN2);

    adc_reading_t value;
    value = adc_read_once();
    float power = value.voltage * value.current;

    MPQ5857_Registers_u_t init_value;
    init_value = mpq5857_demo();

    // 调试日志：系统状态数据
    ESP_LOGI(TAG, "系统状态API - output_enabled:%d, voltage:%.2f, current:%.2f, power:%.2f",
             output_enabled, value.voltage, value.current, power);
    ESP_LOGI(TAG, "系统状态status - normal:true, fault:false, ocp:false, uvp:false, ovp:false, otp:false");

    const char *resp = "{"
        "\"system\":{\"uptime\":0,\"version\":\"1.0.0\"},"
        "\"power\":{\"enabled\":%s,\"voltage\":%f,\"current\":%f,\"power\":%f},"
        "\"protection\":{"
        "\"channels\":["
        "{\"channel\":1,\"type\":\"OCP\",\"threshold\":%d,\"action\":%d,\"active\":false},"
        "{\"channel\":2,\"type\":\"UVP\",\"threshold\":%d,\"action\":%d,\"active\":false},"
        "{\"channel\":3,\"type\":\"OVP\",\"threshold\":%d,\"action\":%d,\"active\":false}"
        "]},"
        "\"sssr\":{\"slope\":0,\"description\":\"0.25V/ms（默认）\"},"
        "\"status\":{\"normal\":true,\"fault\":false,\"ocp\":false,\"uvp\":false,\"ovp\":false,\"otp\":false}"
        "}";
    
    char response[512];
    int len = snprintf(response, sizeof(response), resp, output_enabled ? "true" : "false", value.voltage, value.current, power, \
            init_value.value.reg00.bits.ocplvl, init_value.value.reg00.bits.ocpm,  \
            init_value.value.reg00.bits.uvplvl, init_value.value.reg00.bits.uvpm,  \
            init_value.value.reg01.bits.ovplvl, init_value.value.reg01.bits.ovpm  \
        );
    
    ESP_LOGI(TAG, "系统状态API响应长度: %d", len);
    
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, response, len);
    return ESP_OK;
}

/* ========================== Application Main ========================== */
void app_main(void)
{
    /* Initialize network interface and event loop */
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    /* Initialize hardware */
    configure_pins();
    mpq5857_init();
    adc_init();
    /* Configure MPQ5857 device */
    mpq5857_write_reg(6, 0xFF);
    /* Initialize file system */
    FS_Init();

    params_t cfg = {0};
    
    /* Try to load existing configuration */
    if (!load_params(&cfg)) {
        ESP_LOGW(TAG, "No config file found, creating default configuration");
        
        /* Set default parameters */
        strncpy(cfg.wifi_ssid, "MyESP32-AP", sizeof(cfg.wifi_ssid) - 1);
        cfg.wifi_ssid[sizeof(cfg.wifi_ssid) - 1] = '\0';
        
        cfg.registers = 0x38FBFA1F;
        
        strncpy(cfg.wifi_pwd, "BNGUBNGU", sizeof(cfg.wifi_pwd) - 1);
        cfg.wifi_pwd[sizeof(cfg.wifi_pwd) - 1] = '\0';
        
        if (save_params(&cfg)) {
            ESP_LOGI(TAG, "Default configuration created successfully");
        } else {
            ESP_LOGE(TAG, "Failed to create default configuration");
        }
    } else {
        ESP_LOGI(TAG, "Configuration loaded: ssid=%s, pwd=%s, registers=%x",
                 cfg.wifi_ssid, cfg.wifi_pwd, cfg.registers);
        
        mpq5857_write_reg(0x00, (uint8_t)(cfg.registers & 0x000000FF));
        mpq5857_write_reg(0x01, (uint8_t)((cfg.registers & 0x0000FF00) >> 8));
        mpq5857_write_reg(0x02, (uint8_t)((cfg.registers & 0x00FF0000) >> 16));
        mpq5857_write_reg(0x03, (uint8_t)((cfg.registers & 0xFF000000) >> 24));

        mpq5857_flush();
    }

    /* Initialize Wi-Fi soft AP mode */
    wifi_init_softap(cfg.wifi_ssid, cfg.wifi_pwd);

    /* Main application loop */
    while (1) {
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}