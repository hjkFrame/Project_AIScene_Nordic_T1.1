#ifndef __LCD_CONFIG_H
#define __LCD_CONFIG_H


#define H_VIEW
//#define USB_HARDWARE_SPI


#define LCD_8BIT_MODE   0	//定义液晶驱动为8位模式   1为使能 数据口使用高8位DP_H
#define LCD_16BIT_MODE  1	//定义液晶驱动为16位模式  1为使能

#define LCD_DRIVER_ILI9320  0
#define LCD_DRIVER_ILI9325  0
#define LCD_DRIVER_ILI9328  0
#define LCD_DRIVER_ILI9331  0
#define LCD_DRIVER_R61509V  0
#define LCD_DRIVER_HX8352   1

#ifdef H_VIEW
				#define X_MAX_PIXEL	        240
				#define Y_MAX_PIXEL	        320
#else
				#define X_MAX_PIXEL	        320
				#define Y_MAX_PIXEL	        240 
#endif

#endif

