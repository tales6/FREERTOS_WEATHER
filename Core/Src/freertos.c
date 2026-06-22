/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * File Name          : freertos.c
  * Description        : Code for freertos applications
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "cmsis_os.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "esp8266.h"
#include "weather.h"
#include "oled.h"
#include "ntp.h"
#include "config.h"
#include <stdio.h>
#include <string.h">

static WeatherData_t current_weather;
static volatile uint8_t weather_updated = 0;
static char http_response[1024];

static volatile uint32_t system_time = 0;
static volatile uint8_t time_synced = 0;
static volatile uint8_t current_page = 0;

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define WIFI_CONNECTED_BIT    (1 << 0)
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN Variables */

/* USER CODE END Variables */
/* Definitions for WIFITask */
osThreadId_t WIFITaskHandle;
const osThreadAttr_t WIFITask_attributes = {
  .name = "WIFITask",
  .stack_size = 384 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for WeatherTask */
osThreadId_t WeatherTaskHandle;
const osThreadAttr_t WeatherTask_attributes = {
  .name = "WeatherTask",
  .stack_size = 256 * 4,
  .priority = (osPriority_t) osPriorityLow,
};
/* Definitions for KeyTask */
osThreadId_t KeyTaskHandle;
const osThreadAttr_t KeyTask_attributes = {
  .name = "KeyTask",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityLow,
};
/* Definitions for OledTask */
osThreadId_t OledTaskHandle;
const osThreadAttr_t OledTask_attributes = {
  .name = "OledTask",
  .stack_size = 160 * 4,
  .priority = (osPriority_t) osPriorityLow,
};
/* Definitions for TimeTask */
osThreadId_t TimeTaskHandle;
const osThreadAttr_t TimeTask_attributes = {
  .name = "TimeTask",
  .stack_size = 192 * 4,
  .priority = (osPriority_t) osPriorityLow,
};
/* Definitions for oledMutex */
osMutexId_t oledMutexHandle;
const osMutexAttr_t oledMutex_attributes = {
  .name = "oledMutex"
};
/* Definitions for weatherSem */
osSemaphoreId_t weatherSemHandle;
const osSemaphoreAttr_t weatherSem_attributes = {
  .name = "weatherSem"
};
/* Definitions for systemEventGroup */
osEventFlagsId_t systemEventGroupHandle;
const osEventFlagsAttr_t systemEventGroup_attributes = {
  .name = "systemEventGroup"
};

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */

/* USER CODE END FunctionPrototypes */

void StartWIFITask(void *argument);
void StartWeatherTask(void *argument);
void StartKeyTask(void *argument);
void StartOledTask(void *argument);
void StartTimeTask(void *argument);

void MX_FREERTOS_Init(void); /* (MISRA C 2004 rule 8.1) */

/**
  * @brief  FreeRTOS initialization
  * @param  None
  * @retval None
  */
void MX_FREERTOS_Init(void) {
  /* USER CODE BEGIN Init */

  /* USER CODE END Init */
  /* Create the mutex(es) */
  /* creation of oledMutex */
  oledMutexHandle = osMutexNew(&oledMutex_attributes);

  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* Create the semaphores(s) */
  /* creation of weatherSem */
  weatherSemHandle = osSemaphoreNew(1, 0, &weatherSem_attributes);

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* Create the queue(s) */
  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* creation of WIFITask */
  WIFITaskHandle = osThreadNew(StartWIFITask, NULL, &WIFITask_attributes);

  /* creation of WeatherTask */
  WeatherTaskHandle = osThreadNew(StartWeatherTask, NULL, &WeatherTask_attributes);

  /* creation of KeyTask */
  KeyTaskHandle = osThreadNew(StartKeyTask, NULL, &KeyTask_attributes);

  /* creation of OledTask */
  OledTaskHandle = osThreadNew(StartOledTask, NULL, &OledTask_attributes);

  /* creation of TimeTask */
  TimeTaskHandle = osThreadNew(StartTimeTask, NULL, &TimeTask_attributes);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  /* USER CODE END RTOS_THREADS */

  /* creation of systemEventGroup */
  systemEventGroupHandle = osEventFlagsNew(&systemEventGroup_attributes);

  /* USER CODE BEGIN RTOS_EVENTS */
  /* add events, ... */
  /* USER CODE END RTOS_EVENTS */

}

/* USER CODE BEGIN Header_StartWIFITask */
/**
  * @brief  Function implementing the WIFITask thread.
  * @param  argument: Not used
  * @retval None
  */
/* USER CODE END Header_StartWIFITask */
void StartWIFITask(void *argument)
{
  /* USER CODE BEGIN StartWIFITask */
  uint32_t wifi_retry = 0;

  osMutexAcquire(oledMutexHandle, osWaitForever);
  oled_clear();
  oled_show_string(0, 0, "WiFi connecting...");
  oled_refresh_gram();
  osMutexRelease(oledMutexHandle);

  while (esp8266_init() != 0)
  {
    osMutexAcquire(oledMutexHandle, osWaitForever);
    oled_show_string(0, 16, "ESP8266 init fail");
    oled_refresh_gram();
    osMutexRelease(oledMutexHandle);
    osDelay(3000);
    wifi_retry++;
    if (wifi_retry > 5)
    {
      osMutexAcquire(oledMutexHandle, osWaitForever);
      oled_show_string(0, 16, "ESP hw error!     ");
      oled_refresh_gram();
      osMutexRelease(oledMutexHandle);
      while (1) osDelay(1000);
    }
  }

  while (esp8266_connect_wifi(WIFI_SSID, WIFI_PASSWORD) != 0)
  {
    osMutexAcquire(oledMutexHandle, osWaitForever);
    oled_show_string(0, 16, "WiFi conn fail    ");
    oled_refresh_gram();
    osMutexRelease(oledMutexHandle);
    osDelay(5000);
    wifi_retry++;
    if (wifi_retry > 10)
    {
      osMutexAcquire(oledMutexHandle, osWaitForever);
      oled_show_string(0, 16, "WiFi error!       ");
      oled_refresh_gram();
      osMutexRelease(oledMutexHandle);
      while (1) osDelay(1000);
    }
  }

  osMutexAcquire(oledMutexHandle, osWaitForever);
  oled_show_string(0, 16, "WiFi connected    ");
  oled_show_string(0, 32, "Syncing time...   ");
  oled_refresh_gram();
  osMutexRelease(oledMutexHandle);

  osEventFlagsSet(systemEventGroupHandle, WIFI_CONNECTED_BIT);

  {
    uint32_t ts;
    while (ntp_get_time("pool.ntp.org", &ts) != 0)
    {
      osDelay(1000);
    }
    system_time = ts + 8 * 3600;
    time_synced = 1;
  }

  for(;;)
  {
    char path[160];
    snprintf(path, sizeof(path),
        "/v3/weather/now.json?key=%s&location=hangzhou&language=en&unit=c",
        API_KEY);

    memset(http_response, 0, sizeof(http_response));
    osMutexAcquire(oledMutexHandle, osWaitForever);
    oled_show_string(0, 32, "Fetching weather.");
    oled_refresh_gram();
    osMutexRelease(oledMutexHandle);
    int ret = esp8266_http_get("api.seniverse.com", 80, path,
                                http_response, sizeof(http_response));

    if (ret == 0 && strlen(http_response) > 0)
    {
      osSemaphoreRelease(weatherSemHandle);
    }
    else
    {
      char err[22];
      uint32_t rlen = strlen(http_response);
      snprintf(err, sizeof(err), "Err=%d len=%d", ret, (int)rlen);
      osMutexAcquire(oledMutexHandle, osWaitForever);
      oled_show_string(0, 32, err);
      oled_refresh_gram();
      osMutexRelease(oledMutexHandle);
    }

    osDelay(1800000);
  }
  /* USER CODE END StartWIFITask */
}

/* USER CODE BEGIN Header_StartWeatherTask */
/**
---* @brief Function implementing the WeatherTask thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartWeatherTask */
void StartWeatherTask(void *argument)
{
  /* USER CODE BEGIN StartWeatherTask */
  WeatherData_t parsed;

  for(;;)
  {
    if (osSemaphoreAcquire(weatherSemHandle, 5000) == osOK)
    {
      int parse_ret = weather_parse_now(http_response, &parsed);
      if (parse_ret == 0)
      {
        current_weather = parsed;
        weather_updated = 1;
      }
    }
    osDelay(100);
  }
  /* USER CODE END StartWeatherTask */
}

/* USER CODE BEGIN Header_StartKeyTask */
/**
* @brief Function implementing the KeyTask thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartKeyTask */
void StartKeyTask(void *argument)
{
  /* USER CODE BEGIN StartKeyTask */
  for(;;)
  {
    if (HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_8) == GPIO_PIN_RESET)
    {
      osDelay(50);
      if (HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_8) == GPIO_PIN_RESET)
      {
        current_page ^= 1;
        while (HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_8) == GPIO_PIN_RESET)
          osDelay(10);
      }
    }
    osDelay(50);
  }
  /* USER CODE END StartKeyTask */
}

/* USER CODE BEGIN Header_StartOledTask */
/**
* @brief Function implementing the OledTask thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartOledTask */
void StartOledTask(void *argument)
{
  /* USER CODE BEGIN StartOledTask */
  char temp_str[32];
  char time_buf[16];
  uint32_t last_tick = 0;
  uint8_t disp_page = 0;

  for(;;)
  {
    if (current_page != disp_page)
    {
      disp_page = current_page;
      last_tick = 0;
    }

    if (disp_page == 0)
    {
      /* Page 0: city / weather on 2 lines, temp big below */
      if (weather_updated || (last_tick == 0 && strlen(current_weather.city) > 0))
      {
        if (weather_updated) weather_updated = 0;
        osMutexAcquire(oledMutexHandle, osWaitForever);
        oled_clear_buffer();

        if (strlen(current_weather.city) > 0)
        {
          uint8_t cx, len;

          len = strlen(current_weather.city);
          cx = (128 - len * 8) / 2;
          oled_show_string(cx, 0, current_weather.city);

          len = strlen(current_weather.weather);
          cx = (128 - len * 8) / 2;
          oled_show_string(cx, 16, current_weather.weather);

          snprintf(temp_str, sizeof(temp_str), "%sC", current_weather.temp);
          len = strlen(temp_str);
          cx = (128 - len * 16) / 2;
          oled_show_big_string(cx, 32, temp_str);
        }
        else
        {
          oled_show_string(40, 0, "Loading...");
        }

        oled_refresh_gram();
        osMutexRelease(oledMutexHandle);
      }
    }
    else
    {
      /* Page 1: Time only (big) */
      if (last_tick == 0 || (time_synced && HAL_GetTick() - last_tick >= 1000))
      {
        last_tick = HAL_GetTick();
        if (time_synced)
          ntp_format_time(time_buf, system_time);
        else
          strcpy(time_buf, "--:--:--");
        osMutexAcquire(oledMutexHandle, osWaitForever);
        oled_clear_buffer();
        oled_show_big_string(0, 16, time_buf);
        oled_refresh_gram();
        osMutexRelease(oledMutexHandle);
      }
    }

    osDelay(200);
  }
  /* USER CODE END StartOledTask */
}

/* USER CODE BEGIN Header_StartTimeTask */
/**
* @brief Function implementing the TimeTask thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartTimeTask */
void StartTimeTask(void *argument)
{
  /* USER CODE BEGIN StartTimeTask */
  osEventFlagsWait(systemEventGroupHandle, WIFI_CONNECTED_BIT,
                   osFlagsWaitAny, osWaitForever);

  for(;;)
  {
    if (time_synced) system_time++;
    osDelay(1000);
  }
  /* USER CODE END StartTimeTask */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

/* USER CODE END Application */


