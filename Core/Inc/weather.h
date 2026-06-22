#ifndef __WEATHER_H__
#define __WEATHER_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <string.h>

typedef struct {
    char city[32];
    char weather[32];
    char temp[8];
    char humidity[8];
} WeatherData_t;

int weather_parse_now(const char* json_response, WeatherData_t* weather);

#ifdef __cplusplus
}
#endif

#endif
