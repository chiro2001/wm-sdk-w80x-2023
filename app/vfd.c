#include "vfd.h"
#include "driver/wm_gpio_afsel.h"
#include "platform/drivers/spi/wm_spi_hal.h"

const static u8 cmds[] = {
    VFD_SET_DISPLAY_TIMING, (VFD_DIGITS - 1),
    VFD_SET_DIMMING_DATA, VFD_DIMMING,
    VFD_SET_DISPLAT_LIGHT_ON, 0x00
};

void vfd_display_str(u8 addr, const char *str) {
    u8 cmd;
    while (addr < VFD_DIGITS && str && *str) {
        cmd = VFD_DCRAM_DATA_WRITE | addr;
        tls_spi_write_with_cmd(&cmd, 1, (const u8 *)str, 1);
        addr++;
        str++;
    }
}

void vfd_clear(void) {
    u8 cmd;
    u8 data;
    for (u8 i = 0; i < VFD_DIGITS; i++) {
        cmd = VFD_DCRAM_DATA_WRITE | i;
        data = VFD_DGRAM_DATA_CLAER;
        tls_spi_write_with_cmd(&cmd, 1, &data, 1);
    }
}

void vfd_write_data(u8 addr, u8 *data, u8 len) {
    u8 cmd = VFD_CGRAM_DATA_WRITE | addr;
    tls_spi_write_with_cmd(&cmd, 1, data, len);
}

void vfd_on(void) {
    u8 cmd = VFD_SET_STAND_BY_MODE;
    u8 data = 0x00;
    tls_spi_write_with_cmd(&cmd, 1, &data, 1);
}

void vfd_off(void) {
    u8 cmd = VFD_SET_STAND_BY_MODE | 1;
    u8 data = 0x00;
    tls_spi_write_with_cmd(&cmd, 1, &data, 1);
}

void vfd_init(void) {
    wm_spi_cs_config(VFD_PIN_CS);
    wm_spi_ck_config(VFD_PIN_CP);
    wm_spi_do_config(VFD_PIN_DA);
    tls_gpio_cfg(VFD_PIN_HON, WM_GPIO_DIR_OUTPUT, WM_GPIO_ATTR_PULLHIGH);
    tls_gpio_cfg(VFD_PIN_RST, WM_GPIO_DIR_OUTPUT, WM_GPIO_ATTR_PULLHIGH);
    tls_gpio_write(VFD_PIN_HON, 1);
    tls_gpio_write(VFD_PIN_RST, 1);
    tls_os_time_delay(10);
    tls_gpio_write(VFD_PIN_RST, 0);
    tls_os_time_delay(100);
    tls_gpio_write(VFD_PIN_RST, 1);

    spi_clear_fifo();
    spi_set_mode(0);
    spi_set_endian(1);
    spi_set_sclk(VFD_SPI_CLK);
    tls_spi_setup(TLS_SPI_MODE_0, TLS_SPI_CS_LOW, VFD_SPI_CLK);

    tls_spi_write(cmds, sizeof(cmds) / sizeof(u8));
}