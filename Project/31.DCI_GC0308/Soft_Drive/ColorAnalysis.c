#include "ColorAnalysis.h"
#include "dci_ov2640.h"
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "GUI.h"
#include "EmbeddedFlash.h"
#include "LedControl.h"
#include "math.h"

#define min3v(v1, v2, v3)   ((v1)>(v2)? ((v2)>(v3)?(v3):(v2)):((v1)>(v3)?(v3):(v1)))
#define max3v(v1, v2, v3)   ((v1)<(v2)? ((v2)<(v3)?(v3):(v2)):((v1)<(v3)?(v3):(v1)))

extern uint8_t capture_image[320*240*2];
//high temperature camera test
uint32_t cur_frame_count = 1, data_write_offset = 40, n_write_images = 0, n_crash_times = 0;
#define START_ADDR FLASH_DOWNLOAD_APP1_ADDR + 1024
#define N_IMAGES_ADDR START_ADDR
#define OFFSET_ADDR START_ADDR + 4
#define CRASH_TIMES_ADDR START_ADDR + 8
//light change
State_Clothing_Area clothing_area_state = STATE_CLOTHING_AREA_INIT;
State_Pedestrian_Area pedestrian_area_state = STATE_PEDESTRIAN_AREA_INIT;
Spectrum_Index spectrum_index = SPECTRUM_UNDEFINED;
float clothing_feature_vector_current[6] = {0}, clothing_feature_vector_last_frame[6] = {0}, clothing_feature_vector_ref[6] = {0};
float feature_vec_current[9][5] = {0}, feature_vec_last_frame[9][5] = {0};
int clothing_area_stable_frames = 0, pedestrian_area_stable_frames = 0;
IMAGE_AREA clothing_area = {120, 199, 80, 159};
int if_detect_pedestrian = 0, if_detect_clothing_change = 1;

extern State_LedControl state_ledcontrol;
extern State_Camera state_camera;
extern uint32_t camera_broken_frames;


//high temperature camera test
void camera_test_init(void) {
	uint32_t temp[3];
	GD32FMC_Read(START_ADDR, temp, 3);
	n_write_images = temp[0];
  data_write_offset = temp[1];
	n_crash_times = temp[2] + 1;
	if (n_write_images > 200 || data_write_offset > 5242888 || n_crash_times > 9999) {
		uint32_t data_to_write[3] = {0, 40, 0};
	  GD32FMC_Bank1_Write(START_ADDR, data_to_write, 3);
		n_write_images = 0;
    data_write_offset = 40;
	  n_crash_times = 0;
	}
	GD32FMC_Bank1_Write(CRASH_TIMES_ADDR, &n_crash_times, 1);	
}
void high_temperature_camera_test(void) {
	 if (cur_frame_count % 4001 == 0) {
			write_image_data_to_memory();			 	  
			n_write_images++;
			GD32FMC_Bank1_Write(N_IMAGES_ADDR, &n_write_images, 1);
			cur_frame_count = 0;
	 }
	 show_image();
	 cur_frame_count++;
}
void show_image(void) {
		unsigned char *p = capture_image;
		int y;
		for(y = 0; y < 320; y++) {
			 LCD_ShowPicture(0, y, 240, 2, p);
			 p += 480 ;		
		}
}
void write_image_data_to_memory(void) {
//	const int start_x = 88, start_y = 128, n_image_line_bytes = 480;	
//	int y, side_length = 50;
//	unsigned char *p = capture_image + n_image_line_bytes * (start_y - 1);
//	
//	for(y = 0; y < side_length; y++) {			 
//		 p += (start_x - 1) * 2;
//		 GD32FMC_Bank1_Write(START_ADDR + data_write_offset, (void *)p, side_length * sizeof(u16) / 4);
//		 data_write_offset += side_length * sizeof(u16);
//		 //LCD_ShowPicture(start_x, start_y + y, side_length, 2, p);
//		 p += n_image_line_bytes - (start_x - 1) * 2;		
//	}
//	GD32FMC_Bank1_Write(OFFSET_ADDR, &data_write_offset, 1);	
	GD32FMC_Bank1_Write(START_ADDR, (void *)capture_image,  320 * 240 * sizeof(u16) / 4);
}

void show_flash_image_data(void) {
	const int start_x = 88, start_y = 128;	
	int i, y, temp, side_length = 50, offset = 40;
	uint32_t n_images;
  uint32_t p[25];
	GD32FMC_Read(START_ADDR, &n_images, 1);
	for (i = 0; i < n_images; i++) {
		temp = i;
		for (y = 0; y < side_length; y++) {
			GD32FMC_Read(START_ADDR + offset, p, side_length * sizeof(u16) / 4);
			offset += side_length * sizeof(u16);
			LCD_ShowPicture(start_x, start_y + y, side_length, 2, (void *)p);
		}
	}
	temp = offset;
}

//tool functions
int is_bad_spot(unsigned short spot) {
	if (spot == 65535) return 1;
	if (spot == 0) return 1;
	return 0;
}
int is_bad_spot_yuv(COLOR_YCbCr YCbCr) {
	if (YCbCr.Y == 0 && YCbCr.Cb == 0 && YCbCr.Cr == 0) return 1;
	if (YCbCr.Y == 255 && YCbCr.Cb == 255 && YCbCr.Cr == 255) return 1;
	return 0;
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
	Hsv->H = (unsigned short)(((h > 360) ? 360 : ((h < 0) ? 0: h)));
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
unsigned int read_u16(unsigned char *buf) {
	return (*buf<<8) | *(buf+1);
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
void sort_array_and_indexes(float * vector, int * indexes, int start, int end) {
	int i, j, max_index = -1;
	float swap_temp;
	for (i = start; i <= end; i++) {
		indexes[i - start] = i;
	}
	for (i = start; i <= end; i++) {
		max_index = i;
		for (j = i + 1; j <= end; j++) {
			if (vector[j] - vector[max_index] > 0) max_index = j;
		}
		if (max_index != i) {
			swap_temp = vector[i];
			vector[i] = vector[max_index];
			vector[max_index] = swap_temp;
			
			swap_temp = indexes[i - start];
			indexes[i - start] = indexes[max_index - start];
			indexes[max_index - start] = swap_temp;
		}
	}
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
//camera test
void show_yuv422_image() {
	int x, y, h = 240, w = 320;
	unsigned char cur_line[320], * p = capture_image;
	COLOR_RGB Rgb;
	COLOR_YCbCr YCbCr;
	uint16_t cur_rgb565;
	for (y = 0; y < h; y += 2) {		
		for (x = 0; x < w ; x += 2) {
			YCbCr.Y = p[x * 2 + 0];
			YCbCr.Cb = p[x * 2 + 1];
			YCbCr.Cr = p[x * 2 + 3];
			YUV422toRGB(&YCbCr, &Rgb);
			
			cur_rgb565 = ((Rgb.red << 8) & 0xf800) | ((Rgb.green << 3) & 0x07e0) | ((Rgb.blue >> 3) & 0x001f);
      cur_line[x] = (unsigned char)(cur_rgb565 >> 8);
  		cur_line[x + 1] = (unsigned char)(cur_rgb565 & 0xff);				
		}
		LCD_ShowPicture(0, y / 2, 160, 2, cur_line);
		p += 640 * 2;
	}	
}
//main functions spectrum
void squeeze_color_dict(int * color_dict) {
	if (color_dict[COLOR_RED] > 50 && color_dict[COLOR_LIGHT_RED] > 50) {
		if (color_dict[COLOR_RED] >= color_dict[COLOR_LIGHT_RED]) {
			color_dict[COLOR_RED] += color_dict[COLOR_LIGHT_RED];
			color_dict[COLOR_LIGHT_RED] = 0;
		} else {
			color_dict[COLOR_LIGHT_RED] += color_dict[COLOR_RED];
			color_dict[COLOR_RED] = 0;
		}
	}
	if (color_dict[COLOR_ORANGE] > 50 && color_dict[COLOR_LIGHT_ORANGE] > 50) {
		if (color_dict[COLOR_ORANGE] >= color_dict[COLOR_LIGHT_ORANGE]) {
			color_dict[COLOR_ORANGE] += color_dict[COLOR_LIGHT_ORANGE];
			color_dict[COLOR_LIGHT_ORANGE] = 0;
		} else {
			color_dict[COLOR_LIGHT_ORANGE] += color_dict[COLOR_ORANGE];
			color_dict[COLOR_ORANGE] = 0;
		}
	}
	if (color_dict[COLOR_YELLOW] > 50 && color_dict[COLOR_LIGHT_YELLOW] > 50) {
		if (color_dict[COLOR_YELLOW] >= color_dict[COLOR_LIGHT_YELLOW]) {
			color_dict[COLOR_YELLOW] += color_dict[COLOR_LIGHT_YELLOW];
			color_dict[COLOR_LIGHT_YELLOW] = 0;
		} else {
			color_dict[COLOR_LIGHT_YELLOW] += color_dict[COLOR_YELLOW];
			color_dict[COLOR_YELLOW] = 0;
		}
	}
	if (color_dict[COLOR_YELLOW_GREEN] > 50 && color_dict[COLOR_LIGHT_YELLOW_GREEN] > 50) {
		if (color_dict[COLOR_YELLOW_GREEN] >= color_dict[COLOR_LIGHT_YELLOW_GREEN]) {
			color_dict[COLOR_YELLOW_GREEN] += color_dict[COLOR_LIGHT_YELLOW_GREEN];
			color_dict[COLOR_LIGHT_YELLOW_GREEN] = 0;
		} else {
			color_dict[COLOR_LIGHT_YELLOW_GREEN] += color_dict[COLOR_YELLOW_GREEN];
			color_dict[COLOR_YELLOW_GREEN] = 0;
		}
	}
	if (color_dict[COLOR_GREEN] > 50 && color_dict[COLOR_LIGHT_GREEN] > 50) {
		if (color_dict[COLOR_GREEN] >= color_dict[COLOR_LIGHT_GREEN]) {
			color_dict[COLOR_GREEN] += color_dict[COLOR_LIGHT_GREEN];
			color_dict[COLOR_LIGHT_GREEN] = 0;
		} else {
			color_dict[COLOR_LIGHT_GREEN] += color_dict[COLOR_GREEN];
			color_dict[COLOR_GREEN] = 0;
		}
	}
	if (color_dict[COLOR_GREEN_CYAN] > 50 && color_dict[COLOR_LIGHT_GREEN_CYAN] > 50) {
		if (color_dict[COLOR_GREEN_CYAN] >= color_dict[COLOR_LIGHT_GREEN_CYAN]) {
			color_dict[COLOR_GREEN_CYAN] += color_dict[COLOR_LIGHT_GREEN_CYAN];
			color_dict[COLOR_LIGHT_GREEN_CYAN] = 0;
		} else {
			color_dict[COLOR_LIGHT_GREEN_CYAN] += color_dict[COLOR_GREEN_CYAN];
			color_dict[COLOR_GREEN_CYAN] = 0;
		}
	}
	if (color_dict[COLOR_CYAN] > 50 && color_dict[COLOR_LIGHT_CYAN] > 50) {
		if (color_dict[COLOR_CYAN] >= color_dict[COLOR_LIGHT_CYAN]) {
			color_dict[COLOR_CYAN] += color_dict[COLOR_LIGHT_CYAN];
			color_dict[COLOR_LIGHT_CYAN] = 0;
		} else {
			color_dict[COLOR_LIGHT_CYAN] += color_dict[COLOR_CYAN];
			color_dict[COLOR_CYAN] = 0;
		}
	}
	if (color_dict[COLOR_BLUE] > 50 && color_dict[COLOR_LIGHT_BLUE] > 50) {
		if (color_dict[COLOR_BLUE] >= color_dict[COLOR_LIGHT_BLUE]) {
			color_dict[COLOR_BLUE] += color_dict[COLOR_LIGHT_BLUE];
			color_dict[COLOR_LIGHT_BLUE] = 0;
		} else {
			color_dict[COLOR_LIGHT_BLUE] += color_dict[COLOR_BLUE];
			color_dict[COLOR_BLUE] = 0;
		}
	}
	if (color_dict[COLOR_PURPLE] > 50 && color_dict[COLOR_LIGHT_PURPLE] > 50) {
		if (color_dict[COLOR_PURPLE] >= color_dict[COLOR_LIGHT_PURPLE]) {
			color_dict[COLOR_PURPLE] += color_dict[COLOR_LIGHT_PURPLE];
			color_dict[COLOR_LIGHT_PURPLE] = 0;
		} else {
			color_dict[COLOR_LIGHT_PURPLE] += color_dict[COLOR_PURPLE];
			color_dict[COLOR_PURPLE] = 0;
		}
	}
	if (color_dict[COLOR_PINK] > 50 && color_dict[COLOR_LIGHT_PINK] > 50) {
		if (color_dict[COLOR_PINK] >= color_dict[COLOR_LIGHT_PINK]) {
			color_dict[COLOR_PINK] += color_dict[COLOR_LIGHT_PINK];
			color_dict[COLOR_LIGHT_PINK] = 0;
		} else {
			color_dict[COLOR_LIGHT_PINK] += color_dict[COLOR_PINK];
			color_dict[COLOR_PINK] = 0;
		}
	}
}
int get_main_color_of_image(const IMAGE_AREA * area, int * color_dict, int * sorted_indexes) {
	const int start_x = area->X_Start, start_y = area->Y_Start, end_x = area->X_End, end_y = area->Y_End;
	const int window_w = end_x - start_x + 1, window_h = end_y - start_y + 1, n_image_line_bytes = 640;
	int x, y, i, j, n_bad_spots = 0, n_total_pixel = 0;
	int avg_hsv[9][3] = {0};
	uint16_t cur_rgb565;
	COLOR_RGB Rgb;
	COLOR_YCbCr YCbCr;
	COLOR_HSV Hsv;
	unsigned char * p = capture_image + n_image_line_bytes * start_y, cur_line[160] = {0};
	for (y = 0; y < window_h; y++) {				 
		 p += start_x * 2; 	
		 for(x = 0; x < window_w * 2; x += 4) {
			 YCbCr.Y = p[x];
			 YCbCr.Cb = p[x + 1];
			 YCbCr.Cr = p[x + 3];
			 if (is_bad_spot_yuv(YCbCr)) {
				 n_bad_spots++;
				 continue;
			 }
			 YUV422toRGB(&YCbCr, &Rgb);
			 cur_rgb565 = ((Rgb.red << 8) & 0xf800) | ((Rgb.green << 3) & 0x07e0) | ((Rgb.blue >> 3) & 0x001f);
       cur_line[x] = (unsigned char)(cur_rgb565 >> 8);
  		 cur_line[x + 1] = (unsigned char)(cur_rgb565 & 0xff);
			 
			 RGBtoHSV100(&Rgb, &Hsv);
			 if (Hsv.V <= 30) {
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
			 } else if (Hsv.H >= 10 && Hsv.H < 40) {
				  color_dict[COLOR_ORANGE_S]++;
				  avg_hsv[COLOR_ORANGE_S][0] += Hsv.H;
				  avg_hsv[COLOR_ORANGE_S][1] += Hsv.S;
				  avg_hsv[COLOR_ORANGE_S][2] += Hsv.V;
			 } else if (Hsv.H >= 40 && Hsv.H < 80) {
				  color_dict[COLOR_YELLOW_S]++;
				  avg_hsv[COLOR_YELLOW_S][0] += Hsv.H;
				  avg_hsv[COLOR_YELLOW_S][1] += Hsv.S;
				  avg_hsv[COLOR_YELLOW_S][2] += Hsv.V;
			 } else if (Hsv.H >= 80 && Hsv.H < 150) {
				  color_dict[COLOR_GREEN_S]++;
				  avg_hsv[COLOR_GREEN_S][0] += Hsv.H;
				  avg_hsv[COLOR_GREEN_S][1] += Hsv.S;
				  avg_hsv[COLOR_GREEN_S][2] += Hsv.V;
			 } else if (Hsv.H >= 150 && Hsv.H < 210) {
				  color_dict[COLOR_CYAN_S]++;
				  avg_hsv[COLOR_CYAN_S][0] += Hsv.H;
				  avg_hsv[COLOR_CYAN_S][1] += Hsv.S;
				  avg_hsv[COLOR_CYAN_S][2] += Hsv.V;
			 } else if (Hsv.H >= 210 && Hsv.H < 260) {
				  color_dict[COLOR_BLUE_S]++;
				  avg_hsv[COLOR_BLUE_S][0] += Hsv.H;
				  avg_hsv[COLOR_BLUE_S][1] += Hsv.S;
				  avg_hsv[COLOR_BLUE_S][2] += Hsv.V;
			 } else if (Hsv.H >= 260 && Hsv.H < 300) {
				  color_dict[COLOR_PURPLE_S]++;
				  avg_hsv[COLOR_PURPLE_S][0] += Hsv.H;
				  avg_hsv[COLOR_PURPLE_S][1] += Hsv.S;
				  avg_hsv[COLOR_PURPLE_S][2] += Hsv.V;
			 }	

			 YCbCr.Y = p[x + 2];
			 if (is_bad_spot_yuv(YCbCr)) {
				 n_bad_spots++;
				 continue;
			 }
			 YUV422toRGB(&YCbCr, &Rgb);
			 cur_rgb565 = ((Rgb.red << 8) & 0xf800) | ((Rgb.green << 3) & 0x07e0) | ((Rgb.blue >> 3) & 0x001f);
       cur_line[x + 2] = (unsigned char)(cur_rgb565 >> 8);
  		 cur_line[x + 3] = (unsigned char)(cur_rgb565 & 0xff);
			 RGBtoHSV100(&Rgb, &Hsv);
			 if (Hsv.V <= 30) {
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
			 } else if (Hsv.H >= 10 && Hsv.H < 40) {
				  color_dict[COLOR_ORANGE_S]++;
				  avg_hsv[COLOR_ORANGE_S][0] += Hsv.H;
				  avg_hsv[COLOR_ORANGE_S][1] += Hsv.S;
				  avg_hsv[COLOR_ORANGE_S][2] += Hsv.V;
			 } else if (Hsv.H >= 40 && Hsv.H < 80) {
				  color_dict[COLOR_YELLOW_S]++;
				  avg_hsv[COLOR_YELLOW_S][0] += Hsv.H;
				  avg_hsv[COLOR_YELLOW_S][1] += Hsv.S;
				  avg_hsv[COLOR_YELLOW_S][2] += Hsv.V;
			 } else if (Hsv.H >= 80 && Hsv.H < 150) {
				  color_dict[COLOR_GREEN_S]++;
				  avg_hsv[COLOR_GREEN_S][0] += Hsv.H;
				  avg_hsv[COLOR_GREEN_S][1] += Hsv.S;
				  avg_hsv[COLOR_GREEN_S][2] += Hsv.V;
			 } else if (Hsv.H >= 150 && Hsv.H < 210) {
				  color_dict[COLOR_CYAN_S]++;
				  avg_hsv[COLOR_CYAN_S][0] += Hsv.H;
				  avg_hsv[COLOR_CYAN_S][1] += Hsv.S;
				  avg_hsv[COLOR_CYAN_S][2] += Hsv.V;
			 } else if (Hsv.H >= 210 && Hsv.H < 260) {
				  color_dict[COLOR_BLUE_S]++;
				  avg_hsv[COLOR_BLUE_S][0] += Hsv.H;
				  avg_hsv[COLOR_BLUE_S][1] += Hsv.S;
				  avg_hsv[COLOR_BLUE_S][2] += Hsv.V;
			 } else if (Hsv.H >= 260 && Hsv.H < 300) {
				  color_dict[COLOR_PURPLE_S]++;
				  avg_hsv[COLOR_PURPLE_S][0] += Hsv.H;
				  avg_hsv[COLOR_PURPLE_S][1] += Hsv.S;
				  avg_hsv[COLOR_PURPLE_S][2] += Hsv.V;
			 }	
		 }
		 LCD_ShowPicture(0, 120 + y, window_w, 2, cur_line);
		 p += n_image_line_bytes - start_x * 2;
	}
	if (n_bad_spots >= window_h * window_w / 2) {
		return 0;
	}
	for (i = 0; i < 9; i++) {
		if (color_dict[i] < 20) {
			color_dict[i] = 0;
			continue;
		}
		for (j = 0; j < 3; j++)	avg_hsv[i][j] /= color_dict[i];
	}
	//squeeze color_dict
	for (i = COLOR_RED_S; i < 8; i++) {
		if (color_dict[i] == 0 || color_dict[i + 1] == 0) continue;
		if (abs(avg_hsv[i][0] - avg_hsv[i + 1][0]) <= 5) {
			if (color_dict[i] >= color_dict[i + 1]) {
				avg_hsv[i][0] = (avg_hsv[i][0] * color_dict[i] + avg_hsv[i + 1][0] * color_dict[i + 1]) / (color_dict[i] + color_dict[i + 1]);
				avg_hsv[i][1] = (avg_hsv[i][1] * color_dict[i] + avg_hsv[i + 1][1] * color_dict[i + 1]) / (color_dict[i] + color_dict[i + 1]);
				avg_hsv[i][2] = (avg_hsv[i][2] * color_dict[i] + avg_hsv[i + 1][2] * color_dict[i + 1]) / (color_dict[i] + color_dict[i + 1]);
				avg_hsv[i + 1][0] = 0;
				avg_hsv[i + 1][1] = 0;
				avg_hsv[i + 1][2] = 0;
				color_dict[i] += color_dict[i + 1];
				color_dict[i + 1] = 0;
			} else {
				avg_hsv[i + 1][0] = (avg_hsv[i][0] * color_dict[i] + avg_hsv[i + 1][0] * color_dict[i + 1]) / (color_dict[i] + color_dict[i + 1]);
				avg_hsv[i + 1][1] = (avg_hsv[i][1] * color_dict[i] + avg_hsv[i + 1][1] * color_dict[i + 1]) / (color_dict[i] + color_dict[i + 1]);
				avg_hsv[i + 1][2] = (avg_hsv[i][2] * color_dict[i] + avg_hsv[i + 1][2] * color_dict[i + 1]) / (color_dict[i] + color_dict[i + 1]);
				avg_hsv[i][0] = 0;
				avg_hsv[i][1] = 0;
				avg_hsv[i][2] = 0;
				color_dict[i + 1] += color_dict[i];
				color_dict[i] = 0;
			}
		}
	}
	for (i = 0; i < 9; i++) {
		n_total_pixel += color_dict[i];
	}	
	if (color_dict[COLOR_BLACK_S] < 0.1 * n_total_pixel) color_dict[COLOR_BLACK_S] = 0;
	
	//set color index
	sorted_indexes[COLOR_BLACK_S] = COLOR_BLACK; // black
	
	if (avg_hsv[COLOR_WHITE_S][0] < 80) sorted_indexes[COLOR_WHITE_S] = COLOR_WARM_WHITE; // white
	else sorted_indexes[COLOR_WHITE_S] = COLOR_WHITE;
	
	if (color_dict[COLOR_RED_S] == 0) sorted_indexes[COLOR_RED_S] = COLOR_RED; //red
  else if ((avg_hsv[COLOR_RED_S][0] > 340 || avg_hsv[COLOR_RED_S][0] < 10) && 
		        avg_hsv[COLOR_RED_S][1] >= 60 && avg_hsv[COLOR_RED_S][2] <= 60)	sorted_indexes[COLOR_RED_S] = COLOR_ROSEWOOD;
	else if ((avg_hsv[COLOR_RED_S][0] < 343 && avg_hsv[COLOR_RED_S][0] >= 300) || avg_hsv[COLOR_RED_S][1] <= 40) sorted_indexes[COLOR_RED_S] = COLOR_PINK;
	else sorted_indexes[COLOR_RED_S] = COLOR_RED;//red
	
	if (color_dict[COLOR_ORANGE_S] == 0) sorted_indexes[COLOR_ORANGE_S] = COLOR_ORANGE;
	else if (avg_hsv[COLOR_ORANGE_S][2] <= 50) sorted_indexes[COLOR_ORANGE_S] = COLOR_ORANGE;
	else if (avg_hsv[COLOR_ORANGE_S][1] <= 50 && avg_hsv[COLOR_ORANGE_S][2] >= 50) sorted_indexes[COLOR_ORANGE_S] = COLOR_CHAMPAGNE;
	else sorted_indexes[COLOR_ORANGE_S] = COLOR_ORANGE;
	
	if (avg_hsv[COLOR_YELLOW_S][1] <= 40 && avg_hsv[COLOR_YELLOW_S][2] >= 70) sorted_indexes[COLOR_YELLOW_S] = COLOR_LIGHT_YELLOW;
	else sorted_indexes[COLOR_YELLOW_S] = COLOR_YELLOW;
	
	if (avg_hsv[COLOR_GREEN_S][1] <= 40 && avg_hsv[COLOR_GREEN_S][2] >= 70) sorted_indexes[COLOR_GREEN_S] = COLOR_LIGHT_GREEN;
	else sorted_indexes[COLOR_GREEN_S] = COLOR_GREEN;
	
	if (avg_hsv[COLOR_CYAN_S][1] <= 40 && avg_hsv[COLOR_CYAN_S][2] >= 70) sorted_indexes[COLOR_CYAN_S] = COLOR_LIGHT_CYAN;
	else sorted_indexes[COLOR_CYAN_S] = COLOR_CYAN;
	
	if (avg_hsv[COLOR_BLUE_S][1] <= 40 && avg_hsv[COLOR_BLUE_S][2] >= 70) sorted_indexes[COLOR_BLUE_S] = COLOR_LIGHT_BLUE;
	else sorted_indexes[COLOR_BLUE_S] = COLOR_BLUE;
	
	if (avg_hsv[COLOR_PURPLE_S][1] <= 40 && avg_hsv[COLOR_PURPLE_S][2] >= 70) sorted_indexes[COLOR_PURPLE_S] = COLOR_LIGHT_PURPLE;
	else sorted_indexes[COLOR_PURPLE_S] = COLOR_PURPLE;
	
	sort_array_and_indexes_int(color_dict, sorted_indexes, 0, 8, 0);
	return 1;
}
int count_color_dict26_of_image(const IMAGE_AREA * area, int * color_dict) {
	const int start_x = area->X_Start, start_y = area->Y_Start, end_x = area->X_End, end_y = area->Y_End;
	const int window_w = end_x - start_x + 1, window_h = end_y - start_y + 1, n_image_line_bytes = 480;
	int x, y, i, n_bad_spots = 0, avg_h = 0, avg_s = 0, avg_v = 0;
	uint16_t cur_rgb565; 
	COLOR_RGB Rgb;
	COLOR_HSV Hsv;
	unsigned char * p = capture_image + n_image_line_bytes * start_y;		
	
	for (i = 0; i < COLOR_UNDEFINED; i++) color_dict[i] = 0;
	for (y = 0; y < window_h; y++) {				 
		 p += start_x * 2; 
		 LCD_ShowPicture(start_x, start_y + y, window_w, 2, p);			
		 for(x = 0; x < window_w * 2; x += 2) {
			 cur_rgb565 = read_u16(&p[x]);
			 if (is_bad_spot(cur_rgb565)) {
				 n_bad_spots++;
				 continue;
			 }
			 Rgb.red = (unsigned char)((cur_rgb565 & 0xf800) >> 8);
			 Rgb.green = (unsigned char)((cur_rgb565 & 0x07e0) >> 3);
			 Rgb.blue = (unsigned char)((cur_rgb565 & 0x001f) << 3);
			 RGBtoHSV100(&Rgb, &Hsv);
			 avg_h += Hsv.H;
			 avg_s += Hsv.S;
			 avg_v += Hsv.V;
			 if (Hsv.V <= 35) {color_dict[COLOR_BLACK]++; continue;}
			 else if (Hsv.H >= 20 && Hsv.H <= 80 && Hsv.S >= 2 && Hsv.S <= 30 && Hsv.V >= 70)  {color_dict[COLOR_WARM_WHITE]++; continue;}
			 else if (Hsv.S < 10) {color_dict[COLOR_WHITE]++; continue;}
			 else if (Hsv.H >= 20 && Hsv.H <= 45 && Hsv.S >= 30 && Hsv.S <= 50 && Hsv.V >= 50) {color_dict[COLOR_CHAMPAGNE]++; continue;}
			 //else if (Hsv.H >= 10 && Hsv.H <= 20 && Hsv.S >= 70 && Hsv.V >= 30 && Hsv.V <= 50) {color_dict[COLOR_COFFEE]++; continue;}					 
			 else if (Hsv.H >= 343 || Hsv.H < 15) {
				 if (Hsv.S >= (Hsv.V + 5)) color_dict[COLOR_ROSEWOOD]++;
				 else if (Hsv.S <= 40) color_dict[COLOR_PINK]++;
				 else color_dict[COLOR_RED]++; continue;
			 }
			 else if (Hsv.H >= 15 && Hsv.H < 40) {
				 if (Hsv.S >= 70 && Hsv.V >= 70) color_dict[COLOR_ORANGE]++;
				 else color_dict[COLOR_COFFEE]++; continue;
			 }
			 else if (Hsv.H >= 40 && Hsv.H < 68) {if (Hsv.S < 40) color_dict[COLOR_LIGHT_YELLOW]++; else color_dict[COLOR_YELLOW]++; continue;}
			 else if (Hsv.H >= 68 && Hsv.H < 89) {if (Hsv.S < 40) color_dict[COLOR_LIGHT_YELLOW_GREEN]++; else color_dict[COLOR_YELLOW_GREEN]++; continue;}
			 else if (Hsv.H >= 89 && Hsv.H < 141) {if (Hsv.S < 40) color_dict[COLOR_LIGHT_GREEN]++; else color_dict[COLOR_GREEN]++; continue;}
			 else if (Hsv.H >= 141 && Hsv.H < 165) {if (Hsv.S < 40) color_dict[COLOR_LIGHT_GREEN_CYAN]++; else color_dict[COLOR_GREEN_CYAN]++; continue;}
			 else if (Hsv.H >= 165 && Hsv.H < 211) {if (Hsv.S < 40) color_dict[COLOR_LIGHT_CYAN]++; else color_dict[COLOR_CYAN]++; continue;}
			 else if (Hsv.H >= 211 && Hsv.H < 260) {if (Hsv.S < 40) color_dict[COLOR_LIGHT_BLUE]++; else color_dict[COLOR_BLUE]++; continue;}
			 else if (Hsv.H >= 260 && Hsv.H < 300) {if (Hsv.S < 40) color_dict[COLOR_LIGHT_PURPLE]++; else color_dict[COLOR_PURPLE]++; continue;}
			 else if (Hsv.H >= 300 && Hsv.H < 343) {if (Hsv.S < 40) color_dict[COLOR_LIGHT_PINK]++; else color_dict[COLOR_PINK]++; continue;}						 
		 }	
		 p += n_image_line_bytes - start_x * 2;
	}
	if (n_bad_spots >= window_h * window_w / 2) {
		return 0;
	}
	avg_h /= (window_h * window_w - n_bad_spots);
	avg_s /= (window_h * window_w - n_bad_spots);
	avg_v /= (window_h * window_w - n_bad_spots);
	p = NULL;
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
Spectrum_Index get_spectrum_index_of_image_3(const IMAGE_AREA * window) {
	int i, color_dict[9] = {0}, sorted_indexes[9], sum = 0, nonzero_count = 0;
	int c_max, c_min, c2, c_ww = 0, c_nw = 0, c_cw = 0;
	int color_priority[26] = {3, 1, 2, 24, 23, 19, 17, 15, 13, 9, 7, 5, 11, 21, 22, 18, 16, 14, 12, 8, 6, 4, 10, 20, 0, 25};
	int camera_flag = 0;
	camera_flag = get_main_color_of_image(window, color_dict, sorted_indexes);
	if (camera_flag == 0) {
		return SPECTRUM_UNDEFINED;
	}
	//squeeze_color_dict(color_dict);
	//sort_array_and_indexes_int(color_dict, sorted_indexes, 0, 25, 1);
	for (i = 0; i < 9; i++) sum += color_dict[i];
	if (color_dict[0] > sum * 0.7) return (Spectrum_Index)sorted_indexes[0];
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


//main functions feature
int get_feature_vector_of_image(const IMAGE_AREA * area, float * vec) {
	const int start_x = area->X_Start, start_y = area->Y_Start, end_x = area->X_End, end_y = area->Y_End;
	const int window_w = end_x - start_x + 1, window_h = end_y - start_y + 1, n_image_line_bytes = 640;
	int x, y, i, n_bad_spots = 0, avg_h = 0, avg_s = 0, avg_v = 0;
	float average_gray_left_up = 0, average_gray_left_bottom = 0, average_gray_right_up = 0, average_gray_right_bottom = 0, gray;
	uint16_t cur_rgb565;
	COLOR_RGB Rgb;
	COLOR_YCbCr YCbCr;
	COLOR_HSV Hsv;
	unsigned char * p = capture_image + n_image_line_bytes * start_y, cur_line[160] = {0};	

	for (i = 0 ; i < 6; i++) vec[i] = 0;
	
	for (y = 0; y < window_h; y++) {				 
		 p += start_x * 2; 
		 for(x = 0; x < window_w * 2; x += 4) {
			 YCbCr.Y = p[x];
			 YCbCr.Cb = p[x + 1];
			 YCbCr.Cr = p[x + 3];
			 if (is_bad_spot_yuv(YCbCr)) {
				 n_bad_spots++;
				 continue;
			 }
			 YUV422toRGB(&YCbCr, &Rgb);
			 cur_rgb565 = ((Rgb.red << 8) & 0xf800) | ((Rgb.green << 3) & 0x07e0) | ((Rgb.blue >> 3) & 0x001f);
       cur_line[x] = (unsigned char)(cur_rgb565 >> 8);
  		 cur_line[x + 1] = (unsigned char)(cur_rgb565 & 0xff);
			 RGBtoHSV100(&Rgb, &Hsv);
			 avg_h += Hsv.H;
			 avg_s += Hsv.S;
			 avg_v += Hsv.V;
			 gray = (Rgb.red * 38 + Rgb.green * 75 + Rgb.blue * 15) >> 7;	
			 if (y < window_h / 2) {
				 if (x < window_w) average_gray_left_up += gray / 255.0;
				 else average_gray_right_up += gray / 255.0;
			 } else {
				 if (x < window_w) average_gray_left_bottom += gray / 255.0;
				 else average_gray_right_bottom += gray / 255.0;
			 }
			 
			 YCbCr.Y = p[x + 2];
			 if (is_bad_spot_yuv(YCbCr)) {
				 n_bad_spots++;
				 continue;
			 }
			 YUV422toRGB(&YCbCr, &Rgb);
			 cur_rgb565 = ((Rgb.red << 8) & 0xf800) | ((Rgb.green << 3) & 0x07e0) | ((Rgb.blue >> 3) & 0x001f);
       cur_line[x + 2] = (unsigned char)(cur_rgb565 >> 8);
  		 cur_line[x + 3] = (unsigned char)(cur_rgb565 & 0xff);
			 RGBtoHSV100(&Rgb, &Hsv);
			 avg_h += Hsv.H;
			 avg_s += Hsv.S;
			 avg_v += Hsv.V;
			 gray = (Rgb.red * 38 + Rgb.green * 75 + Rgb.blue * 15) >> 7;	
			 if (y < window_h / 2) {
				 if (x < window_w) average_gray_left_up += gray / 255.0;
				 else average_gray_right_up += gray / 255.0;
			 } else {
				 if (x < window_w) average_gray_left_bottom += gray / 255.0;
				 else average_gray_right_bottom += gray / 255.0;
			 }
		 }
		 LCD_ShowPicture(0, 120 + y, window_w, 2, cur_line);
		 p += n_image_line_bytes - start_x * 2;
	}
	if (n_bad_spots >= window_h * window_w / 2) {
		memset(vec, 0, sizeof(float) * 6);
		return 0;
	}
	avg_h /= (window_h * window_w - n_bad_spots);
	avg_s /= (window_h * window_w - n_bad_spots);
	avg_v /= (window_h * window_w - n_bad_spots);

	vec[0] = avg_h;
	vec[1] = 4 * average_gray_left_up / (window_h * window_w - n_bad_spots);
	vec[2] = 4 * average_gray_left_bottom / (window_h * window_w - n_bad_spots);
	vec[3] = 4 * average_gray_right_up / (window_h * window_w - n_bad_spots);
	vec[4] = 4 * average_gray_right_bottom / (window_h * window_w - n_bad_spots);
	vec[5] = avg_s;
	return 1;
}

int get_9_block_feature_vectors_of_image(float (* vec)[5]) {
	const int start_x = PEDESTRIAN_AREA_START_X, start_y = PEDESTRIAN_AREA_START_Y, end_x = PEDESTRIAN_AREA_END_X, end_y = PEDESTRIAN_AREA_END_Y;
	const int window_w = end_x - start_x + 1, window_h = end_y - start_y + 1, n_image_line_bytes = 640;
	int x, y, i, n_bad_spots = 0, j, block_x, block_y, block_index, sub_block_x_half, sub_block_y_half, sub_block_index;
	float gray;
	int block_n_total_pixel[9];
	u16 cur_rgb565;
	COLOR_RGB Rgb;
	COLOR_YCbCr YCbCr;
	COLOR_HSV Hsv;
	unsigned char * p = capture_image + n_image_line_bytes * start_y;	
	
	for (i = 0; i < 9; i++) {
		for (j = 0; j < 5; j++) {
			vec[i][j] = 0;
		}
	}
	
	block_n_total_pixel[0] = (CLOTHING_AREA_START_X - PEDESTRIAN_AREA_START_X) * (CLOTHING_AREA_START_Y - PEDESTRIAN_AREA_START_Y);
	block_n_total_pixel[1] = (CLOTHING_AREA_END_X - CLOTHING_AREA_START_X + 1) * (CLOTHING_AREA_START_Y - PEDESTRIAN_AREA_START_Y);
	block_n_total_pixel[2] = (PEDESTRIAN_AREA_END_X - CLOTHING_AREA_END_X) * (CLOTHING_AREA_START_Y - PEDESTRIAN_AREA_START_Y);
	block_n_total_pixel[3] = (CLOTHING_AREA_START_X - PEDESTRIAN_AREA_START_X) * (CLOTHING_AREA_END_Y - CLOTHING_AREA_START_Y + 1);
	block_n_total_pixel[4] = (CLOTHING_AREA_END_X - CLOTHING_AREA_START_X + 1) * (CLOTHING_AREA_END_Y - CLOTHING_AREA_START_Y + 1);
	block_n_total_pixel[5] = (PEDESTRIAN_AREA_END_X - CLOTHING_AREA_END_X) * (CLOTHING_AREA_END_Y - CLOTHING_AREA_START_Y + 1);
	block_n_total_pixel[6] = (CLOTHING_AREA_START_X - PEDESTRIAN_AREA_START_X) * (PEDESTRIAN_AREA_END_Y - CLOTHING_AREA_END_Y);
	block_n_total_pixel[7] = (CLOTHING_AREA_END_X - CLOTHING_AREA_START_X + 1) * (PEDESTRIAN_AREA_END_Y - CLOTHING_AREA_END_Y);
	block_n_total_pixel[8] = (PEDESTRIAN_AREA_END_X - CLOTHING_AREA_END_X) * (PEDESTRIAN_AREA_END_Y - CLOTHING_AREA_END_Y);
	
	for (y = 0; y < window_h; y++) {				 
		 p += start_x * 2; 
		 if (y < CLOTHING_AREA_START_Y - start_y) { // < 100
			 block_y = 0; 
			 if (y < ((CLOTHING_AREA_START_Y - PEDESTRIAN_AREA_START_Y) / 2)) sub_block_y_half = 0; // < 50
			 else sub_block_y_half = 1;
		 } else if (y > CLOTHING_AREA_END_Y - start_y) {
			 block_y = 2;
			 if ((y - (CLOTHING_AREA_END_Y + 1 - start_y)) < ((PEDESTRIAN_AREA_END_Y - CLOTHING_AREA_END_Y) / 2)) sub_block_y_half = 0;
			 else sub_block_y_half = 1;
		 } else {
			 block_y = 1;
			 if ((y - (CLOTHING_AREA_START_Y - start_y)) < ((CLOTHING_AREA_END_Y - CLOTHING_AREA_START_Y + 1) / 2)) sub_block_y_half = 0;
			 else sub_block_y_half = 1;
		 }
		 for(x = 0; x < window_w * 2; x += 4) {
			 
			 if ((x / 2) < CLOTHING_AREA_START_X - start_x) {
				 block_x = 0; 
				 if ((x / 2) < ((CLOTHING_AREA_START_X - PEDESTRIAN_AREA_START_X) / 2)) sub_block_x_half = 0;
				 else sub_block_x_half = 1;
			 } else if ((x / 2) > CLOTHING_AREA_END_X) {
				 block_x = 2;
				 if (((x / 2) - (CLOTHING_AREA_END_X + 1 - start_x)) < ((PEDESTRIAN_AREA_END_X - CLOTHING_AREA_END_X) / 2)) sub_block_x_half = 0;
				 else sub_block_x_half = 1;
			 } else {
				 block_x = 1;
				 if (((x / 2) - CLOTHING_AREA_START_X) < ((CLOTHING_AREA_END_X - CLOTHING_AREA_START_X + 1) / 2)) sub_block_x_half = 0;
				 else sub_block_x_half = 1;
			 }			 
			 block_index = block_y * 3 + block_x;
			 sub_block_index = sub_block_y_half * 2 + sub_block_x_half;
			 YCbCr.Y = p[x];
			 YCbCr.Cb = p[x + 1];
			 YCbCr.Cr = p[x + 3];
			 if (is_bad_spot_yuv(YCbCr)) {
				 n_bad_spots++;
				 block_n_total_pixel[block_index]--;
				 continue;
			 }
			 YUV422toRGB(&YCbCr, &Rgb);
			 RGBtoHSV100(&Rgb, &Hsv);
			 gray = (Rgb.red * 38 + Rgb.green * 75 + Rgb.blue * 15) >> 7;	
			 
			 vec[block_index][0] += Hsv.H;
			 vec[block_index][1 + sub_block_index] +=  gray / 255.0;
			 
			 YCbCr.Y = p[x + 2];
			 if (is_bad_spot_yuv(YCbCr)) {
				 n_bad_spots++;
				 block_n_total_pixel[block_index]--;
				 continue;
			 }
			 YUV422toRGB(&YCbCr, &Rgb);
			 RGBtoHSV100(&Rgb, &Hsv);
			 gray = (Rgb.red * 38 + Rgb.green * 75 + Rgb.blue * 15) >> 7;	
			 
			 vec[block_index][0] += Hsv.H;
			 vec[block_index][1 + sub_block_index] +=  gray / 255.0;
		 }	
		 p += n_image_line_bytes - start_x * 2;
	}
	if (n_bad_spots >= window_h * window_w / 2) {
		for (i = 0; i < 9; i++) {
			for (j = 0; j < 5; j++) {
					vec[i][j] = 0;
			}
		}
		return 0;
	}
	for (i = 0; i < 9; i++) {
		for (j = 0; j < 5; j++) {
			vec[i][j] /= (block_n_total_pixel[i] / 4);
		}
	}
	return 1;
}

int if_vectors_similar(float * vector1, float * vector2) {
	int i, sorted_indexes1[4] = {1, 2, 3, 4}, sorted_indexes2[4] = {1, 2, 3, 4};
	float vector1_copy[5], vector2_copy[5], sum_delta;
	
	for (i = 0; i < 4; i++) {
		if (fabs(vector1[i + 1] - vector2[i + 1]) > 0.25) return 0;
		sum_delta += fabs(vector1[i + 1] - vector2[i + 1]);
	}
	if (sum_delta < 0.1) return 1;
	
	if (fabs(vector1[5] - vector2[5]) > 20) return 0;	
	if (fabs(vector1[0] - vector2[0]) > 20) return 0;  
	if (fabs(vector1[5] - vector2[5]) <= 5 && fabs(vector1[0] - vector2[0]) <= 5) return 1;
	
	memcpy(vector1_copy, vector1, 5 * sizeof(float));
	memcpy(vector2_copy, vector2, 5 * sizeof(float));
  sort_array_and_indexes(vector1_copy, sorted_indexes1, 1, 4);
  sort_array_and_indexes(vector2_copy, sorted_indexes2, 1, 4);
	for (i = 0; i < 4; i++){
		if(sorted_indexes1[i] != sorted_indexes2[i]) return 0;
	}
	return 1;
}
int if_2d_vectors_similar(float (* vec1)[5], float (* vec2)[5]) {
	int i, j, sorted_indexes1[4], sorted_indexes2[4];
	float vec_copy[4], sum_delta = 0;
	for (i = 0; i < 9; i++) {
		for (j = 0; j < 4; j++) {
			if (fabs(vec1[i][j + 1] - vec2[i][j + 1]) > 0.2) 
				return 0;
			sum_delta += fabs(vec1[i][j + 1] - vec2[i][j + 1]);
		}
		if (sum_delta < 0.08) {
			sum_delta = 0;
			continue;
		}
		if (fabs(vec1[i][0] - vec2[i][0]) > 10) return 0;
		
		memcpy(vec_copy, vec1[i] + 1, 4 * sizeof(float));
		sort_array_and_indexes(vec_copy, sorted_indexes1, 0, 3);
		memcpy(vec_copy, vec2[i] + 1, 4 * sizeof(float));
		sort_array_and_indexes(vec_copy, sorted_indexes2, 0, 3);
		for (j = 0; j < 4; j++) {
			if (sorted_indexes1[j] != sorted_indexes2[j]) 
				return 0;
		}
	}	
	return 1;
}

//main functions
void analyse_image_and_change_light(void) {
	if (state_ledcontrol != STATE_LEDCONTROL_DO_NOTHING) return;
	if (clothing_area_state == STATE_CLOTHING_AREA_INIT) {
		clothing_area_init();
	}
	if (clothing_area_state == STATE_CLOTHING_AREA_DETECTING_COLOR) {
		clothing_area_detecting_color();
	}
	if ((!if_detect_clothing_change) && (!if_detect_pedestrian)) return;
	if (if_detect_clothing_change) {
		if (clothing_area_state == STATE_CLOTHING_AREA_FINDING_REF) {
			clothing_area_finding_ref();
			return;
		}
		if (clothing_area_state == STATE_CLOTHING_AREA_STABLE) {
			if (if_detect_pedestrian) {
				if (pedestrian_area_state == STATE_PEDESTRIAN_AREA_INIT) {
					pedestrian_area_init();
					return;
				}
				if (pedestrian_area_state == STATE_PEDESTRIAN_NONE) {
					pedestrian_area_none();
					if (pedestrian_area_state != STATE_PEDESTRIAN_NONE) return;
				}
				if (pedestrian_area_state == STATE_PEDESTRIAN_EXISTING) {
					pedestrian_area_existing();
					return;
				}
				if (pedestrian_area_state == STATE_PEDESTRIAN_AREA_WAITING_TO_BE_STABLE) {
					pedestrian_area_waiting_to_be_stable();
					return;
				}
			}
			clothing_area_stable();
		}		
	} else { // pedestrian only
		if (pedestrian_area_state == STATE_PEDESTRIAN_AREA_INIT) {
			pedestrian_area_init();
		}
		if (pedestrian_area_state == STATE_PEDESTRIAN_NONE) {
			pedestrian_area_none();
		}
		if (pedestrian_area_state == STATE_PEDESTRIAN_EXISTING) {
			pedestrian_area_existing();
		}
		if (pedestrian_area_state == STATE_PEDESTRIAN_AREA_WAITING_TO_BE_STABLE) {
			pedestrian_area_waiting_to_be_stable();
		}
	}
}
void clothing_area_init(void) {
	 int camera_flag = 0, change_flag = 0;
	 camera_flag = get_feature_vector_of_image(&clothing_area, clothing_feature_vector_current);
	 if (camera_flag == 0) {
		 if (state_camera == STATE_CAMERA_START_UP) {
			 camera_broken_frames++;
			 if (camera_broken_frames >= 2) {
				 camera_broken_frames = 0;
				 state_camera = STATE_CAMERA_START_UP_FAILED;
				 return;
			 }
		 }
	 } else {
		 camera_broken_frames = 0;
		 state_camera = STATE_CAMERA_WORKING;
	 }
   //todo add bad_frame process here
	 change_flag = if_vectors_similar(clothing_feature_vector_current, clothing_feature_vector_last_frame);
	 if (change_flag == 1) {
		 clothing_area_stable_frames++;
		 if (clothing_area_stable_frames >= 3) {					 
			 clothing_area_stable_frames = 0;					 
			 clothing_area_state = STATE_CLOTHING_AREA_DETECTING_COLOR;
			 return;
		 }
	 } else {
		 clothing_area_stable_frames = 0;
	 }
	 memcpy(clothing_feature_vector_last_frame, clothing_feature_vector_current, 6 * sizeof(float));	
}
void clothing_area_detecting_color(void) {
	 int camera_flag = 0, change_flag = 0;	
	 camera_flag = get_feature_vector_of_image(&clothing_area, clothing_feature_vector_current);
	 if (camera_flag == 0) {
		 camera_broken_frames++;
		 if (camera_broken_frames == 1) state_camera = STATE_CAMERA_OBSERVING;
		 if (state_camera == STATE_CAMERA_OBSERVING) {			 
			 if (camera_broken_frames >= 2) {
				 camera_broken_frames = 0;
				 state_camera = STATE_CAMERA_BROKEN;
				 return;
			 }
		 }
	 } else {
		 camera_broken_frames = 0;
		 state_camera = STATE_CAMERA_WORKING;
	 }
	 change_flag = if_vectors_similar(clothing_feature_vector_current, clothing_feature_vector_last_frame);
	 if (change_flag == 1) {
		 clothing_area_stable_frames++;
		 if (clothing_area_stable_frames >= 3) {
			 clothing_area_stable_frames = 0;
			 spectrum_index = get_spectrum_index_of_image_3(&clothing_area);
			 if (spectrum_index != SPECTRUM_UNDEFINED) {
				 clothing_area_state = STATE_CLOTHING_AREA_FINDING_REF;
				 state_ledcontrol = STATE_LEDCONTROL_RECIPE_LOW;
				 return;
			 }
		 }				 
	 }
	 else {
		 clothing_area_stable_frames = 0;
	 }
	 memcpy(clothing_feature_vector_last_frame, clothing_feature_vector_current, 6 * sizeof(float));
}
void clothing_area_finding_ref(void) {
	 int camera_flag = 0, change_flag = 0;
	 camera_flag = get_feature_vector_of_image(&clothing_area, clothing_feature_vector_current);
	 if (camera_flag == 0) {
		 camera_broken_frames++;
		 if (camera_broken_frames == 1) state_camera = STATE_CAMERA_OBSERVING;
		 if (state_camera == STATE_CAMERA_OBSERVING) {			 
			 if (camera_broken_frames >= 2) {
				 camera_broken_frames = 0;
				 state_camera = STATE_CAMERA_BROKEN;
				 return;
			 }
		 }
	 } else {
		 camera_broken_frames = 0;
		 state_camera = STATE_CAMERA_WORKING;
	 }
	
	 change_flag = if_vectors_similar(clothing_feature_vector_current, clothing_feature_vector_last_frame);
	 if (change_flag == 1) {
		 clothing_area_stable_frames++;
		 if (clothing_area_stable_frames >= 5) {
			 clothing_area_stable_frames = 0;
			 memcpy(clothing_feature_vector_ref, clothing_feature_vector_current, 6 * sizeof(float));
			 clothing_area_state = STATE_CLOTHING_AREA_STABLE;
			 return;
		 }
	 } else {
		 if (clothing_area_stable_frames > 1) {
			 clothing_area_stable_frames = 0;
			 clothing_area_state = STATE_CLOTHING_AREA_DETECTING_COLOR;
			 state_ledcontrol = STATE_LEDCONTROL_MINIMUM_COLD_WHITE;
			 return;
		 }
	 }
	 memcpy(clothing_feature_vector_last_frame, clothing_feature_vector_current, 6 * sizeof(float));
}
void clothing_area_stable(void) {
	int camera_flag, change_flag = 0;
	camera_flag = get_feature_vector_of_image(&clothing_area, clothing_feature_vector_current);
	if (camera_flag == 0) {
		 camera_broken_frames++;
		 if (camera_broken_frames == 1) state_camera = STATE_CAMERA_OBSERVING;
		 if (state_camera == STATE_CAMERA_OBSERVING) {			 
			 if (camera_broken_frames >= 2) {
				 camera_broken_frames = 0;
				 state_camera = STATE_CAMERA_BROKEN;
				 return;
			 }
		 }
	 } else {
		 camera_broken_frames = 0;
		 state_camera = STATE_CAMERA_WORKING;
	 }
	change_flag = if_vectors_similar(clothing_feature_vector_current, clothing_feature_vector_ref);
	if (change_flag == 0) {
		clothing_area_stable_frames = 0;
		memset(clothing_feature_vector_ref, 0, 6 * sizeof(float));
		memset(clothing_feature_vector_last_frame, 0, 6 * sizeof(float));
		clothing_area_state = STATE_CLOTHING_AREA_DETECTING_COLOR;
		state_ledcontrol = STATE_LEDCONTROL_MINIMUM_COLD_WHITE;
		return;
	}	
}
void pedestrian_area_init(void) {
	int camera_flag, change_flag = 0;
	camera_flag = get_9_block_feature_vectors_of_image(feature_vec_current);
	
	if (camera_flag == 0) {
		 camera_broken_frames++;
		 if (camera_broken_frames == 1) state_camera = STATE_CAMERA_OBSERVING;
		 if (state_camera == STATE_CAMERA_OBSERVING) {			 
			 if (camera_broken_frames >= 2) {
				 camera_broken_frames = 0;
				 state_camera = STATE_CAMERA_BROKEN;
				 return;
			 }
		 }
	 } else {
		 camera_broken_frames = 0;
		 state_camera = STATE_CAMERA_WORKING;
	 }
	
	change_flag = if_2d_vectors_similar(feature_vec_current, feature_vec_last_frame);
	if (change_flag == 1) {
		pedestrian_area_stable_frames++;
		if (pedestrian_area_stable_frames >= 3) {
			pedestrian_area_stable_frames = 0;
			pedestrian_area_state = STATE_PEDESTRIAN_NONE;
			return;
		}
	} else {
		pedestrian_area_stable_frames = 0;
	}
	memcpy(feature_vec_last_frame, feature_vec_current, 45 * sizeof(float));
}
void pedestrian_area_none(void) {
	int camera_flag, change_flag = 0;
	camera_flag = get_9_block_feature_vectors_of_image(feature_vec_current);
	
	if (camera_flag == 0) {
		 camera_broken_frames++;
		 if (camera_broken_frames == 1) state_camera = STATE_CAMERA_OBSERVING;
		 if (state_camera == STATE_CAMERA_OBSERVING) {			 
			 if (camera_broken_frames >= 2) {
				 camera_broken_frames = 0;
				 state_camera = STATE_CAMERA_BROKEN;
				 return;
			 }
		 }
	 } else {
		 camera_broken_frames = 0;
		 state_camera = STATE_CAMERA_WORKING;
	 }
	
	change_flag = if_2d_vectors_similar(feature_vec_current, feature_vec_last_frame);
	if (change_flag == 0) {
		pedestrian_area_stable_frames = 0;
		pedestrian_area_state = STATE_PEDESTRIAN_EXISTING;
		state_ledcontrol = STATE_LEDCONTROL_RECIPE_NORMAL;
		return;
	}
	memcpy(feature_vec_last_frame, feature_vec_current, 45 * sizeof(float));
}
void pedestrian_area_existing(void) {
	int camera_flag, change_flag = 0;
	camera_flag = get_9_block_feature_vectors_of_image(feature_vec_current);
	
	if (camera_flag == 0) {
		 camera_broken_frames++;
		 if (camera_broken_frames == 1) state_camera = STATE_CAMERA_OBSERVING;
		 if (state_camera == STATE_CAMERA_OBSERVING) {			 
			 if (camera_broken_frames >= 2) {
				 camera_broken_frames = 0;
				 state_camera = STATE_CAMERA_BROKEN;
				 return;
			 }
		 }
	 } else {
		 camera_broken_frames = 0;
		 state_camera = STATE_CAMERA_WORKING;
	 }
	
	change_flag = if_2d_vectors_similar(feature_vec_current, feature_vec_last_frame);
	if (change_flag == 1) {
		pedestrian_area_stable_frames++;
		if (pedestrian_area_stable_frames >= 2) {
			pedestrian_area_stable_frames = 0;
			pedestrian_area_state = STATE_PEDESTRIAN_AREA_WAITING_TO_BE_STABLE;
			state_ledcontrol = STATE_LEDCONTROL_RECIPE_LOW;
			return;
		}
	}	else {
		pedestrian_area_stable_frames = 0;
	}
	memcpy(feature_vec_last_frame, feature_vec_current, 45 * sizeof(float));
}
void pedestrian_area_waiting_to_be_stable(void) {
	int camera_flag, change_flag = 0;
	camera_flag = get_9_block_feature_vectors_of_image(feature_vec_current);
	
	if (camera_flag == 0) {
		 camera_broken_frames++;
		 if (camera_broken_frames == 1) state_camera = STATE_CAMERA_OBSERVING;
		 if (state_camera == STATE_CAMERA_OBSERVING) {			 
			 if (camera_broken_frames >= 2) {
				 camera_broken_frames = 0;
				 state_camera = STATE_CAMERA_BROKEN;
				 return;
			 }
		 }
	 } else {
		 camera_broken_frames = 0;
		 state_camera = STATE_CAMERA_WORKING;
	 }
	 
	change_flag = if_2d_vectors_similar(feature_vec_current, feature_vec_last_frame);
	if (change_flag == 1) {
		pedestrian_area_stable_frames++;
		if (pedestrian_area_stable_frames >= 2) {
			pedestrian_area_stable_frames = 0;
			pedestrian_area_state = STATE_PEDESTRIAN_NONE;
			return;
		}
	}	else {
		if (pedestrian_area_stable_frames > 0) {
			pedestrian_area_stable_frames = 0;
			pedestrian_area_state = STATE_PEDESTRIAN_EXISTING;
			state_ledcontrol = STATE_LEDCONTROL_RECIPE_NORMAL;
			return;
		}
		pedestrian_area_stable_frames = 0;					
	}
	memcpy(feature_vec_last_frame, feature_vec_current, 45 * sizeof(float));
}
