

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

static int s_activeRenderMode = NULL;
static TaskHandle_t s_currentRenderTask = NULL;

QueueHandle_t m_render_queue;
static ws2812_pixel_t s_pixels[LED_NUMBER];


static void Render_fillBlack(hsv_t pixels[WIDTH][HEIGHT]) {
    for (int x = 0; x < WIDTH; x++) {
        for (int y = 0; y < HEIGHT; y++) {
            pixels[x][y].v = 0;
        }
    }
}

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


static void s_Render_RainbowTask(ws2812_pixel_t *pixels) {
    int index;
    int l_frame = 0;
    while (1) {
        if (xQueueReceive(m_render_queue, &index, 0xffffffffUL)) {
            printf("Running rainbow task\n");
            struct parserData_t *l_params = ModeConfig_getCurrentParams();
            

            int l_ledsInGradient = l_params->rainbowAlign ? HEIGHT : WIDTH;
            hsv_t l_data;
            for (int x = 0; x < WIDTH; x++) {
                int l_locX = l_params->currentDirection ? x : WIDTH - x - 1;
                int l_pos = 0;
                if (!l_params->rainbowAlign)
                    l_pos = (int)(((double)l_frame) * l_params->currentSpeed +
                                (double)l_locX * 360 / l_ledsInGradient) %
                            360;
                for (int y = 0; y < HEIGHT; y++) {
                    int locY = (l_locX % 2) ? y : HEIGHT - y - 1;
                    locY = l_params->currentDirection ? locY : HEIGHT - locY - 1;
                    if (l_params->rainbowAlign)
                        l_pos = (int)(((double)l_frame) * l_params->currentSpeed +
                                    (double)locY * 360 / l_ledsInGradient) %
                                360;
                    l_data.h = (double)l_pos;
                    l_data.s = 1.0;
                    l_data.v = 1.0;

                    pixels[x * HEIGHT + y] = Color_hsv2rgb(l_data);
                }
            }


            ws2812_i2s_update(pixels, PIXEL_RGB);
            l_frame++;
        }
    }
}
static void s_Render_WaveTask(ws2812_pixel_t *pixels) {
    int index;
    int l_frame = 0;
    while (1) {
        if (xQueueReceive(m_render_queue, &index, portMAX_DELAY)) {
            printf("Running wave task\n");
            struct parserData_t *l_params = ModeConfig_getCurrentParams();


            hsv_t data[HEIGHT*WIDTH];
            for (int x = 0; x < WIDTH; x++) {
                int l_locX = l_params->currentMirror ? x : WIDTH - x - 1;
                for (int y = 0; y < HEIGHT; y++) {
                    int l_locY = l_params->currentDirection ? HEIGHT - y - 1 : y;
                    hsv_t l_data;
                    l_data.h = l_params->currentHue;
                    l_data.s = 1;
                    l_data.v = 1;
                    if (((y + l_frame / (11 - l_params->currentSpeed)) % HEIGHT >= (x / 3 + 3)) &&
                        ((y + l_frame / (11 - l_params->currentSpeed)) % HEIGHT < (x / 3 + 6))) {
                    } else {
                        l_data.v = 0;
                    }
                    data[l_locX * HEIGHT + l_locY] = l_data;
                }
            }
            revertOdd(data, pixels);


            ws2812_i2s_update(pixels, PIXEL_RGB);
            l_frame++;
        }
    }
}

static void s_Render_TapesTask(ws2812_pixel_t *pixels) {

    static int m_coef = 3;
    static int m_matrixOffset = (int)ceil(HEIGHT/3);
    static int m_phase = 0;
    static int m_speed = 0;

    int index;
    int l_frame = 0;
    while (1) {
        if (xQueueReceive(m_render_queue, &index, portMAX_DELAY)) {
            printf("Running wave task\n");
            struct parserData_t *l_params = ModeConfig_getCurrentParams();


            hsv_t data[HEIGHT*WIDTH];
            m_speed = l_params->currentSpeed;
            for (int x = 0; x < WIDTH; x++) {
                for (int y = 0; y < HEIGHT; y++) {
                    hsv_t l_data;
                    l_data.h = l_params->currentHue;
                    l_data.s = 1;
                    l_data.v = 0;
                    if (
                        ((l_frame / (10 / l_params->currentSpeed) + y) % (m_matrixOffset * 2) == (int)(x / m_coef)) 
                        || ((l_frame / (10 / l_params->currentSpeed) + y - 3) % (m_matrixOffset * 2) == (int)((x) / -m_coef) + ceil(HEIGHT / 2 -1))
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
            m_phase = (m_phase + l_params->currentSpeed) % 10;
            if(m_speed != l_params->currentSpeed) m_phase = 0;


            ws2812_i2s_update(pixels, PIXEL_RGB);
            l_frame++;
        }
    }
}

static void s_Render_SnowTask(ws2812_pixel_t *pixels) {

    static struct snow_item m_snowArr[SNOW_NUMBER];
    static bool m_vacantPos[SNOW_NUMBER] = {true, true, true, true,
                                        true, true, true};

    int index;
    int l_frame = 0;
    while (1) {
        if (xQueueReceive(m_render_queue, &index, portMAX_DELAY)) {
            printf("Running wave task\n");
            struct parserData_t *l_params = ModeConfig_getCurrentParams();


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
                    l_data.h = l_params->currentHue;
                    l_data.s = 1;
                    pixels[m_snowArr[i].x * HEIGHT + m_snowArr[i].y] =
                        Color_hsv2rgb(l_data);
                    if (l_age <= 0)
                        m_vacantPos[i] = true;
                    else
                        m_snowArr[i].age--;
                }
            }


            ws2812_i2s_update(pixels, PIXEL_RGB);
            l_frame++;
        }
    }
}

static void s_Render_TornadoTask(ws2812_pixel_t *pixels) {
    int index;
    int l_frame = 0;
    while (1) {
        if (xQueueReceive(m_render_queue, &index, portMAX_DELAY)) {
            printf("Running wave task\n");
            struct parserData_t *l_params = ModeConfig_getCurrentParams();


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
            int l_currentPos = l_frame % (WIDTH * HEIGHT);
            int l_currentX = l_currentPos % WIDTH;
            int l_currentY = (int)l_currentPos / WIDTH;
            for (int i = 0; i < l_params->currentTailLength; i++) {
                int l_tailPos = l_currentPos - 1 - i;
                if (l_tailPos < 0)
                    l_tailPos = WIDTH * HEIGHT + l_tailPos;
                int l_tailX = l_tailPos % WIDTH;
                int l_tailY = (int)l_tailPos / WIDTH;
                int l_currTailPos = (l_tailX * HEIGHT + l_tailY) % (WIDTH * HEIGHT);
                hsv_t l_data;
                l_data.h = l_params->currentTailHue;
                l_data.s = 1;
                l_data.v = (float)1 / (i * i * 0.5 + 1);
                data[l_currTailPos] = l_data;
            }
            hsv_t l_data;
            l_data.s = 1;
            l_data.v = 1;
            l_data.h = l_params->currentTailHue;
            data[l_currentX * HEIGHT + l_currentY] = l_data;
            revertOdd(data, pixels);


            ws2812_i2s_update(pixels, PIXEL_RGB);
            l_frame++;
        }
    }
}

static void s_Render_ShutdownTask(ws2812_pixel_t *pixels) {
    int index;
    int l_frame = 0;
    while (1) {
        if (xQueueReceive(m_render_queue, &index, portMAX_DELAY)) {
            printf("Running wave task\n");
            struct parserData_t *l_params = ModeConfig_getCurrentParams();


            hsv_t data[WIDTH][HEIGHT];
            Render_fillBlack(data);
            Render_revertOdd(data, pixels);


            ws2812_i2s_update(pixels, PIXEL_RGB);
            l_frame++;
        }
    }
}

static void s_Render_MatrixTask(ws2812_pixel_t *pixels) {

    int s_ageDivider = 3;
    #define MATRIX_MAX_COUNT 10
    static struct matrixModeState s_matrixItems[MATRIX_MAX_COUNT];

    for (int i = 0; i < MATRIX_MAX_COUNT; i++) {
        s_matrixItems[i].age = 200;
        s_matrixItems[i].col = 0;
    }

    int index;
    int l_frame = 0;
    while (1) {
        if (xQueueReceive(m_render_queue, &index, portMAX_DELAY)) {
            printf("Running wave task\n");
            struct parserData_t *l_params = ModeConfig_getCurrentParams();


            hsv_t data[WIDTH][HEIGHT];
            Render_fillBlack(data);
            int l_emptyPos = -1;
            for (int i = 0; i < MATRIX_MAX_COUNT; i++) {
                if(s_matrixItems[i].age > (HEIGHT - 1)*s_ageDivider + s_matrixItems[i].tail) {
                    l_emptyPos = i;
                    break;
                }
            }
            if(l_emptyPos != -1) {
                if((int)((uint32_t)random()) % 10 > 5) {
                    s_matrixItems[l_emptyPos].col = (int)((uint32_t)random()) % WIDTH;
                    s_matrixItems[l_emptyPos].age = 0;
                    s_matrixItems[l_emptyPos].tail = (int)((uint32_t)random()) % (HEIGHT/2) + 2;
                }

            }
            for(int i = 0; i < MATRIX_MAX_COUNT; i++) {
                hsv_t l_data;
                l_data.h = l_params->currentHue;
                l_data.s = 1;
                l_data.v = 1;
                int l_pixelPosition = HEIGHT - 1 - (int)(s_matrixItems[i].age / s_ageDivider);

                if(s_matrixItems[i].col >= 0 && s_matrixItems[i].col < WIDTH) {
                    
                    if(l_pixelPosition < HEIGHT && l_pixelPosition >= 0){
                        data[s_matrixItems[i].col][l_pixelPosition] = (l_data);
                    }
                    for (int j = 1; j <= s_matrixItems[i].tail; j++) {
                        hsv_t l_tailData;
                        l_tailData.h = l_params->currentHue;
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


            ws2812_i2s_update(pixels, PIXEL_RGB);
            l_frame++;
        }
    }
}

static void s_Render_VagineTask(ws2812_pixel_t *pixels) {
    const uint8_t s_vaginePixels[WIDTH][HEIGHT] = {
    { 0x39, 0xdc, 0xf7, 0xf8, 0xfd, 0x4b, 0xab, 0x77, 0x77, 0xab, 0x4b, 0xfd, 0xf8, 0xf7, 0xdc, 0x39 },
    { 0xdc, 0xf7, 0xf8, 0xfd, 0x4b, 0xab, 0x77, 0xbf, 0xbf, 0x77, 0xab, 0x4b, 0xfd, 0xf8, 0xf7, 0xdc },
    { 0xf7, 0xf8, 0xfd, 0x4b, 0xab, 0x77, 0xbf, 0xdb, 0xdb, 0xbf, 0x77, 0xab, 0x4b, 0xfd, 0xf8, 0xf7 },
    { 0xf8, 0xfd, 0x4b, 0xab, 0x77, 0xbf, 0xdb, 0x80, 0x80, 0xdb, 0xbf, 0x77, 0xab, 0x4b, 0xfd, 0xf8 },
    { 0xfd, 0x4b, 0xab, 0x77, 0xbf, 0xdb, 0x80, 0xe5, 0xe5, 0x80, 0xdb, 0xbf, 0x77, 0xab, 0x4b, 0xfd },
    { 0x4b, 0xab, 0x77, 0xbf, 0xdb, 0x80, 0xe5, 0xe5, 0xe5, 0xe5, 0x80, 0xdb, 0xbf, 0x77, 0xab, 0x4b },
    { 0x4b, 0xab, 0x77, 0xbf, 0xdb, 0x80, 0xe5, 0xfc, 0xfc, 0xe5, 0x80, 0xdb, 0xbf, 0x77, 0xab, 0x4b },
    { 0xab, 0x77, 0xbf, 0xdb, 0x80, 0xe5, 0xe5, 0xfc, 0xfc, 0xe5, 0xe5, 0x80, 0xdb, 0xbf, 0x77, 0xab },
    { 0xab, 0x77, 0xbf, 0xdb, 0x80, 0xe5, 0xe5, 0xfc, 0xfc, 0xe5, 0xe5, 0x80, 0xdb, 0xbf, 0x77, 0xab },
    { 0x4b, 0xab, 0x77, 0xbf, 0xdb, 0x80, 0xe5, 0xfc, 0xfc, 0xe5, 0x80, 0xdb, 0xbf, 0x77, 0xab, 0x4b },
    { 0x4b, 0xab, 0x77, 0xbf, 0xdb, 0x80, 0xe5, 0xe5, 0xe5, 0xe5, 0x80, 0xdb, 0xbf, 0x77, 0xab, 0x4b },
    { 0xfd, 0x4b, 0xab, 0x77, 0xbf, 0xdb, 0x80, 0xe5, 0xe5, 0x80, 0xdb, 0xbf, 0x77, 0xab, 0x4b, 0xfd },
    { 0xf8, 0xfd, 0x4b, 0xab, 0x77, 0xbf, 0xdb, 0x80, 0x80, 0xdb, 0xbf, 0x77, 0xab, 0x4b, 0xfd, 0xf8 },
    { 0xf7, 0xf8, 0xfd, 0x4b, 0xab, 0x77, 0xbf, 0xdb, 0xdb, 0xbf, 0x77, 0xab, 0x4b, 0xfd, 0xf8, 0xf7 },
    { 0xdc, 0xf7, 0xf8, 0xfd, 0x4b, 0xab, 0x77, 0xbf, 0xbf, 0x77, 0xab, 0x4b, 0xfd, 0xf8, 0xf7, 0xdc },
    { 0x39, 0xdc, 0xf7, 0xf8, 0xfd, 0x4b, 0xab, 0x77, 0x77, 0xab, 0x4b, 0xfd, 0xf8, 0xf7, 0xdc, 0x39 },
    };

    #define VAGINA_FRAMES 13

    int colorQue[VAGINA_FRAMES] = { 0xfc,  0xe5,  0x80,  0xdb,  0xbf,  0x77,  0xab,  0x4b,  0xfd,  0xf8,  0xf7,  0xdc,  0x39 };

    int index;
    int l_frame = 0;
    while (1) {
        if (xQueueReceive(m_render_queue, &index, portMAX_DELAY)) {
            printf("Running wave task\n");
            struct parserData_t *l_params = ModeConfig_getCurrentParams();


            hsv_t data[WIDTH][HEIGHT];
            Render_fillBlack(data);
            int localFrame = l_params->currentDirection ? l_frame % 360  : 360 - l_frame % 360 - 1;

            int centralHue = (localFrame * 5);

            hsv_t l_data;
            for (int x = 0; x < WIDTH; x++)
            {
                for (int y = 0; y < HEIGHT; y++)
                {
                l_data.h = centralHue;
                l_data.s = 1;
                l_data.v = 1;
                    int pos = 0;
                    for (int i = 0; i < VAGINA_FRAMES; i++)
                    {
                        if (s_vaginePixels[x][y] == colorQue[i]) {
                            pos = i;
                            break;
                        }
                    }
                    l_data.h = (centralHue + pos*(360/VAGINA_FRAMES))%360;
                    
                    data[x][y] = l_data;
                }
                
            }
            Render_revertOdd(data, pixels);


            ws2812_i2s_update(pixels, PIXEL_RGB);
            l_frame++;
        }
    }
}

static void Render_timerINterruptHandler(void *arg) {
    int l_index = 1;
    xQueueSendFromISR(m_render_queue, &l_index, NULL);
}

void Render_changeRenderHandler() {
    struct parserData_t *l_params = ModeConfig_getCurrentParams();
    if(l_params->currentMode == s_activeRenderMode) {
        return;
    }
    s_activeRenderMode = l_params->currentMode;
    if(s_currentRenderTask != NULL) {
        vTaskDelete(s_currentRenderTask);
    }
    printf("CHANGING MODE: %d\n\n", l_params->currentMode);
    switch (l_params->currentMode) {
    case RAINBOW_MODE:
        xTaskCreate(&s_Render_RainbowTask, "ws2812_i2s", 2048, s_pixels, 10, &s_currentRenderTask);
        break;
    case WAVE_MODE:
         xTaskCreate(&s_Render_WaveTask, "ws2812_i2s", 2048, s_pixels, 10, &s_currentRenderTask);
        break;
    case TAPES_MODE:
        xTaskCreate(&s_Render_TapesTask, "ws2812_i2s", 2048, s_pixels, 10, &s_currentRenderTask);
        break;
    case SNOW_MODE:
        xTaskCreate(&s_Render_SnowTask, "ws2812_i2s", 2048, s_pixels, 10, &s_currentRenderTask);
        break;
    case TORNADO_MODE:
        xTaskCreate(&s_Render_TornadoTask, "ws2812_i2s", 2048, s_pixels, 10, &s_currentRenderTask);
        break;
    case DISABLE_MODE:
        xTaskCreate(&s_Render_ShutdownTask, "ws2812_i2s", 2048, s_pixels, 10, &s_currentRenderTask);
        break;
    case MATRIX_MODE:
        xTaskCreate(&s_Render_MatrixTask, "ws2812_i2s", 2048, s_pixels, 10, &s_currentRenderTask);
        break;
    case VAGINE_MODE:
        xTaskCreate(&s_Render_VagineTask, "ws2812_i2s", 2048, s_pixels, 10, &s_currentRenderTask);
        break;
    default:
        xTaskCreate(&s_Render_RainbowTask, "ws2812_i2s", 2048, s_pixels, 10, s_currentRenderTask);
        break;
    }
}

void Render_init() {
    ws2812_i2s_init(LED_NUMBER, PIXEL_RGB);
    memset(s_pixels, 0, sizeof(ws2812_pixel_t) * LED_NUMBER);

    timer_set_interrupts(FRC1, false);
    timer_set_run(FRC1, false);
    _xt_isr_attach(INUM_TIMER_FRC1, Render_timerINterruptHandler, NULL);
    timer_set_frequency(FRC1, RENDER_FREQ);
    timer_set_interrupts(FRC1, true);
    timer_set_run(FRC1, true);
    m_render_queue = xQueueCreate(10, sizeof(int));
    Render_changeRenderHandler();
}