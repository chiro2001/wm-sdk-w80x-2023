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
#include "wm_rtc.h"
#include "wm_internal_flash.h"
#include "wm_watchdog.h"

#include "vfd.h"
#include <string.h>

#define DEMO_ISR_IO WM_IO_PA_00

typedef enum {
  MODE_BOOT = 1,
  MODE_DISPLAY_TIME,
  MODE_DISPLAY_UPDATE,
  MODE_SETTING,
  MODE_SETTING_INFO,
  MODE_SETTING_WEBCFG,
  MODE_SETTING_BRIGHTNESS,
  MODE_SETTING_NTP,
  MODE_TEST_GRAY,
  MODE_SHOW_DATE_TIME,
} mode_t;

typedef enum {
  SETTING_DATE,
  SETTING_INFO,
  SETTING_WEBCFG,
  SETTING_BRIGHTNESS,
  SETTING_NTP,
  SETTING_DIRECTION,
  SETTING_TEST_GRAY,
  SETTING_RESET,
  SETTING_BACK,
} setting_item_t;

typedef struct {
  u16 magic;
  char ssid[64];
  char passwd[128];
  u8 dimm;
  u8 direction;
} config_t;
extern u8 gucssidData[33];
extern u8 gucpwdData[65];

#define FLASH_CONFIG_ADDR 0x1F1000
// #define FLASH_CONFIG_ADDR 0X1FF04000
// #define FLASH_CONFIG_ADDR 0X0fffff00
#define CONFIG_MAGIC 0x55aa
#define CONFIG_DEFAULT {    \
  .magic = CONFIG_MAGIC,    \
  .ssid = "504B",           \
  .passwd = "2001106504B",  \
  .dimm = VFD_DIMMING,      \
  .direction = 1,           \
}
const static config_t g_config_def = CONFIG_DEFAULT;
config_t g_config = CONFIG_DEFAULT;

void setting_item_str(setting_item_t item, char *str) {
  switch (item) {
  case SETTING_BACK:
    strcpy(str, "Go Back ");
    break;
  case SETTING_DATE:
    strcpy(str, "Show:Date Time");
    break;
  case SETTING_INFO:
    strcpy(str, "Set:Show INFO");
    break;
  case SETTING_WEBCFG:
    strcpy(str, "Set:Web Config");
    break;
  case SETTING_BRIGHTNESS:
    strcpy(str, "Set:Brightess");
    break;
  case SETTING_NTP:
    strcpy(str, "Set:NTP sync now");
    break;
  case SETTING_RESET:
    strcpy(str, "Set:Reset Config");
    break;
  case SETTING_DIRECTION:
    strcpy(str, "Set:Direction");
    break;
  case SETTING_TEST_GRAY:
    strcpy(str, "Test:Gray Display");
    break;
  }
}

u8 btn = 1;
u8 btn_ = 1;
u8 click = 0;
u32 btn_down_time = 0;
u32 btn_up_time = 0;
mode_t mode = MODE_BOOT;
u32 irq_cnt = 0;

u8 configuring = 0;
u8 configured = 0;

struct tm last_ntp = {0};

static void gpio_button_callback(void *context) {
  u16 ret;
  irq_cnt++;

  ret = tls_get_gpio_irq_status(DEMO_ISR_IO);
  // printf("[%d] int flag = %d\n", irq_cnt, ret);
  if (ret) {
    tls_clr_gpio_irq_status(DEMO_ISR_IO);
    ret = tls_gpio_read(DEMO_ISR_IO);

    btn_ = btn;
    btn = ret;
    click = 1;

    if (!btn) {
      btn_down_time = tls_os_get_time();
    } else {
      btn_up_time = tls_os_get_time();
    }
    // printf("[%d] pin %d, btn %d btn_ %d click %d down %d up %d\n",
    //   irq_cnt, ret, btn, btn_, click, btn_down_time, btn_up_time);
  }
}

void setup_button_int(void) {
  u16 gpio_pin = DEMO_ISR_IO;

  // 测试中断
  tls_gpio_cfg(gpio_pin, WM_GPIO_DIR_INPUT, WM_GPIO_ATTR_FLOATING);
  tls_gpio_isr_register(gpio_pin, gpio_button_callback, NULL);
  tls_gpio_irq_enable(gpio_pin, WM_GPIO_IRQ_TRIG_DOUBLE_EDGE);
  // printf("test gpio %d rising isr\n", gpio_pin);
}

void do_config_load() {
  int r;
  if (TLS_FLS_STATUS_OK != (r = tls_fls_read(FLASH_CONFIG_ADDR, (u8 *)&g_config, sizeof(u16)))) {
    printf("flash read failed: %d\n", r);
  }
  if (g_config.magic != CONFIG_MAGIC) {
    printf("g_config read failed, overwrite to 0x%x\n", FLASH_CONFIG_ADDR);
    g_config.magic = CONFIG_MAGIC;
    if (TLS_FLS_STATUS_OK != (r = tls_fls_write(FLASH_CONFIG_ADDR, (u8 *)&g_config, sizeof(g_config)))) {
      printf("flash write failed: %d\n", r);
    }
  }
  if (TLS_FLS_STATUS_OK != (r = tls_fls_read(FLASH_CONFIG_ADDR, (u8 *)&g_config, sizeof(g_config)))) {
    printf("flash read failed: %d\n", r);
  }
  if (g_config.magic != CONFIG_MAGIC) {
    printf("g_config read still failed\n");
  }
  printf("config load: %s %s magic:%x dimm:%d dir:%d\n", 
    g_config.ssid, g_config.passwd, g_config.magic, g_config.dimm, g_config.direction);
}

void do_config_save() {
  int r;
  printf("config save: %s %s magic:%x dimm:%d dir:%d\n", 
    g_config.ssid, g_config.passwd, g_config.magic, g_config.dimm, g_config.direction);
  if (TLS_FLS_STATUS_OK != (r = tls_fls_write(FLASH_CONFIG_ADDR, (u8 *)&g_config, sizeof(g_config)))) {
    printf("flash write failed: %d\n", r);
  }
  if (TLS_FLS_STATUS_OK != (r = tls_fls_read(FLASH_CONFIG_ADDR, (u8 *)&g_config, sizeof(g_config)))) {
    printf("flash read failed: %d\n", r);
  }
  printf("    re-load: %s %s magic:%x dimm:%d dir:%d\n", 
    g_config.ssid, g_config.passwd, g_config.magic, g_config.dimm, g_config.direction);
}

static void task_ap_config(void *sdata) {
  configured = 0;
  extern int demo_webserver_config(void);
  demo_webserver_config();
  const char *msg = "APConfig";
  const char *msg_ok = "Reset...";
  printf("task_ap_config started\n");
  if (vfd_daemon_running()) {
    vfd_draw_str(0, 0, msg);
  } else {
    vfd_display_str(0, msg);
  }
  while (!configured) {
    tls_os_time_delay(1000);
  }
  if (vfd_daemon_running()) {
    vfd_draw_str(0, 0, msg_ok);
  } else {
    vfd_display_str(0, msg_ok);
  }
  tls_sys_set_reboot_reason(REBOOT_REASON_ACTIVE);
  tls_sys_reset();
}

#define AP_TASK_SIZE 2048
static OS_STK ap_task_stack[AP_TASK_SIZE];

static void start_task_ap_config(void) {
  static int started = 0;
  if (started) {
    return;
  }
  started = 1;
  tls_os_task_create(NULL, NULL, task_ap_config, NULL, (void *)ap_task_stack,
                     AP_TASK_SIZE * sizeof(u32), AP_TASK_SIZE, 0);
}

static void net_status_changed_event(u8 status) {
  switch (status) {
  case NETIF_WIFI_JOIN_SUCCESS:
    break;
  case NETIF_WIFI_JOIN_FAILED:
    configuring = 1;
    start_task_ap_config();
    break;
  case NETIF_WIFI_DISCONNECTED:
    break;
  case NETIF_IP_NET_UP:
    configured = 1;
    if (configuring) {
      configuring = 0;
      printf("STA configured, SSID %s passwd %s\n", gucssidData, gucpwdData);
      strcpy(g_config.ssid, (const char *)gucssidData);
      strcpy(g_config.passwd, (const char *)gucpwdData);
      g_config.magic = CONFIG_MAGIC;
      do_config_save();
    }
    break;
  default:
    break;
  }
}

void user_main_loop(void) {
  // const char *str = "Hello World! Hello World! ";
  // int len = strlen(str);
  char now[64] = "";
  char last[64] = "";
  int i = 0;

  // for (u8 x = 0; x < VFD_WIDTH; x++) {
  //   // color: x (0-VFD_WIDTH) -- mapping to --> (0-255)
  //   u8 color = (u8)((u16)(x) * 256 / VFD_WIDTH);
  //   printf("color for %d is %x\n", x, color);
  //   for (u8 y = 0; y < 7; y++) {
  //     if (y == 3)
  //       vfd_draw_pixel(x, y, 0xff);
  //     else if (y == 4)
  //       vfd_draw_pixel(x, y, 0xff * 2 / VFD_GRAY);
  //     else
  //       vfd_draw_pixel(x, y, color);
  //   }
  // }
  vfd_daemon_start();

  // time_t now;
  struct tm tm;
  mode = MODE_DISPLAY_TIME;
  mode_t update_next = MODE_DISPLAY_TIME;
  setting_item_t setting_item = SETTING_INFO;
  const u32 long_press = 300;
  const u32 long_idle = 5000;
  u32 dimm = VFD_DIMMING;
  char buf[64] = "";

  while (true) {
    u32 tick = tls_os_get_time();
    if (mode == MODE_DISPLAY_TIME) {
      tls_get_rtc(&tm);
      strcpy(last, now);
      strftime(now, sizeof(now), "%H:%M:%S", &tm);

      if (strcmp(now, last) != 0) {
        // printf("time %s last %s\n", time, last);
        i = 0;
        mode = MODE_DISPLAY_UPDATE;
        update_next = MODE_DISPLAY_TIME;
      } else {
        vfd_draw_str(0, 0, now);
      }

      tls_os_time_delay(90);
      if (btn && !btn_) {
        mode = MODE_DISPLAY_UPDATE;
        setting_item = SETTING_DATE;
        update_next = MODE_SETTING;
        i = 0;
        setting_item_str(SETTING_INFO, now);
        // printf("goto settings, now %s last %s\n", now, last);
      }
    } else if (mode == MODE_DISPLAY_UPDATE) {
      // shiftting display
      u8 pp = 0;
      while (pp < VFD_DIGITS && last[pp] == now[pp]) {
        pp++;
      }
      vfd_draw_str(pp * 5, i, last + pp);
      vfd_draw_str(pp * 5, i - 7, now + pp);
      if (++i == 8) {
        i = 0;
        mode = update_next;
        strcpy(last, now);
      }
    } else if (mode == MODE_SETTING) {
      if (btn && !btn_) {
        // key up
        u32 period = btn_up_time - btn_down_time;
        // printf("btn up after %d\n", period);
        if (period < long_press) {
          setting_item_t setting_item_last = setting_item;
          setting_item++;
          if (setting_item > SETTING_BACK)
            setting_item = SETTING_DATE;
          if (setting_item != setting_item_last) {
            // printf("goto setting_item %d\n", setting_item);
            i = 0;
            mode = MODE_DISPLAY_UPDATE;
            update_next = MODE_SETTING;
          }
        } else {
          switch (setting_item) {
          case SETTING_INFO:
            mode = MODE_SETTING_INFO;
            break;
          case SETTING_WEBCFG:
            mode = MODE_SETTING_WEBCFG;
            break;
          case SETTING_BRIGHTNESS:
            mode = MODE_SETTING_BRIGHTNESS;
            dimm = g_config.dimm;
            break;
          case SETTING_BACK:
            mode = MODE_DISPLAY_TIME;
            break;
          case SETTING_NTP:
            mode = MODE_SETTING_NTP;
            break;
          case SETTING_RESET:
            memcpy(&g_config, &g_config_def, sizeof(g_config));
            do_config_save();
            tls_sys_set_reboot_reason(REBOOT_REASON_ACTIVE);
            tls_sys_reset();
            break;
          case SETTING_DIRECTION:
            g_config.direction = !g_config.direction;
            do_config_save();
            vfd_set_direction(g_config.direction);
            mode = MODE_DISPLAY_TIME;
            break;
          case SETTING_TEST_GRAY:
            mode = MODE_TEST_GRAY;
            break;
          case SETTING_DATE:
            mode = MODE_SHOW_DATE_TIME;
            break;
          }
          i = 0;
          click = 0;
          btn_ = btn;
          continue;
        }
      }
      strcpy(last, now);
      setting_item_str(setting_item, buf);
      if (i < 32 * 4) {
        strcpy(now, buf);
      } else if (i < 32 * (4 + strlen(buf) - 7)) {
        strcpy(now, buf + ((i / 32) - 4));
      } else {
        strcpy(now, buf + strlen(buf) - 8);
      }
      i++;
      if (i >= 32 * (strlen(buf) + 8 - 7)) {
        i = 0;
      }
      vfd_draw_str(0, 0, now);
      if (!btn) {
        u32 period = tick - btn_down_time;
        if (period > long_press) {
          period = long_press;
        }
        float p = (float)period / long_press;
        // u32 x = period * VFD_WIDTH / long_press;
        u32 x = (u32)((p * p * p) * VFD_WIDTH);
        vfd_draw_or_bg(0, 0, x, 7);
      }
    } else if (mode == MODE_SETTING_INFO) {
      if (i == 0) {
        struct tls_ethif *tmpethif = tls_netif_get_ethif();
        ip_addr_t *ip = &tmpethif->ip_addr;
        sprintf(now, "IP:%d.%d.%d.%d", ip4_addr1(ip_2_ip4(ip)),
                ip4_addr2(ip_2_ip4(ip)), ip4_addr3(ip_2_ip4(ip)),
                ip4_addr4(ip_2_ip4(ip)));
      }
      vfd_draw_str(0, 0, now + i);
      int len = strlen(now);
      if (i == 0) {
        tls_os_time_delay(400);
        i++;
      } else if (i == len - 8) {
        i = 0;
        tls_os_time_delay(400);
      } else {
        i++;
      }
      tls_os_time_delay(100);
      if (btn && !btn_) {
        mode = MODE_DISPLAY_TIME;
        i = 0;
      }
    } else if (mode == MODE_SETTING_WEBCFG) {
      // extern int demo_webserver_config(void);
      if (i == 0) {
        configured = 0;
        // demo_webserver_config();
        configuring = 1;
        start_task_ap_config();
        sprintf(now, "Waiting AP connect");
        i = 1;
      }
      int len = strlen(now);
      if (configured) {
        sprintf(now, "Web Configured, press to go back");
        if (i >= len + 2 - 8) {
          i = 1;
        }
      }
      vfd_draw_str(0, 0, now + i - 1);
      tls_os_time_delay(100);
      if (i == 1 || i == len + 1 - 8) {
        tls_os_time_delay(500);
      }
      i++;
      if (i >= len + 2 - 8) {
        i = 1;
      }
      if (configured && btn && !btn_) {
        mode = MODE_DISPLAY_TIME;
        i = 0;
      }
    } else if (mode == MODE_SETTING_BRIGHTNESS) {
      if (btn && !btn_) {
        // key up
        u32 period = btn_up_time - btn_down_time;
        // printf("btn up after %d\n", period);
        if (period < long_press) {
          u8 step = 32;
          if ((u16)dimm + step < 256) {
            dimm += step;
          } else {
            dimm = step - 1;
          }
          vfd_set_brightness_lock(dimm);
        } else {
          g_config.dimm = dimm;
          do_config_save();
          mode = MODE_DISPLAY_TIME;
          i = 0;
          click = 0;
          btn_ = btn;
          continue;
        }
      }
      strcpy(last, now);
      sprintf(now, "Dimm:%3d", dimm);
      vfd_draw_str(0, 0, now);
      if (!btn) {
        u32 period = tick - btn_down_time;
        if (period > long_press) {
          period = long_press;
        }
        float p = (float)period / long_press;
        // u32 x = period * VFD_WIDTH / long_press;
        u32 x = (u32)((p * p * p) * VFD_WIDTH);
        vfd_draw_or_bg(0, 0, x, 7);
      }
    } else if (mode == MODE_SETTING_NTP) {
      sprintf(now, "NTP sync");
      vfd_draw_str(0, 0, now);
      extern int ntp_demo(void);
      ntp_demo();
      tls_get_rtc(&last_ntp);
      mode = MODE_DISPLAY_TIME;
    } else if (mode == MODE_TEST_GRAY) {
      if (i == 0) {
        for (u8 x = 0; x < VFD_WIDTH; x++) {
          // color: x (0-VFD_WIDTH) -- mapping to --> (0-255)
          u8 color = (u8)((u16)(x) * 256 / VFD_WIDTH);
          printf("color for x:%d is %x\n", x, color);
          for (u8 y = 0; y < 7; y++) {
            if (y == 3)
              vfd_draw_pixel(x, y, 0xff);
            else if (y == 4)
              vfd_draw_pixel(x, y, 0xff * 2 / VFD_GRAY);
            else
              vfd_draw_pixel(x, y, color);
          }
        }
        i = 1;
      }
      if (btn && !btn_) {
        mode = MODE_DISPLAY_TIME;
        i = 0;
      }
    } else if (mode == MODE_SHOW_DATE_TIME) {
      tls_get_rtc(&tm);
      strftime(buf, sizeof(buf), "%c", &tm);

      if (i < 32 * 4) {
        strcpy(now, buf);
      } else if (i < 32 * (4 + strlen(buf) - 7)) {
        strcpy(now, buf + ((i / 32) - 4));
      } else {
        strcpy(now, buf + strlen(buf) - 8);
      }
      vfd_draw_str(0, 0, now);
      i++;
      if (i >= 32 * (strlen(buf) + 8 - 7)) {
        i = 0;
      }

      if (btn && !btn_) {
        mode = MODE_DISPLAY_TIME;
        i = 0;
      }
    }
    if (mode == MODE_SETTING) {
      u32 idle_period = tick - (btn_down_time > btn_up_time ? btn_down_time : btn_up_time);
      if (idle_period > long_idle) {
        // printf("idle_period: %d\n", idle_period);
        mode = MODE_DISPLAY_TIME;
        i = 0;
      }
    }
    if (mode == MODE_DISPLAY_TIME && last_ntp.tm_hour != tm.tm_hour) {
      // sync ntp every hour
      mode = MODE_SETTING_NTP;
    }
    if (click) {
      click = 0;
    }
    btn_ = btn;
    tls_os_time_delay(5);
  }
}

void UserMain(void) {
  printf("hello w801\n");

// #if DEMO_CONSOLE
//   CreateDemoTask();
//   return;
// #endif

  vfd_init();
  vfd_on();
  vfd_clear();

  // load g_config
  tls_fls_init();
  do_config_load();

  vfd_set_direction(g_config.direction);
  vfd_set_brightness(g_config.dimm);
  setup_button_int();

  tls_netif_add_status_event(net_status_changed_event);

  extern int demo_connect_net(char *ssid, char *pwd);
  demo_connect_net(g_config.ssid, g_config.passwd);
  char buf[24];
  sprintf(buf, "ID:%s", g_config.ssid);
  vfd_display_str(0, buf);
  extern int ntp_demo(void);
  ntp_demo();
  vfd_display_str(0, "==!OK!==");
  tls_get_rtc(&last_ntp);
  configuring = 0;

  tls_os_time_delay(200);

  user_main_loop();

  printf("finish\n");
}
