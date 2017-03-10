/*
 * ESPRSSIF MIT License
 *
 * Copyright (c) 2015 <ESPRESSIF SYSTEMS (SHANGHAI) PTE LTD>
 *
 * Permission is hereby granted for use on ESPRESSIF SYSTEMS ESP8266 only, in which case,
 * it is free of charge, to any person obtaining a copy of this software and associated
 * documentation files (the "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the Software is furnished
 * to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all copies or
 * substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#include "esp_common.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include "lwip/opt.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include "openssl/ssl.h"
#include "iothub_client_sample_mqtt.h"
#include "lwip/apps/sntp.h"
#include "lwip/apps/sntp_time.h"
#include "uart.h"

#include "azure_c_shared_utility/platform.h"
#include "espressif/esp_libc.h"
#include "wifi_state_machine.h"

#define OPENSSL_DEMO_THREAD_NAME "ssl_demo"
#define OPENSSL_DEMO_THREAD_STACK_WORDS 1024*2
#define OPENSSL_DEMO_THREAD_PRORIOTY 6

const char * ssid = "your-ssid";
const char * password = "your-pass";

static os_timer_t timer;

static xTaskHandle openssl_handle = NULL;

LOCAL void ICACHE_FLASH_ATTR mqtt_sample()
{
    iothub_client_sample_mqtt_run();
    vTaskDelete(NULL);
    openssl_handle = NULL;
}

LOCAL void ICACHE_FLASH_ATTR start_mqtt_sample(){
    printf("azure iot program starts %d\n", system_get_free_heap_size());

    int result;
    result = xTaskCreate(mqtt_sample,
                  OPENSSL_DEMO_THREAD_NAME,
                  OPENSSL_DEMO_THREAD_STACK_WORDS,
                  NULL,
                  OPENSSL_DEMO_THREAD_PRORIOTY,
                  &openssl_handle);

    if (result != pdPASS){
        os_printf("create thread %s failed\n", OPENSSL_DEMO_THREAD_NAME);
    }
}

LOCAL void ICACHE_FLASH_ATTR wait_for_connection_ready(uint8 flag)
{
    os_timer_disarm(&timer);
    if(wifi_station_connected()){
        start_mqtt_sample();
    } else {
        os_timer_setfn(&timer, (os_timer_func_t *)wait_for_connection_ready, NULL);
        os_timer_arm(&timer, 2000, 0);            
    }
}

LOCAL void ICACHE_FLASH_ATTR on_wifi_connect(){
    platform_init();
    os_timer_disarm(&timer);
    os_timer_setfn(&timer, (os_timer_func_t *)wait_for_connection_ready, NULL);
    os_timer_arm(&timer, 100, 0);
}

LOCAL void ICACHE_FLASH_ATTR on_wifi_disconnect(uint8_t reason){
    //reason can be used to detect wrong station config
    platform_deinit();
}

/******************************************************************************
 * FunctionName : user_init
 * Description  : entry of user application, init user function here
 * Parameters   : none
 * Returns      : none
*******************************************************************************/
void user_init(void)
{
    printf("SDK version:%s\n", system_get_sdk_version());
    uart_div_modify(0, UART_CLK_FREQ / 115200);

    set_on_station_connect(on_wifi_connect);
    set_on_station_disconnect(on_wifi_disconnect);
    init_esp_wifi();
    stop_wifi_ap();
    start_wifi_station(ssid, password);
}

