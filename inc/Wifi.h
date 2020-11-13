#ifndef _INC_WIFI_H_
#define _INC_WIFI_H_

#ifdef __cplusplus
extern "C" {
#endif

#define AP_SSID "Girland"
#define AP_PSK "12345678"

enum Wifi_modes {
  Wifi_modeSta,
  Wifi_modeAp
};

void Wifi_init();
void Wifi_connect();

#ifdef __cplusplus
}  // extern "C" 
#endif 

#endif
