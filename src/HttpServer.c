#include "espressif/esp_common.h"
#include "FreeRTOS.h"
#include "queue.h"
#include "string.h"
#include "task.h"
#include <lwip/api.h>
#include "dhcpserver.h"

#include "Common.h"
#include "Fs.h"
#include "HttpServer.h"
#include "ModeConfig.h"

static enum ActionType HttpServer_getRequestType(char *uri) {
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
    } else if (strstr(uri, "settingsstate")) {
        l_returnType = kSettingsState;
    } else if (strstr(uri, "settings")) {
        l_returnType = kSettings;
    } else if (strstr(uri, "styles.css")) {
        l_returnType = kStyles;
    } else if (strstr(uri, "wifiset")) {
        l_returnType = kWifiSet;
    } else if (strlen(uri) < 3) {
        l_returnType = kRoot;
    }
    return l_returnType;
}

static void HttpServer_router(char *uri, char *bufer, char *otherBody) {
    printf("\n\nOther body:\n%s\n\n", otherBody);
    enum ActionType l_type = HttpServer_getRequestType(uri);
    switch (l_type) {
    case kRoot:
        Fs_readFile("mainpage.html", bufer, PAGE_BUFFER_LENGTH);
        break;
    case kSettingsState:
        Fs_readFile("creds", bufer, PAGE_BUFFER_LENGTH);
        break;
    case kSettings:
        Fs_readFile("settings.html", bufer, PAGE_BUFFER_LENGTH);
        break;
    case kStyles:
        Fs_readFile("styles.css", bufer, PAGE_BUFFER_LENGTH);
        break;
    case kWifiSet:
        Fs_readFile("settings.html", bufer, PAGE_BUFFER_LENGTH);
        break;
    default:
        ModeConfig_setConfig(uri, l_type);
        struct parserData_t *s_currentData = ModeConfig_getCurrentParams();
        snprintf(bufer, PAGE_BUFFER_LENGTH,
                 "{\"m\":%d,\"fh\":%d,\"ch\":%d,\"th\":%d,"
                 "\"tl\":%d}",
                 s_currentData->currentMode, (int)xPortGetFreeHeapSize(),
                 s_currentData->currentHue, s_currentData->currentTailHue,
                 s_currentData->currentTailLength);
        int pageLangth = strlen(bufer);
        bufer[pageLangth] = '\0';
        break;
    }
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
                char uri[32];
                char *uriStart, *uriEnd;
                int methodLength;
                if (!strncmp(data, "GET ", 4)) {
                    methodLength = 4;
                } else if (!strncmp(data, "POST ", 5)) {
                    methodLength = 5;
                }
                /* extract URI */
                uriStart = data + methodLength + 1;
                uriEnd = memchr(uriStart, ' ', MAX_URI_LEN);
                int uriLen = uriEnd - uriStart;
                memcpy(uri, uriStart, uriLen);
                uri[uriLen] = '\0';
                printf("uri: %s\n", uri);
                // закидываем uri в парсер
                HttpServer_router(uri, buf, uriEnd);
                netconn_write(client, buf, strlen(buf), NETCONN_COPY);
                netbuf_delete(nb);
            }
            printf("Closing connection\n");
            netconn_close(client);
            netconn_delete(client);
        }
    }
}

void HttpServer_init() {
    xTaskCreate(&HttpServer_httpd_task, "HTTP_DEAMON", 4096, NULL, 2, NULL);
}
