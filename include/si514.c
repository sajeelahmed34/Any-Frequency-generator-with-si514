#include <stdio.h>
#include <time.h>
#include <sys/time.h>
#include <math.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "sdkconfig.h"
#include "driver/i2c.h"

#include "si514.h"



#define I2C_TAG "I2C_error:"

//GPIO22 is assigned as SCL for I2C
#define I2C_SCL_IO	GPIO_NUM_22  
//GPIO21 is assigned as SDA for I2C          
#define I2C_SDA_IO	GPIO_NUM_21


/*!< I2C master do not need transmit buffer */
#define I2C_TX_BUF_DISABLE 0  
/*!< I2C master do not need receive buffer */
#define I2C_RX_BUF_DISABLE 0   

 /*!< I2C port number for master dev */
#define I2C_PORT_NUM I2C_NUM_1      

/*!< I2C master clock frequency */
#define I2C_FREQ_HZ	100000   

//define default i2c address

#define I2C_ADDR 0x55

/*!< I2C master write */
#define WRITE_BIT I2C_MASTER_WRITE 

/*!< I2C master read */
#define READ_BIT I2C_MASTER_READ  

/*!< I2C master will check ack from slave*/
#define ACK_CHECK_EN 0x1          

/*!< I2C master will not check ack from slave */
#define ACK_CHECK_DIS 0x0    

/*!< I2C ack value */
#define ACK_VAL 0x0    

/*!< I2C non-ack value */
#define NACK_VAL 0x1            


uint8_t enable_output = 0x04;
uint8_t disable_output = 0x00;
uint8_t calb_output = 0x05;

   //function: i2c data transmission
static void i2c_write(uint8_t data, reg_addr_t reg_addr){

    i2c_cmd_handle_t cmd = i2c_cmd_link_create();

    i2c_master_start(cmd);

    // first, send device address (indicating write) & register to be written
    i2c_master_write_byte(cmd, (I2C_ADDR << 1) | WRITE_BIT, ACK_CHECK_EN);

    // send register we want
    i2c_master_write_byte(cmd, reg_addr, ACK_CHECK_EN);

    // write the data
    i2c_master_write_byte(cmd, data, ACK_CHECK_EN);

    i2c_master_stop(cmd);
    esp_err_t ret = i2c_master_cmd_begin(I2C_PORT_NUM, cmd, 1000 / portTICK_RATE_MS);
    if(ret != ESP_OK){
        ESP_LOGI(I2C_TAG, "failed to transmit data over i2c lines");
    }

    i2c_cmd_link_delete(cmd);
}

//si514 output enable disable function 
static void si514_output(uint8_t data, reg_addr_t reg_addr){
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();

    i2c_master_start(cmd);

    // first, send device address (indicating write) & register to be written
    i2c_master_write_byte(cmd, (I2C_ADDR << 1) | WRITE_BIT, ACK_CHECK_EN);

    // send register address we want to modify/write
    i2c_master_write_byte(cmd, reg_addr, ACK_CHECK_EN);

    // write the data to the register
    i2c_master_write_byte(cmd, data, ACK_CHECK_EN);

    i2c_master_stop(cmd);

    esp_err_t ret = i2c_master_cmd_begin(I2C_PORT_NUM, cmd, 1000 / portTICK_RATE_MS);

    i2c_cmd_link_delete(cmd);
}

    // Function: i2c master initialization, configuration setup
static void i2c_master_init()
{
    int i2c_master_port = I2C_PORT_NUM;
    i2c_config_t conf;
    conf.mode = I2C_MODE_MASTER;
    conf.sda_io_num = I2C_SDA_IO;
    conf.sda_pullup_en = GPIO_PULLUP_DISABLE;
    conf.scl_io_num = I2C_SCL_IO;
    conf.scl_pullup_en = GPIO_PULLUP_DISABLE;
    conf.master.clk_speed = I2C_FREQ_HZ;
    i2c_param_config(i2c_master_port, &conf);
    i2c_driver_install(i2c_master_port, conf.mode, I2C_RX_BUF_DISABLE, I2C_TX_BUF_DISABLE, 0);
}
    
void si514_large_freq_xchange(config_t *config_si514, reg_t *reg){

    uint8_t LP1 = 0;
    uint8_t LP2 = 0;

    //create an instance for the return type
    
    float LS_DIV = (config_si514->FVCO_min)/((config_si514->FOUT)*(config_si514->HSDIV_max));
    int LS_DIV_floor = floor(LS_DIV);
    int n = log2(LS_DIV_floor);
    if ((LS_DIV > pow(2, n)) && (LS_DIV <= pow(2, n + 1)))
    {
        LS_DIV = pow(2, n + 1);
    }
    else 
    {
        LS_DIV = 1;
    }

    //Determine the minimum value for HS_DIV
    //this optimized timing margins

    float HS_DIV_min = (config_si514->FVCO_min)/((config_si514->FOUT)*(LS_DIV));
    int HS_DIV_min_ceil = ceil(HS_DIV_min);
    if ((HS_DIV_min_ceil % 2) == 0)
    {
        HS_DIV_min = HS_DIV_min_ceil;
    }
    else 
    {
        HS_DIV_min = HS_DIV_min_ceil + 1;
    }

    //Determine a value for Multiplier
    float M = (LS_DIV*HS_DIV_min*(config_si514->FOUT))/(config_si514->FXO);
    int M_int = floor(M);
    double M_frac = (M - M_int)*(536870912.0);

    //Determine values for LP1 and LP2
    int M_new = round(M);

    if (M_new >= 76 && M_new < 78)
    {
        LP1 = 4; 
        LP2 = 4;
    }
    else if (M_new >= 73 && M_new < 76)
    {
        LP1 = 3; 
        LP2 = 4;
    }
    else if (M_new >= 68 && M_new < 73)
    {
        LP1 = 3;
        LP2 = 3;
    }
    else if (M_new >=65 && M_new < 68)
    {
        LP1 = 2;
        LP2 = 3;
    }
    else if (M_new >=65 && M_new < 66)
    {
        LP1 = 2;
        LP2 = 2;
    }
    else 
    {
        LP1 = 0;
        LP2 = 0;
    }

//register 0 value 
    reg->reg_0 = (LP1 << 4) | (LP2);
    
//register 5 values 
    uint32_t M_frac_round = (uint32_t)(round(M_frac));
    reg->reg_5 = (uint8_t)(M_frac_round & 0x000000FF); //reg_5 = M_frac[7:0]

//register 6 values
    reg->reg_6 = ((M_frac_round & 0x0000FF00) >> 8); //reg_6 = M_frac[15:8]

//register 7 values 
    reg->reg_7 = ((M_frac_round & 0x00FF0000) >> 16);

//register 8 values 
    uint8_t temp_var1 = ((M_frac_round & 0xFF000000) >> 24);
    reg->reg_8 = (temp_var1 | (M_int << 5));

//register 9 values 
    reg->reg_9 = ((M_int & 0x01F8) >> 3);  

//register 10 values 
    uint16_t temp_var2 = (uint16_t)(HS_DIV_min);
    reg->reg_10 = (temp_var2 & 0x00FF);

//register 11 values 
    uint16_t temp_var3 = (uint16_t)(LS_DIV);
    reg->reg_11 = ((temp_var2 & 0x0300) >> 8) | ((temp_var3 & 0x07) << 4);

/* ----------------------------------------------------------------------------------- */

    /* Now we are writing the internal registers of SI514*/

/* ----------------------------------------------------------------------------------- */

//i2c initialization
    i2c_master_init();

//Disable output of si514 by setting register 132 bit 2 to zero
    si514_output(disable_output, reg_132_addr);

//writing to registers 0 
    i2c_write(reg->reg_0, reg_0_addr);

//writing to registers 5
    i2c_write(reg->reg_5, reg_5_addr);

//writing to registers 6 
    i2c_write(reg->reg_6, reg_6_addr);

//writing to registers 7 
    i2c_write(reg->reg_7, reg_7_addr);

//writing to registers 8 
    i2c_write(reg->reg_8, reg_8_addr);

//writing to registers 9 
    i2c_write(reg->reg_9, reg_9_addr);

//writing to registers 10 
    i2c_write(reg->reg_10, reg_10_addr);

//writing to registers 11 
    i2c_write(reg->reg_11, reg_11_addr);

//Calibrate the VCO by writing to register 132 bit 0 to one 
    si514_output(calb_output, reg_132_addr);

//Enable output of si514 by setting register 132 bit 2 to one    
    si514_output(enable_output, reg_132_addr);
}     
