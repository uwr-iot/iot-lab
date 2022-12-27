idf_component_register(
    SRCS    "app_main.c"

    INCLUDE_DIRS    ${CMAKE_SOURCE_DIR}/main
                    $ENV{IDF_PATH}/components/bt/include
                    $ENV{IDF_PATH}/components/bt/host/bluedroid/api/include
                    $ENV{IDF_PATH}/components/bt/host/nimble/nimble/nimble/host/include/host
                    $ENV{IDF_PATH}/components/driver/include

    REQUIRES soc nvs_flash driver bt console
)
