/*!
    \file  main.c
    \brief DCI display demo
*/

/*
    Copyright (C) 2017 GigaDevice

    2015-07-15, V1.0.0, demo for GD32F20x
    2017-06-05, V2.0.0, demo for GD32F20x
*/

#include "gd32f20x.h"
#include "systick.h"
#include <stdio.h>
#include "dci_ov2640.h"
#include "picture.h"
#include "gd32f207i_eval.h"
#include "gd32f207i_sdram_eval.h"
#include "Lcd_Driver.h"
#include "GUI.h"
#include "picture.h"
#include <string.h>
#include "ColorAnalysis.h"
#include "timer.h"
#include "LedControl.h"



//#define IMG_START_X 100	
//#define IMG_START_Y 20 
//#define IMG_START_W 100 
//#define IMG_START_H 100
#define IMG_START_X 0	
#define IMG_START_Y 0 
#define IMG_START_W 240 
#define IMG_START_H 320

IMAGE_AREA Area={IMG_START_X, IMG_START_X + IMG_START_W - 1, IMG_START_Y, IMG_START_Y + IMG_START_H - 1};
uint8_t capture_image[320 * 240 * 2];
uint32_t frame_count = 0;
extern int actual_frames;

tli_parameter_struct               tli_init_struct;
tli_layer_parameter_struct         tli_layer_initstruct;
uint8_t Start_camera_flag = 0;

static void nvic_config(void);
static void key_config(void);

void image_display(uint32_t dispaly_image_addr);
void image_save(void);
void COBledPwmInit(void);
int Picture_SearchRegion(unsigned char *image,const IMAGE_AREA *Area);





/*!
    \brief      main function
    \param[in]  none
    \param[out] none
    \retval     none
*/
int main(void) {   
	  ov2640_id_struct ov2640id;  
    systick_config();
		nvic_config();
		
    gd_eval_com_init(EVAL_COM1); 

    Lcd_Init();	
		LCD_LED_SET;                                            
		Lcd_Clear(WHITE);                                       
		Gui_DrawFont_GBK24(0,0,BLACK,WHITE,"OV7670 TEST!");
		Gui_DrawFont_GBK24(0,30,BLACK,WHITE,"OV7670 initing");
		timer5_init(1200-1, 5000000-1);

    gd_eval_led_init(LED2);	
		COBledPwmInit();
		
    if(0 == dci_ov2640_init())
			Gui_DrawFont_GBK24(0,60,BLACK,WHITE,"Camera Have Init..");
		else
			Gui_DrawFont_GBK24(0,90,BLACK,WHITE,"OV7670 Init fails!!.");
    if(0x9B == dci_ov2640_id_read(&ov2640id))
		  Gui_DrawFont_GBK24(0,60,BLACK,WHITE,"Camera Init OK..");  
		
		//open camera
		dma_channel_enable(DMA1, DMA_CH5);
    dci_interrupt_enable(DCI_INT_EF);	
    dci_enable();
		
		//COBLed_minimum_cold_white();

    while(1){			
			if (Start_camera_flag == 1) {
				dci_byte_write(0x25 , 0x0f);
				dci_capture_enable();
				Start_camera_flag = 0;
			}
			process_camera_exception();
			state_machine_ledcontrol();
			gd_eval_led_on(LED2);
			delay_1ms(200);
			gd_eval_led_off(LED2);
			delay_1ms(100);
    }
}



/*!
    \brief      display image to LCD
    \param[in]  dispaly_image_addr: address of the display image
    \param[out] none
    \retval     none
*/
void image_display(uint32_t dispaly_image_addr){  
    /* input address configuration */    
    tli_layer_initstruct.layer_frame_bufaddr =(uint32_t)dispaly_image_addr;
      
    tli_layer_init(LAYER1, &tli_layer_initstruct);
    /* enable layer 0 */
    tli_layer_enable(LAYER0);
    /* enable layer 1 */
    tli_layer_enable(LAYER1);
    /* reload configuration */
    tli_reload_config(TLI_REQUEST_RELOAD_EN);

    /* enable TLI */
    tli_enable();
}

/*!
    \brief      display image to LCD
    \param[in]  none
    \param[out] none
    \retval     none
*/
void image_save(){
    uint32_t i = 0;
    
    dma_channel_disable(DMA1, DMA_CH5);
    dci_capture_disable();
    
    /* save image to sdram */
    for(i=0;i<32640;i++){
        *(uint32_t *)(0xC0080000+4*i) = *(uint32_t *)(0xC0040000+4*i);
    }
}

/*!
    \brief      configure the nested vectored interrupt controller
    \param[in]  none
    \param[out] none
    \retval     none
*/
static void nvic_config(void){
    nvic_priority_group_set(NVIC_PRIGROUP_PRE1_SUB3);
    /* enable DCI interrupt */
    nvic_irq_enable(DCI_IRQn, 0U, 2U);
	  nvic_irq_enable(TIMER5_IRQn, 1, 1); 
}

/*!
    \brief      configure the key interrupt
    \param[in]  none
    \param[out] none
    \retval     none
*/
static void key_config(void) {
    /* configure Wakeup, Tamper and User key interrupt */
    /* enable the key clock */
    rcu_periph_clock_enable(WAKEUP_KEY_GPIO_CLK);
    rcu_periph_clock_enable(TAMPER_KEY_GPIO_CLK);
    rcu_periph_clock_enable(USER_KEY_GPIO_CLK);
    rcu_periph_clock_enable(RCU_AF);
    
    /* configure button pin as input */
    gpio_init(WAKEUP_KEY_GPIO_PORT, GPIO_MODE_IN_FLOATING, GPIO_OSPEED_50MHZ, WAKEUP_KEY_PIN);
    gpio_init(TAMPER_KEY_GPIO_PORT, GPIO_MODE_IN_FLOATING, GPIO_OSPEED_50MHZ, TAMPER_KEY_PIN);
    gpio_init(USER_KEY_GPIO_PORT, GPIO_MODE_IN_FLOATING, GPIO_OSPEED_50MHZ, USER_KEY_PIN);
    
    nvic_irq_enable(WAKEUP_KEY_EXTI_IRQn, 3, 4);
    nvic_irq_enable(TAMPER_KEY_EXTI_IRQn, 3, 5);
    nvic_irq_enable(USER_KEY_EXTI_IRQn, 3, 6);
    
    /* connect key EXTI line to key GPIO pin */
    gpio_exti_source_select(WAKEUP_KEY_EXTI_PORT_SOURCE, WAKEUP_KEY_EXTI_PIN_SOURCE);
    gpio_exti_source_select(TAMPER_KEY_EXTI_PORT_SOURCE, TAMPER_KEY_EXTI_PIN_SOURCE);
    gpio_exti_source_select(USER_KEY_EXTI_PORT_SOURCE, USER_KEY_EXTI_PIN_SOURCE);

    /* configure key EXTI line */
    exti_init(WAKEUP_KEY_EXTI_LINE, EXTI_INTERRUPT, EXTI_TRIG_FALLING);
    exti_init(TAMPER_KEY_EXTI_LINE, EXTI_INTERRUPT, EXTI_TRIG_FALLING);
    exti_init(USER_KEY_EXTI_LINE, EXTI_INTERRUPT, EXTI_TRIG_FALLING);
    exti_interrupt_flag_clear(WAKEUP_KEY_EXTI_LINE);
    exti_interrupt_flag_clear(TAMPER_KEY_EXTI_LINE);
    exti_interrupt_flag_clear(USER_KEY_EXTI_LINE);
}


/* retarget the C library printf function to the USART */
int fputc(int ch, FILE *f){
    usart_data_transmit(EVAL_COM1, (uint8_t)ch);
    while(RESET == usart_flag_get(EVAL_COM1, USART_FLAG_TBE));
    return ch;
}

	


void DCI_IRQHandler(void) {
	  IMAGE_AREA clothing_area = {120, 199, 80, 159};
		Spectrum_Index index;
    if(RESET != dci_interrupt_flag_get(DCI_INT_EF)){
			dci_byte_write(0x25 , 0x00);
			actual_frames++;
			
			
			//write_image_data_to_memory();
			show_yuv422_image();
			//index = get_spectrum_index_of_image_3(&clothing_area);
			analyse_image_and_change_light();
    }
		dci_interrupt_flag_clear(DCI_INT_EF);
		dma_channel_enable(DMA1, DMA_CH5);	
}
void COBledPwmInit(void){
	  pwm_gpio_config();                                
		pwm_timer_init();     
}
