/**
 * Example of ws2812_i2s library usage.
 *
 * This example shows light that travels in circle with fading tail.
 * As ws2812_i2s library using hardware I2S the output pin is GPIO3 and
 * can not be changed.
 *
 * This sample code is in the public domain.,
 */
#include <FreeRTOS.h>
#include <esp/uart.h>
#include "esp8266.h"
#include "espressif/esp_common.h"
#include "queue.h"
#include "task.h"
#include <math.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <ws2812_i2s/ws2812_i2s.h>

#include "Color.h"
#include "HttpServer.h"
#include "Wifi.h"

// #include "spiffs.h"
// #include "esp_spiffs.h"

#define HEIGHT 16
#define WIDTH 16

#define LED_NUMBER WIDTH *HEIGHT
#define RENDER_FREQ 30

#define SNOW_COLOR 0xB900FF

#define FORWARD true
#define BACKWARD false
#define NOT_MIRROR false
#define MIRROR true
#define IS_RAINBOW_VERTICAL true
#define IS_RAINBOW_HORISONTAL false

#define TORNADO_TAIL_LENGTH 8

#define SNOW_LIFE 30 * 2 // 5sec;
#define SNOW_NUMBER 15

QueueHandle_t render_queue;

struct snow_item {
    int maxAge;
    int age;
    int x;
    int y;
};

void generateRainbow(int frame, ws2812_pixel_t *pixels, bool vertical,
                     int speed, bool direction) {
    int ledsInGradient = vertical ? HEIGHT : WIDTH;
    hsv_t data;
    for (int x = 0; x < WIDTH; x++) {
        int locX = direction ? x : WIDTH - x - 1;
        int pos = 0;
        if (!vertical)
            pos = (int)(((double)frame) * speed +
                        (double)locX * 360 / ledsInGradient) %
                  360;
        for (int y = 0; y < HEIGHT; y++) {
            int locY = (locX % 2) ? y : HEIGHT - y - 1;
            locY = direction ? locY : HEIGHT - locY - 1;
            if (vertical)
                pos = (int)(((double)frame) * speed +
                            (double)locY * 360 / ledsInGradient) %
                      360;
            data.h = (double)pos;
            data.s = 1.0;
            data.v = 1.0;
            pixels[x * HEIGHT + y] = Color_hsv2rgb(data);
        }
    }
}

void generateWave(int frame, ws2812_pixel_t *pixels, bool direction, int speed,
                  bool mirror, int hue) {
    for (int x = 0; x < WIDTH; x++) {
        int locX = mirror ? x : WIDTH - x - 1;
        for (int y = 0; y < HEIGHT; y++) {
            int locY = (x % 2) ? y : HEIGHT - y - 1;
            locY = direction ? HEIGHT - locY - 1 : locY;
            if (((y + frame / (11 - speed)) % HEIGHT >= (x / 3 + 3)) &&
                ((y + frame / (11 - speed)) % HEIGHT < (x / 3 + 6))) {
                hsv_t data;
                data.h = hue;
                data.s = 1;
                data.v = 1;
                pixels[locX * HEIGHT + locY] = Color_hsv2rgb(data);
            } else {
                pixels[locX * HEIGHT + locY] = Color_hexToRGB(0x000000);
            }
        }
    }
}

void generateTapes(int frame, ws2812_pixel_t *pixels, int speed, int hue) {
    for (int x = 0; x < WIDTH; x++) {
        // int locX = mirror ? x : WIDTH - x - 1;
        for (int y = 0; y < HEIGHT; y++) {
            int locY = (x % 2) ? y : HEIGHT - y - 1;
            // locY = direction ? HEIGHT - locY - 1 : locY;
            if (y == (int)(x / 3 + 1 + frame / (12 - speed)) % HEIGHT ||
                y == (int)(HEIGHT - x / 3 + frame / (12 - speed)) % HEIGHT) {
                hsv_t data;
                data.h = hue;
                data.s = 1;
                data.v = 1;
                pixels[x * HEIGHT + locY] = Color_hsv2rgb(data);
            } else {
                pixels[x * HEIGHT + locY] = Color_hexToRGB(0x000000);
            }
        }
    }
}

struct snow_item snowArr[SNOW_NUMBER];
bool vacantPos[SNOW_NUMBER] = {true, true, true, true, true, true, true};
uint32_t max = 0;
void generateSnow(ws2812_pixel_t *pixels, int hue) {
    printf("RND: %d\n", max);
    int vacance = -1;

    for (int i = 0; i < SNOW_NUMBER; i++) {
        if (vacantPos[i]) {
            vacance = i;
            break;
        }
    }

    if (vacance != -1) {
        if ((int)((uint32_t)random()) % 10 > 4) {
            ;
        } else {
            struct snow_item item;

            item.x = (int)((uint32_t)random()) % WIDTH;
            item.y = (int)((uint32_t)random()) % HEIGHT;
            item.age = SNOW_LIFE + (int)((uint32_t)random()) % 35;
            item.maxAge = item.age;
            snowArr[vacance] = item;
            vacantPos[vacance] = false;
        }
    }
    for (int x = 0; x < WIDTH; x++) {
        for (int y = 0; y < HEIGHT; y++) {
            pixels[x * HEIGHT + y] = Color_hexToRGB(0x000000);
        }
    }
    for (int i = 0; i < SNOW_NUMBER; i++) {
        if (!vacantPos[i]) {
            hsv_t data;
            int maxAge = snowArr[i].maxAge;
            float deca = snowArr[i].maxAge / 5;
            int age = snowArr[i].age;
            int maxValueAge = (int)(maxAge - deca);
            if (age > maxValueAge)
                data.v = (maxAge - age) / deca;
            else
                data.v = (float)age / maxValueAge;
            data.h = hue;
            data.s = 1;
            pixels[snowArr[i].x * HEIGHT + snowArr[i].y] = Color_hsv2rgb(data);
            if (age <= 0)
                vacantPos[i] = true;
            else
                snowArr[i].age--;
        }
    }
}

void generateTornado(int frame, ws2812_pixel_t *pixels, int tailHue,
                     int tailLength, int hue) {
    for (int x = 0; x < WIDTH; x++) {
        for (int y = 0; y < HEIGHT; y++) {
            pixels[x * HEIGHT + y] = Color_hexToRGB(0x000000);
        }
    }
    int currentPos = frame % LED_NUMBER;
    int currentX = currentPos % WIDTH;
    int currentY = (int)currentPos / WIDTH;
    if (currentX % 2)
        currentY = HEIGHT - currentY - 1;
    for (int i = 0; i < tailLength; i++) {
        int tailPos = currentPos - 1 - i;
        if (tailPos < 0)
            tailPos = LED_NUMBER + tailPos;
        int tailX = tailPos % WIDTH;
        int tailY = (int)tailPos / WIDTH;
        if (tailX % 2)
            tailY = HEIGHT - tailY - 1;
        int currTailPos = (tailX * HEIGHT + tailY) % LED_NUMBER;
        hsv_t data;
        data.h = tailHue;
        data.s = 1;
        data.v = (float)1 / (i * i * 0.5 + 1);
        pixels[currTailPos] = Color_hsv2rgb(data);
    }
    hsv_t data;
    data.s = 1;
    data.v = 1;
    data.h = hue;
    pixels[currentX * HEIGHT + currentY] = Color_hsv2rgb(data);
}

void shutdown(ws2812_pixel_t *pixels) {
    for (int x = 0; x < WIDTH; x++) {
        for (int y = 0; y < HEIGHT; y++) {
            pixels[x * HEIGHT + y] = Color_hexToRGB(0x000000);
        }
    }
}

void render(int frame, ws2812_pixel_t *pixels) {

    struct parserData_t *params = HttpServer_getCurrentParams();

    int hue = params->currentHue;
    int mode = params->currentMode;
    int speed = params->currentSpeed;
    int tailHue = params->currentTailHue;
    int snowLife = params->currentSnowLife;
    int snowNumber = params->currentSnowNumber;
    int tailLength = params->currentTailLength;
    bool rainbowAlign = params->rainbowAlign;
    bool currentMirror = params->currentMirror;
    bool direction = params->currentDirection;

    switch (mode) {
    case RAINBOW_MODE:
        generateRainbow(frame, pixels, rainbowAlign, speed, direction);
        break;
    case WAVE_MODE:
        generateWave(frame, pixels, direction, speed, currentMirror, hue);
        break;
    case TAPES_MODE:
        generateTapes(frame, pixels, speed, hue);
        break;
    case SNOW_MODE:
        generateSnow(pixels, hue);
        break;
    case TORNADO_MODE:
        generateTornado(frame, pixels, tailHue, tailLength, hue);
        break;
    case DISABLE_MODE:
        shutdown(pixels);
        break;
    default:
        generateRainbow(frame, pixels, rainbowAlign, speed, direction);
        break;
    }
}

static void renderTask(void *pvParameters) {
    ws2812_pixel_t pixels[LED_NUMBER];
    int head_index = 0;

    ws2812_i2s_init(LED_NUMBER, PIXEL_RGB);

    memset(pixels, 0, sizeof(ws2812_pixel_t) * LED_NUMBER);

    int frame = 0;

    while (1) {
        int index;
        if (xQueueReceive(render_queue, &index, portMAX_DELAY)) {
            render(frame, &pixels[0]);
            ws2812_i2s_update(pixels, PIXEL_RGB);
            frame++;
        }
    }
}

void timerINterruptHandler(void *arg) {
    int index = 1;
    xQueueSendFromISR(render_queue, &index, NULL);
}

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
    render_queue = xQueueCreate(10, sizeof(int));
    // sdk_wifi_set_opmode(STATIONAP_MODE);

    Wifi_init();
    HttpServer_init();
    // chaekSettings();

    timer_set_interrupts(FRC1, false);
    timer_set_run(FRC1, false);
    _xt_isr_attach(INUM_TIMER_FRC1, timerINterruptHandler, NULL);
    timer_set_frequency(FRC1, RENDER_FREQ);
    timer_set_interrupts(FRC1, true);
    timer_set_run(FRC1, true);

    xTaskCreate(&renderTask, "ws2812_i2s", 2048, NULL, 10, NULL);
}
