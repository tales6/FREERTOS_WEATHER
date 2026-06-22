#include "weather.h"

static char* json_find_value(const char* json, const char* key, char* value, int value_len)
{
    char* p = strstr(json, key);
    if (p == NULL) return NULL;

    p = strchr(p, ':');
    if (p == NULL) return NULL;

    p++;

    while (*p == ' ' || *p == '\t') p++;

    if (*p == '"')
    {
        p++;
        char* end = strchr(p, '"');
        if (end == NULL) return NULL;

        int len = end - p;
        if (len > value_len - 1) len = value_len - 1;
        strncpy(value, p, len);
        value[len] = '\0';
        return value;
    }
    else if (*p == '{' || *p == '[')
    {
        return NULL;
    }
    else
    {
        char* end = p;
        while (*end != ',' && *end != '}' && *end != '\n' && *end != '\r')
        {
            end++;
        }
        int len = end - p;
        if (len > value_len - 1) len = value_len - 1;
        strncpy(value, p, len);
        value[len] = '\0';
        return value;
    }
}

static void sanitize_to_ascii(char* str)
{
    char* r = str;
    char* w = str;
    while (*r)
    {
        if ((unsigned char)*r < 0x80)
        {
            *w++ = *r;
        }
        r++;
    }
    *w = '\0';
}

int weather_parse_now(const char* json_response, WeatherData_t* weather)
{
    if (json_response == NULL || weather == NULL)
    {
        return -1;
    }

    memset(weather, 0, sizeof(WeatherData_t));

    const char* results_start = strstr(json_response, "results");
    if (results_start == NULL)
    {
        return -2;
    }

    const char* first_brace = strchr(results_start, '[');
    if (first_brace == NULL)
    {
        return -3;
    }
    const char* first_obj = strchr(first_brace, '{');
    if (first_obj == NULL)
    {
        return -4;
    }

    const char* location_start = strstr(first_obj, "location");
    if (location_start != NULL)
    {
        json_find_value(location_start, "name", weather->city, sizeof(weather->city));
    }

    const char* now_start = strstr(first_obj, "now");
    if (now_start != NULL)
    {
        json_find_value(now_start, "text", weather->weather, sizeof(weather->weather));
        json_find_value(now_start, "temperature", weather->temp, sizeof(weather->temp));
        json_find_value(now_start, "humidity", weather->humidity, sizeof(weather->humidity));
    }

    sanitize_to_ascii(weather->city);
    sanitize_to_ascii(weather->weather);
    sanitize_to_ascii(weather->temp);
    sanitize_to_ascii(weather->humidity);

    if (strlen(weather->city) == 0)
    {
        strcpy(weather->city, "Unknown");
    }
    if (strlen(weather->weather) == 0)
    {
        strcpy(weather->weather, "Unknown");
    }
    if (strlen(weather->temp) == 0)
    {
        strcpy(weather->temp, "--");
    }
    if (strlen(weather->humidity) == 0)
    {
        strcpy(weather->humidity, "--");
    }

    return 0;
}
