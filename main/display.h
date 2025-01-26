#include "esp_lcd_ili9341.h"
#include "esp_log.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_lcd_panel_ops.h"
#include "esp_heap_caps.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"

#define LCD_HOST SPI2_HOST

#define PIN_NUM_MISO GPIO_NUM_25
#define PIN_NUM_MOSI GPIO_NUM_23
#define PIN_NUM_SCLK GPIO_NUM_19
#define PIN_NUM_CS GPIO_NUM_22

#define PIN_NUM_DC GPIO_NUM_21
#define PIN_NUM_RST GPIO_NUM_18
#define PIN_NUM_BK_LIGHT GPIO_NUM_5

#define LCD_PIXEL_CLOCK_HZ (20 * 1000 * 1000)
#define LCD_BK_LIGHT_ON_LEVEL 0
#define LCD_BK_LIGHT_OFF_LEVEL !LCD_BK_LIGHT_ON_LEVEL

#define LCD_H_RES 240
#define LCD_V_RES 320

// Bit number used to represent command and parameter
#define LCD_CMD_BITS 8
#define LCD_PARAM_BITS 8

// To speed up transfers, every SPI transfer sends a bunch of lines. This define
// specifies how many. More means more memory use, but less overhead for setting
// up / finishing transfers. Make sure 240 is dividable by this.
#define PARALLEL_LINES 16

#ifdef __cplusplus
extern "C" {
#endif

void initLCD();
void drawText(uint8_t *buffer, int cols, int rows, bool on);

#ifdef __cplusplus
}
#endif