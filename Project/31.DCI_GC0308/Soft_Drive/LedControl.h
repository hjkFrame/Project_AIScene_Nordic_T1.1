#ifndef LED_CONTROL_H
#define LED_CONTROL_H

#include "systick.h"
#include "ColorAnalysis.h"

typedef enum {
	STATE_LEDCONTROL_TEST,
	STATE_LEDCONTROL_INIT,
	STATE_LEDCONTROL_CHANGING_LIGHT,
	STATE_LEDCONTROL_MINIMUM_COLD_WHITE,
	STATE_LEDCONTROL_RECIPE_LOW,
	STATE_LEDCONTROL_RECIPE_NORMAL,
	STATE_LEDCONTROL_DISABLE,
	STATE_LEDCONTROL_4000K,
	STATE_LEDCONTROL_RESERVE_WHITE,
	STATE_LEDCONTROL_DO_NOTHING,
	STATE_LEDCONTROL_UNDEFINED,
}State_LedControl;

typedef enum {
	STATE_CAMERA_START_UP,
	STATE_CAMERA_START_UP_FAILED,
	STATE_CAMERA_WORKING,
	STATE_CAMERA_OBSERVING,
	STATE_CAMERA_BROKEN
}State_Camera;

void process_camera_exception();
void state_machine_ledcontrol();


void COBLed_Test(void);
void COBLed_minimum_cold_white(void);
void COBLed_Disable(void);
void COBLed_recipe_normal_display();
void COBLed_recipe_low_display();
void COBLed_4000k();
void COBLed_reserve_white();



//slowly change color
void switch_pwm_slowly(int old_ww, int old_cw, int old_r, int old_g, int old_b, int old_c,
											 int new_ww, int new_cw, int new_r, int new_g, int new_b, int new_c);
void switch_color(Spectrum_Index spectrum_index);
void switch_color_low(Spectrum_Index spectrum_index, float ratio);
void set_recipe(Spectrum_Index spectrum_index);
void set_recipe_low(Spectrum_Index spectrum_index, float ratio);
void set_6_recipe(int ww, int cw, int r, int g, int b, int c);


#endif