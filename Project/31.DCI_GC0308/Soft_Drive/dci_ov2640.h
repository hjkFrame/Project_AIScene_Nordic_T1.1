/*!
    \file  dci_ov2640.h
    \brief definitions for ov2640 
*/

/*
    Copyright (C) 2017 GigaDevice

    2015-07-15, V1.0.0, demo for GD32F20x
    2017-06-05, V2.0.0, demo for GD32F20x
*/

#ifndef DCI_OV2640_H
#define DCI_OV2640_H

#include "sccb.h"

#define DCI_DR_ADDRESS     (0x50050028U)
#define DCI_TIMEOUT        20000

typedef struct
{
  uint8_t manufacturer_id;

  uint8_t HWversion;

}ov2640_id_struct; 

/* ov2640 registers definition */
#define OV2640_PID        0x0A
#define OV2640_VER        0x0B
#define OV2640_MIDH       0x00 
#define OV2640_MIDL       0x1D 

/* function declarations */
void dci_config(void);
void ckout0_init(void);
uint8_t dci_ov2640_init(void);
uint8_t dci_ov2640_id_read(ov2640_id_struct* ov2640id);
uint8_t ov2640_outsize_set(uint16_t width,uint16_t height);
//uint8_t ov2640_outsize_set(uint16_t startx, uint16_t starty, uint16_t width, uint16_t height);
void DCMI_0V7670_RST_PWDN_Init(void);
static void GC0308InitialSetting(void);
#endif /* DCI_OV2640_H */
