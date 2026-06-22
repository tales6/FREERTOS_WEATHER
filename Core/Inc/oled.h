#ifndef __OLED_H__
#define __OLED_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#define OLED_I2C_ADDR     0x78

#define OLED_CMD          0
#define OLED_DATA          0x40

#define OLED_WIDTH        128
#define OLED_HEIGHT       64

void oled_init(void);
void oled_clear(void);
void oled_clear_buffer(void);
void oled_refresh_gram(void);

void oled_show_string(uint8_t x, uint8_t y, const char* str);
void oled_show_char(uint8_t x, uint8_t y, char c);
void oled_show_big_char(uint8_t x, uint8_t y, char c);
void oled_show_big_string(uint8_t x, uint8_t y, const char* str);

#ifdef __cplusplus
}
#endif

#endif
