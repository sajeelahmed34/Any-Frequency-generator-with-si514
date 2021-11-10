#include <stdio.h>
#include "driver/i2c.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "sdkconfig.h"

#include "si514.h"


static config_t config = {
    .FVCO_min = 2080, 
    .HSDIV_max = 1022, 
    .FOUT = 51.84,
    .FXO = 31.98,
};


static reg_t value = {
    .reg_0 = 0,
    .reg_5 = 0,
    .reg_6 = 0,
    .reg_7 = 0,
    .reg_8 = 0,
    .reg_9 = 0,
    .reg_10 = 0,
    .reg_11 = 0,
};


void app_main(void){

    si514_large_freq_xchange(&config, &value);
    
}