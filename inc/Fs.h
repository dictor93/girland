#ifndef _INC_WIFI_H_
#define _INC_WIFI_H_

#ifdef __cplusplus
extern "C" {
#endif

void Fs_init();
void Fs_readFile(char*name, char*buff, int bufflen);
void Fs_writeFile(char*name, char*data);

#ifdef __cplusplus
}  // extern "C" 
#endif 

#endif
