idf_component_register(
    SRCS "myFS.c"
    INCLUDE_DIRS "include"
    PRIV_REQUIRES littlefs nvs_flash esp_eth driver cjson)

littlefs_create_partition_image(littlefs_data data FLASH_IN_PROJECT)