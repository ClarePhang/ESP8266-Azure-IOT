// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdlib.h>
#ifdef _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif
#include "azure_c_shared_utility/platform.h"
#include "azure_c_shared_utility/xio.h"
#include "azure_c_shared_utility/tlsio_openssl.h"
#include "azure_c_shared_utility/xlogging.h"
#include "lwip/apps/sntp.h"
#include "lwip/apps/sntp_time.h"

#define SNTP_TIME_TO_WAIT   2000    //time to wait after each SNTP request
#define SNTP_SERVERS_NUM    2       //number of SNTP servers in the list

const char * ntp_servers[] = {      //list of SNTP servers to go through
        "pool.ntp.org",
        "time.nist.gov"
};

int ICACHE_FLASH_ATTR platform_init(void)
{
    u8_t current_server = 0;
    u32_t ts = sntp_get_current_timestamp();
    while(ts == 0){
        os_printf("SNTP: trying server[%u] %s \n", current_server, ntp_servers[current_server]);
        sntp_setservername(0, (char *)(ntp_servers[current_server]));
        sntp_init();
        vTaskDelay(SNTP_TIME_TO_WAIT / portTICK_RATE_MS);
        ts = sntp_get_current_timestamp();
        if(!ts){
            os_printf("SNTP: could not get the time from %s.\n", ntp_servers[current_server]);
            current_server = (current_server + 1) % SNTP_SERVERS_NUM;
        }
    }
    os_printf("SNTP: got the time: %s\n", sntp_get_real_time(ts));
    return 0;
}

const IO_INTERFACE_DESCRIPTION* platform_get_default_tlsio(void)
{
    return tlsio_openssl_get_interface_description();
}

void ICACHE_FLASH_ATTR platform_deinit(void)
{
     sntp_stop();
}
