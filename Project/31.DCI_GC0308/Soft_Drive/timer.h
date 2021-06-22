#ifndef _TIMER_H
#define _TIMER_H
#include "gd32f20x.h"
#include "ColorAnalysis.h"





void nvic_config(void);
void pwm_timer1_config(void);
void pwm_timer2_config(void);
void pwm_timer4_config(void);
void pwm_timer_init(void);
void pwm_timer_config(uint32_t WMAX_PA5,uint32_t CMAX_PB0,uint32_t R_PA0,uint32_t G_PA1,
	                  uint32_t B_PA2,uint32_t A_PA3);

void pwm_gpio_config(void);
void timer5_init(uint32_t psr, uint32_t arr);
#endif

