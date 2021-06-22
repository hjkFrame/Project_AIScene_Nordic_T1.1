#ifndef __LCD_CONFIG_H
#define __LCD_CONFIG_H


#define H_VIEW
//#define USB_HARDWARE_SPI


#define LCD_8BIT_MODE   0	//����Һ������Ϊ8λģʽ   1Ϊʹ�� ���ݿ�ʹ�ø�8λDP_H
#define LCD_16BIT_MODE  1	//����Һ������Ϊ16λģʽ  1Ϊʹ��

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

