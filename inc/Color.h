
#ifndef _INC_COLOR_H_
#define _INC_COLOR_H_

#ifdef __cplusplus
extern "C" {
#endif

typedef struct hsv_t {
    double h; // angle in degrees
    double s; // a fraction between 0 and 1
    double v; // a fraction between 0 and 1
} hsv_t;

ws2812_pixel_t Color_hsv2rgb(hsv_t in);

ws2812_pixel_t Color_hexToRGB(uint32_t hex);

#ifdef __cplusplus
} // extern "C"
#endif

#endif
