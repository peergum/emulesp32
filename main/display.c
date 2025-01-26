
#include "apple_font.c"
#include "stdint.h"
#include "display.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "Display";
static uint16_t *buffer;
static esp_lcd_panel_handle_t panel_handle = NULL;

void initLCD() {
  ESP_LOGI(TAG, "Turn off LCD backlight");
  gpio_config_t bk_gpio_config = {.pin_bit_mask = 1ULL << PIN_NUM_BK_LIGHT,
                                  .mode = GPIO_MODE_OUTPUT,
                                  .pull_up_en = GPIO_PULLUP_DISABLE,
                                  .pull_down_en = GPIO_PULLDOWN_DISABLE,
                                  .intr_type = GPIO_INTR_DISABLE};
  ESP_ERROR_CHECK(gpio_config(&bk_gpio_config));

  ESP_LOGI(TAG, "Initialize SPI bus");
  spi_bus_config_t buscfg = {
      .mosi_io_num = PIN_NUM_MOSI,
      .miso_io_num = PIN_NUM_MISO,
      .sclk_io_num = PIN_NUM_SCLK,
      .quadwp_io_num = -1,
      .quadhd_io_num = -1,
      .max_transfer_sz = LCD_H_RES * 80 * sizeof(uint16_t),
  };
  ESP_ERROR_CHECK(spi_bus_initialize(LCD_HOST, &buscfg, SPI_DMA_CH_AUTO));

  ESP_LOGI(TAG, "Install panel IO");
  esp_lcd_panel_io_handle_t io_handle = NULL;
  esp_lcd_panel_io_spi_config_t io_config = {
      .dc_gpio_num = PIN_NUM_DC,
      .cs_gpio_num = PIN_NUM_CS,
      .pclk_hz = LCD_PIXEL_CLOCK_HZ,
      .lcd_cmd_bits = LCD_CMD_BITS,
      .lcd_param_bits = LCD_PARAM_BITS,
      .spi_mode = 0,
      .trans_queue_depth = 10,
  };
  // Attach the LCD to the SPI bus
  ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)LCD_HOST,
                                           &io_config, &io_handle));

  esp_lcd_panel_dev_config_t panel_config = {
      .reset_gpio_num = PIN_NUM_RST,
      .rgb_ele_order = LCD_RGB_ELEMENT_ORDER_BGR,
      .bits_per_pixel = 16,
  };
  ESP_LOGI(TAG, "Install ILI9341 panel driver");
  ESP_ERROR_CHECK(
      esp_lcd_new_panel_ili9341(io_handle, &panel_config, &panel_handle));

  ESP_ERROR_CHECK(esp_lcd_panel_reset(panel_handle));
  ESP_ERROR_CHECK(esp_lcd_panel_init(panel_handle));
  // ESP_ERROR_CHECK(esp_lcd_panel_mirror(panel_handle, true, false));
  ESP_ERROR_CHECK(esp_lcd_panel_swap_xy(panel_handle, true));

  // user can flush pre-defined pattern to the screen before we turn on the
  // screen or backlight
  ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(panel_handle, true));

  ESP_LOGI(TAG, "Turn on LCD backlight");
  gpio_set_level(PIN_NUM_BK_LIGHT, LCD_BK_LIGHT_ON_LEVEL);

  buffer = malloc(240UL * 320UL * sizeof(uint16_t));

  for (int x = 0; x < 320; x++) {
    for (int y = 0; y < 240; y++) {
      buffer[x + y * 320] = 0;
    }
  }
}

void drawText(uint8_t *addr, int cols, int rows, bool on) {
  uint8_t byte;
  uint8_t row;
  bool b;
  for (int r = 0; r < rows; r++) {
    for (int c = 0; c < cols; c++) {
      int X = 20 + c * 7;
      int Y = 20 + r * 8;
      uint8_t *ram = addr + (r / 8) * 40 + (r % 8) * 128;
      byte = *(ram + c);
      bool inv = false;
      if (byte < 0x40 || (byte >= 0x40 && byte < 0x80 && on)) {
        inv = true;
      }
      for (int y = 0; y < 8; y++) {
        row = appleFont[(byte & 0x3f) * 8 + y];
        for (int x = 0; x < 7; x++) {
          row <<= 1;
          b = (row & 0x40) != 0;
          if (inv) b = !b;
          buffer[(Y + y) * 320 + X + x] = b ? 0xffff : 0x0000;
        }
      }
    }
  }
  esp_lcd_panel_draw_bitmap(panel_handle, 0, 0, 320, 240, buffer);
}
