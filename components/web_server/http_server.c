#include <stdio.h>
#include <string.h>

#include "http_server.h"
#include "driver/gpio.h"
#include "esp_timer.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "adc.h"
#include "mpq5857.h"

extern int flag;

static const char *TAG = "http_server";

static httpd_handle_t ws_server = NULL;
static httpd_ws_frame_t ws_pkt;
static esp_timer_handle_t ws_timer = NULL;

#define WS_TIMER_INTERVAL_US 200000U  /* 200ms */
#define MAX_WS_CLIENTS CONFIG_LWIP_MAX_LISTENING_TCP

/**
 * @brief WebSocket push timer callback
 * Broadcasts voltage and current data to all connected WebSocket clients
 */
static void IRAM_ATTR ws_push_timer_cb(void *arg)
{
    /* Check if any WebSocket clients are online */
    size_t fds = MAX_WS_CLIENTS;
    int client_fds[MAX_WS_CLIENTS];
    
    if (httpd_get_client_list(ws_server, &fds, client_fds) != ESP_OK) {
        return;
    }

    bool online = false;
    for (size_t i = 0; i < fds; ++i) {
        if (httpd_ws_get_fd_info(ws_server, client_fds[i]) == HTTPD_WS_CLIENT_WEBSOCKET) {
            online = true;
            break;
        }
    }

    if (!online) {
        return;  /* No WebSocket clients connected */
    }

    /* TODO: Get actual sensor readings */
    //mpq5857_update();
    adc_reading_t value;
    value = adc_read_once();//与adc耦合，需要解耦
    float power = value.voltage * value.current;


    /* Enhanced JSON with more data points and device status */
    
    int ocp = 0, uvp = 0, ovp = 0, otp = 0; // mock values
    int normal = 1; // mock values
 
    uint8_t fault = gpio_get_level(2);
    if(flag){
        flag = 0;
        ESP_LOGI(TAG, "FAULT!!!");
        uint8_t temp;
        mpq5857_read_regs(6, &temp, 1);
        gpio_set_level(8, 0);
        ESP_LOGI(TAG, "%x", temp);
        mpq5857_write_reg(6, 0xFF);
        // if(temp & 0x0100){
        //     otp = 1;
        //     ESP_LOGI(TAG, "otp");
        // }
        if(temp & 0x20){
            ocp = 1;
            ESP_LOGI(TAG, "ocp");
        }
        if(temp & 0x10){
            ovp = 1;
            ESP_LOGI(TAG, "ovp");
        }
        if(temp & 0x08){
            uvp = 1;
            ESP_LOGI(TAG, "uvp");
        }
    }

    int output_enabled = gpio_get_level(8);
    // ESP_LOGI(TAG, "WebSocket状态数据 - output_enabled:%d, ocp:%d, uvp:%d, ovp:%d, otp:%d, normal:%d, fault:%d",
    //          output_enabled, ocp, uvp, ovp, otp, normal, fault);
    
    char json[256];
    int len = snprintf(json, sizeof(json),
        "{\"type\":\"sensor_data\","
         "\"timestamp\":%lld,"
         "\"voltage\":%.2f,"
         "\"current\":%.2f,"
         "\"power\":%.2f,"
         "\"status\":{"
         "\"output_enabled\":%d,\"ocp\":%d,\"uvp\":%d,\"ovp\":%d,\"otp\":%d,"
         "\"normal\":%d,\"fault\":%d"
         "}}",
         esp_timer_get_time() / 1000, // 毫秒时间戳
         value.voltage, value.current, power,
         output_enabled,
         ocp, uvp, ovp, otp,
         normal, fault
    );
    if (len <= 0 || len >= (int)sizeof(json)) {
        ESP_LOGE(TAG, "Failed to format JSON data");
        return;
    }

    /* Broadcast to all connected WebSocket clients */
    ws_pkt.payload = (uint8_t *)json;
    ws_pkt.len = (size_t)len;
    ws_pkt.type = HTTPD_WS_TYPE_TEXT;

    //ESP_LOGI(TAG, "WebSocket广播数据: %s", json);

    if (httpd_get_client_list(ws_server, &fds, client_fds) == ESP_OK) {
        for (size_t i = 0; i < fds; ++i) {
            int fd = client_fds[i];
            if (httpd_ws_get_fd_info(ws_server, fd) == HTTPD_WS_CLIENT_WEBSOCKET) {
                esp_err_t ret = httpd_ws_send_frame_async(ws_server, fd, &ws_pkt);
                if (ret != ESP_OK) {
                    ESP_LOGW(TAG, "Failed to send WebSocket frame to client %d: %s",
                             fd, esp_err_to_name(ret));
                } else {
                    ESP_LOGD(TAG, "成功发送WebSocket数据到客户端 %d", fd);
                }
            }
        }
    }
}

/**
 * @brief WebSocket handshake handler (PATH:/ws)
 */
esp_err_t ws_open_handler(httpd_req_t *req)
{
    if (req->method == HTTP_GET) {
        ESP_LOGI(TAG, "WebSocket client handshake");
    }
    return ESP_OK;
}

/**
 * @brief Register WebSocket URI handler
 */
void register_ws(httpd_handle_t server)
{
    ws_server = server;
    
    httpd_uri_t ws_uri = {
        .uri = "/ws",
        .method = HTTP_GET,
        .handler = ws_open_handler,
        .is_websocket = true,
        .handle_ws_control_frames = true,
        .supported_subprotocol = "json"
    };
    
    esp_err_t ret = httpd_register_uri_handler(server, &ws_uri);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to register WebSocket URI: %s", esp_err_to_name(ret));
        return;
    }

    /* Create 200ms periodic timer for WebSocket broadcasts */
    const esp_timer_create_args_t timer_args = {
        .callback = &ws_push_timer_cb,
        .name = "ws_push"
    };
    
    ret = esp_timer_create(&timer_args, &ws_timer);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create WebSocket timer: %s", esp_err_to_name(ret));
        return;
    }

    ret = esp_timer_start_periodic(ws_timer, WS_TIMER_INTERVAL_US);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start WebSocket timer: %s", esp_err_to_name(ret));
        esp_timer_delete(ws_timer);
        ws_timer = NULL;
    }
}

/* ========================== HTTP Server Management ========================== */
/**
 * @brief Start HTTP server and register all URI handlers
 * @return Server handle on success, NULL on failure
 */
httpd_handle_t start_webserver(void)
{
    ESP_LOGI(TAG, "Starting HTTP server on port 80");

    /* Configure HTTP server */
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.stack_size = 4096;
    config.uri_match_fn = httpd_uri_match_wildcard;
    config.max_uri_handlers = 16;   

    /* Start server */
    httpd_handle_t server = NULL;
    if (httpd_start(&server, &config) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start HTTP server");
        return NULL;
    }

    /* Register new RESTful API endpoints */
    
    /* Power control endpoints */
    httpd_uri_t power_control_uri = {
        .uri = "/api/power-control",
        .method = HTTP_POST,
        .handler = power_control_handler
    };
    httpd_register_uri_handler(server, &power_control_uri);

    /* Protection configuration endpoints */
    httpd_uri_t protection_config_uri = {
        .uri = "/api/protection-config",
        .method = HTTP_POST,
        .handler = protection_config_handler
    };
    httpd_register_uri_handler(server, &protection_config_uri);

    httpd_uri_t protection_status_uri = {
        .uri = "/api/protection-status",
        .method = HTTP_GET,
        .handler = protection_status_handler
    };
    httpd_register_uri_handler(server, &protection_status_uri);


    /* FLTTMR configuration endpoints */
    httpd_uri_t flttmr_config_uri = {
        .uri = "/api/flttmr-config",
        .method = HTTP_POST,
        .handler = flttmr_config_handler
    };
    httpd_register_uri_handler(server, &flttmr_config_uri);

    httpd_uri_t flttmr_status_uri = {
        .uri = "/api/flttmr-status",
        .method = HTTP_GET,
        .handler = flttmr_status_handler
    };
    httpd_register_uri_handler(server, &flttmr_status_uri);

    /* SSSR configuration endpoints */
    httpd_uri_t sssr_config_uri = {
        .uri = "/api/sssr-config",
        .method = HTTP_POST,
        .handler = sssr_config_handler
    };
    httpd_register_uri_handler(server, &sssr_config_uri);

    httpd_uri_t sssr_status_uri = {
        .uri = "/api/sssr-status",
        .method = HTTP_GET,
        .handler = sssr_status_handler
    };
    httpd_register_uri_handler(server, &sssr_status_uri);

    /* System status endpoint */
    httpd_uri_t system_status_uri = {
        .uri = "/api/system-status",
        .method = HTTP_GET,
        .handler = system_status_handler
    };
    httpd_register_uri_handler(server, &system_status_uri);

    /* Register root handler */
    httpd_uri_t root_uri = {
        .uri = "/",
        .method = HTTP_GET,
        .handler = root_get_handler
    };
    httpd_register_uri_handler(server, &root_uri);

    /* Register WebSocket handler */
    register_ws(server);

    /* Register static file handler (must be last due to wildcard) */
    httpd_uri_t static_uri = {
        .uri = "/*",
        .method = HTTP_GET,
        .handler = static_get_handler
    };
    httpd_register_uri_handler(server, &static_uri);

    ESP_LOGI(TAG, "HTTP server started successfully");
    return server;
}

/**
 * @brief Format MAC address as string
 * @param mac MAC address array (6 bytes)
 * @param buf Buffer to store formatted string (must be at least 18 bytes)
 * @return Pointer to formatted string
 */
char *format_mac(const uint8_t mac[6], char buf[18])
{
    if (!mac || !buf) {
        return NULL;
    }
    
    snprintf(buf, 18, "%02X:%02X:%02X:%02X:%02X:%02X",
             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    return buf;
}

/**
 * @brief Wi-Fi event handler
 */
static void wifi_event_handler(void *arg, esp_event_base_t event_base,
                               int32_t event_id, void *event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_AP_STACONNECTED) {
        wifi_event_ap_staconnected_t *e = (wifi_event_ap_staconnected_t *)event_data;
        char mac_str[18];
        ESP_LOGI(TAG, "Station %s joined, AID=%d",
                 format_mac(e->mac, mac_str), e->aid);
        
        /* Start web server only once */
        static bool srv_started = false;
        if (!srv_started) {
            start_webserver();
            srv_started = true;
        }
    }
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_AP_STADISCONNECTED) {
        wifi_event_ap_staconnected_t *e = (wifi_event_ap_staconnected_t *)event_data;
        char mac_str[18];
        ESP_LOGI(TAG, "Station %s left, AID=%d",
                 format_mac(e->mac, mac_str), e->aid);
    }
    else if (event_base == IP_EVENT && event_id == IP_EVENT_AP_STAIPASSIGNED) {
        ip_event_ap_staipassigned_t *e = (ip_event_ap_staipassigned_t *)event_data;
        ESP_LOGI(TAG, "DHCP assigned IP=" IPSTR " to station", IP2STR(&e->ip));
    }
}

/**
 * @brief Initialize Wi-Fi soft AP mode
 */
void wifi_init_softap(const char ssid[32], const char pwd[32])
{
    /* Create default Wi-Fi AP netif */
    esp_netif_create_default_wifi_ap();

    /* Initialize Wi-Fi */
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    /* Register event handlers */
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT,
                                               ESP_EVENT_ANY_ID,
                                               &wifi_event_handler,
                                               NULL));
    
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT,
                                               IP_EVENT_AP_STAIPASSIGNED,
                                               &wifi_event_handler, NULL));

    /* Configure AP parameters */
    wifi_config_t wifi_config = {
        .ap = {
            .channel = 6,
            .max_connection = 4,
            .authmode = WIFI_AUTH_WPA_WPA2_PSK,
            .pairwise_cipher = WIFI_CIPHER_TYPE_CCMP
        },
    };
    strncpy((char *)wifi_config.ap.ssid, ssid, strlen(ssid));
    wifi_config.ap.ssid_len = strlen(ssid);
    strncpy((char *)wifi_config.ap.password, pwd, strlen(pwd));

    /* Set open auth if password is empty */
    if (strlen((char *)wifi_config.ap.password) == 0) {
        wifi_config.ap.authmode = WIFI_AUTH_OPEN;
    }

    /* Start Wi-Fi */
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "SoftAP initialized. SSID:%s password:%s",
             wifi_config.ap.ssid, wifi_config.ap.password);
}
