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
#define VFD_WIDTH (VFD_DIGITS * 5)
#define VFD_DIMMING (32 - 1)
#define VFD_SPI_CLK 100000
// #define VFD_SPI_CLK SPI_DEFAULT_SPEED
#define VFD_GRAY_MAX 32
#define VFD_FRAME_SZ (VFD_DIGITS * VFD_GRAY_MAX)

void vfd_display_str(u8 addr, const char *str);
void vfd_clear(void);
void vfd_set_brightness(u8 brightness);
void vfd_set_brightness_lock(int brightness);
void vfd_write_data(u8 addr, u8 *data, u8 len);
void vfd_on(void);
void vfd_off(void);
void vfd_init(void);
void vfd_update_framebuf(u16 offset);
void vfd_set_direction(u8 direction);
void vfd_draw_char(u8 x, int y, char ch);
void vfd_draw_str(u8 x, int y, const char *str);
void vfd_daemon_start();
int vfd_daemon_running();
void vfd_draw_pixel(u8 x, u8 y, u8 color);
void vfd_draw_pixel_gray(u8 x, u8 y, u8 gray);
void vfd_draw_inverse(u8 sx, u8 sy, u8 ex, u8 ey);
void vfd_draw_or_bg(u8 sx, u8 sy, u8 ex, u8 ey);
u8 vfd_gray_level(void);
void vfd_set_gray(u8 gray);

#endif