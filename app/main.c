/***************************************************************************** 
* 
* File Name : main.c
* 
* Description: main 
* 
* Copyright (c) 2014 Winner Micro Electronic Design Co., Ltd. 
* All rights reserved. 
* 
* Author : dave
* 
* Date : 2014-6-14
*****************************************************************************/ 
#include "wm_include.h"
#include <stdio.h>
#include "wm_i2c.h"
#include "wm_gpio_afsel.h"

#define I2C_FREQ		(200000)
#define ADDR 0xEC
//0x76
#define NUM_CALIB_PARAMS 24

// hardware registers
#define REG_CONFIG 0xF5
#define REG_CTRL_MEAS 0xF4
#define REG_RESET 0xE0

#define REG_TEMP_XLSB 0xFC
#define REG_TEMP_LSB 0xFB
#define REG_TEMP_MSB 0xFA

#define REG_PRESSURE_XLSB 0xF9
#define REG_PRESSURE_LSB 0xF8
#define REG_PRESSURE_MSB 0xF7

// calibration registers
#define REG_DIG_T1_LSB 0x88
#define REG_DIG_T1_MSB 0x89
#define REG_DIG_T2_LSB 0x8A
#define REG_DIG_T2_MSB 0x8B
#define REG_DIG_T3_LSB (0x8C)
#define REG_DIG_T3_MSB (0x8D)
#define REG_DIG_P1_LSB (0x8E)
#define REG_DIG_P1_MSB (0x8F)
#define REG_DIG_P2_LSB (0x90)
#define REG_DIG_P2_MSB (0x91)
#define REG_DIG_P3_LSB (0x92)
#define REG_DIG_P3_MSB (0x93)
#define REG_DIG_P4_LSB (0x94)
#define REG_DIG_P4_MSB (0x95)
#define REG_DIG_P5_LSB (0x96)
#define REG_DIG_P5_MSB (0x97)
#define REG_DIG_P6_LSB (0x98)
#define REG_DIG_P6_MSB (0x99)
#define REG_DIG_P7_LSB (0x9A)
#define REG_DIG_P7_MSB (0x9B)
#define REG_DIG_P8_LSB (0x9C)
#define REG_DIG_P8_MSB (0x9D)
#define REG_DIG_P9_LSB (0x9E)
#define REG_DIG_P9_MSB (0x9F)

struct bmp280_calib_param {
    // temperature params
    uint16_t dig_t1;
    int16_t dig_t2;
    int16_t dig_t3;

    // pressure params
    uint16_t dig_p1;
    int16_t dig_p2;
    int16_t dig_p3;
    int16_t dig_p4;
    int16_t dig_p5;
    int16_t dig_p6;
    int16_t dig_p7;
    int16_t dig_p8;
    int16_t dig_p9;
};

void bmp280_get_calib_params(struct bmp280_calib_param* params) {
    // raw temp and pressure values need to be calibrated according to
    // parameters generated during the manufacturing of the sensor
    // there are 3 temperature params, and 9 pressure params, each with a LSB
    // and MSB register, so we read from 24 registers

    uint8_t buf[NUM_CALIB_PARAMS] = { 0 };
    uint8_t reg = REG_DIG_T1_LSB;
    //i2c_write_blocking(i2c_default, ADDR, &reg, 1, true);  // true to keep master control of bus
	tls_i2c_write_byte(ADDR, 1); 
	tls_i2c_wait_ack();	   
	tls_i2c_write_byte(reg, 0);
	tls_i2c_wait_ack(); 	 										  		   
	//tls_i2c_write_byte(buf[1], 0); 				   
	//tls_i2c_wait_ack();  	   
 	tls_i2c_stop();
	tls_os_time_delay(1);
	
    // read in one go as register addresses auto-increment
    //i2c_read_blocking(i2c_default, ADDR, buf, NUM_CALIB_PARAMS, false);  // false, we're done reading
	int h = wm_i2c_start_read_it(ADDR, REG_DIG_T1_LSB, buf, NUM_CALIB_PARAMS);

    // store these in a struct for later use
    params->dig_t1 = (uint16_t)(buf[1] << 8) | buf[0];
    params->dig_t2 = (int16_t)(buf[3] << 8) | buf[2];
    params->dig_t3 = (int16_t)(buf[5] << 8) | buf[4];

    params->dig_p1 = (uint16_t)(buf[7] << 8) | buf[6];
    params->dig_p2 = (int16_t)(buf[9] << 8) | buf[8];
    params->dig_p3 = (int16_t)(buf[11] << 8) | buf[10];
    params->dig_p4 = (int16_t)(buf[13] << 8) | buf[12];
    params->dig_p5 = (int16_t)(buf[15] << 8) | buf[14];
    params->dig_p6 = (int16_t)(buf[17] << 8) | buf[16];
    params->dig_p7 = (int16_t)(buf[19] << 8) | buf[18];
    params->dig_p8 = (int16_t)(buf[21] << 8) | buf[20];
    params->dig_p9 = (int16_t)(buf[23] << 8) | buf[22];
	
	for(int i=0; i < NUM_CALIB_PARAMS; i++){
		printf("%02X:%02X\n", i, buf[i]);
	}
}

uint8_t i2c_send_addr(uint8_t addr)
{
    //I2C->DATA = addr;           // кладем адрес слейва в регистр данных I2C
	tls_reg_write32(HR_I2C_TX_RX, addr);
    //I2C->CR_SR = I2C_CR_START | I2C_CR_WR | I2C_CR_STOP;        // выдаем на шину START, запуcкаем передачу, по окончании передачи байта выдаем STOP 
	tls_reg_write32(HR_I2C_CR_SR, I2C_CR_STA | I2C_CR_WR | I2C_CR_STO);
    //while(I2C->CR_SR & I2C_SR_TIP){};   // ждем окончания отправки
	while(tls_reg_read32(HR_I2C_CR_SR) & I2C_SR_TIP);
    //return (I2C->CR_SR &  I2C_SR_RXACK);        // если обнаружили NACK, возвращаем "1", если ACK - то "0"
	return (tls_reg_read32(HR_I2C_CR_SR) & I2C_SR_NAK); // NAK - 7bit, same as RXACK
}

void UserMain(void)
{
	uint8_t buf[10];
	
	printf("BMP280 reader\n");
	wm_i2c_scl_config(WM_IO_PA_01);

	tls_gpio_cfg(WM_IO_PB_16, WM_GPIO_DIR_OUTPUT, WM_GPIO_ATTR_FLOATING);
	tls_gpio_cfg(WM_IO_PB_17, WM_GPIO_DIR_OUTPUT, WM_GPIO_ATTR_FLOATING);
	tls_gpio_cfg(WM_IO_PB_18, WM_GPIO_DIR_OUTPUT, WM_GPIO_ATTR_FLOATING);

	tls_gpio_write(WM_IO_PB_16, 0);	
	tls_gpio_write(WM_IO_PB_17, 0);	
	tls_gpio_write(WM_IO_PB_18, 0);	
	
    wm_i2c_sda_config(WM_IO_PA_04);
    
	tls_i2c_init(I2C_FREQ);

	/*
	printf("Start I2C Address scan...\n\r");
    for(uint8_t addr = 0x08; addr<0xF0; addr+=2)
    {
        if(!i2c_send_addr(addr))
        {
            sprintf(buf, "0x%.2X addr ACK found!\n\r", addr);
            printf(buf);
        }
    }
    printf("I2C Address scan finished \n\r");
	while (1){};	// stay forever!
	 */ 
    
    // 500ms sampling time, x16 filter
    const uint8_t reg_config_val = ((0x04 << 5) | (0x05 << 2)) & 0xFC;

    // send register number followed by its corresponding value
    buf[0] = REG_CONFIG;
    buf[1] = reg_config_val;
    //i2c_write_blocking(i2c_default, ADDR, buf, 2, false); // 2 byte
	printf("I2C writing addr\n");

	tls_i2c_write_byte(ADDR, 1); 
	tls_i2c_wait_ack();
	printf("I2C writing reg no\n");
	tls_i2c_write_byte(buf[0], 0);
	tls_i2c_wait_ack();
	printf("I2C writing value\n\r");
	tls_i2c_write_byte(buf[1], 0); 				   
	tls_i2c_wait_ack();  	   
 	tls_i2c_stop();
	tls_os_time_delay(1);
	printf("I2C command complete!\n\r");

    // osrs_t x1, osrs_p x4, normal mode operation
    const uint8_t reg_ctrl_meas_val = (0x01 << 5) | (0x03 << 2) | (0x03);
    buf[0] = REG_CTRL_MEAS;
    buf[1] = reg_ctrl_meas_val;
    //i2c_write_blocking(i2c_default, ADDR, buf, 2, false); // 2 byte

	printf("I2C writing addr\n\r");
	tls_i2c_write_byte(ADDR, 1); 
	tls_i2c_wait_ack();
	printf("I2C writing reg no\n\r");
	tls_i2c_write_byte(buf[0], 0);
	tls_i2c_wait_ack();
	printf("I2C writing value\n\r");
	tls_i2c_write_byte(buf[1], 0); 				   
	tls_i2c_wait_ack();  	   
 	tls_i2c_stop();
	tls_os_time_delay(1);
	printf("I2C command complete!\n\r");

    struct bmp280_calib_param params;
    bmp280_get_calib_params(&params);
	
#if DEMO_CONSOLE
	CreateDemoTask();
#endif
}

