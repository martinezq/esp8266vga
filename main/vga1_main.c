#include <stdio.h>

// #include "freertos/FreeRTOS.h"
// #include "freertos/task.h"
// #include "freertos/queue.h"

#include "esp_log.h"
#include "esp_attr.h"
#include "esp_task_wdt.h"
#include "esp_system.h"
#include "esp_clk.h"

#include "driver/gpio.h"
#include "driver/hw_timer.h"
#include "driver/spi.h"

#include "esp8266/gpio_struct.h"

#define FPS         60
#define ALL_LINES   525
#define FREQ_LINE   1000000 / FPS / ALL_LINES
#define TRUE        1
#define FALSE       0

static const char *TAG = "vga1";

#define GPIO_OUTPUT_HSYNC     4
#define GPIO_OUTPUT_VSYNC     5
#define GPIO_OUTPUT_RED       12
#define GPIO_OUTPUT_GREEN     13
#define GPIO_OUTPUT_BLUE      14
#define GPIO_OUTPUT_X         15
#define GPIO_OUTPUT_PIN_SEL  ((1ULL << GPIO_OUTPUT_HSYNC) | (1ULL << GPIO_OUTPUT_VSYNC) | (1ULL << GPIO_OUTPUT_RED) | (1ULL << GPIO_OUTPUT_GREEN) | (1ULL << GPIO_OUTPUT_BLUE) | (1ULL << GPIO_OUTPUT_X))
#define GPIO_OUTPUT_PIN_RGBX ((1ULL << GPIO_OUTPUT_RED) | (1ULL << GPIO_OUTPUT_GREEN) | (1ULL << GPIO_OUTPUT_BLUE) | (1ULL << GPIO_OUTPUT_X))

// ----------------------------------------------------------------------------

#define NOP_DELAY_(N) asm(".rept " #N "\n\t nop \n\t .endr \n\t":::)
#define NOP_DELAY(N) NOP_DELAY_(N)

// ----------------------------------------------------------------------------

#define VRAM_LENGTH 256 * 256 / 4

static uint32_t line = 0;
static uint32_t frame = 0;
static bool visible = FALSE;
static bool isVsync = FALSE;

volatile static uint32_t vram[VRAM_LENGTH];

// ----------------------------------------------------------------------------

static inline void onVgaLine() {

  // VSYNC PULSE
  if (isVsync) {
    GPIO.out_w1tc = (0x1 << GPIO_OUTPUT_VSYNC);
  } else {
    GPIO.out_w1ts = (0x1 << GPIO_OUTPUT_VSYNC);
  }

  // HSYNC pulse
  GPIO.out_w1tc = (0x1 << GPIO_OUTPUT_HSYNC);
  NOP_DELAY(600);
  GPIO.out_w1ts = (0x1 << GPIO_OUTPUT_HSYNC);

  if (visible) {
    register volatile uint32_t *out = (volatile uint32_t *)0x60000300; //GPO
    register uint32_t gpo0 = GPIO.in & 0xffff0fff;
    register uint32_t *in = (uint32_t*)vram;
    register uint32_t c;

    NOP_DELAY(360);

    *out = 0x000f000U | gpo0;
    // NOP_DELAY(6);

/*
    #define PUT8PIXELS \
      c = *in++; \
      *out = ((c << 12U) & 0x0000f000U) | gpo0; \
      *out = ((c <<  8U) & 0x0000f000U) | gpo0; \
      *out = ((c <<  4U) & 0x0000f000U) | gpo0; \
      *out = ((c <<  0U) & 0x0000f000U) | gpo0; \
      *out = ((c >>  4U) & 0x0000f000U) | gpo0; \
      *out = ((c >>  8U) & 0x0000f000U) | gpo0; \
      *out = ((c >> 12U) & 0x0000f000U) | gpo0; \
      *out = ((c >> 16U) & 0x0000f000U) | gpo0;
    
    #define PUT64PIXELS \
      PUT8PIXELS; PUT8PIXELS; PUT8PIXELS; PUT8PIXELS; \
      PUT8PIXELS; PUT8PIXELS; PUT8PIXELS; PUT8PIXELS;

    PUT64PIXELS;
    PUT64PIXELS;
    PUT64PIXELS;
    PUT64PIXELS;
    PUT64PIXELS;
*/
    *out = gpo0;

  }

  line++;

  switch (line)
  {
  case 40:
    visible = TRUE;
    break;
  case 480:
    visible = FALSE;
    break;
  case 490:
    isVsync = TRUE;
    break;
  case 492:
    isVsync = FALSE;
    break;
  case 525:
    line = 0;
    visible = TRUE;
    frame = (frame + 1) % FPS;
    if (frame == 0) {
      esp_task_wdt_reset();
    }
    break;

  default:
    break;
  }

}

// ----------------------------------------------------------------------------

void setupPins() {
  ESP_LOGI(TAG, "Setting up GPIO");

  gpio_config_t io_conf;

  io_conf.intr_type = GPIO_INTR_DISABLE;
  io_conf.mode = GPIO_MODE_OUTPUT;
  io_conf.pin_bit_mask = GPIO_OUTPUT_PIN_SEL;
  io_conf.pull_down_en = 0;
  io_conf.pull_up_en = 0;
  
  gpio_config(&io_conf);
}

void setupTimer() {
  ESP_LOGI(TAG, "Setting up hardware timer");

  hw_timer_init(onVgaLine, NULL);
  hw_timer_set_clkdiv(TIMER_CLKDIV_256);
  hw_timer_set_intr_type(TIMER_EDGE_INT);
  hw_timer_set_reload(TRUE);
  hw_timer_set_load_data(10);
  hw_timer_enable(TRUE);
}

void setup() {
  setupPins();
  setupTimer();

  /* Print chip information */
  esp_chip_info_t chip_info;
  esp_chip_info(&chip_info);
  printf("This is ESP8266 chip with %d CPU cores, WiFi \n", chip_info.cores);
  printf("CPU freq %d Hz \n", esp_clk_cpu_freq());
  fflush(stdout);

  // for(int i=0; i<3; i++) {
  //   gpio_set_level(2, 0);
  //   vTaskDelay(500 / portTICK_RATE_MS);
  //   gpio_set_level(2, 1);
  //   vTaskDelay(500 / portTICK_RATE_MS);
  // }

  ESP_LOGI(TAG, "Setup done");
}


void initRam() {
  for(int i=0; i<VRAM_LENGTH; i++) {
    vram[i] = 0x0f0f0f0f;
  }
}

void app_main(void)
{
  ESP_LOGI(TAG, "Starting");
  setup();

  initRam();

  while(1) {
   
  }
}