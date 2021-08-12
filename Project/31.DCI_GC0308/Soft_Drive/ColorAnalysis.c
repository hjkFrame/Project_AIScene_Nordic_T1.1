#include "ColorAnalysis.h"
#include "dci_ov2640.h"
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "GUI.h"
#include "EmbeddedFlash.h"
#include "LedControl.h"
#include "math.h"
#include "SEGGER_RTT.h"

#define min3v(v1, v2, v3)   ((v1)>(v2)? ((v2)>(v3)?(v3):(v2)):((v1)>(v3)?(v3):(v1)))
#define max3v(v1, v2, v3)   ((v1)<(v2)? ((v2)<(v3)?(v3):(v2)):((v1)<(v3)?(v3):(v1)))

//#define WRITE_DATA

extern uint8_t i,g_ImageArray[CAPTURE_W * CAPTURE_H * 2];


//testing mode
//uint8_t g_AutoDetectionMode = 1, g_Timer5ControlMode = 0, g_IfDetectPedestrian = 0, g_IfDetectClothingChange = 1;



//clothing change
//State_Clothing_Area g_ClothingAreaState = STATE_CLOTHING_AREA_INIT;
//Commodity_Type g_CommodityType = OTHERS;
//Spectrum_Index spectrum_index = SPECTRUM_4000K; //use 4000K if broken
//IMAGE_AREA g_ClothingArea = {59, 99, 43, 83};



//pedestrian change
//State_Pedestrian_Area g_PedestrianAreaState = STATE_PEDESTRIAN_NONE;
//int g_NoMotionDelayTime_ms = 2000, g_MotionTickCount = 0;
IMAGE_AREA g_PedestrianArea[8] = {{0, 58, 0, 42}, {0, 58, 43, 83}, {0, 58, 84, 119},
																	{59, 99, 0, 42}, {59, 99, 84, 119},
																	{100, 159, 0, 42}, {100, 159, 43, 83}, {100, 159, 84, 119}};

//
//int l_ClothingAreaCounter = 0, l_PedestrianAreaCounter = 0; 


//local variables
//int camera_broken_frames = 0;


//extern
//extern State_LedControl state_ledcontrol;
//extern State_Camera state_camera;
//extern float g_NoMotionRatio;
//extern uint8_t g_IfUseCamera, g_ActiveCameraBlockIndex;
//extern uint32_t g_TempTickCount;
																	
Commodity_Analysis_Info g_CommodityAnalysisInfo;
Pedestrian_Analysis_Info g_PedestrianAnalysisInfo;
extern Camera_Control_Info g_CameraControlInfo;
extern Led_Control_Info g_LedControlInfo;																


void init_commodity_analysis_parameters(void) {
	g_CommodityAnalysisInfo.analysisMode = AUTO_DETECTION_MODE;
	g_CommodityAnalysisInfo.detectionState = STATE_CLOTHING_AREA_INIT;
	g_CommodityAnalysisInfo.commodityType = OTHERS;
	g_CommodityAnalysisInfo.spectrumIndex = SPECTRUM_4000K;
	g_CommodityAnalysisInfo.frameCounter = 0;
	g_CommodityAnalysisInfo.tickCounter = 0;
	g_CommodityAnalysisInfo.commodityArea.X_Start = 59;
	g_CommodityAnalysisInfo.commodityArea.X_End = 99;
	g_CommodityAnalysisInfo.commodityArea.Y_Start = 43;
	g_CommodityAnalysisInfo.commodityArea.Y_End = 83;
}

void init_pedestrian_analysis_parameters(int delay_s, int energy_save_percentage) {
	g_PedestrianAnalysisInfo.frameCounter = 0;
	g_PedestrianAnalysisInfo.tickCounter = 0;
	
	if (delay_s > 0 && energy_save_percentage < 100) {
		g_PedestrianAnalysisInfo.ifDetectPedestrian = 1;
		g_PedestrianAnalysisInfo.noMotionDelayTime_ms = delay_s * 1000;
		g_PedestrianAnalysisInfo.noMotionRatio = energy_save_percentage / 100.0;
		g_PedestrianAnalysisInfo.detectionState = STATE_PEDESTRIAN_NONE;
	} else {
		g_PedestrianAnalysisInfo.ifDetectPedestrian = 0;
		g_PedestrianAnalysisInfo.noMotionDelayTime_ms = 2000;
		g_PedestrianAnalysisInfo.noMotionRatio = 0;
		g_PedestrianAnalysisInfo.detectionState = STATE_PEDESTRIAN_AREA_INIT;
	}
}


void RGBtoHSV100(const COLOR_RGB *Rgb, COLOR_HSV *Hsv) {
	int h, s, v, maxVal, minVal, difVal;
	int r = Rgb->red;
	int g = Rgb->green;
  int b = Rgb->blue;
	maxVal = max3v(r, g, b);
	minVal = min3v(r, g, b);	
	difVal = maxVal - minVal;

  v = maxVal * 100.0 / 255;
	if (maxVal <= 0) {
		s = 0;
	}	else {
		s = 100.0 * difVal / maxVal;
	}

	if(difVal == 0){
		h = 0; 
	}	else{
		if(maxVal == r){
			if(g >= b)
				h = 60*(g - b)/(difVal);
			else
				h = 60*(g - b)/(difVal) + 360;
		}
		else if(maxVal == g)
			h = 60 * (b - r)/(difVal) + 120;
		else if(maxVal == b)
			h = 60 * (r - g)/(difVal) + 240;
	}
	Hsv->H = (short)(((h > 360) ? 360 : ((h < 0) ? 0: h)));
	Hsv->S = (unsigned char)(((s > 100) ? 100 : ((s < 0) ? 0: s)));
	Hsv->V = (unsigned char)(((v > 100) ? 100 : ((v < 0) ? 0: v)));
}
void YUV422toRGB(const COLOR_YCbCr *YCbCr, COLOR_RGB *Rgb) {
	int y = YCbCr->Y, cb = YCbCr->Cb - 128, cr = YCbCr->Cr - 128;
	int r = y + 1.402 * cr, g = y - 0.344 * cb - 0.714 * cr, b = y + 1.772 * cb;
	Rgb->red = (unsigned char)(((r > 255) ? 255 : ((r < 0) ? 0: r)));
	Rgb->green = (unsigned char)(((g > 255) ? 255 : ((g < 0) ? 0: g)));
	Rgb->blue	= (unsigned char)(((b > 255) ? 255 : ((b < 0) ? 0: b)));
}

int is_bad_spot_yuv(COLOR_YCbCr YCbCr) {
	if (YCbCr.Y == 0 && YCbCr.Cb == 0 && YCbCr.Cr == 0) return 1;
	if (YCbCr.Y == 255 && YCbCr.Cb == 255 && YCbCr.Cr == 255) return 1;
	return 0;
}

int check_camera_status(int * p_camera_flag) { //����1 �쳣0
	int camera_flag = (*p_camera_flag);
	if (camera_flag == 0) {
		 g_CameraControlInfo.nBrokenFrames++;
		 if (g_CameraControlInfo.nBrokenFrames == 2) {
			 if (g_CommodityAnalysisInfo.detectionState == STATE_CLOTHING_AREA_INIT || g_CommodityAnalysisInfo.detectionState == STATE_CLOTHING_AREA_DETECTING_COLOR_FIRST_TIME) g_CameraControlInfo.operatingState = STATE_CAMERA_START_UP_FAILED;
			 else g_CameraControlInfo.operatingState = STATE_CAMERA_BROKEN;
		 }
		 if (g_CameraControlInfo.nBrokenFrames == 10) {
			 g_CameraControlInfo.nBrokenFrames = 3;
			 SEGGER_RTT_printf(0, "camera break down, request restart camera");
			 //todo add restart camera here
		 }
		 return 0;
	 } else {
		 g_CameraControlInfo.nBrokenFrames = 0;
		 g_CameraControlInfo.operatingState = STATE_CAMERA_WORKING;
		 return 1;
	 }
}

int if_color_warm(Color_Index color) {
	if (color == COLOR_ROSEWOOD || color == COLOR_CHAMPAGNE || color == COLOR_COFFEE || 
		  color == COLOR_RED || color == COLOR_ORANGE|| color == COLOR_YELLOW|| color == COLOR_PINK ||
	    color == COLOR_LIGHT_RED || color == COLOR_LIGHT_ORANGE|| color == COLOR_LIGHT_YELLOW|| color == COLOR_LIGHT_PINK) return 1;
	return 0;
}
int if_color_neutral(Color_Index color) {
	if (color == COLOR_WARM_WHITE || color == COLOR_YELLOW_GREEN || color == COLOR_GREEN || color == COLOR_PURPLE || color == COLOR_BLACK ||
		  color == COLOR_LIGHT_YELLOW_GREEN|| color == COLOR_LIGHT_GREEN|| color == COLOR_LIGHT_PURPLE || color == COLOR_WHITE) return 1;
	return 0;
}
int if_color_cold(Color_Index color) {
	if (color == COLOR_GREEN_CYAN || color == COLOR_CYAN || color == COLOR_BLUE ||
		  color == COLOR_LIGHT_GREEN_CYAN|| color == COLOR_LIGHT_CYAN|| color == COLOR_LIGHT_BLUE) return 1;
	return 0;
}

void sort_array_and_indexes_int(int * array, int * indexes, int start, int end, int if_set_indexes) {
	int i, j, max_index = -1, swap_temp;
	if (if_set_indexes) {
		for (i = start; i <= end; i++) {
		  indexes[i - start] = i;
	  }
	}		
	
	for (i = start; i <= end; i++) {
		max_index = i;
		for (j = i + 1; j <= end; j++) {
			if (array[j] - array[max_index] > 0) max_index = j;
		}
		if (max_index != i) {
			swap_temp = array[i];
			array[i] = array[max_index];
			array[max_index] = swap_temp;
			
			swap_temp = indexes[i - start];
			indexes[i - start] = indexes[max_index - start];
			indexes[max_index - start] = swap_temp;
		}
	}
}

int get_main_color_of_image(const IMAGE_AREA * area, int * color_dict, int * sorted_indexes) {
	const int window_w = area->X_End - area->X_Start + 1, window_h = area->Y_End - area->Y_Start + 1;
	int x, y, i, j, n_bad_spots = 0;
	float avg_hsv[9][3] = {0};
	COLOR_RGB Rgb;
	COLOR_YCbCr YCbCr;
	COLOR_HSV Hsv;
	unsigned char * p = g_ImageArray + g_CameraControlInfo.activeBlockIndex * CAPTURE_H * CAPTURE_W * 2 +  area->Y_Start * CAPTURE_W * 2;
	for (y = 0; y < window_h; y++) {				 
		 for(x = area->X_Start; x <= area->X_End; x++) {
			 YCbCr.Y = p[x * 2];
			 YCbCr.Cb = p[(x / 2) * 4 + 1];
			 YCbCr.Cr = p[(x / 2) * 4 + 3];
			 if (is_bad_spot_yuv(YCbCr)) {
				 n_bad_spots++;
				 continue;
			 }
			 YUV422toRGB(&YCbCr, &Rgb);			 
			 RGBtoHSV100(&Rgb, &Hsv);
			 if (Hsv.H >= 300) Hsv.H -= 360;
			 if (Hsv.V <= 15) {
					color_dict[COLOR_BLACK_S]++;
					avg_hsv[COLOR_BLACK_S][0] += Hsv.H;
				  avg_hsv[COLOR_BLACK_S][1] += Hsv.S;
				  avg_hsv[COLOR_BLACK_S][2] += Hsv.V;
			 } else if (Hsv.S <= 10)  {
					color_dict[COLOR_WHITE_S]++;
				  avg_hsv[COLOR_WHITE_S][0] += Hsv.H;
				  avg_hsv[COLOR_WHITE_S][1] += Hsv.S;
				  avg_hsv[COLOR_WHITE_S][2] += Hsv.V;
			 } else if (Hsv.H < 10 || Hsv.H >= 300) {
				  color_dict[COLOR_RED_S]++;
				  avg_hsv[COLOR_RED_S][0] += Hsv.H;
				  avg_hsv[COLOR_RED_S][1] += Hsv.S;
				  avg_hsv[COLOR_RED_S][2] += Hsv.V;
			 } else if (Hsv.H >= 10 && Hsv.H < 41) {
				  color_dict[COLOR_ORANGE_S]++;
				  avg_hsv[COLOR_ORANGE_S][0] += Hsv.H;
				  avg_hsv[COLOR_ORANGE_S][1] += Hsv.S;
				  avg_hsv[COLOR_ORANGE_S][2] += Hsv.V;
			 } else if (Hsv.H >= 41 && Hsv.H < 68) {
				  color_dict[COLOR_YELLOW_S]++;
				  avg_hsv[COLOR_YELLOW_S][0] += Hsv.H;
				  avg_hsv[COLOR_YELLOW_S][1] += Hsv.S;
				  avg_hsv[COLOR_YELLOW_S][2] += Hsv.V;
			 } else if (Hsv.H >= 68 && Hsv.H < 165) {
				  color_dict[COLOR_GREEN_S]++;
				  avg_hsv[COLOR_GREEN_S][0] += Hsv.H;
				  avg_hsv[COLOR_GREEN_S][1] += Hsv.S;
				  avg_hsv[COLOR_GREEN_S][2] += Hsv.V;
			 } else if (Hsv.H >= 165 && Hsv.H < 200) {
				  color_dict[COLOR_CYAN_S]++;
				  avg_hsv[COLOR_CYAN_S][0] += Hsv.H;
				  avg_hsv[COLOR_CYAN_S][1] += Hsv.S;
				  avg_hsv[COLOR_CYAN_S][2] += Hsv.V;
			 } else if (Hsv.H >= 200 && Hsv.H < 245) {
				  color_dict[COLOR_BLUE_S]++;
				  avg_hsv[COLOR_BLUE_S][0] += Hsv.H;
				  avg_hsv[COLOR_BLUE_S][1] += Hsv.S;
				  avg_hsv[COLOR_BLUE_S][2] += Hsv.V;
			 } else if (Hsv.H >= 245 && Hsv.H < 300) {
				  color_dict[COLOR_PURPLE_S]++;
				  avg_hsv[COLOR_PURPLE_S][0] += Hsv.H;
				  avg_hsv[COLOR_PURPLE_S][1] += Hsv.S;
				  avg_hsv[COLOR_PURPLE_S][2] += Hsv.V;
			 }	
		 }
		 p += CAPTURE_W * 2;
	}
	if (n_bad_spots >= window_h * window_w / 2) {return 0;}
	for (i = 0; i < 9; i++) {
		if (color_dict[i] < 0.05 * window_h * window_w) {
			color_dict[i] = 0;
			continue;
		}
		for (j = 0; j < 3; j++)	avg_hsv[i][j] /= color_dict[i];
	}
	//squeeze color_dict
//	if (color_dict[COLOR_BLACK_S] > 0 && avg_hsv[COLOR_BLACK_S][2] > 15) { // squeeze with black
//			for (i = COLOR_RED_S; i <= COLOR_ORANGE_S; i++) {
//					if (color_dict[i] == 0) continue;
//					if (fabs(avg_hsv[i][0] - avg_hsv[COLOR_BLACK_S][0]) <= 10 && avg_hsv[i][2] < 60) {
//							avg_hsv[i][0] = (avg_hsv[i][0] * color_dict[i] + avg_hsv[COLOR_BLACK_S][0] * color_dict[COLOR_BLACK_S]) / (color_dict[i] + color_dict[COLOR_BLACK_S]);
//							avg_hsv[i][1] = (avg_hsv[i][1] * color_dict[i] + avg_hsv[COLOR_BLACK_S][1] * color_dict[COLOR_BLACK_S]) / (color_dict[i] + color_dict[COLOR_BLACK_S]);
//							avg_hsv[i][2] = (avg_hsv[i][2] * color_dict[i] + avg_hsv[COLOR_BLACK_S][2] * color_dict[COLOR_BLACK_S]) / (color_dict[i] + color_dict[COLOR_BLACK_S]);
//							avg_hsv[COLOR_BLACK_S][0] = 0;
//							avg_hsv[COLOR_BLACK_S][1] = 0;
//							avg_hsv[COLOR_BLACK_S][2] = 0;
//							color_dict[i] += color_dict[COLOR_BLACK_S];
//							color_dict[COLOR_BLACK_S] = 0;
//							break;
//					}
//			}
//	}
//	for (i = COLOR_RED_S; i < COLOR_PURPLE_S; i++) { //close color
//			if (color_dict[i] == 0 || color_dict[i + 1] == 0) continue;
//			if (fabs(avg_hsv[i][0] - avg_hsv[i + 1][0]) <= 10) {
//					if (color_dict[i] >= color_dict[i + 1]) {
//							avg_hsv[i][0] = (avg_hsv[i][0] * color_dict[i] + avg_hsv[i + 1][0] * color_dict[i + 1]) / (color_dict[i] + color_dict[i + 1]);
//							avg_hsv[i][1] = (avg_hsv[i][1] * color_dict[i] + avg_hsv[i + 1][1] * color_dict[i + 1]) / (color_dict[i] + color_dict[i + 1]);
//							avg_hsv[i][2] = (avg_hsv[i][2] * color_dict[i] + avg_hsv[i + 1][2] * color_dict[i + 1]) / (color_dict[i] + color_dict[i + 1]);
//							avg_hsv[i + 1][0] = 0;
//							avg_hsv[i + 1][1] = 0;
//							avg_hsv[i + 1][2] = 0;
//							color_dict[i] += color_dict[i + 1];
//							color_dict[i + 1] = 0;
//					} else {
//							avg_hsv[i + 1][0] = (avg_hsv[i][0] * color_dict[i] + avg_hsv[i + 1][0] * color_dict[i + 1]) / (color_dict[i] + color_dict[i + 1]);
//							avg_hsv[i + 1][1] = (avg_hsv[i][1] * color_dict[i] + avg_hsv[i + 1][1] * color_dict[i + 1]) / (color_dict[i] + color_dict[i + 1]);
//							avg_hsv[i + 1][2] = (avg_hsv[i][2] * color_dict[i] + avg_hsv[i + 1][2] * color_dict[i + 1]) / (color_dict[i] + color_dict[i + 1]);
//							avg_hsv[i][0] = 0;
//							avg_hsv[i][1] = 0;
//							avg_hsv[i][2] = 0;
//							color_dict[i + 1] += color_dict[i];
//							color_dict[i] = 0;
//					}
//			}
//	}
	
	//set color index
	sorted_indexes[COLOR_BLACK_S] = COLOR_BLACK; // black
	
	if (avg_hsv[COLOR_WHITE_S][0] < 70 && avg_hsv[COLOR_WHITE_S][0] > 20) sorted_indexes[COLOR_WHITE_S] = COLOR_WARM_WHITE; // white
	else sorted_indexes[COLOR_WHITE_S] = COLOR_WHITE;
	
  //if (avg_hsv[COLOR_RED_S][0] < 10 && avg_hsv[COLOR_RED_S][1] >= 60 && avg_hsv[COLOR_RED_S][2] <= 60)	sorted_indexes[COLOR_RED_S] = COLOR_ROSEWOOD;
	if (avg_hsv[COLOR_RED_S][0] < -17 || avg_hsv[COLOR_RED_S][1] <= 40) sorted_indexes[COLOR_RED_S] = COLOR_PINK;
	else sorted_indexes[COLOR_RED_S] = COLOR_RED;//red
	
//	if (color_dict[COLOR_ORANGE_S] == 0) sorted_indexes[COLOR_ORANGE_S] = COLOR_ORANGE;
//	else if (avg_hsv[COLOR_ORANGE_S][2] <= 50) sorted_indexes[COLOR_ORANGE_S] = COLOR_COFFEE;
//	else if (avg_hsv[COLOR_ORANGE_S][1] <= 50 && avg_hsv[COLOR_ORANGE_S][2] >= 50) sorted_indexes[COLOR_ORANGE_S] = COLOR_CHAMPAGNE;
	sorted_indexes[COLOR_ORANGE_S] = COLOR_ORANGE;
	
	if (avg_hsv[COLOR_YELLOW_S][1] <= 40 && avg_hsv[COLOR_YELLOW_S][2] >= 70) sorted_indexes[COLOR_YELLOW_S] = COLOR_LIGHT_YELLOW;
	else sorted_indexes[COLOR_YELLOW_S] = COLOR_YELLOW;
	
	if (avg_hsv[COLOR_GREEN_S][1] <= 40 && avg_hsv[COLOR_GREEN_S][2] >= 70) {
			if (avg_hsv[COLOR_GREEN_S][0] < 89) sorted_indexes[COLOR_GREEN_S] = COLOR_LIGHT_YELLOW_GREEN;
			else if (avg_hsv[COLOR_GREEN_S][0] > 141) sorted_indexes[COLOR_GREEN_S] = COLOR_LIGHT_GREEN_CYAN;
			else sorted_indexes[COLOR_GREEN_S] = COLOR_LIGHT_GREEN;
	}
	else {
			if (avg_hsv[COLOR_GREEN_S][0] < 89) sorted_indexes[COLOR_GREEN_S] = COLOR_YELLOW_GREEN;
			else if (avg_hsv[COLOR_GREEN_S][0] > 141) sorted_indexes[COLOR_GREEN_S] = COLOR_GREEN_CYAN;
			else sorted_indexes[COLOR_GREEN_S] = COLOR_GREEN;
	}
	
	if (avg_hsv[COLOR_CYAN_S][1] <= 40 && avg_hsv[COLOR_CYAN_S][2] >= 70) sorted_indexes[COLOR_CYAN_S] = COLOR_LIGHT_CYAN;
	else sorted_indexes[COLOR_CYAN_S] = COLOR_CYAN;
	
	if (avg_hsv[COLOR_BLUE_S][1] <= 40 && avg_hsv[COLOR_BLUE_S][2] >= 70) sorted_indexes[COLOR_BLUE_S] = COLOR_LIGHT_BLUE;
	else sorted_indexes[COLOR_BLUE_S] = COLOR_BLUE;
	
	if (avg_hsv[COLOR_PURPLE_S][1] <= 40 && avg_hsv[COLOR_PURPLE_S][2] >= 70) sorted_indexes[COLOR_PURPLE_S] = COLOR_LIGHT_PURPLE;
	else sorted_indexes[COLOR_PURPLE_S] = COLOR_PURPLE;
	
	sort_array_and_indexes_int(color_dict, sorted_indexes, 0, 8, 0);
	return 1;
}

int get_main_color_of_image_furniture(const IMAGE_AREA * area, int *color_dict, int * sorted_indexes) {
	const int window_w = area->X_End - area->X_Start + 1, window_h = area->Y_End - area->Y_Start + 1;
	int x, y, i, j, n_bad_spots = 0;
	float avg_hsv[9][3] = {0};
	COLOR_RGB Rgb;
	COLOR_YCbCr YCbCr;
	COLOR_HSV Hsv;
	unsigned char * p = g_ImageArray + g_CameraControlInfo.activeBlockIndex * CAPTURE_H * CAPTURE_W * 2 + area->Y_Start * CAPTURE_W * 2;
	for (y = 0; y < window_h; y++) {				 	
		 for(x = area->X_Start; x <= area->X_End; x++) {
			 YCbCr.Y = p[x * 2];
			 YCbCr.Cb = p[(x / 2) * 4 + 1];
			 YCbCr.Cr = p[(x / 2) * 4 + 3];
			 if (is_bad_spot_yuv(YCbCr)) {
				 n_bad_spots++;
				 continue;
			 }
			 YUV422toRGB(&YCbCr, &Rgb);			 
			 RGBtoHSV100(&Rgb, &Hsv);
			 if (Hsv.H >= 300) Hsv.H -= 360;
			 if (Hsv.V <= 15) {	color_dict[COLOR_BLACK_S]++;} 
			 else if (Hsv.S <= 15)  {
					color_dict[COLOR_WHITE_S]++;
				  avg_hsv[COLOR_WHITE_S][0] += Hsv.H;
				  avg_hsv[COLOR_WHITE_S][1] += Hsv.S;
				  avg_hsv[COLOR_WHITE_S][2] += Hsv.V;
			 } else if (Hsv.H < 10 || Hsv.H >= 300) {
				  color_dict[COLOR_RED_S]++;
				  avg_hsv[COLOR_RED_S][0] += Hsv.H;
				  avg_hsv[COLOR_RED_S][1] += Hsv.S;
				  avg_hsv[COLOR_RED_S][2] += Hsv.V;
			 } else if (Hsv.H >= 10 && Hsv.H < 41) { 
			    color_dict[COLOR_ORANGE_S]++;
				  avg_hsv[COLOR_ORANGE_S][0] += Hsv.H;
				  avg_hsv[COLOR_ORANGE_S][1] += Hsv.S;
				  avg_hsv[COLOR_ORANGE_S][2] += Hsv.V;
			 } else if (Hsv.H >= 41 && Hsv.H < 68) { color_dict[COLOR_YELLOW_S]++;} 
			 else if (Hsv.H >= 68 && Hsv.H < 165) {
				  color_dict[COLOR_GREEN_S]++;
				  avg_hsv[COLOR_GREEN_S][0] += Hsv.H;
				  avg_hsv[COLOR_GREEN_S][1] += Hsv.S;
				  avg_hsv[COLOR_GREEN_S][2] += Hsv.V;
			 } else if (Hsv.H >= 165 && Hsv.H < 200) { color_dict[COLOR_CYAN_S]++;} 
			 else if (Hsv.H >= 200 && Hsv.H < 245) { color_dict[COLOR_BLUE_S]++;} 
			 else if (Hsv.H >= 245 && Hsv.H < 300) { color_dict[COLOR_PURPLE_S]++;}	
		 }
		 p += CAPTURE_W * 2;
	}
	if (n_bad_spots >= window_h * window_w / 2) return 0;
	for (i = 0; i < 9; i++) {
		if (color_dict[i] < 0.05 * window_h * window_w) {
			color_dict[i] = 0;
			continue;
		}
		for (j = 0; j < 3; j++)	avg_hsv[i][j] /= color_dict[i];
	}
	sorted_indexes[COLOR_BLACK_S] = COLOR_BLACK; // black
	
	if (avg_hsv[COLOR_WHITE_S][0] < 70 && avg_hsv[COLOR_WHITE_S][0] > 20) sorted_indexes[COLOR_WHITE_S] = COLOR_WARM_WHITE; // white
	else sorted_indexes[COLOR_WHITE_S] = COLOR_WHITE;
	
	if (avg_hsv[COLOR_RED_S][1] > 70 && avg_hsv[COLOR_RED_S][2] > 70) sorted_indexes[COLOR_RED_S] = COLOR_RED;
	else sorted_indexes[COLOR_RED_S] = COLOR_ROSEWOOD;//red
	
	if (avg_hsv[COLOR_ORANGE_S][1] > 70 && avg_hsv[COLOR_ORANGE_S][2] > 70) sorted_indexes[COLOR_ORANGE_S] = COLOR_ORANGE;
	else if (avg_hsv[COLOR_ORANGE_S][1] > avg_hsv[COLOR_ORANGE_S][2]) sorted_indexes[COLOR_ORANGE_S] = COLOR_COFFEE;
	else sorted_indexes[COLOR_ORANGE_S] = COLOR_CHAMPAGNE;
 
	sorted_indexes[COLOR_YELLOW_S] = COLOR_YELLOW;
	
	if (avg_hsv[COLOR_GREEN_S][0] < 89) sorted_indexes[COLOR_GREEN_S] = COLOR_YELLOW_GREEN;
	else if (avg_hsv[COLOR_GREEN_S][0] > 141) sorted_indexes[COLOR_GREEN_S] = COLOR_GREEN_CYAN;
	else sorted_indexes[COLOR_GREEN_S] = COLOR_GREEN;
	
	sorted_indexes[COLOR_CYAN_S] = COLOR_CYAN;	
  sorted_indexes[COLOR_BLUE_S] = COLOR_BLUE;
  sorted_indexes[COLOR_PURPLE_S] = COLOR_PURPLE;
	
	sort_array_and_indexes_int(color_dict, sorted_indexes, 0, 8, 0);
	return 1;
} 

int get_main_color_of_image_clothing(const IMAGE_AREA * area, int *color_dict, int * sorted_indexes) {
	const int window_w = area->X_End - area->X_Start + 1, window_h = area->Y_End - area->Y_Start + 1;
	int x, y, i, j, n_bad_spots = 0;
	float avg_hsv[9][3] = {0};
	COLOR_RGB Rgb;
	COLOR_YCbCr YCbCr;
	COLOR_HSV Hsv;
	unsigned char * p = g_ImageArray + g_CameraControlInfo.activeBlockIndex * CAPTURE_H * CAPTURE_W * 2 + area->Y_Start * CAPTURE_W * 2;
	for (y = 0; y < window_h; y++) {				 
		for(x = area->X_Start; x <= area->X_End; x++) {
			 YCbCr.Y = p[x * 2];
			 YCbCr.Cb = p[(x / 2) * 4 + 1];
			 YCbCr.Cr = p[(x / 2) * 4 + 3];
			 if (is_bad_spot_yuv(YCbCr)) {
				 n_bad_spots++;
				 continue;
			 }
			 YUV422toRGB(&YCbCr, &Rgb);			 
			 RGBtoHSV100(&Rgb, &Hsv);
			 if (Hsv.H >= 300) Hsv.H -= 360;
			 if (Hsv.V <= 15) {	color_dict[COLOR_BLACK_S]++;} 
			 else if (Hsv.S <= 15)  {
					color_dict[COLOR_WHITE_S]++;
				  avg_hsv[COLOR_WHITE_S][0] += Hsv.H;
				  avg_hsv[COLOR_WHITE_S][1] += Hsv.S;
				  avg_hsv[COLOR_WHITE_S][2] += Hsv.V;
			 } else if (Hsv.H < 10 || Hsv.H >= 300) {
				  color_dict[COLOR_RED_S]++;
				  avg_hsv[COLOR_RED_S][0] += Hsv.H;
				  avg_hsv[COLOR_RED_S][1] += Hsv.S;
				  avg_hsv[COLOR_RED_S][2] += Hsv.V;
			 } else if (Hsv.H >= 10 && Hsv.H < 41) { color_dict[COLOR_ORANGE_S]++;} 
			 else if (Hsv.H >= 41 && Hsv.H < 68) { color_dict[COLOR_YELLOW_S]++;} 
			 else if (Hsv.H >= 68 && Hsv.H < 165) {
				  color_dict[COLOR_GREEN_S]++;
				  avg_hsv[COLOR_GREEN_S][0] += Hsv.H;
				  avg_hsv[COLOR_GREEN_S][1] += Hsv.S;
				  avg_hsv[COLOR_GREEN_S][2] += Hsv.V;
			 } else if (Hsv.H >= 165 && Hsv.H < 200) { color_dict[COLOR_CYAN_S]++;} 
			 else if (Hsv.H >= 200 && Hsv.H < 245) { color_dict[COLOR_BLUE_S]++;} 
			 else if (Hsv.H >= 245 && Hsv.H < 300) { color_dict[COLOR_PURPLE_S]++;}	
		 }
		 p += CAPTURE_W * 2;
	}
	if (n_bad_spots >= window_h * window_w / 2) return 0;
	for (i = 0; i < 9; i++) {
		if (color_dict[i] < 0.05 * window_h * window_w) {
			color_dict[i] = 0;
			continue;
		}
		for (j = 0; j < 3; j++)	avg_hsv[i][j] /= color_dict[i];
	}
	sorted_indexes[COLOR_BLACK_S] = COLOR_BLACK; // black
	
	if (avg_hsv[COLOR_WHITE_S][0] < 70 && avg_hsv[COLOR_WHITE_S][0] > 20) sorted_indexes[COLOR_WHITE_S] = COLOR_WARM_WHITE; // white
	else sorted_indexes[COLOR_WHITE_S] = COLOR_WHITE;
	
	if (avg_hsv[COLOR_RED_S][0] < -17 || avg_hsv[COLOR_RED_S][1] <= 40) sorted_indexes[COLOR_RED_S] = COLOR_PINK;
	else sorted_indexes[COLOR_RED_S] = COLOR_RED;//red
	
	sorted_indexes[COLOR_ORANGE_S] = COLOR_ORANGE;
  sorted_indexes[COLOR_YELLOW_S] = COLOR_YELLOW;
	
	if (avg_hsv[COLOR_GREEN_S][0] < 89) sorted_indexes[COLOR_GREEN_S] = COLOR_YELLOW_GREEN;
	else if (avg_hsv[COLOR_GREEN_S][0] > 141) sorted_indexes[COLOR_GREEN_S] = COLOR_GREEN_CYAN;
	else sorted_indexes[COLOR_GREEN_S] = COLOR_GREEN;
	
	sorted_indexes[COLOR_CYAN_S] = COLOR_CYAN;	
  sorted_indexes[COLOR_BLUE_S] = COLOR_BLUE;
  sorted_indexes[COLOR_PURPLE_S] = COLOR_PURPLE;
	
	sort_array_and_indexes_int(color_dict, sorted_indexes, 0, 8, 0);
	return 1;
}

Spectrum_Index get_2_base_color_spectrum_index(Color_Index c_max, Color_Index c_min) {
	if (if_color_warm(c_max) && if_color_warm(c_min)) return SPECTRUM_3000K;
	if (if_color_cold(c_max) && if_color_cold(c_min)) return SPECTRUM_4000K;
	if (c_max == COLOR_BLACK) {
		if (if_color_warm(c_min)) return SPECTRUM_3000K;
		if (if_color_neutral(c_min)) return SPECTRUM_3500K;
		if (if_color_cold(c_min)) return SPECTRUM_4000K;
	}
	if (c_min == COLOR_BLACK) {
		if (if_color_warm(c_max)) return SPECTRUM_3000K;
		if (if_color_neutral(c_max)) return SPECTRUM_3500K;
		if (if_color_cold(c_max)) return SPECTRUM_4000K;
	}
	return SPECTRUM_3500K;
}

Spectrum_Index get_spectrum_index_of_image_4(const IMAGE_AREA * window) { //diffenrent commodity types
	int i, color_dict[9] = {0}, sorted_indexes[9], sum = 0, nonzero_count = 0;
	int c_max, c_min, c2, c_ww = 0, c_nw = 0, c_cw = 0;
	int color_priority[26] = {3, 1, 2, 24, 23, 19, 17, 15, 13, 9, 7, 5, 11, 21, 22, 18, 16, 14, 12, 8, 6, 4, 10, 20, 0, 25};
	int camera_flag = 0;
	if (g_CommodityAnalysisInfo.commodityType == CLOTHING)	camera_flag = get_main_color_of_image_clothing(window, color_dict, sorted_indexes);
	else if (g_CommodityAnalysisInfo.commodityType == FURNITURE) camera_flag = get_main_color_of_image_furniture(window, color_dict, sorted_indexes);
	else camera_flag = get_main_color_of_image(window, color_dict, sorted_indexes);
	if (camera_flag == 0) {
		return SPECTRUM_UNDEFINED;
	}
	for (i = 0; i < 9; i++) sum += color_dict[i];
	if (color_dict[0] > sum * 0.65) return (Spectrum_Index)sorted_indexes[0];
	for (i = 0; i < 26; i++) {
		if (color_dict[i] > 50) nonzero_count++;
		else break;
	}
	if (nonzero_count == 2) {
		c_max = color_dict[0];
		c_min = color_dict[1];
		sum = c_max + c_min;
		if ((c_max - c_min) >= 0.25 * sum) return (Spectrum_Index)sorted_indexes[0];
		if ((c_max - c_min) >= 0.15 * sum) {
			if (color_priority[sorted_indexes[0]] > color_priority[sorted_indexes[1]]) return (Spectrum_Index)sorted_indexes[0];
			else return (Spectrum_Index)sorted_indexes[1];
		}
		else return get_2_base_color_spectrum_index((Color_Index)sorted_indexes[0], (Color_Index)sorted_indexes[1]);
	} else if (nonzero_count > 2) {
		sum = 0;
		if (nonzero_count > 4) nonzero_count = 4;
		for (i = 0; i < nonzero_count; i++) sum += color_dict[i];
		c_max = color_dict[0];
		c2 = color_dict[1];
		if ((c_max - c2) >= 0.3 * sum) return (Spectrum_Index)sorted_indexes[0];
		if (sorted_indexes[0] == COLOR_WHITE) return SPECTRUM_3500K;
		for (i = 0; i < nonzero_count; i++) {
			if (if_color_warm((Color_Index)sorted_indexes[i])) c_ww += color_dict[i];
			else if (if_color_neutral((Color_Index)sorted_indexes[i])) c_nw += color_dict[i];
			else if (if_color_cold((Color_Index)sorted_indexes[i])) c_cw += color_dict[i];
		}
		if (c_ww > 0.5 * sum) return SPECTRUM_3000K;
		if (c_nw > 0.5 * sum) return SPECTRUM_3500K;
		if (c_cw > 0.5 * sum) return SPECTRUM_4000K;
		return SPECTRUM_3500K;
	}
	return SPECTRUM_FULL;
}



void show_yuv422_image(int block_index, const IMAGE_AREA * area, int start_y) {
	const int window_w = area->X_End - area->X_Start + 1, window_h = area->Y_End - area->Y_Start + 1;
	int x, y;
	unsigned char cur_line[CAPTURE_W * 2], * p = g_ImageArray + block_index * CAPTURE_H * CAPTURE_W * 2 + area->Y_Start * CAPTURE_W * 2;
	COLOR_RGB Rgb;
	COLOR_YCbCr YCbCr;
	uint16_t cur_rgb565;
	if (g_LedControlInfo.controlState != STATE_LEDCONTROL_DO_NOTHING) return;
	for (y = 0; y < window_h; y++) {
		for (x = area->X_Start; x <= area->X_End; x++) {
			YCbCr.Y = p[x * 2];
			YCbCr.Cb = p[(x / 2) * 4 + 1];
			YCbCr.Cr = p[(x / 2) * 4 + 3];
			YUV422toRGB(&YCbCr, &Rgb);
			
			cur_rgb565 = ((Rgb.red << 8) & 0xf800) | ((Rgb.green << 3) & 0x07e0) | ((Rgb.blue >> 3) & 0x001f);
      cur_line[x * 2 - area->X_Start * 2] = (unsigned char)(cur_rgb565 >> 8);
  		cur_line[x * 2 + 1 - area->X_Start * 2] = (unsigned char)(cur_rgb565 & 0xff);	
		}
		LCD_ShowPicture(0, start_y + y, window_w > 240 ? 240: window_w, 2, cur_line);
		p += CAPTURE_W * 2;
	}	
}

void show_difference_image(int block_index, const IMAGE_AREA * area) {
	const int window_w = area->X_End - area->X_Start + 1, window_h = area->Y_End - area->Y_Start + 1;
	int last_block_index = block_index - 1 >= 0 ? block_index - 1: block_index + 4 - 1;	
	int x, y;
	unsigned char cur_line[CAPTURE_W * 2], 
		* p_current = g_ImageArray + block_index * CAPTURE_H * CAPTURE_W * 2 + area->Y_Start * CAPTURE_W * 2, 
		* p_last = g_ImageArray + last_block_index * CAPTURE_H * CAPTURE_W * 2 + area->Y_Start * CAPTURE_W * 2;
	COLOR_RGB Rgb;
	COLOR_YCbCr YCbCr;
	uint16_t cur_rgb565;
	if (g_LedControlInfo.controlState != STATE_LEDCONTROL_DO_NOTHING) return;

	for (y = 0; y < window_h; y++) {
		for (x = area->X_Start; x <= area->X_End; x++) {
			YCbCr.Y = abs(p_current[x * 2] - p_last[x * 2]);			
			YCbCr.Cb = 128 + p_current[(x / 2) * 4 + 1] - p_last[(x / 2) * 4 + 1];
			YCbCr.Cr = 128 + p_current[(x / 2) * 4 + 3] - p_last[(x / 2) * 4 + 3];
			YUV422toRGB(&YCbCr, &Rgb);
			
			cur_rgb565 = ((Rgb.red << 8) & 0xf800) | ((Rgb.green << 3) & 0x07e0) | ((Rgb.blue >> 3) & 0x001f);
      cur_line[x * 2 - area->X_Start * 2] = (unsigned char)(cur_rgb565 >> 8);
  		cur_line[x * 2 + 1 - area->X_Start * 2] = (unsigned char)(cur_rgb565 & 0xff);		
		}
		LCD_ShowPicture(0, y + CAPTURE_H + 1, window_w > 240 ? 240: window_w, 2, cur_line);
		p_current += CAPTURE_W * 2;
		p_last += CAPTURE_W * 2;
	}	
}

float calculate_variance(int block_index, int interval, const IMAGE_AREA * area) {
	const int window_w = area->X_End - area->X_Start + 1, window_h = area->Y_End - area->Y_Start + 1;
	int last_block_index = block_index - interval >= 0 ? block_index - interval: block_index + 4 - interval;	
	int x, y, n_pixel = 0;
	float avg = 0, var = 0;
	unsigned char * p_current = g_ImageArray + block_index * CAPTURE_H * CAPTURE_W * 2 + area->Y_Start * CAPTURE_W * 2, 
								* p_last = g_ImageArray + last_block_index * CAPTURE_H * CAPTURE_W * 2 + area->Y_Start * CAPTURE_W * 2;	
	
	//calculate average delta gray
	for (y = 0; y < window_h; y += 2) {
		for (x = area->X_Start; x <= area->X_End; x += 2) {
			avg += abs(p_current[x * 2] - p_last[x * 2]);
			n_pixel++;
		}		
		p_current += CAPTURE_W * 2;
		p_last += CAPTURE_W * 2;
	}	
	avg /= n_pixel;
	
	n_pixel = 0;
	p_current = g_ImageArray + block_index * CAPTURE_H * CAPTURE_W * 2 + area->Y_Start * CAPTURE_W * 2, 
	p_last = g_ImageArray + last_block_index * CAPTURE_H * CAPTURE_W * 2 + area->Y_Start * CAPTURE_W * 2;
	
	//second loop calculate variance
	for (y = 0; y < window_h; y++) {
		for (x = area->X_Start; x <= area->X_End; x++) {
			var += powf(abs(p_current[x * 2] - p_last[x * 2]) - avg, 2);
			n_pixel++;
		}		
		p_current += CAPTURE_W * 2;
		p_last += CAPTURE_W * 2;
	}	
	var /= n_pixel;
	//SEGGER_RTT_printf(0, "var = %f\n", var);
	return var;
}

int if_block_changes(int block_index, int interval, const IMAGE_AREA * area) {
	if (calculate_variance(block_index, interval, area) < 250) return 0;
	return 1;
}


void analyse_image_and_change_light(void) {
	int i;
	if (g_CommodityAnalysisInfo.detectionState == STATE_CLOTHING_AREA_INIT) {
		if (!if_block_changes(g_CameraControlInfo.activeBlockIndex, 1, &g_CommodityAnalysisInfo.commodityArea))
			g_CommodityAnalysisInfo.detectionState = STATE_CLOTHING_AREA_DETECTING_COLOR_FIRST_TIME;
		return;
	}
	if (g_CommodityAnalysisInfo.detectionState == STATE_CLOTHING_AREA_DETECTING_COLOR_FIRST_TIME || g_CommodityAnalysisInfo.detectionState == STATE_CLOTHING_AREA_DETECTING_COLOR) {
		if (!if_block_changes(g_CameraControlInfo.activeBlockIndex, 1, &g_CommodityAnalysisInfo.commodityArea)) {
			g_CommodityAnalysisInfo.spectrumIndex = get_spectrum_index_of_image_4(&g_CommodityAnalysisInfo.commodityArea);
			g_CommodityAnalysisInfo.detectionState = STATE_CLOTHING_AREA_WAITING_TO_BE_STABLE;
			g_LedControlInfo.controlState	= STATE_LEDCONTROL_RECIPE_LOW;
		}
		return;
	}
	if (g_PedestrianAnalysisInfo.ifDetectPedestrian) {
		if (g_PedestrianAnalysisInfo.detectionState == STATE_PEDESTRIAN_NONE) {
			for (i = 0; i < 8; i++) {
				if (if_block_changes(g_CameraControlInfo.activeBlockIndex, 1, &g_PedestrianArea[i])) {
					g_LedControlInfo.controlState = STATE_LEDCONTROL_RECIPE_NORMAL;
					g_PedestrianAnalysisInfo.detectionState = STATE_PEDESTRIAN_EXISTING;
					return;
				}
			}
			return;
		}
		if (g_PedestrianAnalysisInfo.detectionState == STATE_PEDESTRIAN_EXISTING) {
			for (i = 0; i < 8; i++) {
				if (if_block_changes(g_CameraControlInfo.activeBlockIndex, 1, &g_PedestrianArea[i])) return;
			}
			g_PedestrianAnalysisInfo.detectionState = STATE_PEDESTRIAN_WAITING_TICKCOUNT;
			g_PedestrianAnalysisInfo.tickCounter = 0;
		}
		if (g_PedestrianAnalysisInfo.detectionState == STATE_PEDESTRIAN_WAITING_TICKCOUNT) {
			for (i = 0; i < 8; i++) {
				if (if_block_changes(g_CameraControlInfo.activeBlockIndex, 1, &g_PedestrianArea[i])) {
					g_PedestrianAnalysisInfo.detectionState = STATE_PEDESTRIAN_EXISTING;
					return;
				}
			}
			if (g_PedestrianAnalysisInfo.tickCounter >= g_PedestrianAnalysisInfo.noMotionDelayTime_ms) {
				g_LedControlInfo.controlState = STATE_LEDCONTROL_RECIPE_LOW;
				g_PedestrianAnalysisInfo.detectionState = STATE_PEDESTRIAN_AREA_WAITING_TO_BE_STABLE;
				return;
			}
		}
		if (g_PedestrianAnalysisInfo.detectionState == STATE_PEDESTRIAN_AREA_WAITING_TO_BE_STABLE) {
			g_PedestrianAnalysisInfo.frameCounter++;
			if (g_PedestrianAnalysisInfo.frameCounter >= 2) {
				g_PedestrianAnalysisInfo.detectionState = STATE_PEDESTRIAN_NONE;
				g_PedestrianAnalysisInfo.frameCounter = 0;
				return;
			}			
		}
	}
	if (g_CommodityAnalysisInfo.analysisMode == AUTO_DETECTION_MODE) {
		if (g_CommodityAnalysisInfo.detectionState == STATE_CLOTHING_AREA_WAITING_TO_BE_STABLE) {
			g_CommodityAnalysisInfo.frameCounter++;
			if (g_CommodityAnalysisInfo.frameCounter >= 2) {
				g_CommodityAnalysisInfo.detectionState = STATE_CLOTHING_AREA_STABLE;
				g_CommodityAnalysisInfo.frameCounter = 0;
			}
			return;
		}
		if (g_CommodityAnalysisInfo.detectionState == STATE_CLOTHING_AREA_STABLE) {
			if (if_block_changes(g_CameraControlInfo.activeBlockIndex, 1, &g_CommodityAnalysisInfo.commodityArea)) {
				g_CommodityAnalysisInfo.detectionState = STATE_CLOTHING_AREA_DETECTING_COLOR;
				g_LedControlInfo.controlState = STATE_LEDCONTROL_DETECTING_COLOR;
			}
			return;
		}
	}
}


void open_motion_mode(int delay_s, int energy_save_percentage) {
	g_PedestrianAnalysisInfo.noMotionDelayTime_ms = delay_s * 1000;
	g_PedestrianAnalysisInfo.tickCounter = 0;
	g_PedestrianAnalysisInfo.noMotionRatio = energy_save_percentage / 100.0;
	g_PedestrianAnalysisInfo.ifDetectPedestrian = 1;
	g_PedestrianAnalysisInfo.detectionState = STATE_PEDESTRIAN_NONE;
	
	g_CommodityAnalysisInfo.spectrumIndex = SPECTRUM_4000K;	
	g_LedControlInfo.controlState = STATE_LEDCONTROL_RECIPE_LOW;
	if (g_CommodityAnalysisInfo.analysisMode != NO_DETECTION_MODE) g_CommodityAnalysisInfo.analysisMode = NO_DETECTION_MODE;
}
void close_motion_mode(void) {
	g_PedestrianAnalysisInfo.ifDetectPedestrian = 0;
	g_PedestrianAnalysisInfo.noMotionRatio = 1;
}

void close_auto_mode(void) {
	if (g_CameraControlInfo.ifUseCamera != 0) g_CameraControlInfo.ifUseCamera = 0;
}
void open_auto_mode(void) {
	if (g_CameraControlInfo.ifUseCamera != 1) {
		g_CameraControlInfo.ifUseCamera = 1;
	  g_CameraControlInfo.operatingState = STATE_CAMERA_START_UP;
		g_LedControlInfo.controlState = STATE_LEDCONTROL_BLINK;
		g_CommodityAnalysisInfo.detectionState = STATE_CLOTHING_AREA_DETECTING_COLOR;
		g_PedestrianAnalysisInfo.detectionState = STATE_PEDESTRIAN_NONE;
	}
}

unsigned char get_auto_mode_state(void) {
   return g_CameraControlInfo.ifUseCamera;
}
