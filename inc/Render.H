#ifndef _INC_RENDER_H_
#define _INC_RENDER_H_

#ifdef __cplusplus
extern "C" {
#endif

typedef struct snow_item {
    int maxAge;
    int age;
    int x;
    int y;
} snow_item;

void Render_init();


#ifdef __cplusplus
} // extern "C"
#endif

#endif