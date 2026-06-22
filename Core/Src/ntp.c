#include "ntp.h"
#include "main.h"
#include "esp8266.h"
#include "cmsis_os.h"
#include <string.h>
#include <stdio.h>

#define NTP_EPOCH_OFFSET    2208988800UL
#define NTP_SERVER_PORT     123
#define NTP_PACKET_SIZE     48
#define NTP_RESP_TIMEOUT    10000

static void build_ntp_packet(uint8_t* pkt)
{
    memset(pkt, 0, NTP_PACKET_SIZE);
    pkt[0] = 0x1B;
}

int ntp_get_time(const char* server, uint32_t* timestamp)
{
    char cmd[128];
    uint8_t resp_buf[256];
    uint8_t ntp_req[NTP_PACKET_SIZE];
    int ret;

    if (server == NULL || timestamp == NULL)
        return -1;

    esp8266_rx_flush();

    snprintf(cmd, sizeof(cmd), "AT+CIPSTART=\"UDP\",\"%s\",%d,%d,0\r\n",
             server, NTP_SERVER_PORT, NTP_SERVER_PORT);
    esp8266_send_at(cmd);
    ret = esp8266_wait_str("OK", 5000);
    if (ret != 0)
    {
        esp8266_send_at("AT+CIPCLOSE\r\n");
        return -2;
    }

    snprintf(cmd, sizeof(cmd), "AT+CIPSEND=%d\r\n", NTP_PACKET_SIZE);
    esp8266_send_at(cmd);
    ret = esp8266_wait_str(">", 5000);
    if (ret != 0)
    {
        esp8266_send_at("AT+CIPCLOSE\r\n");
        return -3;
    }

    build_ntp_packet(ntp_req);
    esp8266_send_raw((const char*)ntp_req, NTP_PACKET_SIZE);

    uint32_t start = HAL_GetTick();
    uint32_t idx = 0;

    while (HAL_GetTick() - start < NTP_RESP_TIMEOUT)
    {
        char batch[32];
        int n = esp8266_rx_read(batch, sizeof(batch));
        for (int i = 0; i < n && idx < sizeof(resp_buf) - 1; i++)
            resp_buf[idx++] = batch[i];
        if (n == 0) osDelay(1);
    }

    esp8266_send_at("AT+CIPCLOSE\r\n");

    if (idx == 0)
        return -4;

    char* p = (char*)resp_buf;
    char* ipd = NULL;
    for (uint32_t i = 0; i + 5 < idx; i++)
    {
        if (resp_buf[i] == '+' && resp_buf[i+1] == 'I' &&
            resp_buf[i+2] == 'P' && resp_buf[i+3] == 'D' &&
            resp_buf[i+4] == ',')
        {
            ipd = (char*)&resp_buf[i];
            break;
        }
    }
    if (ipd == NULL)
        return -5;

    p = ipd + 5;
    while ((uint32_t)(p - (char*)resp_buf) < idx && *p >= '0' && *p <= '9')
        p++;
    if (*p != ':' || (uint32_t)(p - (char*)resp_buf) >= idx)
        return -6;
    p++;

    if ((uint32_t)(p + 43 - (char*)resp_buf) >= idx)
        return -7;

    uint32_t ntp_time = ((uint32_t)(uint8_t)p[40] << 24) |
                        ((uint32_t)(uint8_t)p[41] << 16) |
                        ((uint32_t)(uint8_t)p[42] << 8) |
                        (uint32_t)(uint8_t)p[43];

    if (ntp_time < NTP_EPOCH_OFFSET)
        return -8;

    *timestamp = ntp_time - NTP_EPOCH_OFFSET;
    return 0;
}

void ntp_format_time(char* buf, uint32_t timestamp)
{
    uint32_t sec = timestamp % 86400;
    uint32_t h = sec / 3600;
    uint32_t m = (sec % 3600) / 60;
    uint32_t s = sec % 60;

    buf[0] = '0' + h / 10;
    buf[1] = '0' + h % 10;
    buf[2] = ':';
    buf[3] = '0' + m / 10;
    buf[4] = '0' + m % 10;
    buf[5] = ':';
    buf[6] = '0' + s / 10;
    buf[7] = '0' + s % 10;
    buf[8] = '\0';
}
