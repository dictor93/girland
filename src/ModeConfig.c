#include "ModeConfig.h"
#include "espressif/esp_common.h"
#include "string.h"
#include "Common.h"

static struct parserData_t s_currentData;

struct parserData_t *ModeConfig_getCurrentParams() {
    return &s_currentData;
}

void ModeConfig_setMode(char *uri) {
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
    }  else if (strstr(uri, "MATRIX")) {
        s_currentData.currentMode = MATRIX_MODE;
        s_currentData.currentHue = 116;
    } else if(strstr(uri, "VAGINE")) {
        s_currentData.currentMode = VAGINE_MODE;
    } else if (strstr(uri, "DISABLE")) {
        s_currentData.currentMode = DISABLE_MODE;
    }
}

static void ModeConfig_setDirection(char *uri) {
    if (strstr(uri, "FORWARD")) {
        s_currentData.currentDirection = FORWARD;
    } else if (strstr(uri, "BACKWARD")) {
        s_currentData.currentDirection = BACKWARD;
    }
}

static void ModeConfig_setSpeed(char *uri) {
    if (strstr(uri, "FAST")) {
        s_currentData.currentSpeed = FAST;
    } else if (strstr(uri, "MIDDLE")) {
        s_currentData.currentSpeed = MIDDLE;
    } else if (strstr(uri, "SLOW")) {
        s_currentData.currentSpeed = SLOW;
    }
}

static void ModeConfig_setMirror(char *uri) {
    if (strstr(uri, "NOT_MIRROR")) {
        s_currentData.currentMirror = NOT_MIRROR;
    } else if (strstr(uri, "MIRROR")) {
        s_currentData.currentMirror = MIRROR;
    }
}

static void ModeConfig_setAlign(char *uri) {
    if (strstr(uri, "VERTICAL")) {
        s_currentData.rainbowAlign = IS_RAINBOW_VERTICAL;
    } else if (strstr(uri, "HORISONTAL")) {
        s_currentData.rainbowAlign = IS_RAINBOW_HORISONTAL;
    }
}

static void ModeConfig_setMainColor(char *uri) {
    char subBuff[4];
    char *colorStart = memchr(uri + 12, '/', MAX_URI_LEN) + 1;
    memcpy(subBuff, colorStart, 3);
    subBuff[3] = '\0';
    sscanf(subBuff, "%d", &s_currentData.currentHue);
}

static void ModeConfig_setTail(char *uri) {
    char subBuff[4];
    if (strstr(uri, "COLOR")) {
        char *colorStart = memchr(uri + 8, '/', MAX_URI_LEN) + 1;
        memcpy(subBuff, colorStart, 3);
        subBuff[3] = '\0';
        sscanf(subBuff, "%d", &s_currentData.currentTailHue);
    } else if (strstr(uri, "LENGTH")) {
        char *colorStart = memchr(uri + 8, '/', MAX_URI_LEN) + 1;
        memcpy(subBuff, colorStart, 3);
        subBuff[3] = '\0';
        sscanf(subBuff, "%d", &s_currentData.currentTailLength);
    }
}

void ModeConfig_setConfig(char *uri, enum ActionType l_type) {
    // получаем тип и в зависимости от него закидываем в соответсвующу функцию
    // которая уже и устанавливает наш конфиг
    switch (l_type) {
    case kMode:
        ModeConfig_setMode(uri);
        break;
    case kDirection:
        ModeConfig_setDirection(uri);
        break;
    case kSpeed:
        ModeConfig_setSpeed(uri);
        break;
    case kMirror:
        ModeConfig_setMirror(uri);
        break;
    case kAlign:
        ModeConfig_setAlign(uri);
        break;
    case kMainColor:
        ModeConfig_setMainColor(uri);
        break;
    case kTail:
        ModeConfig_setTail(uri);
        break;
    default:
        break;
    }
}

void ModeConfig_init() {
    s_currentData.currentHue = 196;
    s_currentData.currentMode = VAGINE_MODE;
    s_currentData.currentSnowLife = DEFAULT_SNOW_LIFE;
    s_currentData.currentSnowNumber = DEFAULT_SNOW_NUMBER;
    s_currentData.currentSpeed = FAST;
    s_currentData.currentTailHue = DEFAULT_TORNADO_TAIL_HUE;
    s_currentData.currentTailLength = DEFAULT_TORNADO_TAIL_LENGTH;
    s_currentData.currentDirection = FORWARD;
    s_currentData.currentMirror = NOT_MIRROR;
    s_currentData.rainbowAlign = IS_RAINBOW_VERTICAL;
}