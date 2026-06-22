#include "esp8266.h"
#include "main.h"
#include "cmsis_os.h"
#include <stdio.h>

static char esp8266_ring_buf[ESP8266_RING_BUF_SIZE];
static volatile uint16_t ring_head = 0;
static uint16_t ring_tail = 0;

extern UART_HandleTypeDef huart1;
static uint8_t esp8266_rx_byte;

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART1)
    {
        esp8266_rx_callback(esp8266_rx_byte);
        HAL_UART_Receive_IT(&huart1, &esp8266_rx_byte, 1);
    }
}

static void esp8266_send_cmd(const char* cmd)
{
    HAL_UART_Transmit(&huart1, (uint8_t*)cmd, strlen(cmd), 1000);
}

static void esp8266_send_data(const char* data, uint32_t len)
{
    HAL_UART_Transmit(&huart1, (uint8_t*)data, len, 1000);
}

void esp8266_rx_callback(uint8_t data)
{
    uint16_t next = (ring_head + 1) % ESP8266_RING_BUF_SIZE;
    if (next != ring_tail)
    {
        esp8266_ring_buf[ring_head] = data;
        ring_head = next;
    }
}

int esp8266_rx_available(void)
{
    return (ring_head != ring_tail) ? 1 : 0;
}

int esp8266_rx_read(char *buf, uint32_t len)
{
    uint32_t i = 0;
    while (i < len && ring_tail != ring_head)
    {
        buf[i++] = esp8266_ring_buf[ring_tail];
        ring_tail = (ring_tail + 1) % ESP8266_RING_BUF_SIZE;
    }
    return i;
}

void esp8266_rx_flush(void)
{
    ring_head = 0;
    ring_tail = 0;
}

static int esp8266_wait_response(const char* expected_str, uint32_t timeout)
{
    static char line_buf[256];
    static uint16_t idx = 0;
    uint32_t start = HAL_GetTick();

    while (HAL_GetTick() - start < timeout)
    {
        if (esp8266_rx_available())
        {
            char buf[2];
            if (esp8266_rx_read(buf, 1) > 0)
            {
                char ch = buf[0];

                if (ch == '\n')
                {
                    line_buf[idx] = '\0';
                    if (strstr(line_buf, expected_str) != NULL)
                    {
                        idx = 0;
                        return 0;
                    }
                    if (strstr(line_buf, "ERROR") != NULL ||
                        strstr(line_buf, "FAIL") != NULL)
                    {
                        idx = 0;
                        return -1;
                    }
                    idx = 0;
                }
                else if (ch != '\r')
                {
                    if (idx < sizeof(line_buf) - 1)
                        line_buf[idx++] = ch;

                    line_buf[idx] = '\0';
                    if (strstr(line_buf, expected_str) != NULL)
                    {
                        idx = 0;
                        return 0;
                    }
                }
            }
        }
        osDelay(1);
    }
    idx = 0;
    return -2;
}

int esp8266_init(void)
{
    int ret;

    HAL_UART_Receive_IT(&huart1, &esp8266_rx_byte, 1);

    esp8266_reset();
    HAL_Delay(2000);

    esp8266_send_cmd("ATE0\r\n");
    esp8266_wait_response("OK", 1000);

    esp8266_send_cmd("AT\r\n");
    ret = esp8266_wait_response("OK", 3000);
    if (ret != 0) return -1;

    esp8266_send_cmd("AT+CWMODE=1\r\n");
    ret = esp8266_wait_response("OK", 3000);
    if (ret != 0) return -2;

    return 0;
}

int esp8266_connect_wifi(const char* ssid, const char* password)
{
    char cmd[160];
    int ret;

    esp8266_rx_flush();

    snprintf(cmd, sizeof(cmd), "AT+CWJAP=\"%s\",\"%s\"\r\n", ssid, password);
    esp8266_send_cmd(cmd);

    ret = esp8266_wait_response("OK", 20000);
    if (ret == 0)
    {
        return 0;
    }

    return -1;
}

int esp8266_http_get(const char* host, uint16_t port, const char* path, char* response, uint32_t resp_len)
{
    (void)port;

    char cmd[128];
    char http_req[300];
    int ret;

    memset(response, 0, resp_len);
    esp8266_rx_flush();

    snprintf(cmd, sizeof(cmd), "AT+CIPSTART=\"TCP\",\"%s\",80\r\n", host);
    esp8266_send_cmd(cmd);
    ret = esp8266_wait_response("OK", 10000);
    if (ret != 0) {
        return -1;
    }

    snprintf(http_req, sizeof(http_req),
        "GET %s HTTP/1.1\r\nHost: %s\r\nConnection: close\r\n\r\n",
        path, host);

    snprintf(cmd, sizeof(cmd), "AT+CIPSEND=%d\r\n", (int)strlen(http_req));
    esp8266_send_cmd(cmd);
    ret = esp8266_wait_response(">", 5000);
    if (ret != 0) {
        return -3;
    }

    esp8266_send_data(http_req, strlen(http_req));

    uint32_t start = HAL_GetTick();
    uint32_t idx = 0;

    while (HAL_GetTick() - start < 15000)
    {
        char batch[64];
        int n = esp8266_rx_read(batch, sizeof(batch));
        for (int i = 0; i < n && idx < resp_len - 1; i++)
            response[idx++] = batch[i];
        if (n == 0) osDelay(1);
    }

    response[idx] = '\0';

    char *json = strstr(response, "{");
    if (json == NULL) json = strstr(response, "[");
    if (json == NULL)
    {
        char *rnrn = strstr(response, "\r\n\r\n");
        if (rnrn)
        {
            json = strstr(rnrn + 4, "{");
            if (json == NULL) json = strstr(rnrn + 4, "[");
        }
    }
    if (json != NULL)
    {
        char *r = json, *w = json;
        while (*r)
        {
            if (strncmp(r, "+IPD,", 5) == 0)
            {
                r += 5;
                while (*r >= '0' && *r <= '9') r++;
                if (*r == ':') r++;
                continue;
            }
            *w++ = *r++;
        }
        *w = '\0';
        memmove(response, json, w - json + 1);
        idx = w - json;
    }
    else
    {
        idx = 0;
        response[0] = '\0';
    }

    esp8266_send_cmd("AT+CIPCLOSE\r\n");
    HAL_Delay(200);

    return (idx > 0) ? 0 : -2;
}

void esp8266_reset(void)
{
    esp8266_rx_flush();
    esp8266_send_cmd("AT+RST\r\n");
    HAL_Delay(2000);
    esp8266_rx_flush();
}

void esp8266_send_at(const char* cmd)
{
    esp8266_send_cmd(cmd);
}

void esp8266_send_raw(const char* data, uint32_t len)
{
    esp8266_send_data(data, len);
}

int esp8266_wait_str(const char* expect, uint32_t timeout)
{
    return esp8266_wait_response(expect, timeout);
}
