/* Copyright 2015 Samsung Electronics Co., Ltd.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/******************************************************************************
 * Copyright 2013-2014 Espressif Systems (Wuxi)
 *
 * FileName: user_main.c
 *
 * Description: entry file of user application
 *
 * Modification history:
 *     2014/12/1, v1.0 create this file.
*******************************************************************************/

#include "esp_common.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "user_config.h"
#include "esp8266_gpio.h"
#include "esp8266_uart.h"


//-----------------------------------------------------------------------------

void show_free_mem(int idx) {
  size_t res = xPortGetFreeHeapSize();
  printf("dbg free memory(%d): %d\r\n", idx, res);
}


//-----------------------------------------------------------------------------

void gpio_dir_native(int port, int value) {
  if (value) {
    GPIO_AS_OUTPUT(1 << port);
  }
  else {
    GPIO_AS_INPUT(1 << port);
  }
}


void gpio_set_native(int port, int value) {
  GPIO_OUTPUT_SET(port, value);
}


int gpio_get_native(int port) {
  return GPIO_INPUT_GET(port);
}


//-----------------------------------------------------------------------------

#include "esp8266_js.h"

static int jerry_task_init(void) {
  int retcode;
  int src;

  DECLARE_JS_CODES;

  /* run main.js */
  show_free_mem(2);
  retcode = js_entry(js_codes[0].source, js_codes[0].length);
  if (retcode != 0) {
    printf("js_entry failed code(%d) [%s]\r\n", retcode, js_codes[0].name);
    js_exit();
    return -1;
  }
  /* run rest of the js files */
  show_free_mem(3);
  for (src=1; js_codes[src].source; src++) {
    retcode = js_eval(js_codes[src].source, js_codes[src].length);
    if (retcode != 0) {
      printf("js_eval failed code(%d) [%s]\r\n", retcode, js_codes[src].name);
      js_exit();
      return -2;
    }
  }
  show_free_mem(4);
  return 0;
}


void jerry_task(void *pvParameters) {
  const portTickType xDelay = 50 / portTICK_RATE_MS;
  uint32_t ticknow = 0;

  if (jerry_task_init() == 0) {
    for (;;) {
      vTaskDelay(xDelay);
      js_loop(ticknow);
      if (!ticknow) {
        show_free_mem(5);
      }
      ticknow++;
    }
    js_exit();
  }
}


//-----------------------------------------------------------------------------

/*
 * This is entry point for user code
 */
void ICACHE_FLASH_ATTR user_init(void)
{
  const portTickType onesec = 1000 / portTICK_RATE_MS;

  uart_div_modify(UART0, UART_CLK_FREQ / (BIT_RATE_115200));

  printf("\r\nStop softfp dhcp\r\n");
  show_free_mem(0);
  wifi_softap_dhcps_stop();
  show_free_mem(1);

  PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO0_U, FUNC_GPIO0);    // GPIO 0
  PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO2_U, FUNC_GPIO2);    // GPIO 2

  xTaskCreate(jerry_task, "jerry", jerry_STACK_SIZE, NULL, 2, NULL);
}