#include "vfd.h"
#include "driver/wm_gpio_afsel.h"
#include "platform/drivers/spi/wm_spi_hal.h"

const static u8 cmds[] = {
    VFD_SET_DISPLAY_TIMING, (VFD_DIGITS - 1),
    VFD_SET_DIMMING_DATA, VFD_DIMMING,
    VFD_SET_DISPLAT_LIGHT_ON, 0x00
};

#define USE_GPIO 1
// #define USE_GPIO 0

#if USE_GPIO

void _spi_write_u8_msb(u8 data) {
    for (u8 i = 0; i < 8; i++) {
        tls_gpio_write(VFD_PIN_DA, !!(data & 0x80));
        tls_gpio_write(VFD_PIN_CP, 1);
        tls_gpio_write(VFD_PIN_CP, 0);
        data <<= 1;
    }
    tls_gpio_write(VFD_PIN_DA, 1);
}
void _spi_write_u8_lsb(u8 data) {
    for (u8 i = 0; i < 8; i++) {
        tls_gpio_write(VFD_PIN_DA, !!(data & 0x01));
        tls_gpio_write(VFD_PIN_CP, 1);
        tls_gpio_write(VFD_PIN_CP, 0);
        data >>= 1;
    }
    tls_gpio_write(VFD_PIN_DA, 1);
}

#define _spi_write_u8 _spi_write_u8_lsb

void _spi_write(const u8 *data, u8 len) {
    for (u8 i = 0; i < len; i++) {
        _spi_write_u8(data[i]);
    }
}

void spi_write_cmd(u8 cmd, const u8 *data, u8 len) {
    tls_gpio_write(VFD_PIN_CS, 0);
    _spi_write_u8(cmd);
    _spi_write(data, len);
    tls_gpio_write(VFD_PIN_CS, 1);
}

void spi_write_reg(u8 cmd, u8 data) {
    tls_gpio_write(VFD_PIN_CS, 0);
    _spi_write_u8(cmd);
    _spi_write_u8(data);
    tls_gpio_write(VFD_PIN_CS, 1);
}

#else
void spi_write_cmd(u8 cmd, const u8 *data, u8 len) {
    u8 buf[32] = {0};
    buf[0] = cmd;
    memcpy(buf + 1, data, len);
    tls_gpio_write(VFD_PIN_CS, 0);
    tls_spi_write(buf, len + 1);
    tls_gpio_write(VFD_PIN_CS, 1);
}

void spi_write_reg(u8 cmd, u8 data) {
    u8 buf[2];
    buf[0] = cmd;
    buf[1] = data;
    tls_gpio_write(VFD_PIN_CS, 0);
    tls_spi_write(buf, 2);
    tls_gpio_write(VFD_PIN_CS, 1);
}

#endif

void vfd_display_str(u8 addr, const char *str) {
    while (addr < VFD_DIGITS && str && *str) {
        spi_write_reg(VFD_DCRAM_DATA_WRITE | addr, (u8)*str);
        addr++;
        str++;
    }
}

void vfd_clear(void) {
    for (u8 i = 0; i < VFD_DIGITS; i++) {
        spi_write_reg(VFD_DCRAM_DATA_WRITE | i, VFD_DGRAM_DATA_CLAER);
    }
}

void vfd_write_data(u8 addr, u8 *data, u8 len) {
    spi_write_cmd(VFD_CGRAM_DATA_WRITE | addr, data, len);
}

void vfd_on(void) {
    spi_write_reg(VFD_SET_STAND_BY_MODE, 0x00);
}

void vfd_off(void) {
    spi_write_reg(VFD_SET_STAND_BY_MODE | 1, 0x00);
}

void vfd_init(void) {
#if !(USE_GPIO)
    // wm_spi_cs_config(VFD_PIN_CS);
    tls_gpio_cfg(VFD_PIN_CS, WM_GPIO_DIR_OUTPUT, WM_GPIO_ATTR_PULLHIGH);
    wm_spi_ck_config(VFD_PIN_CP);
    wm_spi_do_config(VFD_PIN_DA);
    // wm_spi_di_config(WM_IO_PB_15);
#else
    tls_gpio_cfg(VFD_PIN_CS, WM_GPIO_DIR_OUTPUT, WM_GPIO_ATTR_PULLHIGH);
    tls_gpio_cfg(VFD_PIN_CP, WM_GPIO_DIR_OUTPUT, WM_GPIO_ATTR_PULLHIGH);
    tls_gpio_cfg(VFD_PIN_DA, WM_GPIO_DIR_OUTPUT, WM_GPIO_ATTR_PULLHIGH);
#endif

    tls_gpio_cfg(VFD_PIN_HON, WM_GPIO_DIR_OUTPUT, WM_GPIO_ATTR_PULLHIGH);
    tls_gpio_cfg(VFD_PIN_RST, WM_GPIO_DIR_OUTPUT, WM_GPIO_ATTR_PULLHIGH);

#if !(USE_GPIO)
    // spi_clear_fifo();
    spi_set_mode(0);
    spi_set_endian(1);
    tls_spi_trans_type(0);
    spi_set_chipselect_mode(SPI_CS_INACTIVE_MODE);
    spi_force_cs_out(1);
    tls_spi_setup(TLS_SPI_MODE_0, TLS_SPI_CS_LOW, VFD_SPI_CLK);
#endif

    tls_gpio_write(VFD_PIN_HON, 1);
    tls_gpio_write(VFD_PIN_RST, 1);
    tls_os_time_delay(10);
    tls_gpio_write(VFD_PIN_RST, 0);
    tls_os_time_delay(100);
    tls_gpio_write(VFD_PIN_RST, 1);
    tls_os_time_delay(10);

    for (u8 i = 0; i < sizeof(cmds) / sizeof(u8); i += 2) {
        spi_write_reg(cmds[i], cmds[i + 1]);
    }
}