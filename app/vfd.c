#include "vfd.h"
#include "FreeRTOS.h"
#include "driver/wm_gpio_afsel.h"
#include "font5x7.h"
#include "platform/drivers/spi/wm_spi_hal.h"

const static u8 cmds[] = {VFD_SET_DISPLAY_TIMING,   (VFD_DIGITS - 1),
                          VFD_SET_DIMMING_DATA,     VFD_DIMMING,
                          VFD_SET_DISPLAT_LIGHT_ON, 0x00};

static u8 framebuf[5 * (VFD_FRAME_SZ)] = {0};
static u8 vfd_gray = VFD_GRAY_MAX;

// const static unsigned char lut8[8 * 8] = {
//     0, 0, 0, 0, 0, 0, 0, 0, // 0
//     1, 0, 0, 0, 1, 0, 0, 0, // 1
//     1, 0, 1, 0, 0, 1, 0, 0, // 2
//     1, 0, 1, 0, 1, 0, 1, 0, // 3
//     1, 0, 1, 1, 0, 1, 0, 1, // 4
//     0, 1, 0, 1, 0, 1, 0, 0, // 5
//     1, 1, 0, 1, 1, 0, 1, 0, // 6
//     1, 1, 1, 1, 1, 1, 1, 1  // 7
// };

static tls_os_mutex_t *vfd_mutex = NULL;
static u8 vfd_direction = 0;

#define USE_GPIO 1
// #define USE_GPIO 0

#if USE_GPIO

void small_delay(volatile u32 t) {
  while (t--);
}

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

void vfd_set_brightness(u8 brightness) {
  spi_write_reg(VFD_SET_DIMMING_DATA, brightness);
}

void vfd_set_brightness_lock(int brightness) {
  if (vfd_mutex)
    if (TLS_OS_ERROR == tls_os_mutex_acquire(vfd_mutex, 0))
      printf("vfd mutex acquire failed\n");
  vfd_set_brightness(brightness);
  if (vfd_mutex)
    tls_os_mutex_release(vfd_mutex);
}

void vfd_write_data(u8 addr, u8 *data, u8 len) {
  spi_write_cmd(VFD_CGRAM_DATA_WRITE | addr, data, len);
}

void vfd_on(void) { spi_write_reg(VFD_SET_STAND_BY_MODE, 0x00); }

void vfd_off(void) { spi_write_reg(VFD_SET_STAND_BY_MODE | 1, 0x00); }

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
  tls_os_time_delay(1);
  tls_gpio_write(VFD_PIN_RST, 0);
  tls_os_time_delay(10);
  tls_gpio_write(VFD_PIN_RST, 1);
  tls_os_time_delay(1);

  for (u8 i = 0; i < sizeof(cmds) / sizeof(u8); i += 2) {
    spi_write_reg(cmds[i], cmds[i + 1]);
  }

  memset(framebuf, 0, sizeof(framebuf));
  vfd_mutex = NULL;
}

#define REVERSE_BITS_B(x) ( \
  ((((x) >> 0) & 1) << 7) | \
  ((((x) >> 1) & 1) << 6) | \
  ((((x) >> 2) & 1) << 5) | \
  ((((x) >> 3) & 1) << 4) | \
  ((((x) >> 4) & 1) << 3) | \
  ((((x) >> 5) & 1) << 2) | \
  ((((x) >> 6) & 1) << 1) | \
  ((((x) >> 7) & 1) << 0)   \
)

void vfd_update_framebuf(u16 offset) {
  if (vfd_mutex)
    if (TLS_OS_ERROR == tls_os_mutex_acquire(vfd_mutex, 0))
      printf("vfd mutex acquire failed\n");
  const u8 *data = framebuf + offset;
  if (vfd_direction == 0) {
    for (u8 i = 0; i < VFD_DIGITS; i++) {
      spi_write_cmd(VFD_CGRAM_DATA_WRITE | i, data + i * 5, 5);
    }
    const u8 index[8] = {0, 1, 2, 3, 4, 5, 6, 7};
    spi_write_cmd(VFD_DCRAM_DATA_WRITE, index, VFD_DIGITS);
  } else {
    u8 buf[5];
    for (u8 i = 0; i < VFD_DIGITS; i++) {
      for (u8 j = 0; j < 5; j++) {
        u8 d = data[i * 5 + j];
        buf[4 - j] = REVERSE_BITS_B(d) >> 1;
      }
      spi_write_cmd(VFD_CGRAM_DATA_WRITE | i, buf, 5);
    }
    const u8 index[8] = {7, 6, 5, 4, 3, 2, 1, 0};
    spi_write_cmd(VFD_DCRAM_DATA_WRITE, index, VFD_DIGITS);
  }
  if (vfd_mutex)
    tls_os_mutex_release(vfd_mutex);
}

void vfd_set_direction(u8 direction) {
  vfd_direction = direction;
}

void vfd_draw_char(u8 x, int y, char ch) {
  if (vfd_mutex)
    if (TLS_OS_ERROR == tls_os_mutex_acquire(vfd_mutex, 0))
      printf("vfd mutex acquire failed\n");
  u8 c = ch - ' ';
  const u8 *font = font5x7 + c * 5;
  if (y == 0) {
    for (u8 i = 0; i < vfd_gray; i++) {
      memcpy(framebuf + i * VFD_WIDTH + x, font, 5);
    }
  } else {
    if (y < 0) {
      y = -y;
      // font higher
      for (u8 i = 0; i < vfd_gray; i++) {
        for (u8 k = 0; k < 5; k++) {
          u8 *frame = framebuf + i * VFD_WIDTH + x + k;
          // *frame &= ((1 << y) - 1);
          *frame |= font[k] >> y;
        }
      }
    } else {
      // frame higher
      for (u8 i = 0; i < vfd_gray; i++) {
        for (u8 k = 0; k < 5; k++) {
          u8 *frame = framebuf + i * VFD_WIDTH + x + k;
          *frame &= (~((1 << y) - 1)) << (7 - y);
          *frame |= font[k] << y;
        }
      }
    }
  }
  if (vfd_mutex)
    tls_os_mutex_release(vfd_mutex);
}

void vfd_draw_str(u8 x, int y, const char *str) {
  while (x < VFD_WIDTH && str && *str) {
    vfd_draw_char(x, y, *str);
    x += 5;
    str++;
  }
}

void vfd_draw_pixel_gray(u8 x, u8 y, u8 gray) {
  if (vfd_mutex)
    if (TLS_OS_ERROR == tls_os_mutex_acquire(vfd_mutex, 0))
      printf("vfd mutex acquire failed\n");
  u8 mask = 1 << y;
  // printf("color for %d is %x (gray %d)\n", x, color, gray);
  for (u8 k = 0; k < vfd_gray; k++) {
    if (k < gray) {
      framebuf[x + k * VFD_WIDTH] |= mask;
    } else {
      framebuf[x + k * VFD_WIDTH] &= ~mask;
    }
  }
  if (vfd_mutex)
    tls_os_mutex_release(vfd_mutex);
}

void vfd_draw_pixel(u8 x, u8 y, u8 color) {
  u8 gray = (u16)color * vfd_gray / 255;
  vfd_draw_pixel_gray(x, y, gray);
}

void vfd_draw_inverse(u8 sx, u8 sy, u8 ex, u8 ey) {
  for (u8 x = sx; x < ex; x++) {
    if (sy == 0 && ey >= 7) {
      for (u8 k = 0; k < vfd_gray; k++) {
        framebuf[x + k * VFD_WIDTH] = ~framebuf[x + k * VFD_WIDTH];
      }
    }
  }
}

void vfd_draw_or_bg(u8 sx, u8 sy, u8 ex, u8 ey) {
  for (u8 x = sx; x < ex; x++) {
    if (sy == 0 && ey >= 7) {
      for (u8 k = 0; k < 1; k++) {
        framebuf[x + k * VFD_WIDTH] |= 0xff;
      }
    }
  }
}

#define VFD_TASK_SIZE 2048
static OS_STK vfd_task_stack[VFD_TASK_SIZE];

void vfd_daemon(void *sdata) {
  // u8 dimm = 32;
  // u8 dimmdd = 2;
  // u8 dimmd = dimmdd;
  while (true) {
// #if (vfd_gray == 8)
//     if (vfd_mutex)
//       if (TLS_OS_ERROR == tls_os_mutex_acquire(vfd_mutex, 0))
//         printf("vfd mutex acquire failed\n");
//     // u16 r = rand();
//     for (u8 i = 0; i < vfd_gray; i++) {
//       for (u8 j = 0; j < vfd_gray; j++) {
//         // if (lut8[((j + r) % vfd_gray) * vfd_gray + i]) {
//         if (lut8[j * vfd_gray + i]) {
//           vfd_update_framebuf(j * VFD_WIDTH);
//           // tls_os_time_delay(1);
//         }
//       }
//       // r = (r << 1) | (r >> 15);
//     }
//     if (vfd_mutex)
//       tls_os_mutex_release(vfd_mutex);
// #else
    // vfd_set_brightness(dimm);
    // dimm += dimmd;
    // if (dimm == 0) dimmd = dimmdd;
    // if (dimm == 256 - dimmdd) dimmd = -dimmdd;
    for (u8 i = 0; i < vfd_gray; i++) {
      // spi_write_reg(VFD_SET_DIMMING_DATA, i * (255 / vfd_gray));
      vfd_update_framebuf(i * VFD_WIDTH);
      tls_os_time_delay(vfd_gray > 8 ? 0 : 1);
    }
// #endif
  }
}

static int daemon_running = 0;

void vfd_daemon_start() {
  tls_os_mutex_create(0, &vfd_mutex);
  if (!vfd_mutex) {
    printf("vfd mutex create failed\n");
    return;
  }
  // printf("lut8[]:\n");
  // for (u8 i = 0; i < 8; i++) {
  //   for (u8 j = 0; j < 8; j++) {
  //     printf("%d ", lut8[i * 8 + j]);
  //   }
  //   printf("\n");
  // }
  tls_os_task_create(NULL, NULL, vfd_daemon, NULL, (void *)vfd_task_stack,
                     VFD_TASK_SIZE * sizeof(u32), VFD_TASK_SIZE, 0);
  daemon_running = 1;
}

int vfd_daemon_running() {
  return daemon_running;
}

u8 vfd_gray_level(void) {
  return vfd_gray;
}

void vfd_set_gray(u8 gray) {
  vfd_gray = gray;
}