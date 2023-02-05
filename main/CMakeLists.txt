idf_component_register(
    SRCS    "app_main.c"

    INCLUDE_DIRS    ${CMAKE_SOURCE_DIR}/main
                    $ENV{IDF_PATH}/components/driver/include
                    $ENV{IDF_PATH}/components/esp_http_server/include
                    $ENV{IDF_PATH}/components/nghttp/port/include
                    $ENV{IDF_PATH}/components/spiffs/include

    REQUIRES soc nvs_flash driver console
    PRIV_REQUIRES esp_netif driver esp_wifi vfs spiffs
)

spiffs_create_partition_image(www_data www FLASH_IN_PROJECT)