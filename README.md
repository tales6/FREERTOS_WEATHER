# 基于 FreeRTOS 的 STM32 物联网天气站

基于 **STM32F103C8T6** + **FreeRTOS** 的物联网天气站，通过 ESP8266 模块联网获取心知天气 API 数据，并在 OLED 屏幕（128×64，I2C）上显示天气和时间信息。

## 功能特性

- **WiFi 连接**：ESP8266 AT 指令驱动，自动重连
- **天气显示**：城市名、天气状况、温度（大字）
- **时间同步**：NTP 协议自动校时
- **页面切换**：PB8 按键切换「天气页 ⇄ 时间页」
- **多任务调度**：5 个 FreeRTOS 任务协同工作

## 硬件接线

| 外设 | 接口 | STM32 引脚 |
|------|------|------------|
| OLED | I2C1 | PB6(SCL), PB7(SDA) |
| ESP8266 | USART1 | PA9(TX), PA10(RX) |
| 按键 | GPIO | PB8(外部下拉) |

## 软件架构

```
main()
├── 外设初始化 (GPIO / I2C1 / USART1)
├── OLED 初始化
├── osKernelInitialize()
├── MX_FREERTOS_Init()
│   ├── WIFITask    - ESP8266 驱动 + NTP 校时 + HTTP 请求
│   ├── WeatherTask - 解析 JSON 天气数据
│   ├── KeyTask     - PB8 按键扫描 & 防抖
│   ├── OledTask    - 双页面显示驱动
│   └── TimeTask    - 系统时钟计数
├── 同步机制：oledMutex / weatherSem / systemEventGroup
└── osKernelStart()
```

## 依赖

- MCU: STM32F103C8T6 (Blue Pill)
- IDE: Keil MDK-ARM
- HAL 库: STM32Cube_FW_F1
- RTOS: FreeRTOS (CMSIS-RTOS2)
- 模块: ESP8266-01S, 0.96" OLED SSD1306

## 快速开始

### 1. 配置本地 config.h

本项目使用 `config.h` 管理敏感配置（WiFi 密码、API Key），**该文件不会被提交到 GitHub**。

```bash
# 复制模板并填入你的实际信息
cp Core/Inc/config.template.h Core/Inc/config.h
```

编辑 `Core/Inc/config.h`，填入你的信息：

```c
#define WIFI_SSID       "你的WiFi名称"
#define WIFI_PASSWORD   "你的WiFi密码"
#define API_KEY         "你的心知天气API Key"   // 注册: https://www.seniverse.com/
```

### 2. 编译下载

使用 Keil MDK 打开 `MDK-ARM/FREERTOS_WEATHER.uvprojx`，编译下载到 STM32。

## 文档

详见 `docs/` 目录或通过 GitHub Pages 在线浏览：

https://tales6.github.io/FREERTOS_WEATHER/

| 目录 | 内容 |
|------|------|
| `设计报告/` | 项目设计报告（DOCX） |
| `代码详解/` | 各模块代码详解（HTML，支持网页预览） |
| `接线图/` | 硬件接线示意图 |
| `运行截图/` | 运行效果截图 |
| `演示视频/` | 功能演示视频 |

## Git 提交历史分类

| 类型 | 说明 |
|------|------|
| `feat` | 新增功能 |
| `fix` | 修复问题 |
| `docs` | 文档 & 资源文件 |
| `chore` | 杂物 & 视频资源 |

## License

MIT
