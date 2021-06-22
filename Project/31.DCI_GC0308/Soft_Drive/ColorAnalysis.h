#ifndef COLOR_ANALYSIS_H
#define COLOR_ANALYSIS_H

#define CLOTHING_AREA_START_X 80
#define CLOTHING_AREA_START_Y 110
#define CLOTHING_AREA_END_X 159
#define CLOTHING_AREA_END_Y 209

#define PEDESTRIAN_AREA_START_X 0
#define PEDESTRIAN_AREA_START_Y 10
#define PEDESTRIAN_AREA_END_X 239
#define PEDESTRIAN_AREA_END_Y 309

typedef struct{
    unsigned int X_Start;              
    unsigned int X_End;
	  unsigned int Y_Start;              
    unsigned int Y_End;
}IMAGE_AREA;

typedef enum {		
	  SPECTRUM_ROSEWOOD           = 0,   //0
	  SPECTRUM_CHAMPAGNE          = 1,   //1
	  SPECTRUM_COFFEE             = 2,   //2
	  SPECTRUM_WARM_WHITE         = 3,   //3
	  SPECTRUM_RED                = 4,   //4
	  SPECTRUM_ORANGE             = 5,   //5
	  SPECTRUM_YELLOW             = 6,   //6
	  SPECTRUM_YELLOW_GREEN       = 7,   //7
	  SPECTRUM_GREEN              = 8,   //8
	  SPECTRUM_GREEN_CYAN         = 9,   //9
	  SPECTRUM_CYAN               = 10,  //10
	  SPECTRUM_BLUE               = 11,  //11
	  SPECTRUM_PURPLE             = 12,  //12
	  SPECTRUM_PINK	              = 13,  //13
	  SPECTRUM_LIGHT_RED          = 14,  //14
	  SPECTRUM_LIGHT_ORANGE       = 15,  //15                       	
	  SPECTRUM_LIGHT_YELLOW       = 16,  //16
		SPECTRUM_LIGHT_YELLOW_GREEN = 17,  //17
		SPECTRUM_LIGHT_GREEN        = 18,  //18
		SPECTRUM_LIGHT_GREEN_CYAN   = 19,  //19
		SPECTRUM_LIGHT_CYAN         = 20,  //20
		SPECTRUM_LIGHT_BLUE         = 21,  //21
	  SPECTRUM_LIGHT_PURPLE       = 22,  //22
	  SPECTRUM_LIGHT_PINK	        = 23,  //23
	  SPECTRUM_BLACK              = 24,  //24
	  SPECTRUM_WHITE              = 25,  //25
	  SPECTRUM_3000K              = 26,  //26
	  SPECTRUM_3500K              = 27,  //27
	  SPECTRUM_4000K              = 28,  //28
	  SPECTRUM_FULL               = 29,  //29
	  SPECTRUM_UNDEFINED,
}Spectrum_Index;

typedef enum {	  
		COLOR_ROSEWOOD              ,   //0
		COLOR_CHAMPAGNE             ,   //1
	  COLOR_COFFEE                ,   //2
	  COLOR_WARM_WHITE            ,   //3
	  COLOR_RED                   ,   //4
	  COLOR_ORANGE                ,   //5
		COLOR_YELLOW                ,   //6
		COLOR_YELLOW_GREEN          ,   //7
		COLOR_GREEN                 ,   //8
		COLOR_GREEN_CYAN            ,   //9
	  COLOR_CYAN                  ,  //10
		COLOR_BLUE                  ,  //11
		COLOR_PURPLE                ,  //12
		COLOR_PINK	                ,  //13
		COLOR_LIGHT_RED             ,  //14
		COLOR_LIGHT_ORANGE          ,  //15
		COLOR_LIGHT_YELLOW          ,  //16
		COLOR_LIGHT_YELLOW_GREEN    ,  //17
		COLOR_LIGHT_GREEN           ,  //18
		COLOR_LIGHT_GREEN_CYAN      ,  //19
		COLOR_LIGHT_CYAN            ,  //20
		COLOR_LIGHT_BLUE            ,  //21
		COLOR_LIGHT_PURPLE          ,  //22
		COLOR_LIGHT_PINK	          ,  //23
		COLOR_BLACK                 ,  //24
		COLOR_WHITE                 ,  //25
	  COLOR_UNDEFINED,   
}Color_Index; 

typedef enum {
	  COLOR_BLACK_S,
		COLOR_WHITE_S,
		COLOR_RED_S,
		COLOR_ORANGE_S,
		COLOR_YELLOW_S,
		COLOR_GREEN_S,
		COLOR_CYAN_S,
		COLOR_BLUE_S,
		COLOR_PURPLE_S,
}Color_Index_Simplified;


typedef struct{
    unsigned char  red;  
    unsigned char  green;
    unsigned char  blue; 
}COLOR_RGB;

typedef struct{
    unsigned short H; 
    unsigned char S;  
    unsigned char V;
}COLOR_HSV;

typedef struct{
	  unsigned char Y;
		unsigned char Cb;
		unsigned char Cr;
}COLOR_YCbCr;

typedef enum {
		STATE_CLOTHING_AREA_INIT = 0,
		STATE_CLOTHING_AREA_DETECTING_COLOR,
		STATE_CLOTHING_AREA_FINDING_REF,
		STATE_CLOTHING_AREA_STABLE,
		STATE_CLOTHING_AREA_WAITING_TO_BE_STABLE,
		STATE_CLOTHING_AREA_UNDEFINED,
}State_Clothing_Area;

typedef enum {
		STATE_PEDESTRIAN_AREA_INIT = 0,
		STATE_PEDESTRIAN_NONE,
		STATE_PEDESTRIAN_EXISTING,
		STATE_PEDESTRIAN_AREA_WAITING_TO_BE_STABLE,
		STATE_PEDESTRIAN_AREA_UNDEFINED,
}State_Pedestrian_Area;


//high temparature camera test
void camera_test_init(void);
void high_temperature_camera_test(void);
void show_image(void);
void write_image_data_to_memory(void);
void show_flash_image_data(void);

//tool functions
int is_bad_spot(unsigned short spot);
int is_bad_spot_yuv(COLOR_YCbCr YCbCr);
void RGBtoHSV100(const COLOR_RGB *Rgb, COLOR_HSV *Hsv);
void YUV422toRGB(const COLOR_YCbCr *YCbCr, COLOR_RGB *Rgb);
unsigned int read_u16(unsigned char *buf);
int if_color_warm(Color_Index color);
int if_color_neutral(Color_Index color);
int if_color_cold(Color_Index color);
void sort_array_and_indexes(float * vector, int * indexes, int start, int end);
void sort_array_and_indexes_int(int * array, int * indexes, int start, int end, int if_set_indexes);

//camera test
void show_yuv422_image();

//main functions spectrum
void squeeze_color_dict(int * color_dict);
int count_color_dict26_of_image(const IMAGE_AREA * area, int * color_dict);
int get_main_color_of_image(const IMAGE_AREA * area, int * color_dict, int * sorted_indexes);
Spectrum_Index get_2_base_color_spectrum_index(Color_Index c_max, Color_Index c_min);
Spectrum_Index get_spectrum_index_of_image_3(const IMAGE_AREA * window);

//main functions feature
int get_feature_vector_of_image(const IMAGE_AREA * area, float * vector);
int get_9_block_feature_vectors_of_image(float (* vec)[5]);
int if_vectors_similar(float * vector1, float * vector2);
int if_2d_vectors_similar(float (* vec1)[5], float (* vec2)[5]);


//main functions
void analyse_image_and_change_light(void);
void clothing_area_init(void);
void clothing_area_detecting_color(void);
void clothing_area_finding_ref(void);
void clothing_area_stable(void);
void pedestrian_area_init(void);
void pedestrian_area_none(void);
void pedestrian_area_existing(void);
void pedestrian_area_waiting_to_be_stable(void);

#endif
