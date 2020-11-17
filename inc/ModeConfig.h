
#ifndef _INC_MODECONFIG_H_
#define _INC_MODECONFIG_H_

#ifdef __cplusplus
extern "C" {
#endif
#include <stdbool.h>

#define RAINBOW_MODE 0
#define WAVE_MODE 1
#define TAPES_MODE 2
#define SNOW_MODE 3
#define TORNADO_MODE 4
#define MATRIX_MODE 5
#define VAGINE_MODE 6
#define DISABLE_MODE -1

#define FAST 10
#define MIDDLE 5
#define SLOW 2

#define DEFAULT_SNOW_LIFE 30 * 2 // 5sec;
#define DEFAULT_SNOW_NUMBER 15

#define DEFAULT_TORNADO_TAIL_LENGTH 8
#define DEFAULT_TORNADO_TAIL_HUE 16

#define FORWARD true
#define BACKWARD false
#define NOT_MIRROR false
#define MIRROR true
#define IS_RAINBOW_VERTICAL true
#define IS_RAINBOW_HORISONTAL false

enum ActionType {
    kMode,
    kDirection,
    kSpeed,
    kMirror,
    kAlign,
    kMainColor,
    kTail,
    kStatus,
    kRoot,
    kSettings,
    kSettingsState,
    kStyles,
    kWifiSet,
    kResetWifi,
    kOther,
};

struct parserData_t {
  int currentHue;
  int currentMode;
  int currentSpeed;
  int currentTailHue;
  int currentSnowLife;
  int currentSnowNumber;
  int currentTailLength;
  bool rainbowAlign;
  bool currentMirror;
  bool currentDirection;
};


void ModeConfig_init();
void ModeConfig_setConfig(char *uri, enum ActionType l_type);
struct parserData_t *ModeConfig_getCurrentParams();

#ifdef __cplusplus
}  // extern "C" 
#endif 

#endif
