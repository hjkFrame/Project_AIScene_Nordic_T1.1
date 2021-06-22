#include "gd32f20x.h"
#include "timer.h"
#include "systick.h"

/*!
    \brief      configure the nested vectored interrupt controller
    \param[in]  none
    \param[out] none
    \retval     none
*/
void nvic_config(void)
{
    nvic_priority_group_set(NVIC_PRIGROUP_PRE1_SUB3);

//	  nvic_irq_enable(DMA0_Channel1_IRQn, 0U, 3U);
//		
//    nvic_irq_enable(TIMER5_IRQn, 1, 3); 
    
	  nvic_irq_enable(USART0_IRQn, 1, 2); 
    
}





/* 基本定时器5初始化函数
 * 参数：  psr:时钟预分频系数
           arr:自动重装载值
 * 返回值：无	*/
void timer5_init(uint32_t psr, uint32_t arr)
{
	/* 定义一个定时器初始化结构体 */
	timer_parameter_struct timer_init_struct;
	
	/* 开启定时器时钟 */
	rcu_periph_clock_enable(RCU_TIMER5);
	
	/* 初始化定时器 */
	timer_deinit(TIMER5);
	timer_init_struct.prescaler			= psr;	                /* 预分频系数 */
	timer_init_struct.period			= arr;	                  /* 自动重装载值 */
	timer_init_struct.alignedmode		= TIMER_COUNTER_EDGE;	  /* 计数器对齐模式，边沿对齐（定时器5无效）*/
	timer_init_struct.counterdirection	= TIMER_COUNTER_UP;	/* 计数器计数方向，向上（定时器5无效）*/
	timer_init_struct.clockdivision		= TIMER_CKDIV_DIV1;		/* DTS时间分频值（定时器5无效） */
	timer_init_struct.repetitioncounter = 0;					      /* 重复计数器的值（定时器5无效）*/
	timer_init(TIMER5, &timer_init_struct);
	
	/* 使能Timer5更新中断 */
   timer_interrupt_enable(TIMER5, TIMER_INT_UP);
	
	/* 使能Timer5 */
	timer_enable(TIMER5);
}

void pwm_gpio_config(void)
{
    rcu_periph_clock_enable(RCU_GPIOA);
	  //rcu_periph_clock_enable(RCU_GPIOC);
	  rcu_periph_clock_enable(RCU_GPIOB);
    rcu_periph_clock_enable(RCU_AF);
    
    /* Configure PA2 (TIMER1 CH2) as alternate function */
		gpio_init(GPIOA, GPIO_MODE_AF_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_0);              //PWM-R-PA0
	  gpio_bit_reset(GPIOA, GPIO_PIN_0);
	
    gpio_init(GPIOA, GPIO_MODE_AF_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_1);              //PWM-G-PA1
	  gpio_bit_reset(GPIOA, GPIO_PIN_1);
	
	  gpio_init(GPIOA, GPIO_MODE_AF_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_2);              //PWM-B-PA2
	  gpio_bit_reset(GPIOA, GPIO_PIN_2);
	
    gpio_init(GPIOA, GPIO_MODE_AF_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_3);              //PWM-A-PA3
	  gpio_bit_reset(GPIOA, GPIO_PIN_3);  
//>> Warm
	  gpio_pin_remap1_config(GPIO_PCF5,GPIO_PCF5_TIMER1_CH0_REMAP,ENABLE);
	  gpio_init(GPIOA, GPIO_MODE_AF_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_5);              //PWM-Wmax-PA5
	  gpio_bit_reset(GPIOA, GPIO_PIN_5);
	
		gpio_init(GPIOA, GPIO_MODE_AF_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_7);              //PWM-Wmin-PA7
	  gpio_bit_reset(GPIOA, GPIO_PIN_7);
//>> Cool		
	  gpio_init(GPIOB, GPIO_MODE_AF_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_0);              //PWM-Cmax-PB0
	  gpio_bit_reset(GPIOB, GPIO_PIN_0);
		
	  gpio_init(GPIOB, GPIO_MODE_AF_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_1);              //PWM-Cmin-PB1
	  gpio_bit_reset(GPIOB, GPIO_PIN_1);
}


/**
    \brief      configure the TIMER1 peripheral
    \param[in]  none
    \param[out] none
    \retval     none
  */
void pwm_timer1_config(void)
{
    /* TIMER1 configuration: generate PWM signals with different duty cycles:
       TIMER1CLK = SystemCoreClock / 120 = 1MHz */
    timer_oc_parameter_struct timer_ocintpara;
    timer_parameter_struct timer_initpara;

    rcu_periph_clock_enable(RCU_TIMER1);

    timer_deinit(TIMER1);

    /* TIMER1 configuration */
    timer_initpara.prescaler         = 119;
    timer_initpara.alignedmode       = TIMER_COUNTER_EDGE;
    timer_initpara.counterdirection  = TIMER_COUNTER_UP;
    timer_initpara.period            = 1000;
    timer_initpara.clockdivision     = TIMER_CKDIV_DIV1;
    timer_initpara.repetitioncounter = 0;
    timer_init(TIMER1, &timer_initpara);

    /* CH2 configuration in PWM mode 0 */
    timer_ocintpara.ocpolarity  = TIMER_OC_POLARITY_HIGH;
    timer_ocintpara.outputstate = TIMER_CCX_ENABLE;
		
    timer_channel_output_config(TIMER1, TIMER_CH_0, &timer_ocintpara);
//		timer_channel_output_config(TIMER1, TIMER_CH_1, &timer_ocintpara);
//		timer_channel_output_config(TIMER1, TIMER_CH_2, &timer_ocintpara);
//		timer_channel_output_config(TIMER1, TIMER_CH_3, &timer_ocintpara);
		
		/* CH2 configuration in PWM mode 0,duty cycle 25% */
    timer_channel_output_pulse_value_config(TIMER1, TIMER_CH_0, 0);
    timer_channel_output_mode_config(TIMER1, TIMER_CH_0, TIMER_OC_MODE_PWM0);
    timer_channel_output_shadow_config(TIMER1, TIMER_CH_0, TIMER_OC_SHADOW_DISABLE);
		
//		/* CH2 configuration in PWM mode 0,duty cycle 25% */
//    timer_channel_output_pulse_value_config(TIMER1, TIMER_CH_1, 0);
//    timer_channel_output_mode_config(TIMER1, TIMER_CH_1, TIMER_OC_MODE_PWM0);
//    timer_channel_output_shadow_config(TIMER1, TIMER_CH_1, TIMER_OC_SHADOW_DISABLE);
//		
//    /* CH2 configuration in PWM mode 0,duty cycle 25% */
//    timer_channel_output_pulse_value_config(TIMER1, TIMER_CH_2, 0);
//    timer_channel_output_mode_config(TIMER1, TIMER_CH_2, TIMER_OC_MODE_PWM0);
//    timer_channel_output_shadow_config(TIMER1, TIMER_CH_2, TIMER_OC_SHADOW_DISABLE);
//		
//    /* CH2 configuration in PWM mode 0,duty cycle 25% */
//    timer_channel_output_pulse_value_config(TIMER1, TIMER_CH_3, 0);
//    timer_channel_output_mode_config(TIMER1, TIMER_CH_3, TIMER_OC_MODE_PWM0);
//    timer_channel_output_shadow_config(TIMER1, TIMER_CH_3, TIMER_OC_SHADOW_DISABLE);
		
    /* auto-reload preload enable */
    timer_auto_reload_shadow_enable(TIMER1);
    /* TIMER1 enable */
    timer_enable(TIMER1);
}

/**
    \brief      configure the TIMER1 peripheral
    \param[in]  none
    \param[out] none
    \retval     none
  */
void pwm_timer2_config(void)
{
    /* TIMER1 configuration: generate PWM signals with different duty cycles:
       TIMER1CLK = SystemCoreClock / 120 = 1MHz */
    timer_oc_parameter_struct timer_ocintpara;
    timer_parameter_struct timer_initpara;

    rcu_periph_clock_enable(RCU_TIMER2);

    timer_deinit(TIMER2);

    /* TIMER1 configuration */
    timer_initpara.prescaler         = 119;
    timer_initpara.alignedmode       = TIMER_COUNTER_EDGE;
    timer_initpara.counterdirection  = TIMER_COUNTER_UP;
    timer_initpara.period            = 1000;
    timer_initpara.clockdivision     = TIMER_CKDIV_DIV1;
    timer_initpara.repetitioncounter = 0;
    timer_init(TIMER2, &timer_initpara);

    /* CH2 configuration in PWM mode 0 */
    timer_ocintpara.ocpolarity  = TIMER_OC_POLARITY_HIGH;
    timer_ocintpara.outputstate = TIMER_CCX_ENABLE;
		
   // timer_channel_output_config(TIMER2, TIMER_CH_0, &timer_ocintpara);
		timer_channel_output_config(TIMER2, TIMER_CH_1, &timer_ocintpara);
		timer_channel_output_config(TIMER2, TIMER_CH_2, &timer_ocintpara);
		timer_channel_output_config(TIMER2, TIMER_CH_3, &timer_ocintpara);
		
//		/* CH2 configuration in PWM mode 0,duty cycle 25% */
//    timer_channel_output_pulse_value_config(TIMER2, TIMER_CH_0, 0);
//    timer_channel_output_mode_config(TIMER2, TIMER_CH_0, TIMER_OC_MODE_PWM0);
//    timer_channel_output_shadow_config(TIMER2, TIMER_CH_0, TIMER_OC_SHADOW_DISABLE);
		
		/* CH2 configuration in PWM mode 0,duty cycle 25% */
    timer_channel_output_pulse_value_config(TIMER2, TIMER_CH_1, 0);
    timer_channel_output_mode_config(TIMER2, TIMER_CH_1, TIMER_OC_MODE_PWM0);
    timer_channel_output_shadow_config(TIMER2, TIMER_CH_1, TIMER_OC_SHADOW_DISABLE);
		
    /* CH2 configuration in PWM mode 0,duty cycle 25% */
    timer_channel_output_pulse_value_config(TIMER2, TIMER_CH_2, 0);
    timer_channel_output_mode_config(TIMER2, TIMER_CH_2, TIMER_OC_MODE_PWM0);
    timer_channel_output_shadow_config(TIMER2, TIMER_CH_2, TIMER_OC_SHADOW_DISABLE);
		
    /* CH2 configuration in PWM mode 0,duty cycle 25% */
    timer_channel_output_pulse_value_config(TIMER2, TIMER_CH_3, 0);
    timer_channel_output_mode_config(TIMER2, TIMER_CH_3, TIMER_OC_MODE_PWM0);
    timer_channel_output_shadow_config(TIMER2, TIMER_CH_3, TIMER_OC_SHADOW_DISABLE);
		
    /* auto-reload preload enable */
    timer_auto_reload_shadow_enable(TIMER2);
    /* TIMER1 enable */
    timer_enable(TIMER2);
}

/**
    \brief      configure the TIMER1 peripheral
    \param[in]  none
    \param[out] none
    \retval     none
  */
void pwm_timer4_config(void)
{
    /* TIMER1 configuration: generate PWM signals with different duty cycles:
       TIMER1CLK = SystemCoreClock / 120 = 1MHz */
    timer_oc_parameter_struct timer_ocintpara;
    timer_parameter_struct timer_initpara;

    rcu_periph_clock_enable(RCU_TIMER4);

    timer_deinit(TIMER4);

    /* TIMER1 configuration */
    timer_initpara.prescaler         = 119;
    timer_initpara.alignedmode       = TIMER_COUNTER_EDGE;
    timer_initpara.counterdirection  = TIMER_COUNTER_UP;
    timer_initpara.period            = 1000;
    timer_initpara.clockdivision     = TIMER_CKDIV_DIV1;
    timer_initpara.repetitioncounter = 0;
    timer_init(TIMER4, &timer_initpara);

    /* CH2 configuration in PWM mode 0 */
    timer_ocintpara.ocpolarity  = TIMER_OC_POLARITY_HIGH;
    timer_ocintpara.outputstate = TIMER_CCX_ENABLE;
		
    timer_channel_output_config(TIMER4, TIMER_CH_0, &timer_ocintpara);
		timer_channel_output_config(TIMER4, TIMER_CH_1, &timer_ocintpara);
		timer_channel_output_config(TIMER4, TIMER_CH_2, &timer_ocintpara);
		timer_channel_output_config(TIMER4, TIMER_CH_3, &timer_ocintpara);
		
		/* CH2 configuration in PWM mode 0,duty cycle 25% */
    timer_channel_output_pulse_value_config(TIMER4, TIMER_CH_0, 0);
    timer_channel_output_mode_config(TIMER4, TIMER_CH_0, TIMER_OC_MODE_PWM0);
    timer_channel_output_shadow_config(TIMER4, TIMER_CH_0, TIMER_OC_SHADOW_DISABLE);
		
		/* CH2 configuration in PWM mode 0,duty cycle 25% */
    timer_channel_output_pulse_value_config(TIMER4, TIMER_CH_1, 0);
    timer_channel_output_mode_config(TIMER4, TIMER_CH_1, TIMER_OC_MODE_PWM0);
    timer_channel_output_shadow_config(TIMER4, TIMER_CH_1, TIMER_OC_SHADOW_DISABLE);
		
    /* CH2 configuration in PWM mode 0,duty cycle 25% */
    timer_channel_output_pulse_value_config(TIMER4, TIMER_CH_2, 0);
    timer_channel_output_mode_config(TIMER4, TIMER_CH_2, TIMER_OC_MODE_PWM0);
    timer_channel_output_shadow_config(TIMER4, TIMER_CH_2, TIMER_OC_SHADOW_DISABLE);
		
    /* CH2 configuration in PWM mode 0,duty cycle 25% */
    timer_channel_output_pulse_value_config(TIMER4, TIMER_CH_3, 0);
    timer_channel_output_mode_config(TIMER4, TIMER_CH_3, TIMER_OC_MODE_PWM0);
    timer_channel_output_shadow_config(TIMER4, TIMER_CH_3, TIMER_OC_SHADOW_DISABLE);
		
    /* auto-reload preload enable */
    timer_auto_reload_shadow_enable(TIMER4);
    /* TIMER1 enable */
    timer_enable(TIMER4);
}

void pwm_timer7_config(void)
{
	 /* TIMER2 configuration: generate PWM signals with different duty cycles:
       TIMER1CLK = SystemCoreClock / 120 = 1MHz */
    timer_oc_parameter_struct timer_ocintpara;
    timer_parameter_struct timer_initpara;

    rcu_periph_clock_enable(RCU_TIMER7);

    timer_deinit(TIMER7);
   //注意!当你实用timer0高级定时器的时候必须使能这个,才能有pwm输出
    timer_primary_output_config(TIMER7,ENABLE);
	
    /* TIMER1 configuration */
    timer_initpara.prescaler         = 119;
    timer_initpara.alignedmode       = TIMER_COUNTER_EDGE;
    timer_initpara.counterdirection  = TIMER_COUNTER_UP;
    timer_initpara.period            = 1000;
    timer_initpara.clockdivision     = TIMER_CKDIV_DIV1;
    timer_initpara.repetitioncounter = 0;
    timer_init(TIMER7, &timer_initpara);

    /* CH2 configuration in PWM mode 0 */
//    timer_ocintpara.ocpolarity  = TIMER_OC_POLARITY_HIGH;
//    timer_ocintpara.outputstate = TIMER_CCX_ENABLE;
		
		timer_ocintpara.outputstate  = TIMER_CCX_ENABLE;
		timer_ocintpara.outputnstate = TIMER_CCXN_DISABLE;
		timer_ocintpara.ocpolarity   = TIMER_OC_POLARITY_HIGH;
		timer_ocintpara.ocnpolarity  = TIMER_OCN_POLARITY_HIGH;
		timer_ocintpara.ocidlestate  = TIMER_OC_IDLE_STATE_LOW;
		timer_ocintpara.ocnidlestate = TIMER_OCN_IDLE_STATE_LOW;
				
    timer_channel_output_config(TIMER7, TIMER_CH_0, &timer_ocintpara);
		timer_channel_output_config(TIMER7, TIMER_CH_2, &timer_ocintpara);
		 
		/* CH2 configuration in PWM mode 0,duty cycle 25% */
    timer_channel_output_pulse_value_config(TIMER7, TIMER_CH_0, 0);
    timer_channel_output_mode_config(TIMER7, TIMER_CH_0, TIMER_OC_MODE_PWM0);
    timer_channel_output_shadow_config(TIMER7, TIMER_CH_0, TIMER_OC_SHADOW_DISABLE);
	
	 
	  timer_channel_output_pulse_value_config(TIMER7, TIMER_CH_2, 0);
    timer_channel_output_mode_config(TIMER7, TIMER_CH_2, TIMER_OC_MODE_PWM0);
    timer_channel_output_shadow_config(TIMER7, TIMER_CH_2, TIMER_OC_SHADOW_DISABLE);
		
		/* auto-reload preload enable */
    timer_auto_reload_shadow_enable(TIMER7);
	  //timer_primary_output_config(TIMER7, ENABLE);
    /* TIMER1 enable */
    timer_enable(TIMER7);
		
}



void pwm_timer_init(void)
{
	  /* configure TIMER4 channel output pulse value */
	  pwm_timer4_config();
	
	  //PWM-R-PA0
	  delay_1ms(100);
	  timer_channel_output_pulse_value_config(TIMER4, TIMER_CH_0, 0);
	
	  //PWM-G-PA1
	  delay_1ms(100);
	  timer_channel_output_pulse_value_config(TIMER4, TIMER_CH_1, 0);
	
	  //PWM-B-PA2
	  delay_1ms(100);
	  timer_channel_output_pulse_value_config(TIMER4, TIMER_CH_2, 0);

    //PWM-A-PA3
		delay_1ms(100);
		timer_channel_output_pulse_value_config(TIMER4, TIMER_CH_3, 0);
		
		/* configure TIMER1 channel output pulse value */
		pwm_timer1_config();
		//PWM-Wmax-PA5
		delay_1ms(100);
		timer_channel_output_pulse_value_config(TIMER1, TIMER_CH_0, 0);
		
	/* configure TIMER2 channel output pulse value */
		pwm_timer2_config();	
		
		//PWM6-Wmin-PA7
		delay_1ms(100);
		timer_channel_output_pulse_value_config(TIMER2, TIMER_CH_1, 0);
		//PWM6-Cmax-PB0
		delay_1ms(100);
		timer_channel_output_pulse_value_config(TIMER2, TIMER_CH_2, 0);
		//PWM6-Cmin-PB1
		delay_1ms(100);
		timer_channel_output_pulse_value_config(TIMER2, TIMER_CH_3, 0);

}


void pwm_timer_config(uint32_t WMAX_PA5,uint32_t CMAX_PB0,uint32_t R_PA0,uint32_t G_PA1,
	                  uint32_t B_PA2,uint32_t A_PA3)
{
	 	/* configure TIMER1 channel output pulse value */
		timer_channel_output_pulse_value_config(TIMER1, TIMER_CH_0, WMAX_PA5);
	
	  /* configure TIMER2 channel output pulse value */
	  //timer_channel_output_pulse_value_config(TIMER2, TIMER_CH_1, CMAX_PB0);
		timer_channel_output_pulse_value_config(TIMER2, TIMER_CH_2, CMAX_PB0);
	
		/* configure TIMER4 channel output pulse value */
		timer_channel_output_pulse_value_config(TIMER4, TIMER_CH_0, R_PA0);
		timer_channel_output_pulse_value_config(TIMER4, TIMER_CH_1, G_PA1);
		timer_channel_output_pulse_value_config(TIMER4, TIMER_CH_2, B_PA2);
		timer_channel_output_pulse_value_config(TIMER4, TIMER_CH_3, A_PA3);
}