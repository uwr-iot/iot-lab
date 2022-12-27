#include <esp_log.h>
#include <nvs_flash.h>

#include <esp_nimble_hci.h>
#include <driver/gpio.h>

#include <nimble/nimble_port.h>
#include <nimble/nimble_port_freertos.h>
#include <host/ble_hs.h>
#include <host/util/util.h>

#include <services/gap/ble_svc_gap.h>
#include <services/gatt/ble_svc_gatt.h>

// Links:
// - https://mynewt.apache.org/latest/tutorials/ble/bleprph/bleprph-sections/bleprph-adv.html
// - file://esp-idf-v4.4.2/examples/peripherals/gpio/generic_gpio/main/gpio_example_main.c
// - file://esp-idf-v4.4.2/components/esp_hw_support/include/esp_random.h
// - file://esp-idf-v4.4.2/examples/bluetooth/nimble/blehr/main/gatt_svr.c


// Heart-rate BLE identifiers
#define GATT_HRS_UUID                           0x180D
#define GATT_HRS_MEASUREMENT_UUID               0x2A37
#define GATT_HRS_BODY_SENSOR_LOC_UUID           0x2A38


static int GetHeartRateValue(uint16_t conn_handle, uint16_t attr_handle, struct ble_gatt_access_ctxt *ctxt, void *arg) {
    static struct __attribute__((packed)) {
        uint8_t contact;
        uint8_t heart_rate;
    } hrm = {
        .contact = 0x06,
        .heart_rate = 80
    };

    int rc = os_mbuf_append(ctxt->om, &hrm, sizeof(hrm));
    return rc == 0 ? 0 : BLE_ATT_ERR_INSUFFICIENT_RES;
}

static int GetBodyLocation(uint16_t conn_handle, uint16_t attr_handle, struct ble_gatt_access_ctxt *ctxt, void *arg) {
    static const uint8_t chest = 0x01;

    int rc = os_mbuf_append(ctxt->om, &chest, sizeof(chest));
    return rc == 0 ? 0 : BLE_ATT_ERR_INSUFFICIENT_RES;
}

static const struct ble_gatt_svc_def kBleServices[] = {
    {
        .type = BLE_GATT_SVC_TYPE_PRIMARY,
        .uuid = BLE_UUID16_DECLARE(GATT_HRS_UUID),
        .characteristics = (struct ble_gatt_chr_def[])
        { {
                // characteristic: Heart-rate measurement
                .uuid = BLE_UUID16_DECLARE(GATT_HRS_MEASUREMENT_UUID),
                .access_cb = GetHeartRateValue,
                .flags = BLE_GATT_CHR_F_READ,
            }, {
                // characteristic: Body sensor location
                .uuid = BLE_UUID16_DECLARE(GATT_HRS_BODY_SENSOR_LOC_UUID),
                .access_cb = GetBodyLocation,
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
            break;

        case BLE_GAP_EVENT_DISCONNECT:
            ESP_LOGI("BLE GAP Event", "Disconnected");
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

static void StartBleService(void *param) {
    ESP_LOGI("BLE task", "BLE Host Task Started");

    nimble_port_run();
    nimble_port_freertos_deinit();
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

    // Make device discoverable
    StartAdvertisement();

error:
    while (1) {
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    };
}
