/*!
    \file  gd32f20x_it.h
    \brief the header file of the ISR
*/

/*
    Copyright (C) 2017 GigaDevice

    2015-07-15, V1.0.0, demo for GD32F20x
    2017-06-05, V2.0.0, demo for GD32F20x
*/

#ifndef GD32F20X_IT_H
#define GD32F20X_IT_H

#include "gd32f20x.h"

/* function declarations */
/* this function handles NMI exception */
void NMI_Handler(void);
/* this function handles HardFault exception */
void HardFault_Handler(void);
/* this function handles MemManage exception */
void MemManage_Handler(void);
/* this function handles BusFault exception */
void BusFault_Handler(void);
/* this function handles UsageFault exception */
void UsageFault_Handler(void);
/* this function handles SVC exception */
void SVC_Handler(void);
/* this function handles DebugMon exception */
void DebugMon_Handler(void);
/* this function handles PendSV exception */
void PendSV_Handler(void);
/* this function handles SysTick exception */
void SysTick_Handler(void);
/* this function handles DCI interrupt request */
void DCI_IRQHandler(void);
/* this function handles EXTI line0 interrupt request */
void EXTI0_IRQHandler(void);
/* this function handles EXTI line10 to EXTI line15 interrupt request */
void EXTI10_15_IRQHandler(void);

void TIMER5_IRQHandler(void);
#endif /* GD32F20X_IT_H */
