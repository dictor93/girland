
#include "Color.h"
#include <ws2812_i2s/ws2812_i2s.h>

ws2812_pixel_t Color_hexToRGB(uint32_t hex) {
    ws2812_pixel_t l_data;
    l_data.red = (hex & 0xFF0000) >> 16;
    l_data.green = (hex & 0x00FF00) >> 8;
    l_data.blue = (hex & 0x0000FF) >> 0;
    return l_data;
}

ws2812_pixel_t Color_hsv2rgb(hsv_t in) {
    double l_hh, l_p, l_q, l_t, l_ff;
    long l_i;
    ws2812_pixel_t l_out;
    if (in.s <= 0.0) { // < is bogus, just shuts up warnings

        l_out.red = (int)(in.v * 255);
        l_out.green = (int)(in.v * 255);
        l_out.blue = (int)(in.v * 255);
        return l_out;
    }
    l_hh = in.h;
    if (l_hh >= 360.0)
        l_hh = 0.0;
    l_hh /= 60.0;
    l_i = (long)l_hh;
    l_ff = l_hh - l_i;
    l_p = in.v * (1.0 - in.s);
    l_q = in.v * (1.0 - (in.s * l_ff));
    l_t = in.v * (1.0 - (in.s * (1.0 - l_ff)));

    switch (l_i) {
    case 0:
        l_out.red = (int)(in.v * 255);
        l_out.green = (int)(l_t * 255);
        l_out.blue = (int)(l_p * 255);
        break;
    case 1:
        l_out.red = (int)(l_q * 255);
        l_out.green = (int)(in.v * 255);
        l_out.blue = (int)(l_p * 255);
        break;
    case 2:
        l_out.red = (int)(l_p * 255);
        l_out.green = (int)(in.v * 255);
        l_out.blue = (int)(l_t * 255);
        break;

    case 3:
        l_out.red = (int)(l_p * 255);
        l_out.green = (int)(l_q * 255);
        l_out.blue = (int)(in.v * 255);
        break;
    case 4:
        l_out.red = (int)(l_t * 255);
        l_out.green = (int)(l_p * 255);
        l_out.blue = (int)(in.v * 255);
        break;
    case 5:
    default:
        l_out.red = (int)(in.v * 255);
        l_out.green = (int)(l_p * 255);
        l_out.blue = (int)(l_q * 255);
        break;
    }
    return l_out;
}
