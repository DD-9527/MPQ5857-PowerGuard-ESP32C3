#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "myFS.h"
#include "cJSON.h"

#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_littlefs.h"

static const char *TAG = "myFS";

#define CONFIG_FILE_PATH "/littlefs/config.json"

bool save_params(const params_t *p)
{
    if (!p) {
        ESP_LOGE(TAG, "Invalid parameters");
        return false;
    }

    cJSON *root = cJSON_CreateObject();
    if (!root) {
        ESP_LOGE(TAG, "Failed to create JSON object");
        return false;
    }

    cJSON_AddStringToObject(root, "wifi_ssid", p->wifi_ssid);
    cJSON_AddStringToObject(root, "wifi_pwd", p->wifi_pwd);
    cJSON_AddNumberToObject(root, "registers", p->registers);

    char *json = cJSON_Print(root);
    if (!json) {
        ESP_LOGE(TAG, "Failed to print JSON");
        cJSON_Delete(root);
        return false;
    }

    FILE *f = fopen(CONFIG_FILE_PATH, "w");
    if (!f) {
        ESP_LOGE(TAG, "Failed to open config file for writing");
        free(json);
        cJSON_Delete(root);
        return false;
    }

    size_t length = strlen(json);
    size_t written = fwrite(json, 1, length, f);
    fclose(f);
    free(json);
    cJSON_Delete(root);

    if (written != length) {
        ESP_LOGE(TAG, "Failed to write complete config file");
        return false;
    }

    ESP_LOGI(TAG, "Parameters saved successfully");
    return true;
}

bool load_params(params_t *p)
{
    if (!p) {
        ESP_LOGE(TAG, "Invalid parameters");
        return false;
    }

    FILE *f = fopen(CONFIG_FILE_PATH, "r");
    if (!f) {
        ESP_LOGW(TAG, "Config file not found");
        return false;
    }

    /* Get file size */
    if (fseek(f, 0, SEEK_END) != 0) {
        ESP_LOGE(TAG, "Failed to seek to end of file");
        fclose(f);
        return false;
    }

    long len = ftell(f);
    if (len <= 0 || len > 4096) {  /* Reasonable size limit */
        ESP_LOGE(TAG, "Invalid file size: %ld", len);
        fclose(f);
        return false;
    }

    if (fseek(f, 0, SEEK_SET) != 0) {
        ESP_LOGE(TAG, "Failed to seek to beginning of file");
        fclose(f);
        return false;
    }

    char *buf = malloc(len + 1);
    if (!buf) {
        ESP_LOGE(TAG, "Failed to allocate memory for config file");
        fclose(f);
        return false;
    }

    size_t read_size = fread(buf, 1, len, f);
    fclose(f);

    if (read_size != (size_t)len) {
        ESP_LOGE(TAG, "Failed to read complete config file");
        free(buf);
        return false;
    }

    buf[len] = '\0';

    cJSON *root = cJSON_Parse(buf);
    free(buf);

    if (!root) {
        ESP_LOGE(TAG, "Failed to parse JSON config");
        return false;
    }

    cJSON *wifi_ssid_item = cJSON_GetObjectItem(root, "wifi_ssid");
    cJSON *wifi_pwd_item = cJSON_GetObjectItem(root, "wifi_pwd");
    cJSON *registers_item = cJSON_GetObjectItem(root, "registers");

    if (!wifi_ssid_item || !wifi_pwd_item || !registers_item) {
        ESP_LOGE(TAG, "Missing required config fields");
        cJSON_Delete(root);
        return false;
    }

    if (!cJSON_IsString(wifi_ssid_item)|| !cJSON_IsString(wifi_pwd_item) || !cJSON_IsNumber(registers_item) ) {
        ESP_LOGE(TAG, "Invalid config field types");
        cJSON_Delete(root);
        return false;
    }

    strncpy(p->wifi_ssid, wifi_ssid_item->valuestring, sizeof(p->wifi_ssid) - 1);
    p->wifi_ssid[sizeof(p->wifi_ssid) - 1] = '\0';
    
    p->registers = registers_item->valueint;
    
    strncpy(p->wifi_pwd, wifi_pwd_item->valuestring, sizeof(p->wifi_pwd) - 1);
    p->wifi_pwd[sizeof(p->wifi_pwd) - 1] = '\0';

    cJSON_Delete(root);
    ESP_LOGI(TAG, "Parameters loaded successfully");
    return true;
}

void FS_Init(void)
{
    /* Initialize NVS flash */
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    /* Mount littleFS */
    esp_vfs_littlefs_conf_t littlefs_conf = {
        .base_path = "/littlefs",
        .partition_label = "littlefs_data",
        .format_if_mount_failed = true,
        .dont_mount = false,
        .grow_on_mount = true,
    };

    ret = esp_vfs_littlefs_register(&littlefs_conf);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to mount littleFS: %s", esp_err_to_name(ret));
        return;
    }

    /* Get filesystem usage info */
    size_t total = 0, used = 0;
    ret = esp_littlefs_info(littlefs_conf.partition_label, &total, &used);
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "LittleFS mounted: %zu/%zu bytes used (%.1f%%)",
                 used, total, (float)used / total * 100.0f);
    } else {
        ESP_LOGW(TAG, "Failed to get filesystem info: %s", esp_err_to_name(ret));
    }
}