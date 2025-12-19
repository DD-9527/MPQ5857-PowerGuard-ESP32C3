idf_component_register(
    SRCS "http_server.c"
    INCLUDE_DIRS "include"
    PRIV_REQUIRES esp_timer  esp_https_server esp_wifi adc mpq5857
                  esp_eth driver)
