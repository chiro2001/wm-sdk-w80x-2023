#ifndef __VFD_H__
#define __VFD_H__

#include "wm_include.h"

/*
CP - HSPI_CK - PB12
DA - HSPI_DO - PB11
CS - HSPI_CS - PB14
HON- PB10
RST- PB15
*/
// pins
#define VFD_PIN_CP WM_IO_PB_12
#define VFD_PIN_DA WM_IO_PB_11
#define VFD_PIN_CS WM_IO_PB_14
#define VFD_PIN_HON WM_IO_PB_10
#define VFD_PIN_RST WM_IO_PB_15

// commands
#define VFD_DCRAM_DATA_WRITE 0x20
#define VFD_DGRAM_DATA_CLAER 0x10
#define VFD_CGRAM_DATA_WRITE 0x40
#define VFD_SET_DISPLAY_TIMING 0xE0
#define VFD_SET_DIMMING_DATA 0xE4
#define VFD_SET_DISPLAT_LIGHT_ON 0xE8
#define VFD_SET_DISPLAT_LIGHT_OFF 0xEA
#define VFD_SET_STAND_BY_MODE 0xEC

// options
#define VFD_DIGITS 8
#define VFD_DIMMING 128
#define VFD_SPI_CLK 100000
// #define VFD_SPI_CLK SPI_DEFAULT_SPEED

void vfd_display_str(u8 addr, const char *str);
void vfd_clear(void);
void vfd_write_data(u8 addr, u8 *data, u8 len);
void vfd_on(void);
void vfd_off(void);
void vfd_init(void);

#endif