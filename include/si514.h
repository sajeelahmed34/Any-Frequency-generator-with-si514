#pragma once

#include <inttypes.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// Register addresses 
typedef enum {
    reg_0_addr = 0x00,
    reg_5_addr = 0x05,
    reg_6_addr = 0x06,
    reg_7_addr = 0x07,
    reg_8_addr = 0x08,
    reg_9_addr = 0x09,
    reg_10_addr = 0x0A,
    reg_11_addr = 0x0B,
    reg_132_addr = 0x84
} reg_addr_t;

/*
SI514 desired frequency setting
*/
typedef struct {
    int FVCO_min;
    int HSDIV_max;
    float FOUT; 
    float FXO; //fxed frequency 31.98MHz
} config_t;


typedef struct {
    uint8_t reg_0;
    uint8_t reg_5;
    uint8_t reg_6;
    uint8_t reg_7;
    uint8_t reg_8;
    uint8_t reg_9;
    uint8_t reg_10;
    uint8_t reg_11;
} reg_t;


// large frequency change function
void si514_large_freq_xchange(config_t *config_si514, reg_t *reg);

#ifdef __cplusplus
}
#endif  


