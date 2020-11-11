#include "espressif/esp_common.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "LocalConfig.h"
#include "string.h"
#include "Fs.h"

bool s_isConnectionFault = false;

static void Wifi_connect() {
    sdk_wifi_set_opmode(STATION_MODE);
    sdk_wifi_station_set_auto_connect(1);
    // printf("SSID: %s, PWD: %s", config.ssid, config.password);
    Wifi_resetConfig();
}

void Wifi_resetConfig() {
    sdk_wifi_station_disconnect();
    char *credsBuffer[64];
    Fs_readFile("creds", credsBuffer, 64);
    char *pwd = strstr(credsBuffer, "PWD=")+4;
    char *pwdEnd = strstr(pwd, "\n");
    char *ssid = strstr(credsBuffer, "SSID=")+5;
    char *ssidEnd = strstr(ssid, "\n");

    if(pwdEnd) {
        pwdEnd[0] = '\0';
    }
    ssidEnd[0] = '\0';

    struct sdk_station_config config = {
        .ssid = malloc(strlen(ssid)),
        .password = malloc(strlen(pwd)),
    };

    strcpy(config.ssid, ssid);
    strcpy(config.password, pwd);

    sdk_wifi_station_set_config(&config);
    sdk_wifi_station_connect();
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
                Wifi_statusDecider(l_stationStatus);
            }
        }
        vTaskDelay(10000 / portTICK_PERIOD_MS);
    }
}

void Wifi_init() {
    Wifi_connect();
    xTaskCreate(&Wifi_checkStatusTask, "main-task", 1024, NULL, 10, NULL);
}
