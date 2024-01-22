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

#include "vfd.h"

void UserMain(void)
{
	printf("hello w801\n");

	vfd_init();
	vfd_on();
	vfd_clear();

	const char *str = "Hello World! Hello World! ";
	int len = strlen(str);
	for (;;) {
		for (int i = 0; i < len / 2; i++) {
			vfd_display_str(0, str + i);
			tls_os_time_delay(100);
		}
	}

	printf("finish\n");
	
// #if DEMO_CONSOLE
// 	CreateDemoTask();
// #endif
//用户自己的task
}

