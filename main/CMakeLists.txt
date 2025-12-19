idf_component_register(SRCS "main.c"
                       INCLUDE_DIRS "."
                       PRIV_REQUIRES esp_timer littlefs esp_https_server esp_wifi nvs_flash esp_eth driver cjson)
