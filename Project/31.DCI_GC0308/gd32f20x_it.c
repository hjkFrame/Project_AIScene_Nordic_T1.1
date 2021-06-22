/*!
    \file  gd32f20x_it.c
    \brief interrupt service routines
*/

/*
    Copyright (C) 2017 GigaDevice

    2015-07-15, V1.0.0, demo for GD32F20x
    2017-06-05, V2.0.0, demo for GD32F20x
*/

#include "gd32f20x_it.h"
#include "systick.h"
#include "dci_ov2640.h"
#include "picture.h"
#include "gd32f207i_eval.h"
#include "GUI.h"
#include <string.h>
#include "ColorAnalysis.h"
#include "LedControl.h"



extern uint8_t capture_image[];
extern void lcd_config(void);
extern void image_save(void);
extern void image_display(uint32_t dispaly_image_addr);
extern uint8_t Start_camera_flag;
extern int clock_frames, actual_frames;
extern State_Camera state_camera;

/*!
    \brief      this function handles NMI exception
    \param[in]  none
    \param[out] none
    \retval     none
*/
void NMI_Handler(void)
{
}

/*!
    \brief      this function handles HardFault exception
    \param[in]  none
    \param[out] none
    \retval     none
*/
void HardFault_Handler(void)
{
    /* if Hard Fault exception occurs, go to infinite loop */
    while(1){
    }
}

/*!
    \brief      this function handles MemManage exception
    \param[in]  none
    \param[out] none
    \retval     none
*/
void MemManage_Handler(void)
{
    /* if Memory Manage exception occurs, go to infinite loop */
    while(1){
    }
}

/*!
    \brief      this function handles BusFault exception
    \param[in]  none
    \param[out] none
    \retval     none
*/
void BusFault_Handler(void)
{
    /* if Bus Fault exception occurs, go to infinite loop */
    while(1){
    }
}

/*!
    \brief      this function handles UsageFault exception
    \param[in]  none
    \param[out] none
    \retval     none
*/
void UsageFault_Handler(void)
{
    /* if Usage Fault exception occurs, go to infinite loop */
    while(1){
    }
}

/*!
    \brief      this function handles SVC exception
    \param[in]  none
    \param[out] none
    \retval     none
*/
void SVC_Handler(void)
{
}

/*!
    \brief      this function handles DebugMon exception
    \param[in]  none
    \param[out] none
    \retval     none
*/
void DebugMon_Handler(void)
{
}

/*!
    \brief      this function handles PendSV exception
    \param[in]  none
    \param[out] none
    \retval     none
*/
void PendSV_Handler(void)
{
}

/*!
    \brief      this function handles SysTick exception
    \param[in]  none
    \param[out] none
    \retval     none
*/
void SysTick_Handler(void)
{
    delay_decrement();
}

/*!
    \brief      this function handles DCI interrupt request
    \param[in]  none
    \param[out] none
    \retval     none
*/

/*!
    \brief      this function handles EXTI line0 interrupt request
    \param[in]  none
    \param[out] none
    \retval     none
*/
void EXTI0_IRQHandler(void)
{
    /* press the "Wakeup" key, enter the interrupt */

}

/*!
    \brief      this function handles EXTI line10 to EXTI line15 interrupt request
    \param[in]  none
    \param[out] none
    \retval     none
*/
void EXTI10_15_IRQHandler(void)
{
    if(RESET != exti_interrupt_flag_get(USER_KEY_EXTI_LINE)){
        /* press the "User" key, enter the interrupt, and save photo */
        exti_interrupt_flag_clear(USER_KEY_EXTI_LINE);
        tli_layer_disable(LAYER0);
        tli_layer_disable(LAYER1);
        tli_reload_config(TLI_REQUEST_RELOAD_EN);
        tli_enable();
        
        /* save and display the photo */
        image_save();
        //image_display((uint32_t)image_background1);
    }else if(RESET != exti_interrupt_flag_get(TAMPER_KEY_EXTI_LINE)){
        /* press the "Tamper" key, enter the interrupt, and display photo */
        exti_interrupt_flag_clear(TAMPER_KEY_EXTI_LINE);
        image_display(0xC0080000);
        /* disable "User" key */
        exti_interrupt_disable(USER_KEY_EXTI_LINE);
    }
}


void TIMER5_IRQHandler(void){
	if(timer_interrupt_flag_get(TIMER5, TIMER_INT_FLAG_UP))	{
		Start_camera_flag = 1;
		clock_frames++;
		timer_interrupt_flag_clear(TIMER5, TIMER_INT_FLAG_UP);
	}
}
