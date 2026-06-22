#ifndef __ESP8266_H__
#define __ESP8266_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <string.h>

#define ESP8266_RING_BUF_SIZE 512

void esp8266_rx_callback(uint8_t data);
int  esp8266_rx_available(void);
int  esp8266_rx_read(char *buf, uint32_t len);
void esp8266_rx_flush(void);

int  esp8266_init(void);
int  esp8266_connect_wifi(const char* ssid, const char* password);
int  esp8266_http_get(const char* host, uint16_t port, const char* path, char* response, uint32_t resp_len);
void esp8266_reset(void);

void esp8266_send_at(const char* cmd);
void esp8266_send_raw(const char* data, uint32_t len);
int  esp8266_wait_str(const char* expect, uint32_t timeout);

#ifdef __cplusplus
}
#endif

#endif
