#include "FreeRTOS.h"
#include "espressif/esp_common.h"
#include "queue.h"
#include "task.h"
#include "dhcpserver.h"
#include <lwip/api.h>

#include "HttpServer.h"


struct parserData_t s_currentData;

struct parserData_t * HttpServer_getCurrentParams() {
    return &s_currentData;
}

static void HttpServer_getWifiSetingsPage(char *buf) {
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

static void HttpServer_getMainPage(char *buf) {
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

static void Config_setMode(uri) {
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
enum Type Config_getType(uri) {
    Type l_returnType;
    if (strstr(uri, "mode")) {
        l_returnType = kMode;
    } else if (strstr(uri, "dir") {
        l_returnType = kDirection;
    }
    return l_returnType;
}

static void Config_parceUri(char *uri) {
    // получаем тип и в зависимости от него закидываем в соответсвующу функцию которая уже и устанавливает наш конфиг
    Type l_type = Config_getType(uri);
    switch (l_type) {
    case kMode:
        Config_setMode(uri);
        break;
    
    default:
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
                    // закидываем uri в парсер
                    Config_parceUri(char *uri);
                    /*
                    if (strstr(uri, "mode")) {
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
                    } else if (strstr(uri, "dir")) {
                        if (strstr(uri, "FORWARD")) {
                            s_currentData.currentDirection = FORWARD;
                        } else if (strstr(uri, "BACKWARD")) {
                            s_currentData.currentDirection = BACKWARD;
                        }
                    } else if (strstr(uri, "speed")) {
                        if (strstr(uri, "FAST")) {
                            s_currentData.currentSpeed = FAST;
                        } else if (strstr(uri, "MIDDLE")) {
                            s_currentData.currentSpeed = MIDDLE;
                        } else if (strstr(uri, "SLOW")) {
                            s_currentData.currentSpeed = SLOW;
                        }
                    } else if (strstr(uri, "mirror")) {
                        if (strstr(uri, "NOT_MIRROR")) {
                            s_currentData.currentMirror = NOT_MIRROR;
                        } else if (strstr(uri, "MIRROR")) {
                            s_currentData.currentMirror = MIRROR;
                        }
                    } else if (strstr(uri, "align")) {
                        if (strstr(uri, "VERTICAL")) {
                            s_currentData.rainbowAlign = IS_RAINBOW_VERTICAL;
                        } else if (strstr(uri, "HORISONTAL")) {
                            s_currentData.rainbowAlign = IS_RAINBOW_HORISONTAL;
                        }
                    } else if (strstr(uri, "maincolor")) {
                        int colorStart = memchr(uri + 12, '/', max_uri_len) + 1;
                        memcpy(subBuff, colorStart, 3);
                        subBuff[3] = '\0';
                        sscanf(subBuff, "%d", &s_currentData.currentHue);
                    } else if (strstr(uri, "tail")) {
                        if (strstr(uri, "COLOR")) {
                            int colorStart =
                                memchr(uri + 8, '/', max_uri_len) + 1;
                            memcpy(subBuff, colorStart, 3);
                            subBuff[3] = '\0';
                            sscanf(subBuff, "%d", &s_currentData.currentTailHue);
                        } else if (strstr(uri, "LENGTH")) {
                            int colorStart =
                                memchr(uri + 8, '/', max_uri_len) + 1;
                            memcpy(subBuff, colorStart, 3);
                            subBuff[3] = '\0';
                            sscanf(subBuff, "%d", &s_currentData.currentTailLength);
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
                                 s_currentData.currentMode, (int)xPortGetFreeHeapSize(),
                                 s_currentData.currentHue, s_currentData.currentTailHue, s_currentData.currentTailLength);
                        netconn_write(client, buf, strlen(buf), NETCONN_COPY);
                    } else if (strlen(uri) < 3) {
                        HttpServer_getMainPage(&buf);
                        printf("Buffer: \n %s\n", buf);
                        netconn_write(client, buf, strlen(buf), NETCONN_COPY);
                    } else {
                        snprintf(buf, sizeof(buf),
                                 "{\"m\":%d,\"fh\":%d,\"ch\":%d,\"th\":%d,"
                                 "\"tl\":%d}",
                                 s_currentData.currentMode, (int)xPortGetFreeHeapSize(),
                                 s_currentData.currentHue, s_currentData.currentTailHue, s_currentData.currentTailLength);
                        netconn_write(client, buf, strlen(buf), NETCONN_COPY);
                    }
                }
                free(subBuff); */
            }
            netbuf_delete(nb);
        }
        printf("Closing connection\n");
        netconn_close(client);
        netconn_delete(client);
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

    xTaskCreate(&HttpServer_httpd_task, 'HTTP_DEAMON', 4096, NULL, 2, NULL);
}