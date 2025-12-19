#ifndef HTTP_SERVER_H
#define HTTP_SERVER_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#include "esp_err.h"
#include "esp_http_server.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ========================== Configuration ========================== */
#define PATH_MAX_LEN 1024U  /**< Maximum static file path length */

/* ========================== HTTP Request Handlers ========================== */
/**
 * @brief GET / handler - Returns /littlefs/index.html
 */
esp_err_t root_get_handler(httpd_req_t *req);

/**
 * @brief Static resources handler - Supports css/js/svg/json/etc.
 */
esp_err_t static_get_handler(httpd_req_t *req);

/**
 * @brief POST /api/power-control handler - Control power output
 *        Request body: {"enabled":true}
 *        Response: {"enabled":true,"success":true}
 */
esp_err_t power_control_handler(httpd_req_t *req);

/**
 * @brief POST /api/protection-config handler - Configure protection settings
 *        Expected: {"channel":1,"threshold":75,"action":1}
 *        Response: {"channel":1,"threshold":75,"action":1,"success":true}
 */
esp_err_t protection_config_handler(httpd_req_t *req);

/**
 * @brief GET /api/protection-status handler - Get protection configuration
 *        Response: {"channels":[{"channel":1,"type":"OCP","threshold":75,"action":1}]}
 */
esp_err_t protection_status_handler(httpd_req_t *req);

/**
 * @brief POST /api/flttmr-config handler - Used for setting the deglitch time before shutoff
 *        Expected: {"slope":0}
 *        Response: {"slope":0,"description":"2ms","success":true}
 */
esp_err_t flttmr_config_handler(httpd_req_t *req);

/**
 * @brief GET /api/flttmr-status handler - Get FLTTMR configuration
 *        Response: {"slope":0,"description":"2ms","options":[...]}
 */
esp_err_t flttmr_status_handler(httpd_req_t *req);

/**
 * @brief POST /api/sssr-config handler - Configure soft start slope
 *        Expected: {"slope":0}
 *        Response: {"slope":0,"description":"0.25V/ms","success":true}
 */
esp_err_t sssr_config_handler(httpd_req_t *req);

/**
 * @brief GET /api/sssr-status handler - Get SSSR configuration
 *        Response: {"slope":0,"description":"0.25V/ms","options":[...]}
 */
esp_err_t sssr_status_handler(httpd_req_t *req);

/**
 * @brief GET /api/system-status handler - Get complete system status
 *        Response: Complete system status including all subsystems
 */
esp_err_t system_status_handler(httpd_req_t *req);

/* ========================== WebSocket Handlers ========================== */
/**
 * @brief WebSocket handshake handler (PATH:/ws)
 */
esp_err_t ws_open_handler(httpd_req_t *req);

/**
 * @brief Register WebSocket URI handler
 */
void register_ws(httpd_handle_t server);

/* ========================== HTTP Server Management ========================== */
/**
 * @brief Start HTTP server and register all URI handlers
 * @return Server handle on success, NULL on failure
 */
httpd_handle_t start_webserver(void);

/**
 * @brief Format MAC address as string
 * @param mac MAC address array (6 bytes)
 * @param buf Buffer to store formatted string (must be at least 18 bytes)
 * @return Pointer to formatted string, NULL on invalid parameters
 */
char *format_mac(const uint8_t mac[6], char buf[18]);

/**
 * @brief Initialize Wi-Fi soft AP mode
 */
void wifi_init_softap(const char ssid[32], const char pwd[32]);

#ifdef __cplusplus
}
#endif

#endif /* HTTP_SERVER_H */