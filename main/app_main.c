#include <esp_log.h>
#include <nvs_flash.h>

#include <esp_nimble_hci.h>
#include <driver/gpio.h>
#include <driver/i2c.h>

#include <nimble/nimble_port.h>
#include <nimble/nimble_port_freertos.h>
#include <host/ble_hs.h>
#include <host/util/util.h>

#include <services/gap/ble_svc_gap.h>
#include <services/gatt/ble_svc_gatt.h>


// BLE specification: Assigned Numbers
// link: https://btprodspecificationrefs.blob.core.windows.net/assigned-numbers/Assigned%20Number%20Types/Assigned%20Numbers.pdf
//
// This is a regularly updated document listing assigned numbers, codes, and
// identifiers in the Bluetooth specifications.

// BLE specification: GATT Specification Supplement 5
// link: https://www.bluetooth.org/DocMan/handlers/DownloadDoc.ashx?doc_id=524815
//
// This specification contains the normative definitions for all GATT characteristics and
// characteristic descriptors, with the exception of those defined in the Bluetooth Core Specification
// or in Bluetooth Service specifications.

// ESP-IDF
// I2C documentation: https://docs.espressif.com/projects/esp-idf/en/latest/esp32s3/api-reference/peripherals/i2c.html
// I2C examples: esp-idf-v4.4.2/examples/peripherals/i2c/i2c_simple
// BLE documentation: https://docs.espressif.com/projects/esp-idf/en/latest/esp32s3/api-reference/bluetooth/nimble/index.html
// BLE examples: esp-idf-v4.4.2/examples/bluetooth/nimble

// AHT20 sensor
// website: http://www.aosong.com/en/products-32.html
// documentation: https://files.seeedstudio.com/wiki/Grove-AHT20_I2C_Industrial_Grade_Temperature_and_Humidity_Sensor/AHT20-datasheet-2020-4-16.pdf
// I2C example: esp-idf-v4.4.2/examples/peripherals/i2c/i2c_simple/main/i2c_simple_main.c
// 3rdParty library: https://github.com/adafruit/Adafruit_AHTX0

// Tools
// Saleae: https://www.saleae.com


// Hardware configuration
#define LED_GPIO_PIN                            GPIO_NUM_1

#define I2C_PORT_NUMBER                         I2C_NUM_0
#define I2C_CLK_FREQUENCY                       100000
#define I2C_SDA_PIN                             GPIO_NUM_4
#define I2C_SCL_PIN                             GPIO_NUM_5
#define I2C_TIMEOUT                             (100 / portTICK_RATE_MS)
#define I2C_AHT20_ADDRESS                       0x38


// Bluetooth configuration (Environmental Sensing Service)
#define GATT_ESS_UUID                           0x181A
#define GATT_ESS_TEMPERATURE_UUID               0x2A6E
#define GATT_ESS_HUMIDITY_UUID                  0x2A6F


static void SetLedState(bool state) {
    gpio_set_direction(LED_GPIO_PIN, GPIO_MODE_OUTPUT);
    gpio_set_level(LED_GPIO_PIN, state ? 1 : 0);
}

static void WaitMs(unsigned delay) {
    vTaskDelay(delay / portTICK_PERIOD_MS);
}


static int GetTemperature(uint16_t conn_handle, uint16_t attr_handle, struct ble_gatt_access_ctxt *ctxt, void *arg) {
    // int rc = os_mbuf_append(ctxt->om, ...);
    // return rc == 0 ? 0 : BLE_ATT_ERR_INSUFFICIENT_RES;
    return BLE_ATT_ERR_INSUFFICIENT_RES;
}

static int GetHumidity(uint16_t conn_handle, uint16_t attr_handle, struct ble_gatt_access_ctxt *ctxt, void *arg) {
    // int rc = os_mbuf_append(ctxt->om, ...);
    // return rc == 0 ? 0 : BLE_ATT_ERR_INSUFFICIENT_RES;
    return BLE_ATT_ERR_INSUFFICIENT_RES;
}

static const struct ble_gatt_svc_def kBleServices[] = {
    {
        .type = BLE_GATT_SVC_TYPE_PRIMARY,
        .uuid = BLE_UUID16_DECLARE(GATT_ESS_UUID),
        .characteristics = (struct ble_gatt_chr_def[])
        { {
                .uuid = BLE_UUID16_DECLARE(GATT_ESS_TEMPERATURE_UUID),
                .access_cb = GetTemperature,
                .flags = BLE_GATT_CHR_F_READ,
            }, {
                .uuid = BLE_UUID16_DECLARE(GATT_ESS_HUMIDITY_UUID),
                .access_cb = GetHumidity,
                .flags = BLE_GATT_CHR_F_READ,
            }, {
                0,  // no more characteristics
            },
        }
    }, {
        0,  // no more services
    },
};


static void StartAdvertisement(void);

static int OnBleGapEvent(struct ble_gap_event *event, void *arg) {
    switch (event->type) {
        case BLE_GAP_EVENT_CONNECT:
            ESP_LOGI("BLE GAP Event", "Connected");
            SetLedState(true);
            break;

        case BLE_GAP_EVENT_DISCONNECT:
            ESP_LOGI("BLE GAP Event", "Disconnected");
            SetLedState(false);
            StartAdvertisement();
            break;

        default:
            ESP_LOGI("BLE GAP Event", "Type: 0x%02X", event->type);
            break;
    }

    return 0;
}

static void StartAdvertisement(void) {
    struct ble_gap_adv_params adv_parameters;
    memset(&adv_parameters, 0, sizeof(adv_parameters));

    adv_parameters.conn_mode = BLE_GAP_CONN_MODE_UND;
    adv_parameters.disc_mode = BLE_GAP_DISC_MODE_GEN;

    if (ble_gap_adv_start(BLE_OWN_ADDR_PUBLIC, NULL, BLE_HS_FOREVER,
                         &adv_parameters,
                         OnBleGapEvent, NULL) != 0) {
        ESP_LOGE("BLE", "Can't start Advertisement");
        return;
    }

    ESP_LOGI("BLE", "Advertisement started...");
}

static void SetDeviceName(const char *device_name) {
    struct ble_hs_adv_fields fields;
    memset(&fields, 0, sizeof(fields));

    fields.flags = BLE_HS_ADV_F_DISC_GEN | BLE_HS_ADV_F_BREDR_UNSUP;

    fields.name = (uint8_t*) device_name;
    fields.name_len = strlen(device_name);
    fields.name_is_complete = 1;

    if (ble_gap_adv_set_fields(&fields) != 0) {
        ESP_LOGE("BLE", "Can't configure BLE advertisement fields");
        return;
    }

    ble_svc_gap_device_name_set(device_name);
}

static void StartBleService(void *param) {
    ESP_LOGI("BLE task", "BLE Host Task Started");

    nimble_port_run();
    nimble_port_freertos_deinit();
}

static void InitializeI2C(void) {
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = I2C_SDA_PIN,
        .scl_io_num = I2C_SCL_PIN,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = I2C_CLK_FREQUENCY,
    };

    i2c_param_config(I2C_PORT_NUMBER, &conf);
    i2c_driver_install(I2C_PORT_NUMBER, conf.mode, 0, 0, 0);
}

static void WriteToTheSensor(uint8_t *data, size_t length) {
    i2c_master_write_to_device(I2C_PORT_NUMBER, I2C_AHT20_ADDRESS, data, length, I2C_TIMEOUT);
}

static void ReadFromTheSensor(uint8_t *buffer, size_t length) {
    i2c_master_read_from_device(I2C_PORT_NUMBER, I2C_AHT20_ADDRESS, buffer, length, I2C_TIMEOUT);
}






void app_main(void) {
    // Initialize Non-Volatile Memory
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_LOGI("NVS", "Initializing NVS...");
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }

    // Initialize BLE peripheral
    esp_nimble_hci_and_controller_init();
    nimble_port_init();

    // Initialize BLE library (nimble)
    ble_svc_gap_init();
    ble_svc_gatt_init();

    // Configure BLE library (nimble)
    int rc = ble_gatts_count_cfg(kBleServices);
    if (rc != 0) {
        ESP_LOGE("BLE GATT", "Service registration failed");
        goto error;
    }

    rc = ble_gatts_add_svcs(kBleServices);
    if (rc != 0) {
        ESP_LOGE("BLE GATT", "Service registration failed");
        goto error;
    }

    // Run BLE
    nimble_port_freertos_init(StartBleService);

    SetDeviceName("AHT20 sensor");
    StartAdvertisement();

    InitializeI2C();


error:
    while (1) {
        WaitMs(1000);
    };
}
