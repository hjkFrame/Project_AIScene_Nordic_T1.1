/*!
    \file  dci_ov2640.c
    \brief DCI config file
*/

/*
    Copyright (C) 2017 GigaDevice

    2015-07-15, V1.0.0, demo for GD32F20x
    2017-06-05, V2.0.0, demo for GD32F20x
*/

#include "dci_ov2640.h"
#include "dci_ov2640_init_table.h"
#include "gd32f20x_dci.h"
#include "systick.h"

extern uint8_t capture_image[320*240*2];
#define GC0308_SET_PAGE0 dci_byte_write(0xfe , 0x00)
#define GC0308_SET_PAGE1 dci_byte_write(0xfe , 0x01)

/*!
    \brief      configure the DCI to interface with the camera module
    \param[in]  none
    \param[out] none
    \retval     none
*/
void dci_config(void) {
    dci_parameter_struct dci_struct;
    dma_parameter_struct dma_struct;

    rcu_periph_clock_enable(RCU_GPIOA);
    rcu_periph_clock_enable(RCU_GPIOB);
    rcu_periph_clock_enable(RCU_GPIOC);
    //rcu_periph_clock_enable(RCU_GPIOD);
    rcu_periph_clock_enable(RCU_DCI);
    rcu_periph_clock_enable(RCU_AF);
    
    /* DCI GPIO remap configuration */
    gpio_pin_remap1_config(GPIO_PCF2, GPIO_PCF2_DCI_D0_PC6_REMAP | 
                           GPIO_PCF2_DCI_D1_PC7_REMAP , ENABLE);  
	
    /* configure DCI_PIXCLK(PA6), DCI_VSYNC(PB7), DCI_HSYNC(PA4), 
       DCI_D0(PC6), DCI_D1(PC7), DCI_D2(PC8), DCI_D3(PC9), DCI_D4(PC11), DCI_D5(PB6), DCI_D6(PB8), DCI_D7(PB9) */
    gpio_init(GPIOA, GPIO_MODE_IN_FLOATING, GPIO_OSPEED_50MHZ, GPIO_PIN_6);
    
    gpio_init(GPIOB, GPIO_MODE_IN_FLOATING, GPIO_OSPEED_50MHZ, GPIO_PIN_6 | GPIO_PIN_8 | GPIO_PIN_9);
    
    gpio_init(GPIOC, GPIO_MODE_IN_FLOATING, GPIO_OSPEED_50MHZ, GPIO_PIN_6 | GPIO_PIN_7 |GPIO_PIN_8 | 
                                                                GPIO_PIN_9 | GPIO_PIN_11);
    
    gpio_init(GPIOB, GPIO_MODE_IN_FLOATING, GPIO_OSPEED_50MHZ, GPIO_PIN_7);
    
    gpio_init(GPIOA, GPIO_MODE_IN_FLOATING, GPIO_OSPEED_50MHZ, GPIO_PIN_4);
    
    /* DCI configuration */ 
    dci_struct.capture_mode = DCI_CAPTURE_MODE_CONTINUOUS;
    dci_struct.clock_polarity = DCI_CK_POLARITY_FALLING;
    dci_struct.hsync_polarity = DCI_HSYNC_POLARITY_LOW;
    dci_struct.vsync_polarity = DCI_VSYNC_POLARITY_HIGH;
    dci_struct.frame_rate = DCI_FRAME_RATE_1_4;//DCI_FRAME_RATE_ALL;
    dci_struct.interface_format = DCI_INTERFACE_FORMAT_8BITS;
    dci_init(&dci_struct);

    /* DCI DMA configuration */ 
    rcu_periph_clock_enable(RCU_DMA1);
    dma_deinit(DMA1, DMA_CH5);
    dma_struct.periph_addr = (uint32_t)DCI_DR_ADDRESS;
    dma_struct.memory_addr = (uint32_t)capture_image;
    dma_struct.direction = DMA_PERIPHERAL_TO_MEMORY;
    dma_struct.number = 38400;
    dma_struct.periph_inc = DMA_PERIPH_INCREASE_DISABLE;
    dma_struct.memory_inc = DMA_MEMORY_INCREASE_ENABLE;
    dma_struct.periph_width = DMA_PERIPHERAL_WIDTH_32BIT;
    dma_struct.memory_width = DMA_MEMORY_WIDTH_32BIT;
    dma_struct.priority = DMA_PRIORITY_HIGH;
    dma_init(DMA1, DMA_CH5, dma_struct);
    dma_circulation_enable(DMA1, DMA_CH5);
}

/*!
    \brief      set DCI camera outsize 
    \param[in]  width: outsize width
    \param[in]  height: outsize height
    \param[out] none
    \retval     0x00 or 0xFF
*/
uint8_t ov2640_outsize_set(uint16_t width,uint16_t height){
    uint16_t outh;
    uint16_t outw;
    uint8_t temp; 
    if(width%4)return 0xFF;
    if(height%4)return 0xFF;
    outw=width/4;
    outh=height/4; 
	
	  dci_byte_write(0x46,0x80);
    dci_byte_write(0x49,0x01);
	  dci_byte_write(0x4a,outh&0xFF);
	
    dci_byte_write(0x4b,0x00);
    dci_byte_write(0x4c,outw&0xFF);
    
    temp=(outw>>8)&0x00;
    temp|=(outh>>6)&0x01;
    dci_byte_write(0x5C,temp);
    dci_byte_write(0xE0,0x00);
    return 0;
}

/*!
    \brief      DCI camera initialization
    \param[in]  none
    \param[out] none
    \retval     0x00 or 0xFF
*/
uint8_t dci_ov2640_init(void){
    sccb_config();
    dci_config();
    DCMI_0V7670_RST_PWDN_Init();
    ckout0_init();
		GC0308InitialSetting();
		
		
    return 0;
}

/*!
    \brief      ckout0 initialization
    \param[in]  none
    \param[out] none
    \retval     none
*/
void ckout0_init(void){
    rcu_periph_clock_enable(RCU_GPIOA);
    rcu_periph_clock_enable(RCU_AF);
    gpio_init(GPIOA, GPIO_MODE_AF_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_8);
    
    rcu_ckout0_config( RCU_CKOUT0SRC_CKSYS, RCU_CKOUT0_DIV5);
}


/*!
    \brief      read the ov2640 manufacturer identifier
    \param[in]  ov2640id: pointer to the ov2640 manufacturer struct
    \param[out] none
    \retval     0x00 or 0xFF
*/
uint8_t dci_ov2640_id_read(ov2640_id_struct* ov2640id){
    uint8_t temp;
    if(0 != dci_byte_read(OV2640_MIDH,&temp))
        return 0xFF;
    ov2640id->manufacturer_id = temp;
    return ov2640id->manufacturer_id;
}

void DCMI_0V7670_RST_PWDN_Init(void){
	/* Enable GPIOs clocks */
	rcu_periph_clock_enable(RCU_GPIOC);	
	gpio_init(GPIOC, GPIO_MODE_OUT_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_0| GPIO_PIN_1);
	

	/*PWDN*/
	gpio_bit_reset(GPIOC,GPIO_PIN_1);
	/*Reset*/
	gpio_bit_set(GPIOC,GPIO_PIN_0);
}






static void GC0308InitialSetting(void) {
	dci_byte_write(0xfe , 0x80);   	
		
	GC0308_SET_PAGE0;       // set page0

	dci_byte_write(0xd2 , 0x10);   // close AEC
	dci_byte_write(0x22 , 0x55);   // close AWB

	dci_byte_write(0x5a , 0x56); 
	dci_byte_write(0x5b , 0x40);
	dci_byte_write(0x5c , 0x4a);			

	dci_byte_write(0x22 , 0x57);  // Open AWB
				
	dci_byte_write(0x01 , 0xfa); 
	dci_byte_write(0x02 , 0x70); 
	dci_byte_write(0x0f , 0x01); 

	dci_byte_write(0x03 , 0x01); 
	dci_byte_write(0x04 , 0x2c); 

	dci_byte_write(0xe2 , 0x00); 	//anti-flicker step [11:8]
	dci_byte_write(0xe3 , 0x64);   //anti-flicker step [7:0]
		
	dci_byte_write(0xe4 , 0x02);   //exp level 0  16.67fps
	dci_byte_write(0xe5 , 0x58); 
	dci_byte_write(0xe6 , 0x03);   //exp level 1  12.5fps
	dci_byte_write(0xe7 , 0x20); 
	dci_byte_write(0xe8 , 0x04);   //exp level 2  8.33fps
	dci_byte_write(0xe9 , 0xb0); 
	dci_byte_write(0xea , 0x09);   //exp level 3  4.00fps
	dci_byte_write(0xeb , 0xc4); 

	dci_byte_write(0x05 , 0x00); //640 * 480 //row start
	dci_byte_write(0x06 , 0x00); //row start
	dci_byte_write(0x07 , 0x00); //col start
	dci_byte_write(0x08 , 0x00); //col start 
	dci_byte_write(0x09 , 0x01); //height 
	dci_byte_write(0x0a , 0xe8); //height
	dci_byte_write(0x0b , 0x02); //width
	dci_byte_write(0x0c , 0x88); //width
//	dci_byte_write(0x05 , 0x00);
//	dci_byte_write(0x06 , 0x78);
//	dci_byte_write(0x07 , 0x00);
//	dci_byte_write(0x08 , 0xa0);
//	dci_byte_write(0x09 , 0x01);
//	dci_byte_write(0x0a , 0x48);
//	dci_byte_write(0x0b , 0x00);
//	dci_byte_write(0x0c , 0xf8);
	
	dci_byte_write(0x0d , 0x02);
	dci_byte_write(0x0e , 0x02);
	dci_byte_write(0x10 , 0x22);
	dci_byte_write(0x11 , 0xfd);
	dci_byte_write(0x12 , 0x2a);
	dci_byte_write(0x13 , 0x00);
	dci_byte_write(0x14 , 0x10);
	dci_byte_write(0x15 , 0x0a);
	dci_byte_write(0x16 , 0x05);
	dci_byte_write(0x17 , 0x01);
	dci_byte_write(0x18 , 0x44);
	dci_byte_write(0x19 , 0x44);
	dci_byte_write(0x1a , 0x1e);
	dci_byte_write(0x1b , 0x00);
	dci_byte_write(0x1c , 0xc1);
	dci_byte_write(0x1d , 0x08);
	dci_byte_write(0x1e , 0x60);
	dci_byte_write(0x1f , 0x16);

	
	dci_byte_write(0x20 , 0xff);
	dci_byte_write(0x21 , 0xf8);
	dci_byte_write(0x22 , 0x57);
	dci_byte_write(0x24 , 0xa2);   //output format  
	dci_byte_write(0x26 , 0x02);
	dci_byte_write(0x2f , 0x01);
	dci_byte_write(0x30 , 0xf7);
	dci_byte_write(0x31 , 0x50);
	dci_byte_write(0x32 , 0x00);
	dci_byte_write(0x39 , 0x04);
	dci_byte_write(0x3a , 0x18);
	dci_byte_write(0x3b , 0x20);
	dci_byte_write(0x3c , 0x00);
	dci_byte_write(0x3d , 0x00);
	dci_byte_write(0x3e , 0x00);
	dci_byte_write(0x3f , 0x00);
	dci_byte_write(0x50 , 0x10);
	dci_byte_write(0x53 , 0x82);
	dci_byte_write(0x54 , 0x80);
	dci_byte_write(0x55 , 0x80);
	dci_byte_write(0x56 , 0x82);
	dci_byte_write(0x8b , 0x40);
	dci_byte_write(0x8c , 0x40);
	dci_byte_write(0x8d , 0x40);
	dci_byte_write(0x8e , 0x2e);
	dci_byte_write(0x8f , 0x2e);
	dci_byte_write(0x90 , 0x2e);
	dci_byte_write(0x91 , 0x3c);
	dci_byte_write(0x92 , 0x50);
	dci_byte_write(0x5d , 0x12);
	dci_byte_write(0x5e , 0x1a);
	dci_byte_write(0x5f , 0x24);
	dci_byte_write(0x60 , 0x07);
	dci_byte_write(0x61 , 0x15);
	dci_byte_write(0x62 , 0x08);
	dci_byte_write(0x64 , 0x03);
	dci_byte_write(0x66 , 0xe8);
	dci_byte_write(0x67 , 0x86);
	dci_byte_write(0x68 , 0xa2);
	dci_byte_write(0x69 , 0x18);
	dci_byte_write(0x6a , 0x0f);
	dci_byte_write(0x6b , 0x00);
	dci_byte_write(0x6c , 0x5f);
	dci_byte_write(0x6d , 0x8f);
	dci_byte_write(0x6e , 0x55);
	dci_byte_write(0x6f , 0x38);
	dci_byte_write(0x70 , 0x15);
	dci_byte_write(0x71 , 0x33);
	dci_byte_write(0x72 , 0xdc);
	dci_byte_write(0x73 , 0x80);
	dci_byte_write(0x74 , 0x02);
	dci_byte_write(0x75 , 0x3f);
	dci_byte_write(0x76 , 0x02);
	dci_byte_write(0x77 , 0x36);
	dci_byte_write(0x78 , 0x88);
	dci_byte_write(0x79 , 0x81);
	dci_byte_write(0x7a , 0x81);
	dci_byte_write(0x7b , 0x22);
	dci_byte_write(0x7c , 0xff);
	dci_byte_write(0x93 , 0x48);
	dci_byte_write(0x94 , 0x00);
	dci_byte_write(0x95 , 0x05);
	dci_byte_write(0x96 , 0xe8);
	dci_byte_write(0x97 , 0x40);
	dci_byte_write(0x98 , 0xf0);
	dci_byte_write(0xb1 , 0x38);
	dci_byte_write(0xb2 , 0x38);
	dci_byte_write(0xbd , 0x38);
	dci_byte_write(0xbe , 0x36);
	dci_byte_write(0xd0 , 0xc9);
	dci_byte_write(0xd1 , 0x10);
	dci_byte_write(0xd2 , 0x90);
	dci_byte_write(0xd3 , 0x80);
	dci_byte_write(0xd5 , 0xf2);
	dci_byte_write(0xd6 , 0x16);
	dci_byte_write(0xdb , 0x92);
	dci_byte_write(0xdc , 0xa5);
	dci_byte_write(0xdf , 0x23);
	dci_byte_write(0xd9 , 0x00);
	dci_byte_write(0xda , 0x00);
	dci_byte_write(0xe0 , 0x09);
	dci_byte_write(0xec , 0x20);
	dci_byte_write(0xed , 0x04);
	dci_byte_write(0xee , 0x40);
	dci_byte_write(0xef , 0x40);
	dci_byte_write(0x80 , 0x03);
	dci_byte_write(0x80 , 0x03);
	dci_byte_write(0x9F , 0x10);
	dci_byte_write(0xA0 , 0x20);
	dci_byte_write(0xA1 , 0x38);
	dci_byte_write(0xA2 , 0x4E);
	dci_byte_write(0xA3 , 0x63);
	dci_byte_write(0xA4 , 0x76);
	dci_byte_write(0xA5 , 0x87);
	dci_byte_write(0xA6 , 0xA2);
	dci_byte_write(0xA7 , 0xB8);
	dci_byte_write(0xA8 , 0xCA);
	dci_byte_write(0xA9 , 0xD8);
	dci_byte_write(0xAA , 0xE3);
	dci_byte_write(0xAB , 0xEB);
	dci_byte_write(0xAC , 0xF0);
	dci_byte_write(0xAD , 0xF8);
	dci_byte_write(0xAE , 0xFD);
	dci_byte_write(0xAF , 0xFF);
	dci_byte_write(0xc0 , 0x00);
	dci_byte_write(0xc1 , 0x10);
	dci_byte_write(0xc2 , 0x1C);
	dci_byte_write(0xc3 , 0x30);
	dci_byte_write(0xc4 , 0x43);
	dci_byte_write(0xc5 , 0x54);
	dci_byte_write(0xc6 , 0x65);
	dci_byte_write(0xc7 , 0x75);
	dci_byte_write(0xc8 , 0x93);
	dci_byte_write(0xc9 , 0xB0);
	dci_byte_write(0xca , 0xCB);
	dci_byte_write(0xcb , 0xE6);
	dci_byte_write(0xcc , 0xFF);
	dci_byte_write(0xf0 , 0x02);
	dci_byte_write(0xf1 , 0x01);
	dci_byte_write(0xf2 , 0x01);
	dci_byte_write(0xf3 , 0x30);
	dci_byte_write(0xf9 , 0x9f);
	dci_byte_write(0xfa , 0x78);

	//---------------------------------------------------------------
	GC0308_SET_PAGE1;

	dci_byte_write(0x00 , 0xf5);
	dci_byte_write(0x02 , 0x1a);
	dci_byte_write(0x0a , 0xa0);
	dci_byte_write(0x0b , 0x60);
	dci_byte_write(0x0c , 0x08);
	dci_byte_write(0x0e , 0x4c);
	dci_byte_write(0x0f , 0x39);
	dci_byte_write(0x11 , 0x3f);
	dci_byte_write(0x12 , 0x72);
	dci_byte_write(0x13 , 0x13);
	dci_byte_write(0x14 , 0x42);
	dci_byte_write(0x15 , 0x43);
	dci_byte_write(0x16 , 0xc2);
	dci_byte_write(0x17 , 0xa8);
	dci_byte_write(0x18 , 0x18);
	dci_byte_write(0x19 , 0x40);
	dci_byte_write(0x1a , 0xd0);
	dci_byte_write(0x1b , 0xf5);
	dci_byte_write(0x70 , 0x40);
	dci_byte_write(0x71 , 0x58);
	dci_byte_write(0x72 , 0x30);
	dci_byte_write(0x73 , 0x48);
	dci_byte_write(0x74 , 0x20);
	dci_byte_write(0x75 , 0x60);
	dci_byte_write(0x77 , 0x20);
	dci_byte_write(0x78 , 0x32);
	dci_byte_write(0x30 , 0x03);
	dci_byte_write(0x31 , 0x40);
	dci_byte_write(0x32 , 0xe0);
	dci_byte_write(0x33 , 0xe0);
	dci_byte_write(0x34 , 0xe0);
	dci_byte_write(0x35 , 0xb0);
	dci_byte_write(0x36 , 0xc0);
	dci_byte_write(0x37 , 0xc0);
	dci_byte_write(0x38 , 0x04);
	dci_byte_write(0x39 , 0x09);
	dci_byte_write(0x3a , 0x12);
	dci_byte_write(0x3b , 0x1C);
	dci_byte_write(0x3c , 0x28);
	dci_byte_write(0x3d , 0x31);
	dci_byte_write(0x3e , 0x44);
	dci_byte_write(0x3f , 0x57);
	dci_byte_write(0x40 , 0x6C);
	dci_byte_write(0x41 , 0x81);
	dci_byte_write(0x42 , 0x94);
	dci_byte_write(0x43 , 0xA7);
	dci_byte_write(0x44 , 0xB8);
	dci_byte_write(0x45 , 0xD6);
	dci_byte_write(0x46 , 0xEE);
	dci_byte_write(0x47 , 0x0d); 
	
	dci_byte_write(0x53 , 0x83); //subsample settings
	dci_byte_write(0x54 , 0x22);
	dci_byte_write(0x55 , 0x03);
	dci_byte_write(0x56 , 0x00);
	dci_byte_write(0x57 , 0x00);
	dci_byte_write(0x58 , 0x00);
	dci_byte_write(0x59 , 0x00);
	
	
	GC0308_SET_PAGE0;
  dci_byte_write(0xd2 , 0x90);  // Open AEC at last.	
	dci_byte_write(0x25 , 0x00);
}
