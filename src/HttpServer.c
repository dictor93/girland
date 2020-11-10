#include "FreeRTOS.h"
#include "espressif/esp_common.h"
#include "queue.h"
#include "task.h"
#include <lwip/api.h>
#include "dhcpserver.h"

#include "HttpServer.h"
#include "Fs.h"

struct parserData_t s_currentData;

struct parserData_t *HttpServer_getCurrentParams() {
    return &s_currentData;
}

static void Config_setMode(char *uri) {
    if (strstr(uri, "RAINBOW")) {
        s_currentData.currentMode = RAINBOW_MODE;
    } else if (strstr(uri, "WAVE")) {
        s_currentData.currentMode = WAVE_MODE;
    } else if (strstr(uri, "TAPES")) {
        s_currentData.currentMode = TAPES_MODE;
    } else if (strstr(uri, "SNOW")) {
        s_currentData.currentMode = SNOW_MODE;
    } else if (strstr(uri, "TORNADO")) {
        s_currentData.currentMode = TORNADO_MODE;
    } else if (strstr(uri, "DISABLE")) {
        s_currentData.currentMode = DISABLE_MODE;
    }
}

static void Config_setDirection(char *uri) {
    if (strstr(uri, "FORWARD")) {
        s_currentData.currentDirection = FORWARD;
    } else if (strstr(uri, "BACKWARD")) {
        s_currentData.currentDirection = BACKWARD;
    }
}

static void Config_setSpeed(char *uri) {
    if (strstr(uri, "FAST")) {
        s_currentData.currentSpeed = FAST;
    } else if (strstr(uri, "MIDDLE")) {
        s_currentData.currentSpeed = MIDDLE;
    } else if (strstr(uri, "SLOW")) {
        s_currentData.currentSpeed = SLOW;
    }
}

static void Config_setMirror(char *uri) {
    if (strstr(uri, "NOT_MIRROR")) {
        s_currentData.currentMirror = NOT_MIRROR;
    } else if (strstr(uri, "MIRROR")) {
        s_currentData.currentMirror = MIRROR;
    }
}

static void Config_setAlign(char *uri) {
    if (strstr(uri, "VERTICAL")) {
        s_currentData.rainbowAlign = IS_RAINBOW_VERTICAL;
    } else if (strstr(uri, "HORISONTAL")) {
        s_currentData.rainbowAlign = IS_RAINBOW_HORISONTAL;
    }
}

static void Config_setMainColor(char *uri) {
    char subBuff[4];
    int colorStart = memchr(uri + 12, '/', MAX_URI_LEN) + 1;
    memcpy(subBuff, colorStart, 3);
    subBuff[3] = '\0';
    sscanf(subBuff, "%d", &s_currentData.currentHue);
}

static void Config_setTail(char *uri) {
    char subBuff[4];
    if (strstr(uri, "COLOR")) {
        int colorStart = memchr(uri + 8, '/', MAX_URI_LEN) + 1;
        memcpy(subBuff, colorStart, 3);
        subBuff[3] = '\0';
        sscanf(subBuff, "%d", &s_currentData.currentTailHue);
    } else if (strstr(uri, "LENGTH")) {
        int colorStart = memchr(uri + 8, '/', MAX_URI_LEN) + 1;
        memcpy(subBuff, colorStart, 3);
        subBuff[3] = '\0';
        sscanf(subBuff, "%d", &s_currentData.currentTailLength);
    }
}

static enum ActionType Config_getType(char *uri) {
    enum ActionType l_returnType;
    if (strstr(uri, "mode")) {
        l_returnType = kMode;
    } else if (strstr(uri, "dir")) {
        l_returnType = kDirection;
    } else if (strstr(uri, "speed")) {
        l_returnType = kSpeed;
    } else if (strstr(uri, "mirror")) {
        l_returnType = kMirror;
    } else if (strstr(uri, "align")) {
        l_returnType = kAlign;
    } else if (strstr(uri, "maincolor")) {
        l_returnType = kMainColor;
    } else if (strstr(uri, "tail")) {
        l_returnType = kTail;
    } else if (strstr(uri, "settings")) {
        l_returnType = kSettings;
    } else if (strlen(uri) < 3) {
        l_returnType = kRoot;
    }
    return l_returnType;
}

static enum ActionType Config_parceUri(char *uri) {
    // получаем тип и в зависимости от него закидываем в соответсвующу функцию
    // которая уже и устанавливает наш конфиг
    enum ActionType l_type = Config_getType(uri);
    switch (l_type) {
    case kMode:
        Config_setMode(uri);
        break;
    case kDirection:
        Config_setDirection(uri);
        break;
    case kSpeed:
        Config_setSpeed(uri);
        break;
    case kMirror:
        Config_setMirror(uri);
        break;
    case kAlign:
        Config_setAlign(uri);
        break;
    case kMainColor:
        Config_setMainColor(uri);
        break;
    case kTail:
        Config_setTail(uri);
        break;
    default:
        break;
    }
    return l_type;
}

static void HttpServer_httpd_task(void *pvParameters) {
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
                u16_t len;
                netbuf_data(nb, &data, &len);
                /* check for a GET request */
                if (!strncmp(data, "GET ", 4)) {
                    char uri[32];
                    char *uriStart, *uriEnd;
                    /* extract URI */
                    uriStart = data + 5;
                    uriEnd = memchr(uriStart, ' ', MAX_URI_LEN);
                    int len = uriEnd - uriStart;
                    memcpy(uri, uriStart, len);
                    uri[len] = '\0';
                    printf("uri: %s\n", uri);
                    // закидываем uri в парсер
                    enum ActionType l_type = Config_parceUri(uri);
                    switch (l_type) {
                    case kRoot:
                        Fs_readFile("mainpage", buf, PAGE_BUFFER_LENGTH);
                        netconn_write(client, buf, strlen(buf), NETCONN_COPY);
                        break;
                    case kSettings:
                        Fs_readFile("settings", buf, PAGE_BUFFER_LENGTH);
                        netconn_write(client, buf, strlen(buf), NETCONN_COPY);
                        break;
                    default:
                        snprintf(buf, sizeof(buf),
                                 "{\"m\":%d,\"fh\":%d,\"ch\":%d,\"th\":%d,"
                                 "\"tl\":%d}",
                                 s_currentData.currentMode,
                                 (int)xPortGetFreeHeapSize(),
                                 s_currentData.currentHue,
                                 s_currentData.currentTailHue,
                                 s_currentData.currentTailLength);
                        netconn_write(client, buf, strlen(buf), NETCONN_COPY);
                        break;
                    }
                }
                netbuf_delete(nb);
            }
            printf("Closing connection\n");
            netconn_close(client);
            netconn_delete(client);
        }
    }
}

void HttpServer_init() {

    s_currentData.currentHue = 196;
    s_currentData.currentMode = RAINBOW_MODE;
    s_currentData.currentSnowLife = DEFAULT_SNOW_LIFE;
    s_currentData.currentSnowNumber = DEFAULT_SNOW_NUMBER;
    s_currentData.currentSpeed = FAST;
    s_currentData.currentTailHue = DEFAULT_TORNADO_TAIL_HUE;
    s_currentData.currentTailLength = DEFAULT_TORNADO_TAIL_LENGTH;
    s_currentData.currentDirection = FORWARD;
    s_currentData.currentMirror = NOT_MIRROR;
    s_currentData.rainbowAlign = IS_RAINBOW_VERTICAL;

    xTaskCreate(&HttpServer_httpd_task, "HTTP_DEAMON", 4096, NULL, 2, NULL);
}
