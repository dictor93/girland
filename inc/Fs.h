#ifndef _INC_FS_H_
#define _INC_FS_H_

#ifdef __cplusplus
extern "C" {
#endif

void Fs_init();
int Fs_readFile(char*name, char*buff, int bufflen);
void Fs_writeFile(char*name, char*data, int size);

#ifdef __cplusplus
}  // extern "C" 
#endif 

#endif
