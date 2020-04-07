

#include <ws2812_i2s/ws2812_i2s.h>
#include <FreeRTOS.h>
#include "queue.h"
#include "task.h"

#include <esp/uart.h>
#include "esp8266.h"
#include "espressif/esp_common.h"

#include "HttpServer.h"
#include "Color.h"
#include "Render.H"

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

QueueHandle_t m_render_queue;

static void Render_generateRainbow(int frame, ws2812_pixel_t *pixels, bool vertical,
                     int speed, bool direction) {
    int l_ledsInGradient = vertical ? HEIGHT : WIDTH;
    hsv_t l_data;
    for (int x = 0; x < WIDTH; x++) {
        int l_locX = direction ? x : WIDTH - x - 1;
        int l_pos = 0;
        if (!vertical)
            l_pos = (int)(((double)frame) * speed +
                        (double)l_locX * 360 / l_ledsInGradient) %
                  360;
        for (int y = 0; y < HEIGHT; y++) {
            int locY = (l_locX % 2) ? y : HEIGHT - y - 1;
            locY = direction ? locY : HEIGHT - locY - 1;
            if (vertical)
                l_pos = (int)(((double)frame) * speed +
                            (double)locY * 360 / l_ledsInGradient) %
                      360;
            l_data.h = (double)l_pos;
            l_data.s = 1.0;
            l_data.v = 1.0;
            pixels[x * HEIGHT + y] = Color_hsv2rgb(l_data);
        }
    }
}

static void Render_generateWave(int frame, ws2812_pixel_t *pixels, bool direction, int speed,
                  bool mirror, int hue) {
    for (int x = 0; x < WIDTH; x++) {
        int l_locX = mirror ? x : WIDTH - x - 1;
        for (int y = 0; y < HEIGHT; y++) {
            int l_locY = (x % 2) ? y : HEIGHT - y - 1;
            l_locY = direction ? HEIGHT - l_locY - 1 : l_locY;
            if (((y + frame / (11 - speed)) % HEIGHT >= (x / 3 + 3)) &&
                ((y + frame / (11 - speed)) % HEIGHT < (x / 3 + 6))) {
                hsv_t l_data;
                l_data.h = hue;
                l_data.s = 1;
                l_data.v = 1;
                pixels[l_locX * HEIGHT + l_locY] = Color_hsv2rgb(l_data);
            } else {
                pixels[l_locX * HEIGHT + l_locY] = Color_hexToRGB(0x000000);
            }
        }
    }
}

static void Render_generateTapes(int frame, ws2812_pixel_t *pixels, int speed, int hue) {
    for (int x = 0; x < WIDTH; x++) {
        // int locX = mirror ? x : WIDTH - x - 1;
        for (int y = 0; y < HEIGHT; y++) {
            int l_locY = (x % 2) ? y : HEIGHT - y - 1;
            // locY = direction ? HEIGHT - locY - 1 : locY;
            if (y == (int)(x / 3 + 1 + frame / (12 - speed)) % HEIGHT ||
                y == (int)(HEIGHT - x / 3 + frame / (12 - speed)) % HEIGHT) {
                hsv_t l_data;
                l_data.h = hue;
                l_data.s = 1;
                l_data.v = 1;
                pixels[x * HEIGHT + l_locY] = Color_hsv2rgb(l_data);
            } else {
                pixels[x * HEIGHT + l_locY] = Color_hexToRGB(0x000000);
            }
        }
    }
}

struct snow_item m_snowArr[SNOW_NUMBER];
bool m_vacantPos[SNOW_NUMBER] = {true, true, true, true, true, true, true};

static void Render_generateSnow(ws2812_pixel_t *pixels, int hue) {
    
    int l_vacance = -1;

    for (int i = 0; i < SNOW_NUMBER; i++) {
        if (m_vacantPos[i]) {
            l_vacance = i;
            break;
        }
    }

    if (l_vacance != -1) {
        if ((int)((uint32_t)random()) % 10 > 4) {
            ;
        } else {
            struct snow_item l_item;

            l_item.x = (int)((uint32_t)random()) % WIDTH;
            l_item.y = (int)((uint32_t)random()) % HEIGHT;
            l_item.age = SNOW_LIFE + (int)((uint32_t)random()) % 35;
            l_item.maxAge = l_item.age;
            m_snowArr[l_vacance] = l_item;
            m_vacantPos[l_vacance] = false;
        }
    }
    for (int x = 0; x < WIDTH; x++) {
        for (int y = 0; y < HEIGHT; y++) {
            pixels[x * HEIGHT + y] = Color_hexToRGB(0x000000);
        }
    }
    for (int i = 0; i < SNOW_NUMBER; i++) {
        if (!m_vacantPos[i]) {
            hsv_t l_data;
            int l_maxAge = m_snowArr[i].maxAge;
            float l_deca = m_snowArr[i].maxAge / 5;
            int l_age = m_snowArr[i].age;
            int l_maxValueAge = (int)(l_maxAge - l_deca);
            if (l_age > l_maxValueAge)
                l_data.v = (l_maxAge - l_age) / l_deca;
            else
                l_data.v = (float)l_age / l_maxValueAge;
            l_data.h = hue;
            l_data.s = 1;
            pixels[m_snowArr[i].x * HEIGHT + m_snowArr[i].y] = Color_hsv2rgb(l_data);
            if (l_age <= 0)
                m_vacantPos[i] = true;
            else
                m_snowArr[i].age--;
        }
    }
}

static void Render_generateTornado(int frame, ws2812_pixel_t *pixels, int tailHue,
                     int tailLength, int hue) {
    for (int x = 0; x < WIDTH; x++) {
        for (int y = 0; y < HEIGHT; y++) {
            pixels[x * HEIGHT + y] = Color_hexToRGB(0x000000);
        }
    }
    int l_currentPos = frame % LED_NUMBER;
    int l_currentX = l_currentPos % WIDTH;
    int l_currentY = (int)l_currentPos / WIDTH;
    if (l_currentX % 2)
        l_currentY = HEIGHT - l_currentY - 1;
    for (int i = 0; i < tailLength; i++) {
        int l_tailPos = l_currentPos - 1 - i;
        if (l_tailPos < 0)
            l_tailPos = LED_NUMBER + l_tailPos;
        int l_tailX = l_tailPos % WIDTH;
        int l_tailY = (int)l_tailPos / WIDTH;
        if (l_tailX % 2)
            l_tailY = HEIGHT - l_tailY - 1;
        int l_currTailPos = (l_tailX * HEIGHT + l_tailY) % LED_NUMBER;
        hsv_t l_data;
        l_data.h = tailHue;
        l_data.s = 1;
        l_data.v = (float)1 / (i * i * 0.5 + 1);
        pixels[l_currTailPos] = Color_hsv2rgb(l_data);
    }
    hsv_t l_data;
    l_data.s = 1;
    l_data.v = 1;
    l_data.h = hue;
    pixels[l_currentX * HEIGHT + l_currentY] = Color_hsv2rgb(l_data);
}

static void Render_shutdown(ws2812_pixel_t *pixels) {
    for (int x = 0; x < WIDTH; x++) {
        for (int y = 0; y < HEIGHT; y++) {
            pixels[x * HEIGHT + y] = Color_hexToRGB(0x000000);
        }
    }
}

static void Render_render(int frame, ws2812_pixel_t *pixels) {

    struct parserData_t *l_params = HttpServer_getCurrentParams();

    int l_hue = l_params->currentHue;
    int l_mode = l_params->currentMode;
    int l_speed = l_params->currentSpeed;
    int l_tailHue = l_params->currentTailHue;
    int l_snowLife = l_params->currentSnowLife;
    int l_snowNumber = l_params->currentSnowNumber;
    int l_tailLength = l_params->currentTailLength;
    bool l_rainbowAlign = l_params->rainbowAlign;
    bool l_currentMirror = l_params->currentMirror;
    bool l_direction = l_params->currentDirection;

    switch (l_mode) {
    case RAINBOW_MODE:
        Render_generateRainbow(frame, pixels, l_rainbowAlign, l_speed, l_direction);
        break;
    case WAVE_MODE:
        Render_generateWave(frame, pixels, l_currentMirror, l_speed, l_direction, l_hue);
        break;
    case TAPES_MODE:
        Render_generateTapes(frame, pixels, l_speed, l_hue);
        break;
    case SNOW_MODE:
        Render_generateSnow(pixels, l_hue);
        break;
    case TORNADO_MODE:
        Render_generateTornado(frame, pixels, l_tailHue, l_tailLength, l_hue);
        break;
    case DISABLE_MODE:
        Render_shutdown(pixels);
        break;
    default:
        Render_generateRainbow(frame, pixels, l_rainbowAlign, l_speed, l_direction);
        break;
    }
}

static void Render_loop(void *pvParameters) {
    ws2812_pixel_t l_pixels[LED_NUMBER];
    int head_index = 0;

    ws2812_i2s_init(LED_NUMBER, PIXEL_RGB);

    memset(l_pixels, 0, sizeof(ws2812_pixel_t) * LED_NUMBER);

    int l_frame = 0;

    while (1) {
        int index;
        if (xQueueReceive(m_render_queue, &index, portMAX_DELAY)) {
            Render_render(l_frame, &l_pixels[0]);
            ws2812_i2s_update(l_pixels, PIXEL_RGB);
            l_frame++;
        }
    }
}

static void Render_timerINterruptHandler(void *arg) {
    int l_index = 1;
    xQueueSendFromISR(m_render_queue, &l_index, NULL);
}

void Render_init() {
    timer_set_interrupts(FRC1, false);
    timer_set_run(FRC1, false);
    _xt_isr_attach(INUM_TIMER_FRC1, Render_timerINterruptHandler, NULL);
    timer_set_frequency(FRC1, RENDER_FREQ);
    timer_set_interrupts(FRC1, true);
    timer_set_run(FRC1, true);
    m_render_queue = xQueueCreate(10, sizeof(int));
    xTaskCreate(&Render_loop, "ws2812_i2s", 2048, NULL, 10, NULL);
}