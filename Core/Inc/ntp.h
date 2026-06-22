#ifndef __NTP_H__
#define __NTP_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

int ntp_get_time(const char* server, uint32_t* timestamp);
void ntp_format_time(char* buf, uint32_t timestamp);

#ifdef __cplusplus
}
#endif

#endif
