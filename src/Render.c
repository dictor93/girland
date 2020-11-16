

#include <ws2812_i2s/ws2812_i2s.h>
#include <FreeRTOS.h>
#include "queue.h"
#include "task.h"
#include <string.h>
#include <esp/uart.h>
#include "esp8266.h"
#include "espressif/esp_common.h"
#include <math.h>

#include "HttpServer.h"
#include "Color.h"
#include "Render.h"
#include "LocalConfig.h"
#include "ModeConfig.h"

#define LED_NUMBER WIDTH * HEIGHT
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

int s_frame = 0;

void Render_resetFrame() {
    s_frame = 0;
}

static void Render_fillBlack(hsv_t pixels[WIDTH][HEIGHT]) {
    for (int x = 0; x < WIDTH; x++) {
        for (int y = 0; y < HEIGHT; y++) {
            pixels[x][y].v = 0;
        }
    }
}

static void Render_flatArray(hsv_t src[WIDTH][HEIGHT], hsv_t *flated) {
    for (int x = 0; x < WIDTH; x++) {
        for (int y = 0; y < HEIGHT; y++) {
            flated[y + HEIGHT * x] = src[x][y];
        }
    }
}

static QueueHandle_t m_render_queue;

static void revertOdd(hsv_t *rawData, ws2812_pixel_t *pixels) {

    for (int x = 0; x < WIDTH; x++) {
        bool odd = x % 2;
        for (int y = 0; y < HEIGHT; y++) {
            int locY = !odd ? y : HEIGHT - y - 1;
            pixels[y + x * HEIGHT] = Color_hsv2rgb(rawData[locY + x * HEIGHT]);
        }
    }
}

static void Render_revertOdd(hsv_t rawData[WIDTH][HEIGHT], ws2812_pixel_t *pixels) {

    for (int x = 0; x < WIDTH; x++) {
        bool odd = x % 2;
        for (int y = 0; y < HEIGHT; y++) {
            int locY = !odd ? y : HEIGHT - y - 1;
            pixels[y + x * HEIGHT] = Color_hsv2rgb(rawData[x][locY]);
        }
    }
}

static void Render_generateRainbow(int frame, ws2812_pixel_t *pixels,
                                   bool vertical, int speed, bool direction) {
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

static void Render_generateWave(int frame, ws2812_pixel_t *pixels,
                                bool direction, int speed, bool mirror,
                                int hue) {

    hsv_t data[HEIGHT*WIDTH];

    for (int x = 0; x < WIDTH; x++) {
        int l_locX = mirror ? x : WIDTH - x - 1;
        for (int y = 0; y < HEIGHT; y++) {
            int l_locY = direction ? HEIGHT - y - 1 : y;
            hsv_t l_data;
            l_data.h = hue;
            l_data.s = 1;
            l_data.v = 1;
            if (((y + frame / (11 - speed)) % HEIGHT >= (x / 3 + 3)) &&
                ((y + frame / (11 - speed)) % HEIGHT < (x / 3 + 6))) {
            } else {
                l_data.v = 0;
            }
            data[l_locX * HEIGHT + l_locY] = l_data;
        }
    }
    revertOdd(data, pixels);
}

static int m_coef = 3;
static int m_matrixOffset = (int)ceil(HEIGHT/3);
static int m_phase = 0;
static int m_speed = 0;
static void Render_generateTapes(int frame, ws2812_pixel_t *pixels, int speed,
                                 int hue) {
    hsv_t data[HEIGHT*WIDTH];
    m_speed = speed;
    for (int x = 0; x < WIDTH; x++) {
        for (int y = 0; y < HEIGHT; y++) {
            hsv_t l_data;
            l_data.h = hue;
            l_data.s = 1;
            l_data.v = 0;
            if (
                ((frame / (10 / speed) + y) % (m_matrixOffset * 2) == (int)(x / m_coef)) 
                || ((frame / (10 / speed) + y - 3) % (m_matrixOffset * 2) == (int)((x) / -m_coef) + ceil(HEIGHT / 2 -1))
            ) {
                l_data.v = m_phase ? (float)(1 - (float)m_phase / 10) : 1;
                if(m_phase > 0 && y > 0) {
                    data[x * WIDTH + y - 1].v =  (float)m_phase / 10;
                }
            }
            data[x * HEIGHT + y] = l_data;
        }
    }
    revertOdd(data, pixels);
    m_phase = (m_phase + speed) % 10;
    if(m_speed != speed) m_phase = 0;
}

static struct snow_item m_snowArr[SNOW_NUMBER];
static bool m_vacantPos[SNOW_NUMBER] = {true, true, true, true,
                                        true, true, true};

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
            pixels[m_snowArr[i].x * HEIGHT + m_snowArr[i].y] =
                Color_hsv2rgb(l_data);
            if (l_age <= 0)
                m_vacantPos[i] = true;
            else
                m_snowArr[i].age--;
        }
    }
}

int s_ageDivider = 3;
#define MATRIX_MAX_COUNT 10
static struct matrixModeState s_matrixItems[MATRIX_MAX_COUNT];

static void Render_generateMatrix(ws2812_pixel_t *pixels, int hue, int tailLength) {
    hsv_t data[WIDTH][HEIGHT];
    Render_fillBlack(data);
    int l_emptyPos = -1;
    for (int i = 0; i < MATRIX_MAX_COUNT; i++) {
        if(s_matrixItems[i].age > (HEIGHT - 1)*s_ageDivider + 4) {
            l_emptyPos = i;
            break;
        }
    }
    if(l_emptyPos != -1) {
        if((int)((uint32_t)random()) % 10 > 5) {
            s_matrixItems[l_emptyPos].col = (int)((uint32_t)random()) % WIDTH;
            s_matrixItems[l_emptyPos].age = 0;
        }

    }
    for(int i = 0; i < MATRIX_MAX_COUNT; i++) {
        hsv_t l_data;
        l_data.h = hue;
        l_data.s = 1;
        l_data.v = 1;
        int l_pixelPosition = HEIGHT - 1 - (int)(s_matrixItems[i].age / s_ageDivider);

        if(s_matrixItems[i].col >= 0 && s_matrixItems[i].col < WIDTH) {
            
            if(l_pixelPosition < HEIGHT && l_pixelPosition >= 0){
                data[s_matrixItems[i].col][l_pixelPosition] = (l_data);
            }
            for (int j = 1; j <= tailLength/10; j++) {
                hsv_t l_tailData;
                l_tailData.h = hue;
                l_tailData.s = 1;
                l_tailData.v = 0.05;
                int l_tailPosition = l_pixelPosition + j;
                if(l_tailPosition < HEIGHT && l_tailPosition >= 0) {
                    data[s_matrixItems[i].col][l_tailPosition] = l_tailData;
                }
            }
        }
        s_matrixItems[i].age++;
    }
    Render_revertOdd(data, pixels);
}



static void Render_generateTornado(int frame, ws2812_pixel_t *pixels,
                                   int tailHue, int tailLength, int hue) {

    hsv_t data[HEIGHT*WIDTH];

    for (int x = 0; x < WIDTH; x++) {
        for (int y = 0; y < HEIGHT; y++) {
            hsv_t l_data;
            l_data.h = 0;
            l_data.s = 1;
            l_data.v = 0;

            data[x * HEIGHT + y] = l_data;
        }
    }
    int l_currentPos = frame % (WIDTH * HEIGHT);
    int l_currentX = l_currentPos % WIDTH;
    int l_currentY = (int)l_currentPos / WIDTH;
    for (int i = 0; i < tailLength; i++) {
        int l_tailPos = l_currentPos - 1 - i;
        if (l_tailPos < 0)
            l_tailPos = WIDTH * HEIGHT + l_tailPos;
        int l_tailX = l_tailPos % WIDTH;
        int l_tailY = (int)l_tailPos / WIDTH;
        int l_currTailPos = (l_tailX * HEIGHT + l_tailY) % (WIDTH * HEIGHT);
        hsv_t l_data;
        l_data.h = tailHue;
        l_data.s = 1;
        l_data.v = (float)1 / (i * i * 0.5 + 1);
        data[l_currTailPos] = l_data;
    }
    hsv_t l_data;
    l_data.s = 1;
    l_data.v = 1;
    l_data.h = hue;
    data[l_currentX * HEIGHT + l_currentY] = l_data;
    revertOdd(data, pixels);
}

static void Render_shutdown(ws2812_pixel_t *pixels) {
    hsv_t data[WIDTH][HEIGHT];
    Render_fillBlack(data);
    Render_revertOdd(data, pixels);
}

static void Render_render(int frame, ws2812_pixel_t *pixels) {

    struct parserData_t *l_params = ModeConfig_getCurrentParams();

    switch (l_params->currentMode) {
    case RAINBOW_MODE:
        Render_generateRainbow(
            frame,
            pixels,
            l_params->rainbowAlign,
            l_params->currentSpeed,
            l_params->currentDirection
        );
        break;
    case WAVE_MODE:
        Render_generateWave(
            frame,
            pixels,
            l_params->currentMirror,
            l_params->currentSpeed,
            l_params->currentDirection,
            l_params->currentHue
        );
        break;
    case TAPES_MODE:
        Render_generateTapes(
            frame,
            pixels,
            l_params->currentSpeed,
            l_params->currentHue
        );
        break;
    case SNOW_MODE:
        Render_generateSnow(pixels, l_params->currentHue);
        break;
    case TORNADO_MODE:
        Render_generateTornado(
            frame,
            pixels,
            l_params->currentTailHue,
            l_params->currentTailLength,
            l_params->currentHue
        );
        break;
    case DISABLE_MODE:
        Render_shutdown(pixels);
        break;
    case MATRIX_MODE:
        Render_generateMatrix(pixels, l_params->currentHue, l_params->currentTailLength);
        break;
    default:
        Render_generateRainbow(
            frame,
            pixels,
            l_params->rainbowAlign,
            l_params->currentSpeed,
            l_params->currentDirection
        );
        break;
    }
}

static void Render_loop(void *pvParameters) {
    ws2812_pixel_t l_pixels[LED_NUMBER];
    int head_index = 0;

    ws2812_i2s_init(LED_NUMBER, PIXEL_RGB);

    memset(l_pixels, 0, sizeof(ws2812_pixel_t) * LED_NUMBER);

    while (1) {
        int index;
        if (xQueueReceive(m_render_queue, &index, portMAX_DELAY)) {
            Render_render(s_frame, &l_pixels[0]);
            ws2812_i2s_update(l_pixels, PIXEL_RGB);
            s_frame++;
        }
    }
}

static void Render_timerINterruptHandler(void *arg) {
    int l_index = 1;
    xQueueSendFromISR(m_render_queue, &l_index, NULL);
}

void Render_init() {
    for (int i = 0; i < MATRIX_MAX_COUNT; i++) {
        s_matrixItems[i].age = 200;
        s_matrixItems[i].col = 0;
    }
    timer_set_interrupts(FRC1, false);
    timer_set_run(FRC1, false);
    _xt_isr_attach(INUM_TIMER_FRC1, Render_timerINterruptHandler, NULL);
    timer_set_frequency(FRC1, RENDER_FREQ);
    timer_set_interrupts(FRC1, true);
    timer_set_run(FRC1, true);
    m_render_queue = xQueueCreate(10, sizeof(int));
    xTaskCreate(&Render_loop, "ws2812_i2s", 2048, NULL, 10, NULL);
}