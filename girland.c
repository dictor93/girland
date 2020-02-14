/**
 * Example of ws2812_i2s library usage.
 *
 * This example shows light that travels in circle with fading tail.
 * As ws2812_i2s library using hardware I2S the output pin is GPIO3 and
 * can not be changed.
 *
 * This sample code is in the public domain.,
 */
#include "FreeRTOS.h"
#include "esp/uart.h"
#include "esp8266.h"
#include "espressif/esp_common.h"
#include "queue.h"
#include "task.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <lwip/api.h>
#include <math.h>

#include "dhcpserver.h"
#include "ws2812_i2s/ws2812_i2s.h"
#include <Wifi.h>

// #include "spiffs.h"
// #include "esp_spiffs.h"

#define HEIGHT 9
#define WIDTH 17

#define LED_NUMBER 153
#define RENDER_FREQ 30

#define SNOW_COLOR 0xB900FF

#define FORWARD true
#define BACKWARD false
#define FAST 10
#define MIDDLE 5
#define SLOW 2
#define NOT_MIRROR false
#define MIRROR true

#define IS_RAINBOW_VERTICAL true
#define IS_RAINBOW_HORISONTAL false

#define TORNADO_TAIL_LENGTH 8

#define SNOW_LIFE 30 * 2 // 5sec;
#define SNOW_NUMBER 15

#define RAINBOW_MODE 0
#define WAVE_MODE 1
#define TAPES_MODE 2
#define SNOW_MODE 3
#define TORNADO_MODE 4
#define DISABLE_MODE -1
#define PAGE_BUFFER_LENGTH 3400

int currentMode = RAINBOW_MODE;
int currentSpeed = FAST;
bool currentDirection = FORWARD;
int currentSnowNumber = SNOW_NUMBER;
int currentSnowLife = SNOW_LIFE;
uint32_t currentColor = SNOW_COLOR;
bool currentMirror = NOT_MIRROR;
int currentTornadoTailLength = TORNADO_TAIL_LENGTH;
bool rainbowAlign = IS_RAINBOW_HORISONTAL;
int currentHue = 196;
int currentTailHue = 16;
int currentTailLength = 8;

QueueHandle_t render_queue;

typedef struct {
    double h; // angle in degrees
    double s; // a fraction between 0 and 1
    double v; // a fraction between 0 and 1
} hsv;

typedef struct {
    int maxAge;
    int age;
    int x;
    int y;
} snow_item;

ws2812_pixel_t hexToRGB(uint32_t hex) {
    ws2812_pixel_t data;
    data.red = (hex & 0xFF0000) >> 16;
    data.green = (hex & 0x00FF00) >> 8;
    data.blue = (hex & 0x0000FF) >> 0;
    return data;
}

bool getAuthData();

ws2812_pixel_t hsv2rgb(hsv in) {
    double hh, p, q, t, ff;
    long i;
    ws2812_pixel_t out;
    if (in.s <= 0.0) { // < is bogus, just shuts up warnings

        out.red = (int)(in.v * 255);
        out.green = (int)(in.v * 255);
        out.blue = (int)(in.v * 255);
        return out;
    }
    hh = in.h;
    if (hh >= 360.0)
        hh = 0.0;
    hh /= 60.0;
    i = (long)hh;
    ff = hh - i;
    p = in.v * (1.0 - in.s);
    q = in.v * (1.0 - (in.s * ff));
    t = in.v * (1.0 - (in.s * (1.0 - ff)));

    switch (i) {
    case 0:
        out.red = (int)(in.v * 255);
        out.green = (int)(t * 255);
        out.blue = (int)(p * 255);
        break;
    case 1:
        out.red = (int)(q * 255);
        out.green = (int)(in.v * 255);
        out.blue = (int)(p * 255);
        break;
    case 2:
        out.red = (int)(p * 255);
        out.green = (int)(in.v * 255);
        out.blue = (int)(t * 255);
        break;

    case 3:
        out.red = (int)(p * 255);
        out.green = (int)(q * 255);
        out.blue = (int)(in.v * 255);
        break;
    case 4:
        out.red = (int)(t * 255);
        out.green = (int)(p * 255);
        out.blue = (int)(in.v * 255);
        break;
    case 5:
    default:
        out.red = (int)(in.v * 255);
        out.green = (int)(p * 255);
        out.blue = (int)(q * 255);
        break;
    }
    return out;
}

void generateRainbow(int frame, ws2812_pixel_t *pixels, bool vertical,
                     int speed, bool direction) {
    int ledsInGradient = vertical ? HEIGHT : WIDTH;
    hsv data;
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
            pixels[x * HEIGHT + y] = hsv2rgb(data);
        }
    }
}

void generateWave(int frame, ws2812_pixel_t *pixels, bool direction, int speed,
                  bool mirror) {
    for (int x = 0; x < WIDTH; x++) {
        int locX = mirror ? x : WIDTH - x - 1;
        for (int y = 0; y < HEIGHT; y++) {
            int locY = (x % 2) ? y : HEIGHT - y - 1;
            locY = direction ? HEIGHT - locY - 1 : locY;
            if (((y + frame / (11 - speed)) % HEIGHT >= (x / 3 + 3)) &&
                ((y + frame / (11 - speed)) % HEIGHT < (x / 3 + 6))) {
                hsv data;
                data.h = currentHue;
                data.s = 1;
                data.v = 1;
                pixels[locX * HEIGHT + locY] = hsv2rgb(data);
            } else {
                pixels[locX * HEIGHT + locY] = hexToRGB(0x000000);
            }
        }
    }
}

void generateTapes(int frame, ws2812_pixel_t *pixels, int speed) {
    for (int x = 0; x < WIDTH; x++) {
        // int locX = mirror ? x : WIDTH - x - 1;
        for (int y = 0; y < HEIGHT; y++) {
            int locY = (x % 2) ? y : HEIGHT - y - 1;
            // locY = direction ? HEIGHT - locY - 1 : locY;
            if (y == (int)(x / 3 + 1 + frame / (12 - speed)) % HEIGHT ||
                y == (int)(HEIGHT - x / 3 + frame / (12 - speed)) % HEIGHT) {
                hsv data;
                data.h = currentHue;
                data.s = 1;
                data.v = 1;
                pixels[x * HEIGHT + locY] = hsv2rgb(data);
            } else {
                pixels[x * HEIGHT + locY] = hexToRGB(0x000000);
            }
        }
    }
}

snow_item snowArr[SNOW_NUMBER];
bool vacantPos[SNOW_NUMBER] = {true, true, true, true, true, true, true};
uint32_t max = 0;
void generateSnow(ws2812_pixel_t *pixels) {
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
            snow_item item;

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
            pixels[x * HEIGHT + y] = hexToRGB(0x000000);
        }
    }
    for (int i = 0; i < SNOW_NUMBER; i++) {
        if (!vacantPos[i]) {
            hsv data;
            int maxAge = snowArr[i].maxAge;
            float deca = snowArr[i].maxAge / 5;
            int age = snowArr[i].age;
            int maxValueAge = (int)(maxAge - deca);
            if (age > maxValueAge)
                data.v = (maxAge - age) / deca;
            else
                data.v = (float)age / maxValueAge;
            data.h = currentHue;
            data.s = 1;
            pixels[snowArr[i].x * HEIGHT + snowArr[i].y] = hsv2rgb(data);
            if (age <= 0)
                vacantPos[i] = true;
            else
                snowArr[i].age--;
        }
    }
}

void generateTornado(int frame, ws2812_pixel_t *pixels) {
    for (int x = 0; x < WIDTH; x++) {
        for (int y = 0; y < HEIGHT; y++) {
            pixels[x * HEIGHT + y] = hexToRGB(0x000000);
        }
    }
    int currentPos = frame % LED_NUMBER;
    int currentX = currentPos % WIDTH;
    int currentY = (int)currentPos / WIDTH;
    if (currentX % 2)
        currentY = HEIGHT - currentY - 1;
    for (int i = 0; i < currentTailLength; i++) {
        int tailPos = currentPos - 1 - i;
        if (tailPos < 0)
            tailPos = LED_NUMBER + tailPos;
        int tailX = tailPos % WIDTH;
        int tailY = (int)tailPos / WIDTH;
        if (tailX % 2)
            tailY = HEIGHT - tailY - 1;
        int currTailPos = (tailX * HEIGHT + tailY) % LED_NUMBER;
        hsv data;
        data.h = currentTailHue;
        data.s = 1;
        data.v = (float)1 / (i * i * 0.5 + 1);
        pixels[currTailPos] = hsv2rgb(data);
    }
    hsv data;
    data.s = 1;
    data.v = 1;
    data.h = currentHue;
    pixels[currentX * HEIGHT + currentY] = hsv2rgb(data);
}

void shutdown(ws2812_pixel_t *pixels) {
    for (int x = 0; x < WIDTH; x++) {
        for (int y = 0; y < HEIGHT; y++) {
            pixels[x * HEIGHT + y] = hexToRGB(0x000000);
        }
    }
}

void render(int frame, ws2812_pixel_t *pixels) {
    switch (currentMode) {
    case RAINBOW_MODE:
        generateRainbow(frame, pixels, rainbowAlign, currentSpeed,
                        currentDirection);
        break;
    case WAVE_MODE:
        generateWave(frame, pixels, currentDirection, currentSpeed,
                     currentMirror);
        break;
    case TAPES_MODE:
        generateTapes(frame, pixels, currentSpeed);
        break;
    case SNOW_MODE:
        generateSnow(pixels);
        break;
    case TORNADO_MODE:
        generateTornado(frame, pixels);
        break;
    case DISABLE_MODE:
        shutdown(pixels);
        break;
    default:
        generateRainbow(frame, pixels, rainbowAlign, currentSpeed,
                        currentDirection);
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

void getWifiStingsPage(char *buf) {
    const char *webpage =
        "<html><head><style></style></head><body><div class=\"root\"><form "
        "method=\"post\" id=\"settings\"><div><label>SSID<input type=\"text\" "
        "name=\"SSID\" "
        "placeholder=\"SSID\"></label></div><div><label>PASSWORD<input "
        "type=\"text\"nname=\"PWD\" "
        "placeholder=\"PASSWORD\"></label></div><button>Send</button></form></"
        "div></body><script>const "
        "form=document.getElementById('settings');form.addEventListener('"
        "submit',(e)=>{e.preventDefault();fetch(`/wifiset/"
        "${e.target[0].value},${e.target[1].value}`);});</script></html>";
    snprintf(buf, PAGE_BUFFER_LENGTH, webpage);
}

void getMainPage(char *buf) {
    const char *webpage =
        "<html><head> <title>HTTP Server</title> <style> div.main { "
        "font-family: Arial; padding: 0.01em 16px; box-shadow: 2px 2px 1px 1px "
        "#d2d2d2; background-color: #f1f1f1; } #controls>div { padding: 10px; "
        "border-bottom: 1px solid rgba(0, 0, 0, 0.3); } @media screen and "
        "(max-device-width: 500px) { html { font-size: 50px; } html button { "
        "font-size: 50px; width: 400px; /* height: 60px; */ } } "
        "input[type=\"button\"]{ color: #fff; background-color: #007bff; "
        "border-color: #007bff; font-weight: 400; text-align: center; border: "
        "1px solid transparent; padding: .375rem .75rem; margin:7px 12px 5px; "
        "font-size: 1rem; line-height: 1.5; border-radius: .25rem; transition: "
        "color .15s ease-in-out, background-color .15s "
        "ease-in-out,border-color .15s ease-in-out,box-shadow .15s "
        "ease-in-out; } input[type=\"button\"]:hover{background-color: "
        "#0069d9;border-color: #0062cc;} "
        "input[type=\"button\"]:active{background-color: #0062cc;border-color: "
        "#005cbf;} input[type=\"button\"]:focus{background-color: "
        "#0069d9;border-color: #0062cc;box-shadow: 0 0 0 0.2rem "
        "rgba(38,143,255,.5)} </style></head><body> <div class='main'> "
        "<h3>Garland Control</h3> <p>Free heap: <span id='heap'></span></p> "
        "<div id='controls'> </div> </div></body><script> const g = [ { a: "
        "['all'], n: 'Mode set', p: '/mode', c: [ { n: 'RAINBOW', t: 'button', "
        "}, { n: 'WAVE', t: 'button', }, { n: 'TAPES', t: 'button', }, { n: "
        "'SNOW', t: 'button', }, { n: 'TORNADO', t: 'button', }, { n: "
        "'DISABLE', t: 'button', } ] }, { a: [0, 1], n: 'Direction set', p: "
        "'/dir', c: [ { n: 'FORWARD', t: 'button', }, { n: 'BACKWARD', t: "
        "'button', }, ] }, { a: [0, 1, 2], n: 'Speed set', p: '/speed', c: [ { "
        "n: 'FAST', t: 'button', }, { n: 'MIDDLE', t: 'button', }, { n: "
        "'SLOW', t: 'button', }, ] }, { a: [0], n: 'Align set', p: '/align', "
        "c: [ { n: 'VERTICAL', t: 'button', }, { n: 'HORISONTAL', t: 'button', "
        "} ] }, { a: [1], n: 'Mirror set', p: '/mirror', c: [ { n: 'MIRROR', "
        "t: 'button', }, { n: 'NOT_MIRROR', t: 'button', } ] }, { a: "
        "[1,2,3,4], n: 'Main color set', p: '/maincolor', c: [ { n: "
        "'MAIN_COLOR', t: 'range', f: 'ch', min: 0, max: 360, }, ] }, { a: "
        "[4], n: 'Tail set', p: '/tail', c: [ { n: 'COLOR', t: 'range', f: "
        "'th', min: 0, max:360, }, { n: 'LENGTH', t: 'range', f: 'tl', min: 0, "
        "max: 100, }, ] }, ]; function s(p) { fetch(p).then(r => "
        "r.json().then(render)); }; function render(state) { "
        "console.log(state); "
        "document.getElementById('heap').innerText=state.fh; const root = "
        "document.getElementById('controls'); root.innerHTML = ''; "
        "g.forEach((cg => { if (cg.a.includes(state.m) || "
        "cg.a.includes('all')) { const gn = document.createElement('div'); "
        "gn.innerHTML = `<span>${cg.n}: </span><br/>`; cg.c.forEach(c=>{ const "
        "e = document.createElement('input'); e.setAttribute('type', c.t); "
        "if(c.t==='button') { e.addEventListener('click', "
        "()=>s(`${cg.p}/${c.n}`)); gn.appendChild(e); e.setAttribute('value', "
        "c.n); } else if(c.t==='range') { const cr = "
        "document.createElement('div'); cr.appendChild(e); cr.innerText=c.n; "
        "gn.appendChild(cr); e.setAttribute('min', c.min); "
        "e.setAttribute('max', c.max); e.value = state[c.f]; "
        "e.addEventListener('input', "
        "(e)=>s(`${cg.p}/${c.n}/${String(e.target.value).padStart(3,0)}`)); } "
        "gn.appendChild(e); }); root.appendChild(gn); } })); }; "
        "s('/status');</script></html>";
    snprintf(buf, PAGE_BUFFER_LENGTH, webpage);
}

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

void httpd_task(void *pvParameters) {
    ip_addr_t first_client_ip;
    IP4_ADDR(&first_client_ip, 192, 168, 0, 2);
    dhcpserver_start(&first_client_ip, 4);

    struct netconn *client = NULL;
    struct netconn *nc = netconn_new(NETCONN_TCP);
    if (nc == NULL) {
        printf("Failed to allocate socket.\n");
        vTaskDelete(NULL);
    }
    netconn_bind(nc, IP_ADDR_ANY, 80);
    netconn_listen(nc);
    char buf[PAGE_BUFFER_LENGTH];

    while (1) {
        err_t err = netconn_accept(nc, &client);
        if (err == ERR_OK) {
            struct netbuf *nb;
            if ((err = netconn_recv(client, &nb)) == ERR_OK) {
                void *data;
                char *subBuff;
                subBuff = malloc(4);
                u16_t len;
                netbuf_data(nb, &data, &len);
                /* check for a GET request */
                if (!strncmp(data, "GET ", 4)) {
                    char uri[32];
                    const int max_uri_len = 32;
                    char *sp1, *sp2;
                    /* extract URI */
                    sp1 = data + 5;
                    sp2 = memchr(sp1, ' ', max_uri_len);
                    int len = sp2 - sp1;
                    memcpy(uri, sp1, len);
                    uri[len] = '\0';
                    printf("uri: %s\n", uri);

                    if (strstr(uri, "mode")) {
                        if (strstr(uri, "RAINBOW")) {
                            currentMode = RAINBOW_MODE;
                        } else if (strstr(uri, "WAVE")) {
                            currentMode = WAVE_MODE;
                        } else if (strstr(uri, "TAPES")) {
                            currentMode = TAPES_MODE;
                        } else if (strstr(uri, "SNOW")) {
                            currentMode = SNOW_MODE;
                        } else if (strstr(uri, "TORNADO")) {
                            currentMode = TORNADO_MODE;
                        } else if (strstr(uri, "DISABLE")) {
                            currentMode = DISABLE_MODE;
                        }
                    } else if (strstr(uri, "dir")) {
                        if (strstr(uri, "FORWARD")) {
                            currentDirection = FORWARD;
                        } else if (strstr(uri, "BACKWARD")) {
                            currentDirection = BACKWARD;
                        }
                    } else if (strstr(uri, "speed")) {
                        if (strstr(uri, "FAST")) {
                            currentSpeed = FAST;
                        } else if (strstr(uri, "MIDDLE")) {
                            currentSpeed = MIDDLE;
                        } else if (strstr(uri, "SLOW")) {
                            currentSpeed = SLOW;
                        }
                    } else if (strstr(uri, "mirror")) {
                        if (strstr(uri, "NOT_MIRROR")) {
                            currentMirror = NOT_MIRROR;
                        } else if (strstr(uri, "MIRROR")) {
                            currentMirror = MIRROR;
                        }
                    } else if (strstr(uri, "align")) {
                        if (strstr(uri, "VERTICAL")) {
                            rainbowAlign = IS_RAINBOW_VERTICAL;
                        } else if (strstr(uri, "HORISONTAL")) {
                            rainbowAlign = IS_RAINBOW_HORISONTAL;
                        }
                    } else if (strstr(uri, "maincolor")) {
                        int colorStart = memchr(uri + 12, '/', max_uri_len) + 1;
                        memcpy(subBuff, colorStart, 3);
                        subBuff[3] = '\0';
                        sscanf(subBuff, "%d", &currentHue);
                    } else if (strstr(uri, "tail")) {
                        if (strstr(uri, "COLOR")) {
                            int colorStart =
                                memchr(uri + 8, '/', max_uri_len) + 1;
                            memcpy(subBuff, colorStart, 3);
                            subBuff[3] = '\0';
                            sscanf(subBuff, "%d", &currentTailHue);
                        } else if (strstr(uri, "LENGTH")) {
                            int colorStart =
                                memchr(uri + 8, '/', max_uri_len) + 1;
                            memcpy(subBuff, colorStart, 3);
                            subBuff[3] = '\0';
                            sscanf(subBuff, "%d", &currentTailLength);
                        }
                        // } else if (strstr(uri, "wifiset")) {
                        //     char * param1Addr = memchr(uri+5, '/', 32) + 1;
                        //     char * dividerAddr = memchr(uri+5, ',', 32);
                        //     char * end = memchr(uri+5, '\0', 32)+1;
                        //     char SSID[32];
                        //     char PWD[32];
                        //     memcpy(SSID, param1Addr, dividerAddr-param1Addr);
                        //     SSID[dividerAddr-param1Addr] = '\0';
                        //     memcpy(PWD, dividerAddr + 1, end -
                        //     dividerAddr-1); PWD[end - dividerAddr-1] = '\0';
                        //     saveAuthData(SSID, PWD);
                        //     vTaskDelay(500);
                        //     getAuthData();
                        //     // sdk_system_restart();
                    }

                    if (strstr(uri, "status")) {
                        snprintf(buf, sizeof(buf),
                                 "{\"m\":%d,\"fh\":%d,\"ch\":%d,\"th\":%d,"
                                 "\"tl\":%d}",
                                 currentMode, (int)xPortGetFreeHeapSize(),
                                 currentHue, currentTailHue, currentTailLength);
                        netconn_write(client, buf, strlen(buf), NETCONN_COPY);
                    } else if (strlen(uri) < 3) {
                        getMainPage(&buf);
                        printf("Buffer: \n %s\n", buf);
                        netconn_write(client, buf, strlen(buf), NETCONN_COPY);
                    } else {
                        snprintf(buf, sizeof(buf),
                                 "{\"m\":%d,\"fh\":%d,\"ch\":%d,\"th\":%d,"
                                 "\"tl\":%d}",
                                 currentMode, (int)xPortGetFreeHeapSize(),
                                 currentHue, currentTailHue, currentTailLength);
                        netconn_write(client, buf, strlen(buf), NETCONN_COPY);
                    }
                }
                free(subBuff);
            }
            netbuf_delete(nb);
        }
        printf("Closing connection\n");
        netconn_close(client);
        netconn_delete(client);
    }
}

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

    // chaekSettings();

    timer_set_interrupts(FRC1, false);
    timer_set_run(FRC1, false);
    _xt_isr_attach(INUM_TIMER_FRC1, timerINterruptHandler, NULL);
    timer_set_frequency(FRC1, RENDER_FREQ);
    timer_set_interrupts(FRC1, true);
    timer_set_run(FRC1, true);

    xTaskCreate(&renderTask, "ws2812_i2s", 2048, NULL, 10, NULL);
    xTaskCreate(&httpd_task, 'HTTP_DEAMON', 4096, NULL, 2, NULL);
}
