#ifndef _INC_HTTP_SERVER_H_
#define _INC_HTTP_SERVER_H_

#ifdef __cplusplus
extern "C" {
#endif


#define RAINBOW_MODE 0
#define WAVE_MODE 1
#define TAPES_MODE 2
#define SNOW_MODE 3
#define TORNADO_MODE 4
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

#define PAGE_BUFFER_LENGTH 3400
#define MAX_URI_LEN 32


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

void HttpServer_init();

struct parserData_t* HttpServer_getCurrentParams();

#ifdef __cplusplus
}  // extern "C" 
#endif 

#endif