#ifndef __EMBEDDEDFLASH_h
#define __EMBEDDEDFLASH_h
//#include "includes.h"
#include "gd32f20x.h"
#ifndef WB_NULL
#define WB_NULL 0
#endif

#define INT8U	  unsigned char
#define INT16U	unsigned short
#define INT32U	unsigned int
#define INT8S   signed  char  

//#define  GD32_SECTOR_SIZE  2048
//#define  GD32_FMC_BASE     0x08000000
//#define  GD32_FMC_SIZE  512

#define  GD32_BANK0_SECTOR_SIZE  2048
#define  GD32_BANK1_SECTOR_SIZE  4096
#define  GD32_FMC_SIZE  1024

//FLASH��ʼ��ַ
//bank0
#define GD32_FMC_BASE                    0x08000000 	      //GD32 FLASH����ʼ��ַ
//#define PAGE_SIZE  (unsigned int)0x800  /* Page size = 2KByte */

#define FLASH_APP1_ADDR		               0x0800A000        //�û������ִ�е�ַ
#define FLASH_APP1_LAST_PAGE_ADDR        0x0803B800
#define FLASH_APP1_END_ADDR              0x0803BFFF

#define FLASH_APP1_DATA_ADDR             0x0803C000        //�洢���յ����û�����ĵ�ַ
#define FLASH_APP1_DATA_END_ADDR         0x0803CFFF


//bank1
#define FLASH_DOWNLOAD_APP1_ADDR		          0x08080000  //�û������ִ�е�ַ
#define FLASH_DOWNLOAD_APP1_LAST_PAGE_ADDR		0x080B1000
#define FLASH_DOWNLOAD_APP1_END_ADDR		      0x080B1FFF

#define FLASH_DOWNLOAD_APP1_ADDR_SCT 204

#define FLASH_DOWNLOAD_DATA1_ADDR             0x080B2000  //�û����ݵ�ִ�е�ַ
#define FLASH_DOWNLOAD_DATA1_END_ADDR         0x080B2FFF

#define FLASH_DOWNLOAD_APP2_ADDR              0x080B3000  //�洢���յ����û�����ĵ�ַ
#define FLASH_DOWNLOAD_APP2_LAST_PAGE_ADDR		0x080E4000
#define FLASH_DOWNLOAD_APP2_END_ADDR		      0x080E4FFF

#define FLASH_DOWNLOAD_DATA2_ADDR             0x080E5000  //�洢���յ����û�����ĵ�ַ
#define FLASH_DOWNLOAD_DATA2_END_ADDR         0x080E5FFF



#define APP_MAX_VERSION          999999                       //���汾��99.99.99(0x000F423F)

__packed  typedef struct FIRMWAREUPGRADE_INFO_DEF
{
	unsigned int flag;
	unsigned int writen;
	unsigned int trycnt;//��������д��ͳ�������������7�Σ���8��д����һ������ĳ���
	unsigned int success;
	unsigned char ver[4];
	unsigned int  bin_len;
}FIRMUP_FLASHINFO;


__packed typedef struct EMBATTR_DEFINE
{

	INT8U	LEDID_attri[10];
	INT8U	ETHMAC[8];
	INT16U	BootUpdateFlag;
}EMBEDATTR;//����Ϊż�����ֽ�

/* �汾���ƿ� */
typedef struct{
    INT32U version;
    INT32U totalSize;
}BootFileInfo_T;

/* ���п��ƿ� */
typedef struct QUEUE{
    unsigned char* parr;
    unsigned int head;
    unsigned int tail;  //ָ���´ν���ʱ�洢��λ��
    unsigned char isFull;
    unsigned int size;  //ʵ����Ч����Ϊsize-1
    unsigned int length;
}Queue_T;


/* boot�ļ����ƿ� */
typedef struct{
    u8 header[2];
    u16 len;
    u32 pos;            //��ǰ��ƫ�Ƶ�ַ��һ����FLASH_PAGE_SIZE����������
    Queue_T* pq;
    u8 *pdata; 
    u32 crc;
    u32 totalSize;      //�ļ��ܴ�С
    u16 pageNumber;     //��ҳ��
    u32 version;        //�汾��Ϣ
}BootFile_T;

extern  EMBEDATTR	EmbedAttr;








extern void EmbedFlashAttriGet(void);
extern void InitEmbedAttr(void);
extern unsigned char WriteAttrToEmbedFlash(unsigned char *buf);
unsigned char Flash_Write_Byte(INT32U uaddr,INT32U *data16,unsigned short int datanum);
uint8_t boot_check_flash_begin(uint32_t appxaddr);
fmc_state_enum boot_write_info(uint32_t flash_addr, BootFile_T boot_file);

void GD32FMC_Read(uint32_t ReadAddr,uint32_t *pBuffer,uint16_t NumToRead);
void GD32FMC_Half_Read(uint32_t ReadAddr,uint16_t *pBuffer,uint16_t NumToRead);

void GD32FMC_Write(uint32_t WriteAddr,uint32_t *pBuffer ,uint16_t NumToWrite);
uint8_t GD32FMC_Half_Write(uint32_t WriteAddr,uint16_t *pBuffer ,uint16_t NumToWrite);
uint8_t GD32FMC_Bank1_Write(uint32_t WriteAddr,uint32_t *pBuffer ,uint16_t NumToWrite);

void GD32FLASH_Erase(uint32_t WriteAddr,uint16_t NumToWrite);
void GD32FMC_Write_Nocheck(uint32_t WriteAddr,uint32_t *pBuffer,uint16_t NumToWrite);
uint32_t GD32FMC_ReadWord(uint32_t addr);
void GD32FMC_HalfRead(uint32_t ReadAddr,uint16_t *pBuffer,uint16_t NumToRead); 
uint32_t WriteFirmToEmbeddedFlashBank1(uint32_t WriteAddr,uint32_t *pBuffer,uint32_t len);
int8_t FLASH_If_Erase(uint32_t StartSector,uint32_t EndSector);
void FLASH_If_Write(__IO uint32_t* FlashAddress, uint32_t* Data ,uint16_t DataLength,uint32_t FlashEndAddr);

#endif
