#include "gd32f20x.h"
#include "timer.h"
#include "LedControl.h"

uint32_t color_recipe_normal[30][6]={
	{588,0,143,0,0,0},
	{588,24,86,86,0,0},
	{588,0,86,86,0,0}, //coffee2
	{412,235,0,171,0,114}, //warm white  	3
	{424,200,86,0,0,0}, //red4
	//{500,500,500,0,0,0}, //red4
	{424,59,57,29,0,0}, //orange
	{588,106,86,86,0,0}, //yellow
	{424,200,29,114,0,0}, //yellow_green
	//{424,318,29,114,0,86}, //green
	//{424,377,0,71,0,114}, //green_cyan9
	{500,500,0,500,0,0}, //green
	{500,500,0,500,0,0}, //green_cyan9
  {424,200,0,71,71,71}, //cyan
	{418,165,29,143,71,71}, //blue
	{418,235,0,100,86,0},  //purple 
	{424,259,57,0,0,57},  //pink
	{424,200,43,0,0,0}, //light_red14
	{424,59,29,14,0,0},  //light_orange
	{588,106,43,43,0,0},  //light_yellow
	{424,200,14,57,0,0},  //light_yellow_green 
//	{424,318,14,57,0,43},  //light_green
//	{424,377,0,36,0,57}, //light_green_cyan19
	{500,500,0,500,0,0}, //green
	{500,500,0,500,0,0}, //green_cyan9
  {424,200,0,36,36,36}, //light_cyan 
	{418,165,14,71,36,36}, //light_blue
	{418,235,0,50,43,0}, //light_purple  22
	{424,259,29,0,0,29}, //light_pink
	{418,235,0,86,0,57}, //black
	{418,235,0,86,0,57}, //white
	{400,29,29,29,29,29}, //3000K 26
	{400,82,29,57,57,57}, //3500K 27
	{400,200,29,86,71,71}, //4000K28
	{424,47,14,43,43,43}, //3200K
};
float low_battery_ratio = 1;
State_Camera state_camera = STATE_CAMERA_START_UP;
int camera_broken_frames = 0, clock_frames = 0, actual_frames = 0;
State_LedControl state_ledcontrol = STATE_LEDCONTROL_TEST;
uint32_t recipe_current[6] = {0, 0, 0, 0, 0, 0};
int minimum_cold_white = 20;
extern Spectrum_Index spectrum_index;


void process_camera_exception() {
	if (state_camera == STATE_CAMERA_START_UP_FAILED) {
		state_ledcontrol = STATE_LEDCONTROL_4000K;
	}
	if (state_camera == STATE_CAMERA_BROKEN) {
		state_ledcontrol = STATE_LEDCONTROL_RESERVE_WHITE;
	}
}
void state_machine_ledcontrol() {
	switch (state_ledcontrol) {
		case STATE_LEDCONTROL_TEST:
			COBLed_Test();
			state_ledcontrol = STATE_LEDCONTROL_INIT;
			break;
		case STATE_LEDCONTROL_INIT:
			COBLed_minimum_cold_white();
		  state_ledcontrol = STATE_LEDCONTROL_DO_NOTHING;
			break;
		case STATE_LEDCONTROL_CHANGING_LIGHT:
			state_ledcontrol = STATE_LEDCONTROL_DO_NOTHING;
			break;
		case STATE_LEDCONTROL_MINIMUM_COLD_WHITE:
			COBLed_minimum_cold_white();
			state_ledcontrol = STATE_LEDCONTROL_DO_NOTHING;
			break;
		case STATE_LEDCONTROL_RECIPE_LOW:
			COBLed_recipe_low_display();
			state_ledcontrol = STATE_LEDCONTROL_DO_NOTHING;
			break;
		case STATE_LEDCONTROL_RECIPE_NORMAL:
			COBLed_recipe_normal_display();
			state_ledcontrol = STATE_LEDCONTROL_DO_NOTHING;
			break;
		case STATE_LEDCONTROL_4000K:
			COBLed_4000k();
			break;
	  case STATE_LEDCONTROL_RESERVE_WHITE:
			COBLed_reserve_white();
			break;
		case STATE_LEDCONTROL_DISABLE:
			break;
		case STATE_LEDCONTROL_DO_NOTHING:
			break;
		default:
			break;
	}
}

void COBLed_recipe_normal_display(){
	state_ledcontrol = STATE_LEDCONTROL_CHANGING_LIGHT;
	switch_pwm_slowly(recipe_current[0],
										recipe_current[1],
										recipe_current[2],
										recipe_current[3],
										recipe_current[4],
										recipe_current[5],	
										color_recipe_normal[spectrum_index][0], 
										color_recipe_normal[spectrum_index][1], 
										color_recipe_normal[spectrum_index][2],  
										color_recipe_normal[spectrum_index][3], 
										color_recipe_normal[spectrum_index][4],  
										color_recipe_normal[spectrum_index][5]);
	set_recipe(spectrum_index);
}
void COBLed_recipe_low_display(){
	state_ledcontrol = STATE_LEDCONTROL_CHANGING_LIGHT;
	switch_pwm_slowly(recipe_current[0],
										recipe_current[1],
										recipe_current[2],
										recipe_current[3],
										recipe_current[4],
										recipe_current[5],	
									  (int)(color_recipe_normal[spectrum_index][0] * low_battery_ratio), 
	                  (int)(color_recipe_normal[spectrum_index][1] * low_battery_ratio), 
	                  (int)(color_recipe_normal[spectrum_index][2] * low_battery_ratio),  
	                  (int)(color_recipe_normal[spectrum_index][3] * low_battery_ratio), 
	                  (int)(color_recipe_normal[spectrum_index][4] * low_battery_ratio),  
	                  (int)(color_recipe_normal[spectrum_index][5] * low_battery_ratio));
	set_recipe_low(spectrum_index, low_battery_ratio);
}

void COBLed_Test(void){
	int i;
	pwm_timer_config(100,0,0,0,0,0);
	delay_1ms(1000);
	pwm_timer_config(0,100,0,0,0,0);
	delay_1ms(1000);
	pwm_timer_config(0,0,100,0,0,0);
	delay_1ms(1000);
	pwm_timer_config(0,0,0,100,0,0);
	delay_1ms(1000);
	pwm_timer_config(0,0,0,0,100,0);
	delay_1ms(1000);
	pwm_timer_config(0,0,0,0,0,100);
	delay_1ms(1000);	
//	pwm_timer_config(500,500,0,0,0,0);
//	delay_1ms(10000);	
//	for (i = 0; i < 30; i++) {
//		pwm_timer_config(color_recipe_normal[i][0], 
//										 color_recipe_normal[i][1], 
//										 color_recipe_normal[i][2],  
//										 color_recipe_normal[i][3], 
//										 color_recipe_normal[i][4],  
//										 color_recipe_normal[i][5]);
//		delay_1ms(100);
//	}	
}

void COBLed_minimum_cold_white(void) {
	state_ledcontrol = STATE_LEDCONTROL_CHANGING_LIGHT;
	switch_pwm_slowly(recipe_current[0], recipe_current[1], recipe_current[2], recipe_current[3], recipe_current[4], recipe_current[5], 
										0, minimum_cold_white, 0, 0, 0, 0);
	recipe_current[0] = 0;
	recipe_current[1] = minimum_cold_white;
	recipe_current[2] = 0;
	recipe_current[3] = 0;
	recipe_current[4] = 0;
	recipe_current[5] = 0;
}

void COBLed_Disable(void){
	state_ledcontrol = STATE_LEDCONTROL_CHANGING_LIGHT;
	switch_pwm_slowly(recipe_current[0], recipe_current[1], recipe_current[2], recipe_current[3], recipe_current[4], recipe_current[5], 0,0,0,0,0,0);
	recipe_current[0] = 0;
	recipe_current[1] = 0;
	recipe_current[2] = 0;
	recipe_current[3] = 0;
	recipe_current[4] = 0;
	recipe_current[5] = 0;
}
void COBLed_4000k() {
	state_ledcontrol = STATE_LEDCONTROL_CHANGING_LIGHT;
	switch_pwm_slowly(recipe_current[0], 
										recipe_current[1], 
										recipe_current[2], 
										recipe_current[3], 
										recipe_current[4], 
										recipe_current[5], 
										color_recipe_normal[SPECTRUM_4000K][0], 
										color_recipe_normal[SPECTRUM_4000K][1], 
										color_recipe_normal[SPECTRUM_4000K][2],  
										color_recipe_normal[SPECTRUM_4000K][3], 
										color_recipe_normal[SPECTRUM_4000K][4],  
										color_recipe_normal[SPECTRUM_4000K][5]);
	set_recipe(SPECTRUM_4000K);
}
void COBLed_reserve_white() {
	state_ledcontrol = STATE_LEDCONTROL_CHANGING_LIGHT;
	switch_pwm_slowly(recipe_current[0], 
										recipe_current[1], 
										recipe_current[2], 
										recipe_current[3], 
										recipe_current[4], 
										recipe_current[5], 
										recipe_current[0], 
										recipe_current[1], 0,0,0,0);
	set_6_recipe(recipe_current[0], recipe_current[1], 0, 0, 0, 0);
}





//slowly change color
void switch_pwm_slowly(int old_ww, int old_cw, int old_r, int old_g, int old_b, int old_c,
											 int new_ww, int new_cw, int new_r, int new_g, int new_b, int new_c){
	int i, delay_time_ms = 50;
	float change_times = 40;
	float ww_step = (new_ww - old_ww) / change_times;
	float cw_step = (new_cw - old_cw) / change_times;
	float r_step = (new_r - old_r) / change_times;
	float g_step = (new_g - old_g) / change_times;
	float b_step = (new_b - old_b) / change_times;
	float c_step = (new_c - old_c) / change_times;
	int ww, cw, r, g, b, c;
	
	for (i = 0; i < change_times; i++) {
		ww = (int)(i * ww_step + old_ww);
		if (ww > 500) ww = 500;
		if (ww < 0) ww = 0;
		cw = (int)(i * cw_step + old_cw);
		if (cw > 500) cw = 500;
		if (cw < 0) cw = 0;
		r = (int)(i * r_step + old_r);
		if (r < 0) r = 0;
		g = (int)(i * g_step + old_g);
		if (g < 0) g = 0;
		b = (int)(i * b_step + old_b);
		if (b < 0) b = 0;
		c = (int)(i * c_step + old_c);
		if (c < 0) c = 0;
		pwm_timer_config(ww, cw, r, g, b, c);
	  delay_1ms(delay_time_ms);
	}
	pwm_timer_config(new_ww, new_cw, new_r, new_g, new_b, new_c);												 
}
											 
void switch_color(Spectrum_Index spectrum_index) {
	switch_pwm_slowly(recipe_current[0], recipe_current[1], recipe_current[2], recipe_current[3], recipe_current[4], recipe_current[5], 
	color_recipe_normal[spectrum_index][0],color_recipe_normal[spectrum_index][1],color_recipe_normal[spectrum_index][2],
	color_recipe_normal[spectrum_index][3],color_recipe_normal[spectrum_index][4],color_recipe_normal[spectrum_index][5]);
}
void switch_color_low(Spectrum_Index spectrum_index, float ratio) {
	switch_pwm_slowly(recipe_current[0], recipe_current[1], recipe_current[2], recipe_current[3], recipe_current[4], recipe_current[5], 
	color_recipe_normal[spectrum_index][0] * ratio,color_recipe_normal[spectrum_index][1] * ratio,color_recipe_normal[spectrum_index][2] * ratio,
	color_recipe_normal[spectrum_index][3] * ratio,color_recipe_normal[spectrum_index][4] * ratio,color_recipe_normal[spectrum_index][5] * ratio);
}
void set_recipe(Spectrum_Index spectrum_index) {
	recipe_current[0] = color_recipe_normal[spectrum_index][0];
	recipe_current[1] = color_recipe_normal[spectrum_index][1];
	recipe_current[2] = color_recipe_normal[spectrum_index][2];
	recipe_current[3] = color_recipe_normal[spectrum_index][3];
	recipe_current[4] = color_recipe_normal[spectrum_index][4];
	recipe_current[5] = color_recipe_normal[spectrum_index][5];
}
void set_recipe_low(Spectrum_Index spectrum_index, float ratio) {
	recipe_current[0] = color_recipe_normal[spectrum_index][0] * ratio;
	recipe_current[1] = color_recipe_normal[spectrum_index][1] * ratio;
	recipe_current[2] = color_recipe_normal[spectrum_index][2] * ratio;
	recipe_current[3] = color_recipe_normal[spectrum_index][3] * ratio;
	recipe_current[4] = color_recipe_normal[spectrum_index][4] * ratio;
	recipe_current[5] = color_recipe_normal[spectrum_index][5] * ratio;
}
void set_6_recipe(int ww, int cw, int r, int g, int b, int c) {
	recipe_current[0] = ww;
	recipe_current[1] = cw;
	recipe_current[2] = r;
	recipe_current[3] = g;
	recipe_current[4] = b;
	recipe_current[5] = c;
}