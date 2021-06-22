#ifndef __LCD_DRIVER_H
#define __LCD_DRIVER_H


#include "gd32f20x.h"
#define RED  	  0xf800
#define RED1  	  0xF9E0      //0xFE3D01
#define GREEN	  0x07e0
#define DRAK_GREEN	  0x1DE6  //0x1ABC32
#define BLUE 	  0x001f
#define BLUE1   0x25be
#define BLUE2   0x0097
#define BLUE3   0x2C5D        //0x2E8BEE
#define VIOLET  0X481F        //��ɫ 0X4E00FE
#define WHITE	  0xffff
#define BLACK	  0x0000
#define YELLOW  0xFFE0
#define ORANGE  0xFE00        //0xFEC300
//#define GRAY0   0xEF7D   	  //��ɫ0 3165 00110 001011 00101
#define GRAY0   0xffff 
#define GRAY1   0x8410      	//��ɫ1  00000 000000 00000
#define GRAY2   0x4208      	//��ɫ2  1111111111011111

#define USE_HORIZONTAL 0  //���ú�������������ʾ 0��1Ϊ���� 2��3Ϊ���


//#define LCD_CTRL   	  	GPIOE			//����TFT���ݶ˿�ΪPE��
//#define LCD_LED        	GPIO_PIN_3  //MCU_PC3   		��Ӧ��Һ����(����ģ��)TFT --PIN_LED�������������������IO���ṩ�������������3.3V��ѹ��
//#define LCD_CS        	GPIO_PIN_2  //MCU_PC2			  ��Ӧ��Һ����(����ģ��)TFT --CS
//#define LCD_SCL        	GPIO_PIN_3	//MCU_PB3			  ��Ӧ��Һ����(����ģ��)TFT --SCL
//#define LCD_SDA        	GPIO_PIN_5	//MCU_PB5 MOSI	��Ӧ��Һ����(����ģ��)TFT --SDA 
//#define LCD_SDO        	GPIO_PIN_4	//MCU_PB4 MISO	��Ӧ��Һ����(����ģ��)TFT --SDO 
//#define LCD_RS         	GPIO_PIN_1	//MCU_PB1		  ��Ӧ��Һ����(����ģ��)TFT --RS/DC
//#define LCD_RST     	  GPIO_PIN_0	//MCU_PB0			  ��Ӧ��Һ����(����ģ��)TFT --RST

#define LCD_POWER       GPIO_Pin_5  //LCD ��Դ


//#define LCD_CS_SET(x) LCD_CTRL->ODR=(LCD_CTRL->ODR&~LCD_CS)|(x ? LCD_CS:0)

//Һ�����ƿ���1�������궨��
#define	LCD_CS_SET  	gpio_bit_set( GPIOC, GPIO_PIN_10 )//GPIOC->BOR=LCD_CS    
#define	LCD_RS_SET  	gpio_bit_set( GPIOC, GPIO_PIN_5 )//GPIOB->BOR=LCD_RS     
#define	LCD_RST_SET  	gpio_bit_set( GPIOC, GPIO_PIN_4 )//GPIOB->BOR=LCD_RST    
	  
#define	LCD_LED_SET  	gpio_bit_set( GPIOC, GPIO_PIN_3 )//GPIOC->BOR=LCD_LED 

#define	LCD_SCL_SET  	gpio_bit_set( GPIOB, GPIO_PIN_3 )//GPIOB->BOR=LCD_SCL 
#define	LCD_SDA_SET  	gpio_bit_set( GPIOB, GPIO_PIN_5 )//GPIOB->BOR=LCD_SDA
#define	LCD_SDO_SET  	gpio_bit_set( GPIOB, GPIO_PIN_4 )//GPIOB->BOR=LCD_SDO

//Һ�����ƿ���0�������궨��
#define	LCD_CS_CLR  	gpio_bit_reset( GPIOC, GPIO_PIN_10 )//GPIOC->BCR=LCD_CS    
#define	LCD_RS_CLR  	gpio_bit_reset( GPIOC, GPIO_PIN_5 )//GPIOB->BCR=LCD_RS    
#define	LCD_RST_CLR  	gpio_bit_reset( GPIOC, GPIO_PIN_4 )//GPIOB->BCR=LCD_RST

#define	LCD_LED_CLR  	gpio_bit_reset( GPIOC, GPIO_PIN_3 )//GPIOC->BCR=LCD_LED  

#define	LCD_SCL_CLR   	gpio_bit_reset( GPIOB, GPIO_PIN_3 )//GPIOB->BOR=LCD_SCL 
#define	LCD_SDA_CLR   	gpio_bit_reset( GPIOB, GPIO_PIN_5 )//GPIOB->BOR=LCD_SDA
#define	LCD_SDO_CLR   	gpio_bit_reset( GPIOB, GPIO_PIN_4 )//GPIOB->BOR=LCD_SDO

//void delay_ms(u32 n);
void Lcd_WriteIndex(u8 Index);
void Lcd_WriteData(u8 Data);
//void Lcd_WriteReg(u8 Index,u8 Data);
u16 Lcd_ReadReg(u8 LCD_Reg);
void Lcd_Reset(void);
void Lcd_Init(void);
void Lcd_Clear(u16 Color);
void Lcd_DrawColor(u16 x,u16 y,u16 Color);
void Lcd_DrawColorRet(u16 x,u16 y,u16 w,u16 h, u16 Color);
void Lcd_SetXY(u16 x,u16 y);
void Gui_DrawPoint(u16 x,u16 y,u16 Data);
unsigned int Lcd_ReadPoint(u16 x,u16 y);
void Lcd_SetRegion(u16 x_start,u16 y_start,u16 x_end,u16 y_end);

#endif
