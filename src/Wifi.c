#include "espressif/esp_common.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

#include "Wifi.h"

#define SSID "Jesus"
#define PWD "8715387153"

bool s_isConnectionFault = false;

static void Wifi_connect() {
    sdk_wifi_set_opmode(STATION_MODE);
    sdk_wifi_station_set_auto_connect(1);
    struct sdk_station_config config = {
        .ssid = SSID,
        .password = PWD,
    };
    sdk_wifi_station_set_config(&config);
    // printf("SSID: %s, PWD: %s", config.ssid, config.password);
}

static void onConnFault() {
    s_isConnectionFault = true;
    printf("Connection aborted\n");
}

static void Wifi_statusDecider(uint8_t status) {
    switch (status) {
    case STATION_CONNECTING:
        printf(".");
        break;
    case STATION_WRONG_PASSWORD:
        printf("WRONG PASSWORD\n");
        onConnFault();
        break;
    case STATION_NO_AP_FOUND:
        printf("NO AP FOUND\n");
        onConnFault();
        break;
    case STATION_CONNECT_FAIL:
        printf("CONNECT FAIL\n");
        onConnFault();
        break;
    case STATION_GOT_IP:
        printf("GOT IP\n");
        break;
    default:
        printf("\nStatus undefined: %d\n", status);
        // sdk_system_restart();
        break;
    }
}

static void Wifi_checkStatusTask(void *parameters) {
    uint8_t l_stationStatus = sdk_wifi_station_get_connect_status();
    while (1) {
        if (!s_isConnectionFault) {
            l_stationStatus = sdk_wifi_station_get_connect_status();
            if (l_stationStatus != STATION_GOT_IP) {
                Wifi_checkConnection(l_stationStatus);
            }
        }
        vTaskDelay(10000 / portTICK_PERIOD_MS);
    }
}

void Wifi_init() {
    Wifi_connect();
    xTaskCreate(&Wifi_checkTask, "main-task", 1024, NULL, 10, NULL);
}