#include <string.h>

#include <esp_log.h>
#include <nvs_flash.h>
#include <esp_wifi.h>
#include <esp_event.h>
#include <esp_spiffs.h>
#include <esp_http_server.h>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/event_groups.h>

#include <driver/gpio.h>

#define WIFI_CONNECTED_FLAG     BIT0

static struct {
    struct {
        EventGroupHandle_t wifi;
    } event_groups;

    char response_buffer[4096];
} ctx = {
    .response_buffer = { 0 }
};

static void WaitMs(unsigned delay) {
    vTaskDelay(delay / portTICK_PERIOD_MS);
}

static void OnWiFiStackEvent(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data) {
    if (event_base != WIFI_EVENT)
        return;

    switch (event_id) {
        case WIFI_EVENT_STA_START:
        case WIFI_EVENT_STA_DISCONNECTED:
            esp_wifi_connect();
            break;
        default:
            break;
    }
}

static void OnIpStackEvent(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data) {
    if (event_base != IP_EVENT)
        return;

    if (event_id != IP_EVENT_STA_GOT_IP)
        return;

    ip_event_got_ip_t *event = (ip_event_got_ip_t*) event_data;
    ESP_LOGI("WIFI", "AP address:" IPSTR, IP2STR(&event->ip_info.ip));
    xEventGroupSetBits(ctx.event_groups.wifi, WIFI_CONNECTED_FLAG);
}

void ConnectWiFi(char *ssid, char *password) {
    esp_netif_init();

    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&cfg);

    esp_event_handler_instance_t wifi_handler_instance;
    esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID,
                                        &OnWiFiStackEvent, NULL, &wifi_handler_instance);

    esp_event_handler_instance_t ip_handler_instance;
    esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP,
                                        &OnIpStackEvent, NULL, &ip_handler_instance);

    esp_wifi_set_mode(WIFI_MODE_STA);

    wifi_config_t wifi_config = {
        .sta = {
            .threshold.authmode = WIFI_AUTH_WPA2_PSK,
        },
    };

    strncpy((char*)wifi_config.sta.ssid, ssid, sizeof(wifi_config.sta.ssid));
    strncpy((char*)wifi_config.sta.password, password, sizeof(wifi_config.sta.password));
    esp_wifi_set_config(WIFI_IF_STA, &wifi_config);

    esp_wifi_start();
}

static bool WaitForConnection(void) {
    EventBits_t flags = xEventGroupWaitBits(ctx.event_groups.wifi, WIFI_CONNECTED_FLAG,
                                            pdFALSE, pdFALSE, portMAX_DELAY);
    bool connected = false;
    if (flags & WIFI_CONNECTED_FLAG)
        connected = true;

    ESP_LOGI("WIFI", "%s", connected ? "Connected" : "Connection failed");

    vEventGroupDelete(ctx.event_groups.wifi);
    return connected;
}

static void LoadFile(const char *filename, char *buffer, size_t buffer_size) {
    memset(buffer, 0, buffer_size);

    FILE *file = fopen(filename, "r");
    if (file == NULL) {
        ESP_LOGE("FS", "File %s not found", filename);
        return;
    }

    if (fread(buffer, 1, buffer_size, file) == 0)
        ESP_LOGE("FS", "Can't read file %s", filename);

    fclose(file);
}

static esp_err_t GetMainPage(httpd_req_t *request) {
    LoadFile("/www/index.html", ctx.response_buffer, sizeof(ctx.response_buffer));
    return httpd_resp_send(request, ctx.response_buffer, HTTPD_RESP_USE_STRLEN);
}

static bool CreateWWWServer(httpd_handle_t *server) {
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();

    httpd_uri_t uri_get = {
        .uri = "/",
        .method = HTTP_GET,
        .handler = GetMainPage,
        .user_ctx = NULL
    };

    if (httpd_start(server, &config) == ESP_OK)
        httpd_register_uri_handler(*server, &uri_get);

    return true;
}

static void InitializeFilesystem(void) {
    esp_vfs_spiffs_conf_t fs_config = {
        .base_path = "/www",
        .partition_label = "www_data",
        .max_files = 3,
        .format_if_mount_failed = false
    };

    esp_vfs_spiffs_register(&fs_config);
}

static void InitializeNVS(void) {
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        nvs_flash_erase();
        nvs_flash_init();
    }
}

void app_main(void) {
    InitializeNVS();
    InitializeFilesystem();

    esp_event_loop_create_default();
    ctx.event_groups.wifi = xEventGroupCreate();

    ConnectWiFi("some-ssid", "some-password");

    bool connected = WaitForConnection();
    if (connected == false)
        goto error;

    httpd_handle_t server;
    bool running = CreateWWWServer(&server);
    if (running == false)
        goto error;

error:
    while (1) {
        WaitMs(1000);
    };
}
