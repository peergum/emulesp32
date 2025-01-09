#include <stdio.h>
#include "esp_log.h"
#include "CPU6502.h"

#include "Apple2plus.h"
#include "main.h"

static const char *TAG = "Main";

int app_main(void)
{
    ESP_LOGD(TAG,"Starting");
    initEmulation();
    return 0;
}

