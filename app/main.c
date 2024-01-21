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
	vfd_display_str(0, "Hello~");
	
	printf("finish\n");
	
// #if DEMO_CONSOLE
// 	CreateDemoTask();
// #endif
//用户自己的task
}

