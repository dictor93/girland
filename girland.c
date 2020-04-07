/**
 * Example of ws2812_i2s library usage.
 *
 * This example shows light that travels in circle with fading tail.
 * As ws2812_i2s library using hardware I2S the output pin is GPIO3 and
 * can not be changed.
 *
 * This sample code is in the public domain.,
 */

#include <esp/uart.h>
#include "esp8266.h"
#include "espressif/esp_common.h"

#include <math.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "HttpServer.h"
#include "Wifi.h"
#include "Render.H"

// #include "spiffs.h"
// #include "esp_spiffs.h"



// void startAp() {
//     sdk_wifi_set_opmode(SOFTAP_MODE);
//     struct ip_info ap_ip;
//     IP4_ADDR(&ap_ip.ip, 192, 168, 0, 1);
//     IP4_ADDR(&ap_ip.gw, 0, 0, 0, 0);
//     IP4_ADDR(&ap_ip.netmask, 255, 255, 0, 0);
//     sdk_wifi_set_ip_info(1, &ap_ip);

//     struct sdk_softap_config ap_config = {
//         .ssid = "girland",
//         .ssid_hidden = 0,
//         .channel = 3,
//         .ssid_len = strlen("girland"),
//         .authmode = AUTH_WPA_WPA2_PSK,
//         .password = "12345678",
//         .max_connection = 3,
//         .beacon_interval = 100,
//     };
//     sdk_wifi_softap_set_config(&ap_config);
// }

// void removeConfig() {
//     printf("Remooving config");
//     SPIFFS_remove(&fs, "ssid.txt");
//     SPIFFS_remove(&fs, "pwd.txt");
// }

// void onConnFault() {
//     connectionFault = true;
//     removeConfig();
//     sdk_system_restart();
// }

// void saveAuthData(char*ssid, char*pwd) {
//     char *ssidbuf = ssid;
//     char *pwdbuf = pwd;
//      printf("SVE SSID %s, PWD %s", ssidbuf, pwdbuf);
//     int fd = open("ssid.txt", O_WRONLY|O_CREAT, 0);
//     if (fd < 0) {
//         printf("Error opening file\n");
//         return;
//     }

//     int written = write(fd, ssidbuf, 0xF);
//     printf("Written %d bytes\n", written);

//     fd = open("pwd.txt", O_WRONLY|O_CREAT, 0);
//     if (fd < 0) {
//         printf("Error opening file\n");
//         return;
//     }

//     written = write(fd, pwdbuf, 0xF);
//     printf("Written %d bytes\n", written);

//     close(fd);
// }

// bool getAuthData() {
//     int fileLength = 0xF;
//     char ssidbuf[fileLength];
//     char pwdbuf[fileLength];

//     spiffs_file fd = SPIFFS_open(&fs, "ssid.txt", SPIFFS_RDONLY, 0);
//     if (fd < 0) {
//         printf("Error opening file\n");
//         return false;
//     }

//     int read_bytes = SPIFFS_read(&fs, fd, ssidbuf, fileLength);
//     ssidbuf[read_bytes] = '\0';
//     printf("Ssid: %s\n", ssidbuf);
//     SPIFFS_close(&fs, fd);

//     fd = SPIFFS_open(&fs, "pwd.txt", SPIFFS_RDONLY, 0);
//     if (fd < 0) {
//         printf("Error opening file\n");
//         return false;
//     }
//     if(read_bytes < 2){
//         return false;
//     }
//     read_bytes = SPIFFS_read(&fs, fd, pwdbuf, fileLength);
//     pwdbuf[read_bytes] = '\0';
//     printf("Pwd: %s\n", pwdbuf);
//     SPIFFS_close(&fs, fd);
//     if(read_bytes < 2){
//         return false;
//     } else {
//         struct sdk_station_config config;
//         memcpy(config.ssid, ssidbuf, strlen(ssidbuf));
//         config.ssid[strlen(ssidbuf)] = '\0';
//         memcpy(config.password, pwdbuf, strlen(pwdbuf));
//         config.ssid[strlen(pwdbuf)] = '\0';
//         printf("Config at getAuthData: %s, %s\n", config.ssid,
//         config.password); sdk_wifi_station_set_config(&config); return true;
//     }
// }

// void chaekSettings() {
//     esp_spiffs_init();
//     if (esp_spiffs_mount() != SPIFFS_OK) {
//         printf("Error mount SPIFFS\n");
//     }
//     if(getAuthData()) {
//         connect();
//     } else {
//         startAp();
//     }

// }

void user_init(void) {
    uart_set_baud(0, 115200);

    struct ip_info ipInfo;
    /* required to call wifi_set_opmode before station_set_config */
    // sdk_wifi_set_opmode(STATIONAP_MODE);

    Wifi_init();
    HttpServer_init();
    Render_init();
    // chaekSettings();

}
