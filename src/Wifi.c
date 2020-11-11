#include "espressif/esp_common.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "LocalConfig.h"
#include "string.h"
#include "Fs.h"
#include "Wifi.h"

bool s_isConnectionFault = false;

static void Wifi_connect() {
    
    // printf("SSID: %s, PWD: %s", config.ssid, config.password);
    Wifi_resetConfig();
}

void Wifi_resetConfig() {
    sdk_wifi_station_disconnect();
    sdk_wifi_set_opmode(STATION_MODE);
    sdk_wifi_station_set_auto_connect(1);
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

static void Wifi_startAp() {
    sdk_wifi_station_disconnect();
    sdk_wifi_set_opmode(SOFTAP_MODE);
    struct ip_info ap_ip;
    IP4_ADDR(&ap_ip.ip, 192, 168, 0, 1);
    IP4_ADDR(&ap_ip.gw, 0, 0, 0, 0);
    IP4_ADDR(&ap_ip.netmask, 255, 255, 0, 0);
    sdk_wifi_set_ip_info(1, &ap_ip);

    struct sdk_softap_config ap_config = {
        .ssid = AP_SSID,
        .ssid_hidden = 0,
        .channel = 3,
        .ssid_len = strlen(AP_SSID),
        .authmode = AUTH_WPA_WPA2_PSK,
        .password = AP_PSK,
        .max_connection = 3,
        .beacon_interval = 100,
    };
    sdk_wifi_softap_set_config(&ap_config);
}

static void onConnFault() {
    s_isConnectionFault = true;
    Wifi_startAp();
    printf("Connection aborted\nStarting AP");
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
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}

void Wifi_init() {
    Wifi_startAp();
    xTaskCreate(&Wifi_checkStatusTask, "main-task", 1024, NULL, 10, NULL);
}
