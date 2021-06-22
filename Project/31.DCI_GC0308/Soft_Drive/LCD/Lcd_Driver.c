/**
  ******************************************************************************
  * @file    Lcd_Driver.c
  * @author  wangyongqing
  * @version V1.0
  * @date    2015-08-08
  * @brief   
  ******************************************************************************
***/ 

#include "Lcd_Driver.h"
#include "LCD_Config.h"
#include "systick.h"
//#include "delay.h"

//void delay_ms(u32 n)
//{
//	u32 i,j;
//	for(i=0;i<n;i++)
//	{
//		for(j=0;j<4100;j++)
//	  {
//		;
//	  }
//	}
//}

/****************************************************************************
* 名    称：u8 SPI_WriteByte(SPI_TypeDef* SPIx,u8 Byte)
* 功    能：spi 写一个字节
* 入口参数：SPIx->选择SPI1或者SPI1;Byte->需要写的数据
* 出口参数：无
* 说    明：
****************************************************************************/
//u8 SPI_WriteByte(uint32_t spi_periph,u8 Byte)//SPI_TypeDef* SPIx
//{
////	 uint32_t reg1 = SPI_STAT(spi_periph);
////   uint32_t reg2 = SPI_DATA(spi_periph);
//	while((SPI_STAT(spi_periph)&I2S_FLAG_TBE)==RESET);		 //等待发送区空	  
//		SPI_DATA(spi_periph)=Byte;	 	                         //发送一个byte   
//	while((SPI_DATA(spi_periph)&I2S_FLAG_RBNE)==RESET);   //等待接收完一个byte  
//	return SPI_DATA(spi_periph);          	               //返回收到的数据			
//} 

u8 SPI_WriteByte(uint32_t spi_periph,uint8_t byte)
{
    /* loop while data register in not emplty */
    while(RESET == spi_i2s_flag_get(spi_periph,SPI_FLAG_TBE));

    /* send byte through the SPI0 peripheral */
    spi_i2s_data_transmit(spi_periph,byte);

    /* wait to receive a byte */
    while(RESET == spi_i2s_flag_get(spi_periph,SPI_FLAG_RBNE));

    /* return the byte read from the SPI bus */
    return(spi_i2s_data_receive(spi_periph));
}

/****************************************************************************
* 名    称：void SPI1_Init(void)	
* 功    能：硬件SPI配置
* 入口参数：无
* 出口参数：无
* 说    明：使用STM32片上SPI1接口，使用的是PA5->SCK，PA6->MISO，PA7->MOSI
            PE1->RESET,PE3->LED(背光)，PE2->CS,PE0->D/C
****************************************************************************/
void SPI2_Init(void)	
{
	
	  spi_parameter_struct spi_init_struct;

    //rcu_periph_clock_enable(RCU_GPIOA);
    rcu_periph_clock_enable(RCU_GPIOB);
	  rcu_periph_clock_enable(RCU_GPIOC);
    rcu_periph_clock_enable(RCU_AF);
    rcu_periph_clock_enable(RCU_SPI2);

	gpio_pin_remap_config(GPIO_SWJ_SWDPENABLE_REMAP,ENABLE);//AFIO_PCFR1_SWJ_CFG_JTAGDISABLE 
	//gpio_pin_remap_config(GPIO_SPI2_REMAP,DISABLE);
	
    /* SPI0_CLK(PA5), SPI0_MISO_IO1(PA6), SPI0_MOSI_IO0(PA7),SPI0_IO2(PA2) and SPI0_IO3(PA3) GPIO pin configuration */
    gpio_init(GPIOB, GPIO_MODE_AF_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_3 | GPIO_PIN_5 );
	
	  gpio_init(GPIOB, GPIO_MODE_IN_FLOATING, GPIO_OSPEED_50MHZ, GPIO_PIN_4 );
    /* SPI0_CS(PB1) GPIO pin configuration */
    gpio_init(GPIOC, GPIO_MODE_OUT_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_10 | GPIO_PIN_3);

	  gpio_init(GPIOC, GPIO_MODE_OUT_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_4 | GPIO_PIN_5);
    /* chip select invalid */
	
   // SPI_FLASH_CS_HIGH();

    /* SPI0 parameter config */
    spi_init_struct.trans_mode           = SPI_TRANSMODE_FULLDUPLEX;
    spi_init_struct.device_mode          = SPI_MASTER;
    spi_init_struct.frame_size           = SPI_FRAMESIZE_8BIT;
    spi_init_struct.clock_polarity_phase = SPI_CK_PL_HIGH_PH_2EDGE;//SPI_CK_PL_HIGH_PH_1EDGE;
    spi_init_struct.nss                  = SPI_NSS_SOFT;
    spi_init_struct.prescale             = SPI_PSC_2;
    spi_init_struct.endian               = SPI_ENDIAN_MSB;//SPI_ENDIAN_LSB;
    spi_init(SPI2, &spi_init_struct);

//    /* set crc polynomial */
//    spi_crc_polynomial_set(SPI2,7);

//    /* quad wire SPI_IO2 and SPI_IO3 pin output enable */
//    qspi_io23_output_enable(SPI0);

    /* enable SPI0 */
    spi_enable(SPI2);
//	SPI_InitPara  SPI_InitStructure;
//	GPIO_InitPara GPIO_InitStructure;
//	
//	RCC_APB2PeriphClock_Enable(RCC_APB2PERIPH_AF|RCC_APB2PERIPH_GPIOB|RCC_APB2PERIPH_GPIOC, ENABLE);//
//	RCC_APB1PeriphClock_Enable(RCC_APB1PERIPH_SPI3 ,ENABLE);
//	GPIO_PinRemapConfig(GPIO_REMAP_SWJ_JTAGDISABLE,ENABLE);//AFIO_PCFR1_SWJ_CFG_JTAGDISABLE 
//	GPIO_PinRemapConfig(GPIO_REMAP_SPI3,DISABLE);
//	//AFIO_PCF0 = (AFIO_PCF0 & 0xF8FFFFFF) | 0x04000000;
//	//gpio_pin_remap_config(GPIO_SWJ_DISABLE_REMAP, ENABLE);
//	
//	//配置SPI1管脚
//	
//	GPIO_InitStructure.GPIO_Pin = GPIO_PIN_3 | GPIO_PIN_5;
//	GPIO_InitStructure.GPIO_Speed = GPIO_SPEED_50MHZ;
//	GPIO_InitStructure.GPIO_Mode = GPIO_MODE_AF_PP;
//	GPIO_Init(GPIOB, &GPIO_InitStructure);
//	
//	GPIO_InitStructure.GPIO_Pin = GPIO_PIN_4;
//	GPIO_InitStructure.GPIO_Speed = GPIO_SPEED_50MHZ;
//	GPIO_InitStructure.GPIO_Mode = GPIO_MODE_IN_FLOATING;
//	GPIO_Init(GPIOB, &GPIO_InitStructure);

//	GPIO_InitStructure.GPIO_Pin = GPIO_PIN_0|GPIO_PIN_1;
//	GPIO_InitStructure.GPIO_Speed = GPIO_SPEED_50MHZ;
//	GPIO_InitStructure.GPIO_Mode = GPIO_MODE_OUT_PP;
//	GPIO_Init(GPIOB, &GPIO_InitStructure);
//	
//	GPIO_InitStructure.GPIO_Pin = GPIO_PIN_2 | GPIO_PIN_3;
//	GPIO_InitStructure.GPIO_Speed = GPIO_SPEED_50MHZ;
//	GPIO_InitStructure.GPIO_Mode = GPIO_MODE_OUT_PP;
//  GPIO_Init(GPIOC,&GPIO_InitStructure);
//	
//	

//	//SPI1配置选项
//	//RCC_APB1PeriphClock_Enable(RCC_APB1PERIPH_SPI3 ,ENABLE);
//	//RCC_PCLK1Config(RCC_HCLK_Div2);  // 36M
//	SPI_InitStructure.SPI_TransType = SPI_TRANSTYPE_FULLDUPLEX;
//	SPI_InitStructure.SPI_Mode = SPI_MODE_MASTER;
//	SPI_InitStructure.SPI_FrameFormat = SPI_FRAMEFORMAT_8BIT;
//	SPI_InitStructure.SPI_SCKPL = SPI_SCKPL_HIGH;
//	SPI_InitStructure.SPI_SCKPH = SPI_SCKPH_1EDGE;
//	SPI_InitStructure.SPI_SWNSSEN = SPI_SWNSS_SOFT;
//	SPI_InitStructure.SPI_PSC = SPI_PSC_2;
//	SPI_InitStructure.SPI_FirstBit = SPI_FIRSTBIT_MSB;
//	SPI_InitStructure.SPI_CRCPOL = 7;
//	//QSPI_IO34DRV(ENABLE);
//	SPI_Init(SPI3, &SPI_InitStructure);

//	//使能SPI1
//	SPI_Enable(SPI3, ENABLE);
}

/****************************************************************************
* 名    称：void ili9220B_WriteIndex(u16 idx)
* 功    能：写 ili9220B 控制器寄存器地址
* 入口参数：Index   寄存器地址
* 出口参数：无
* 说    明：调用前需先选中控制器，内部函数
****************************************************************************/
void Lcd_WriteIndex(u8 Index)
{
	LCD_RS_CLR;//SPI 写命令时序开始
	SPI_WriteByte(SPI2,Index);
}

/****************************************************************************
* 名    称：void ili9220B_WriteData(u16 dat)
* 功    能：写 ili9220B 寄存器数据
* 入口参数：dat     寄存器数据
* 出口参数：无
* 说    明：向控制器指定地址写入数据，调用前需先写寄存器地址，内部函数
****************************************************************************/
void Lcd_WriteData(u8 Data)
{
	LCD_RS_SET;
	SPI_WriteByte(SPI2,Data);
}

void Lcd_WriteData16Bit(u8 DataH,u8 DataL)
{
	Lcd_WriteData(DataH);
	Lcd_WriteData(DataL);
}

void Lcd_WriteIndex16Bit(u8 DataH,u8 DataL)
{
	Lcd_WriteIndex(DataH);
	Lcd_WriteIndex(DataL);
}

/****************************************************************************
* 名    称：void Lcd_Reset(void)
* 功    能：LCD 复位
* 入口参数：无
* 出口参数：无
* 说    明：函数中
****************************************************************************/
void Lcd_Reset(void)
{
	LCD_RST_CLR;
	delay_1ms(100);
	LCD_RST_SET;
	delay_1ms(50);
}

/****************************************************************************
* 名    称：void Lcd_Init(void)
* 功    能：LCD 初始化
* 入口参数：无
* 出口参数：无
* 说    明：函数中对使用的SPI接口做了配置，可以使用模拟或者stm32硬件的SPI
****************************************************************************/
void Lcd_Init(void)
{
	SPI2_Init();
	Lcd_Reset();
	
	
	//************* Start Initial Sequence **********//
	Lcd_WriteIndex(0x11); //Sleep out 
	delay_1ms(120);              //Delay 120ms 
	//************* Start Initial Sequence **********// 
	Lcd_WriteIndex(0xCF);
	Lcd_WriteData(0x00);
	Lcd_WriteData(0xC1);
	Lcd_WriteData(0X30);
	
	Lcd_WriteIndex(0xED);
	Lcd_WriteData(0x64);
	Lcd_WriteData(0x03);
	Lcd_WriteData(0X12);
	Lcd_WriteData(0X81);
	
	Lcd_WriteIndex(0xE8);
	Lcd_WriteData(0x85);
	Lcd_WriteData(0x00);
	Lcd_WriteData(0x79);
	
	Lcd_WriteIndex(0xCB);
	Lcd_WriteData(0x39);
	Lcd_WriteData(0x2C);
	Lcd_WriteData(0x00);
	Lcd_WriteData(0x34);
	Lcd_WriteData(0x02);
	
	Lcd_WriteIndex(0xF7);
	Lcd_WriteData(0x20);
	
	Lcd_WriteIndex(0xEA);
	Lcd_WriteData(0x00);
	Lcd_WriteData(0x00);
	
	Lcd_WriteIndex(0xC0); //Power control
	Lcd_WriteData(0x1D); //VRH[5:0]
	
	Lcd_WriteIndex(0xC1); //Power control
	Lcd_WriteData(0x12); //SAP[2:0];BT[3:0]
	
	Lcd_WriteIndex(0xC5); //VCM control
	Lcd_WriteData(0x33);
	Lcd_WriteData(0x3F);
	
	Lcd_WriteIndex(0xC7); //VCM control
	Lcd_WriteData(0x92);
	
	Lcd_WriteIndex(0x3A); // Memory Access Control
	Lcd_WriteData(0x55);
	
	Lcd_WriteIndex(0x36); // Memory Access Control
	if(USE_HORIZONTAL==0)Lcd_WriteData(0x08);
	else if(USE_HORIZONTAL==1)Lcd_WriteData(0xC8);
	else if(USE_HORIZONTAL==2)Lcd_WriteData(0x78);
	else Lcd_WriteData(0xA8);
	
	Lcd_WriteIndex(0xB1);
	Lcd_WriteData(0x00);
	Lcd_WriteData(0x12);
	
	Lcd_WriteIndex(0xB6); // Display Function Control
	Lcd_WriteData(0x0A);
	Lcd_WriteData(0xA2);

	Lcd_WriteIndex(0x44);
	Lcd_WriteData(0x02);

	Lcd_WriteIndex(0xF2); // 3Gamma Function Disable
	Lcd_WriteData(0x00);
	
	Lcd_WriteIndex(0x26); //Gamma curve selected
	Lcd_WriteData(0x01);
	
	Lcd_WriteIndex(0xE0); //Set Gamma
	Lcd_WriteData(0x0F);
	Lcd_WriteData(0x22);
	Lcd_WriteData(0x1C);
	Lcd_WriteData(0x1B);
	Lcd_WriteData(0x08);
	Lcd_WriteData(0x0F);
	Lcd_WriteData(0x48);
	Lcd_WriteData(0xB8);
	Lcd_WriteData(0x34);
	Lcd_WriteData(0x05);
	Lcd_WriteData(0x0C);
	Lcd_WriteData(0x09);
	Lcd_WriteData(0x0F);
	Lcd_WriteData(0x07);
	Lcd_WriteData(0x00);
	
	Lcd_WriteIndex(0XE1); //Set Gamma
	Lcd_WriteData(0x00);
	Lcd_WriteData(0x23);
	Lcd_WriteData(0x24);
	Lcd_WriteData(0x07);
	Lcd_WriteData(0x10);
	Lcd_WriteData(0x07);
	Lcd_WriteData(0x38);
	Lcd_WriteData(0x47);
	Lcd_WriteData(0x4B);
	Lcd_WriteData(0x0A);
	Lcd_WriteData(0x13);
	Lcd_WriteData(0x06);
	Lcd_WriteData(0x30);
	Lcd_WriteData(0x38);
	Lcd_WriteData(0x0F);
	
	Lcd_WriteIndex(0x29); //Display on
	
//	Lcd_WriteIndex(0xCB);
//	Lcd_WriteData(0x39);
//	Lcd_WriteData(0x2C);
//	Lcd_WriteData(0x00);
//	Lcd_WriteData(0x34);
//	Lcd_WriteData(0x02);

//	Lcd_WriteIndex(0xCF);  
//	Lcd_WriteData(0x00); 
//	Lcd_WriteData(0XC1); 
//	Lcd_WriteData(0X30); 

//	Lcd_WriteIndex(0xE8);  
//	Lcd_WriteData(0x85); 
//	Lcd_WriteData(0x00); 
//	Lcd_WriteData(0x78); 

//	Lcd_WriteIndex(0xEA);  
//	Lcd_WriteData(0x00); 
//	Lcd_WriteData(0x00); 

//	Lcd_WriteIndex(0xED);  
//	Lcd_WriteData(0x64); 
//	Lcd_WriteData(0x03); 
//	Lcd_WriteData(0X12); 
//	Lcd_WriteData(0X81); 

//	Lcd_WriteIndex(0xF7);  
//	Lcd_WriteData(0x20); 

//	Lcd_WriteIndex(0xC0);   //Power control 
//	Lcd_WriteData(0x23);    //VRH[5:0] 

//	Lcd_WriteIndex(0xC1);   //Power control 
//	Lcd_WriteData(0x10);    //SAP[2:0];BT[3:0] 

//	Lcd_WriteIndex(0xC5);   //VCM control 
//	Lcd_WriteData(0x3e);    //对比度调节
//	Lcd_WriteData(0x28); 

//	Lcd_WriteIndex(0xC7);   //VCM control2 
//	Lcd_WriteData(0x86);    //--

//	Lcd_WriteIndex(0x36);   // Memory Access Control 
//#ifdef H_VIEW
//	Lcd_WriteData(0xE8);    //C8//48 68竖屏//28 E8 横屏
//#else
//	Lcd_WriteData(0x48); 
//#endif

//	Lcd_WriteIndex(0x3A);    
//	Lcd_WriteData(0x55); 

//	Lcd_WriteIndex(0xB1);    
//	Lcd_WriteData(0x00);  
//	Lcd_WriteData(0x18); 

//	Lcd_WriteIndex(0xB6);    // Display Function Control 
//	Lcd_WriteData(0x08); 
//	Lcd_WriteData(0x82);
//	Lcd_WriteData(0x27);  

//	Lcd_WriteIndex(0xF2);    // 3Gamma Function Disable 
//	Lcd_WriteData(0x00); 

//	Lcd_WriteIndex(0x26);    //Gamma curve selected 
//	Lcd_WriteData(0x01); 

//	Lcd_WriteIndex(0xE0);    //Set Gamma 
//	Lcd_WriteData(0x0F); 
//	Lcd_WriteData(0x31); 
//	Lcd_WriteData(0x2B); 
//	Lcd_WriteData(0x0C); 
//	Lcd_WriteData(0x0E); 
//	Lcd_WriteData(0x08); 
//	Lcd_WriteData(0x4E); 
//	Lcd_WriteData(0xF1); 
//	Lcd_WriteData(0x37); 
//	Lcd_WriteData(0x07); 
//	Lcd_WriteData(0x10); 
//	Lcd_WriteData(0x03); 
//	Lcd_WriteData(0x0E); 
//	Lcd_WriteData(0x09); 
//	Lcd_WriteData(0x00); 

//	Lcd_WriteIndex(0XE1);    //Set Gamma 
//	Lcd_WriteData(0x00); 
//	Lcd_WriteData(0x0E); 
//	Lcd_WriteData(0x14); 
//	Lcd_WriteData(0x03); 
//	Lcd_WriteData(0x11); 
//	Lcd_WriteData(0x07); 
//	Lcd_WriteData(0x31); 
//	Lcd_WriteData(0xC1); 
//	Lcd_WriteData(0x48); 
//	Lcd_WriteData(0x08); 
//	Lcd_WriteData(0x0F); 
//	Lcd_WriteData(0x0C); 
//	Lcd_WriteData(0x31); 
//	Lcd_WriteData(0x36); 
//	Lcd_WriteData(0x0F); 

//	Lcd_WriteIndex(0x11);    //Exit Sleep 
//	delay_ms(120); 
//	
//	Lcd_WriteIndex(0x29);    //Display on 
//	Lcd_WriteIndex(0x2c); 
}


/****************************************************************************
名    称：LCD_Set_Region
功    能：设置lcd显示区域，在此区域写点数据自动换行
入口参数：xy起点和终点,Y_IncMode表示先自增y再自增x
出口参数：无
****************************************************************************/
void Lcd_SetRegion(u16 x_start,u16 y_start,u16 x_end,u16 y_end)
{	
	Lcd_WriteIndex(0x2a);
	Lcd_WriteData16Bit(x_start>>8,x_start);
	Lcd_WriteData16Bit(x_end>>8,x_end);
	Lcd_WriteIndex(0x2b);
	Lcd_WriteData16Bit(y_start>>8,y_start);
	Lcd_WriteData16Bit(y_end>>8,y_end);
	Lcd_WriteIndex(0x2c);
}

/****************************************************************************
名    称：LCD_Set_XY
功    能：设置lcd显示起始点
入口参数：x，y坐标
出口参数：无
****************************************************************************/
void Lcd_SetXY(u16 x,u16 y)
{
	Lcd_WriteIndex(0x2a);
	Lcd_WriteData16Bit(x>>8,x);
	Lcd_WriteIndex(0x2b);
	Lcd_WriteData16Bit(y>>8,y);
	Lcd_WriteIndex(0x2c);
}

/****************************************************************************
名    称：LCD_DrawPoint
功    能：画一个点
入口参数：无
出口参数：无
****************************************************************************/
void Gui_DrawPoint(u16 x,u16 y,u16 Data)
{
	Lcd_SetXY(x,y);
	Lcd_WriteData(Data>>8);
	Lcd_WriteData(Data);
}

/****************************************************************************
名    称：unsigned int Lcd_ReadPoint(u16 x,u16 y)
功    能：读TFT某一点的颜色        
入口参数：x,y的坐标位置
出口参数：color  点颜色值           
****************************************************************************/
unsigned int Lcd_ReadPoint(u16 x,u16 y)
{
//	unsigned int Data;
//	Lcd_SetXY(x,y);

//	//Lcd_ReadData();//丢掉无用字节
//	Data=Lcd_ReadData();
//	//Lcd_WriteData(Data);
//	return Data;
	
	
	
	u8 r,g,b;
	u16 R,G,B,Data;
	Lcd_SetXY(x,y);  
	LCD_CS_CLR;
	Lcd_WriteIndex(0X2E);       //?????????
  LCD_RS_SET;
	LCD_RS_CLR;                 //????
  SPI_WriteByte(SPI2,0xff);   //??????DUMMY CLOCK
	r=SPI_WriteByte(SPI2,0xff); //?????
	g=SPI_WriteByte(SPI2,0xff); //?????
	b=SPI_WriteByte(SPI2,0xff); //?????
	LCD_CS_SET;
	R = (r<<1)&0x00FF;
	G = g&0x00FF;
	B = (b<<1)&0x00FF;
	Data = 	(R<<8)|(G<<5)|(B>>3);
	LCD_CS_CLR;
	return Data;
}

/****************************************************************************
名    称：Lcd_Clear
功    能：全屏清屏函数
入口参数：填充颜色COLOR
出口参数：无
****************************************************************************/
void Lcd_Clear(u16 Color)               
{	
	unsigned int i,m;
	Lcd_SetRegion(0,0,X_MAX_PIXEL-1,Y_MAX_PIXEL-1);
	LCD_RS_SET;
	for(i=0;i<Y_MAX_PIXEL;i++)
	{
		for(m=0;m<X_MAX_PIXEL;m++)
		{	 
			SPI_WriteByte(SPI2,Color>>8);
			SPI_WriteByte(SPI2,Color);
		}   
	}
}

void Lcd_DrawColor(u16 x,u16 y,u16 Color)
{
	unsigned int i,m;
	Lcd_SetRegion(x,y,X_MAX_PIXEL-1,Y_MAX_PIXEL-1);
	LCD_RS_SET;
	for(i=0;i<Y_MAX_PIXEL-y;i++)
	{
		for(m=0;m<X_MAX_PIXEL-x;m++)
		{	 
			SPI_WriteByte(SPI2,Color>>8);
			SPI_WriteByte(SPI2,Color);
		}
	}
}
/****************************************************************************
名    称：Lcd_DrawColorRet
功    能：绘制方形色块
入口参数：填充颜色COLOR
出口参数：x，y-->绘制的色块的起始位置
          w，h-->色块的宽和高
          Color->色块的颜色
****************************************************************************/
void Lcd_DrawColorRet(u16 x,u16 y,u16 w,u16 h, u16 Color)
{
  unsigned int i,m;
	Lcd_SetRegion(x,y,x+w-1,y+h-1);
	LCD_RS_SET;
	for(i=y;i<h+y;i++)
	{
		for(m=x;m<w+x;m++)
		{
			SPI_WriteByte(SPI2,Color>>8);
			SPI_WriteByte(SPI2,Color);
		}
	}
	Lcd_SetRegion(0,0,X_MAX_PIXEL-1,Y_MAX_PIXEL-1);
	//LCD_RS_SET;
}
