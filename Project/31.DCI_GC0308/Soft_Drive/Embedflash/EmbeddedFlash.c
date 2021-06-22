#include "EmbeddedFlash.h"
#include "string.h"
#include "gd32f20x_fmc.h"
//#include "includes.h"
#include "systick.h"

const INT8U		VERSION[8]			= {'k','i','n','g','l','u','m','i'};
const INT8U		LEDID_DF[10]			= {'0','0','0','0','6','8','9','0','0','0'};

#define Base_Addr 0x0870f800   //第255页  bank1  大小两Kb
#define EEP_MAX 256     //256字节的存储空间   一方面减轻ram压力  另一方面减少flash操作时间

EMBEDATTR	EmbedAttr;
BootFileInfo_T  app_info,download_info;
void EmbedFlashAttriGet(void);
void InitEmbedAttr(void);
INT8U WriteAttrToEmbedFlash(INT8U *buf);
uint16_t  GD32_BANK0_BUF[GD32_BANK0_SECTOR_SIZE/2];
uint32_t  GD32_BANK1_BUF[GD32_BANK1_SECTOR_SIZE/4];


void EmbedFlashAttriGet(void){

	INT8U i;
	
	INT16U *temp;

	temp = (INT16U*)&EmbedAttr;

	for(i=0;i<sizeof(EMBEDATTR)/2;i++){
		*(temp+i) = (*(__IO INT16U*)(FLASH_APP1_DATA_ADDR+i*2));
	}
}
void InitEmbedAttr(void)
{

 
 	memcpy((void*)EmbedAttr.ETHMAC,(void*)VERSION,sizeof(VERSION));
	memcpy((void*)EmbedAttr.LEDID_attri,(void*)LEDID_DF,sizeof(LEDID_DF));
	WriteAttrToEmbedFlash((INT8U *)&EmbedAttr);

}

//1成功，0失败
INT8U WriteAttrToEmbedFlash(INT8U *buf)
{
	INT8U i;
	INT16U *data;	
	fmc_state_enum FlashStatus;
	fmc_bank0_unlock();
	if (fmc_page_erase(FLASH_APP1_DATA_ADDR) != FMC_READY){
		fmc_bank0_lock();
		return 0;
	}	 
	for(i=0;i<sizeof(EMBEDATTR)/2;i++)
	{
	      data = (INT16U*)(buf+i*2);
		    FlashStatus = fmc_halfword_program(FLASH_APP1_DATA_ADDR+i*2, *data);
	      /* If program operation was failed, a Flash error code is returned */
	      if (FlashStatus != FMC_READY)
	      {
	        fmc_bank0_lock();
			return 0;
	      }
   }
   fmc_bank0_lock();
   return 1;
}




/***********************************************************************
函  数  名：Flash_Read_HalfWord
功       能：flash半字读取
输入参数：相对地址 0-128
返回参数：读出来的值
说      明：
***********************************************************************/
unsigned short int Flash_Read_HalfWord(unsigned  faddr)
{
        return *(unsigned short int*)(faddr*2+FLASH_BASE);//返回输入地址中的值
}
/***********************************************************************
函  数  名：Flash_Read
功       能：读取指定长度的数据
输入参数：相对地址 0-128   读取使用的数组 16位   要读取的长度
返回参数：
说      明：
***********************************************************************/
void Flash_Read(unsigned int addr,unsigned short int *redata,unsigned short int renum)
{
          unsigned short int i;
          for(i = 0;i<renum;i++)
          {
                 *(redata+i*2) = Flash_Read_HalfWord(addr+i);//开始读
          }
}
/***********************************************************************
函  数 名：Flash_Write_Byte
功      能：flash半字写
输入参数：相对地址 0-128，16位的数据指针    写入的数量
返回参数：1成功  2地址超出设置范围
说      明：
***********************************************************************/
unsigned char Flash_Write_Byte(INT32U uaddr,INT32U *data16,unsigned short int datanum)
{
          unsigned short int i;
          unsigned char Nclear = 0;
          unsigned short int tempdata[EEP_MAX/2];
          if(uaddr>EEP_MAX/2)return 2;//输入超出设置范围

          fmc_bank0_unlock();//Flash解锁
          fmc_flag_clear(FMC_FLAG_BANK0_END | FMC_FLAG_BANK0_WPERR | FMC_FLAG_BANK0_PGERR);
 
          for(i = 0;i<datanum;i++)
          {
              if(Flash_Read_HalfWord(uaddr+i)!=0xffff)Nclear = 1;//需要清除flash
          }
          if(Nclear == 1)
          {
							for(i = 0;i<EEP_MAX/2;i++)//读出数据
							{
									tempdata[i]=Flash_Read_HalfWord(uaddr+i);
							}
							for(i = 0;i<datanum;i++)
							{
									tempdata[uaddr+i] = *(data16+i*2);//写入数据到缓存数组
							}
							fmc_page_erase(Base_Addr);
							for(i = 0;i<EEP_MAX/2;i++)
							{
									fmc_halfword_program(Base_Addr+i*2,tempdata[i]);//整个区域写入
							}
          }
          else
          {
                for(i = 0;i<datanum;i++)
                {
                         fmc_halfword_program(Base_Addr+(uaddr+i)*2,*(data16+i*2));//写入
                }                                
          }
          fmc_bank0_lock();//Flash上锁
          return 1;
}





//读取制定地址的字  32位数据
//addr :指定读取的地址  必须是4 的倍数
//返回值：对应的数据
uint32_t GD32FMC_ReadWord(uint32_t addr)
{
	return *(vu32*)addr;
}

uint32_t GD32FMC_ReadHalfWord(uint32_t addr)
{
	return *(vu16*)addr;
}


//不检查地写入
//WriteAddr:起始地址
//pBuffer:数据指针
//NumToWrite:字   32位数据
void GD32FMC_Write_Nocheck(uint32_t WriteAddr,uint32_t *pBuffer,uint16_t NumToWrite)
{
	uint16_t i;
	for(i=0;i<NumToWrite;i++)
	{
		  fmc_word_program(WriteAddr,pBuffer[i]);
		  WriteAddr+=4;//地址自增4   32位数据（字）	
	}
	
}


//不检查地写入
//WriteAddr:起始地址
//pBuffer:数据指针
//NumToWrite:字   32位数据
void GD32FMC_Half_Write_Nocheck(uint32_t WriteAddr,uint16_t *pBuffer,uint16_t NumToWrite)
{
	uint16_t i;
	for(i=0;i<NumToWrite;i++)
	{
		  fmc_halfword_program(WriteAddr,pBuffer[i]);
		  WriteAddr+=2;//地址自增4   32位数据（字）	
	}
	
}

//WriteAddr:起始地址   4的倍数
//pBuffer:数据指针
//NumToWrite:要写入数据的个数


void GD32FMC_Write(uint32_t WriteAddr,uint32_t *pBuffer ,uint16_t NumToWrite)
{
	uint32_t secpos;//
	uint16_t secoff;//
	uint16_t secremain;//
	uint16_t i;
	uint32_t offaddr;//
	if(WriteAddr<GD32_FMC_BASE||(WriteAddr>=(GD32_FMC_BASE+512*GD32_BANK0_SECTOR_SIZE))) return;//检查地址的合法性
	fmc_bank0_unlock();//Flash解锁
	offaddr=WriteAddr-GD32_FMC_BASE;//实际偏移地址
	secpos=offaddr/GD32_BANK0_SECTOR_SIZE;//扇区地址
	secoff=(offaddr%GD32_BANK0_SECTOR_SIZE)/4;//
	secremain=GD32_BANK0_SECTOR_SIZE/4-secoff;//扇区剩余空间的大小
	if(NumToWrite<=secremain)  secremain=NumToWrite;//不大于该扇区的范围   
	
	while(1)
	{
		GD32FMC_Read(secpos*GD32_BANK0_SECTOR_SIZE+GD32_FMC_BASE,GD32_BANK1_BUF,GD32_BANK0_SECTOR_SIZE/4);//读取该扇区的数据
	  for(i=0;i<secremain;i++)//
		{
		 if(GD32_BANK1_BUF[secoff+i]!=0XFFFFFFFF) break;//
		}
		if(i<secremain)//需要擦除
		{

			fmc_page_erase(secpos*GD32_BANK0_SECTOR_SIZE+GD32_FMC_BASE);//擦除扇区
			for(i=0;i<secremain;i++)//复制要写入的数据
			{
			  GD32_BANK1_BUF[secoff+i]=pBuffer[i];
			}
			GD32FMC_Write_Nocheck(secpos*GD32_BANK0_SECTOR_SIZE+GD32_FMC_BASE,GD32_BANK1_BUF,GD32_BANK0_SECTOR_SIZE/4);//写入数据到扇区
		}
		else
			GD32FMC_Write_Nocheck(WriteAddr,pBuffer,secremain);//
		if(NumToWrite==secremain) break;
		else
		{
		  secpos++;
			secoff=0;
			pBuffer+=secremain;//指针偏移
			WriteAddr+=secremain;//写指针偏移
			NumToWrite-=secremain;//
			
			if(NumToWrite>(GD32_BANK0_SECTOR_SIZE/4))  secremain=GD32_BANK0_SECTOR_SIZE/4;//
			else
				secremain=NumToWrite;//
		}
		fmc_bank0_lock();//Flash上锁
	}
}


uint8_t GD32FMC_Half_Write(uint32_t WriteAddr,uint16_t *pBuffer ,uint16_t NumToWrite)
{
	uint32_t secpos;//
	uint16_t secoff;//
	uint16_t secremain;//
	uint16_t i;
	uint32_t offaddr;//
	if(WriteAddr<GD32_FMC_BASE||(WriteAddr>=(GD32_FMC_BASE+512*GD32_BANK0_SECTOR_SIZE))) //检查地址的合法性
		return 0;
	fmc_bank0_unlock();//Flash解锁
	offaddr=WriteAddr-GD32_FMC_BASE;//实际偏移地址
	secpos=offaddr/GD32_BANK0_SECTOR_SIZE;//扇区地址
	secoff=(offaddr%GD32_BANK0_SECTOR_SIZE)/2;//
	secremain=GD32_BANK0_SECTOR_SIZE/2-secoff;//扇区剩余空间的大小
	if(NumToWrite<=secremain)  
		secremain=NumToWrite;//不大于该扇区的范围   
	while(1)
	{
		GD32FMC_Half_Read(secpos*GD32_BANK0_SECTOR_SIZE+GD32_FMC_BASE,GD32_BANK0_BUF,GD32_BANK0_SECTOR_SIZE/2);//读取该扇区的数据
	  for(i=0;i<secremain;i++)//
		{
		 if(GD32_BANK0_BUF[secoff+i]!=0XFFFF) break;//
		}
		if(i<secremain)//需要擦除
		{

			fmc_page_erase(secpos*GD32_BANK0_SECTOR_SIZE+GD32_FMC_BASE);//擦除扇区
			for(i=0;i<secremain;i++)//复制要写入的数据
			{
			  GD32_BANK0_BUF[secoff+i]=pBuffer[i];
			}
			GD32FMC_Half_Write_Nocheck(secpos*GD32_BANK0_SECTOR_SIZE+GD32_FMC_BASE,GD32_BANK0_BUF,GD32_BANK0_SECTOR_SIZE/2);//写入数据到扇区
		}
		else
			GD32FMC_Half_Write_Nocheck(WriteAddr,pBuffer,secremain);//
		if(NumToWrite==secremain) break;
		else
		{
		  secpos++;
			secoff=0;
			pBuffer+=secremain;//指针偏移
			WriteAddr+=secremain;//写指针偏移
			NumToWrite-=secremain;//
			
			if(NumToWrite>(GD32_BANK0_SECTOR_SIZE/2))  
				secremain=GD32_BANK0_SECTOR_SIZE/2;//
			else
				secremain=NumToWrite;//
		}
		fmc_bank0_lock();//Flash上锁
	}
	return 1;
}


uint8_t GD32FMC_Bank1_Write(uint32_t WriteAddr,uint32_t *pBuffer ,uint16_t NumToWrite)
{
	uint32_t secpos;//
	uint16_t secoff;//
	uint16_t secremain;//
	uint32_t offaddr;//
	uint16_t i;

	if(WriteAddr<(GD32_FMC_BASE+512*GD32_FMC_SIZE)||(WriteAddr>=(FLASH_DOWNLOAD_APP1_ADDR+512*GD32_FMC_SIZE))) //检查地址的合法性
		return 0;
	fmc_bank1_unlock();//Flash解锁
	offaddr=WriteAddr-GD32_FMC_BASE;//实际偏移地址
	secpos=offaddr/GD32_BANK1_SECTOR_SIZE;//扇区地址
	secoff=(offaddr%GD32_BANK1_SECTOR_SIZE)/4;//
	secremain=GD32_BANK1_SECTOR_SIZE/4-secoff;//扇区剩余空间的大小
	if(NumToWrite<=secremain)
		secremain = NumToWrite;
	while(1)
	{
		GD32FMC_Read(secpos*GD32_BANK1_SECTOR_SIZE+GD32_FMC_BASE,GD32_BANK1_BUF,GD32_BANK1_SECTOR_SIZE/4);//读取该扇区的数据
	  for(i=0;i<secremain;i++)
		{
		 if(GD32_BANK1_BUF[secoff+i]!=0XFFFFFFFF) break;//
		}
		if(i<secremain)//需要擦除
		{
			fmc_page_erase(secpos*GD32_BANK1_SECTOR_SIZE+GD32_FMC_BASE);//擦除扇区
			for(i=0;i<secremain;i++)//复制要写入的数据
			{
			  GD32_BANK1_BUF[secoff+i]=pBuffer[i];
			}
			GD32FMC_Write_Nocheck(secpos*GD32_BANK1_SECTOR_SIZE+GD32_FMC_BASE,GD32_BANK1_BUF,GD32_BANK1_SECTOR_SIZE/4);//写入数据到扇区
		}
		else
			GD32FMC_Write_Nocheck(WriteAddr,pBuffer,secremain);//
		if(NumToWrite == secremain)
			break;
		else
		{
		  secpos++;
			secoff=0;
			pBuffer+=secremain;//指针偏移
			WriteAddr+=secremain;//写指针偏移
			NumToWrite-=secremain;
			if(NumToWrite>(GD32_BANK1_SECTOR_SIZE/4))  
				secremain=GD32_BANK1_SECTOR_SIZE/4;//
			else
				secremain=NumToWrite;

		}
		fmc_bank1_lock();//Flash上锁
	}
	return 1;
}

//从制定地址读出制定数量的数据
//ReadAddr :起始地址
//pBuffer:
//NumToRead:

void GD32FMC_Read(uint32_t ReadAddr,uint32_t *pBuffer,uint16_t NumToRead)
{
	uint16_t i;
	for(i=0;i<NumToRead;i++)
	{
	  pBuffer[i]=GD32FMC_ReadWord(ReadAddr);
		ReadAddr+=4;//偏移4个字节
	}
}


void GD32FMC_Half_Read(uint32_t ReadAddr,uint16_t *pBuffer,uint16_t NumToRead)
{
	uint16_t i;
	for(i=0;i<NumToRead;i++)
	{
	  pBuffer[i]=GD32FMC_ReadHalfWord(ReadAddr);
		ReadAddr+=2;//偏移2个字节
	}
}


void GD32FLASH_Erase(uint32_t WriteAddr,uint16_t NumToWrite)
{
  uint32_t secpos;	   //扇区地址
	uint16_t secoff;	   //扇区内偏移地址(16位字计算)
	uint16_t secremain; //扇区内剩余地址(16位字计算)	   
 	uint16_t i;    
	uint32_t offaddr;   //去掉0X08000000后的地址
	if(WriteAddr<GD32_FMC_BASE||(WriteAddr>=(GD32_FMC_BASE+512*GD32_BANK0_SECTOR_SIZE)))return;//非法地址
	fmc_bank0_unlock();						//解锁
	
	offaddr=WriteAddr-GD32_FMC_BASE;		//实际偏移地址.
	secpos=offaddr/GD32_BANK0_SECTOR_SIZE;			//扇区地址  
	secoff=(offaddr%GD32_BANK0_SECTOR_SIZE)/2;		//在扇区内的偏移(2个字节为基本单位.)
	secremain=GD32_BANK0_SECTOR_SIZE/2-secoff;		//扇区剩余空间大小 
  
	if(NumToWrite<=secremain)
		secremain=NumToWrite;//不大于该扇区范围
	
	while(1) 
	{	
		GD32FMC_Half_Read(secpos*GD32_FMC_SIZE+GD32_FMC_BASE,GD32_BANK0_BUF,GD32_BANK0_SECTOR_SIZE/2);//读出整个扇区的内容
		for(i=0;i<secremain;i++)//校验数据
		{
			if(GD32_BANK0_BUF[secoff+i]!=0XFFFF)
				break;//需要擦除  	  
		}
		if(i<secremain)//需要擦除
		{
			fmc_page_erase(secpos*GD32_FMC_SIZE+GD32_FMC_BASE);
				//printf("eraser the page successful\r\n");//擦除这个扇区
		}
		if(NumToWrite==secremain)
			break;//写入结束了
		else//写入未结束
		{
			secpos++;				//扇区地址增1
			secoff=0;				//偏移位置为0 	 
			WriteAddr+=secremain;	//写地址偏移	   
		  NumToWrite-=secremain;	//字节(16位)数递减
			if(NumToWrite>(GD32_BANK0_SECTOR_SIZE/2))
				secremain=GD32_BANK0_SECTOR_SIZE/2;//下一个扇区还是写不完
			else 
				secremain=NumToWrite;//下一个扇区可以写完了
		}	 
	};	
	fmc_bank0_lock();//上锁
}



uint32_t WriteFirmToEmbeddedFlashBank1(uint32_t WriteAddr,uint32_t *pBuffer,uint32_t len)
{
	
	if(WriteAddr<(GD32_FMC_BASE+512*GD32_FMC_SIZE)||(WriteAddr>=(FLASH_DOWNLOAD_APP1_ADDR+512*GD32_FMC_SIZE))) //检查地址的合法性
		return 0;
	else if(WriteAddr<FLASH_DOWNLOAD_APP2_ADDR){
		fmc_bank1_unlock();						//解锁
		if(FLASH_If_Erase(WriteAddr,FLASH_DOWNLOAD_APP1_LAST_PAGE_ADDR))
		{
			fmc_bank1_lock();
			return 0;
		}
		while(len >= 4096){
			FLASH_If_Write(&WriteAddr,(unsigned int*)pBuffer,GD32_BANK1_SECTOR_SIZE/4,FLASH_DOWNLOAD_APP1_END_ADDR);
			len -= GD32_BANK1_SECTOR_SIZE;	
		}
		if(len){
			FLASH_If_Write(&WriteAddr,(unsigned int*)pBuffer,len/4,FLASH_DOWNLOAD_APP1_END_ADDR);
			if(len%4){
				FLASH_If_Write(&WriteAddr,(unsigned int*)pBuffer+len/4,4,FLASH_DOWNLOAD_APP1_END_ADDR);
			}
		}
		fmc_bank1_lock();//上锁

		return 1;
	}
  else{
	  fmc_bank1_unlock();						//解锁
		if(FLASH_If_Erase(WriteAddr,FLASH_DOWNLOAD_APP2_LAST_PAGE_ADDR))
		{
			fmc_bank1_lock();
			return 0;
		}
		while(len >= 4096){
			FLASH_If_Write(&WriteAddr,(unsigned int*)pBuffer,GD32_BANK1_SECTOR_SIZE/4,FLASH_DOWNLOAD_APP2_END_ADDR);
			len -= GD32_BANK1_SECTOR_SIZE;	
		}
		if(len){
			FLASH_If_Write(&WriteAddr,(unsigned int*)pBuffer,len/4,FLASH_DOWNLOAD_APP2_END_ADDR);
			if(len%4){
				FLASH_If_Write(&WriteAddr,(unsigned int*)pBuffer+len/4,4,FLASH_DOWNLOAD_APP2_END_ADDR);
			}
		}
		fmc_bank1_lock();//上锁
		
  }
	return 1;
}


/**
  * @brief  This function does an erase of all user flash area
  * @param  StartSector: start of user flash area
  * @retval 0 if success, -1 if error
  */
int8_t FLASH_If_Erase(uint32_t StartSector,uint32_t EndSector)
{
  uint32_t FlashAddress;
  
  FlashAddress = StartSector;
  
  while (FlashAddress <=  EndSector)
  {
   
	  if (fmc_page_erase(FlashAddress) == FMC_READY)
    {
      FlashAddress += GD32_BANK0_SECTOR_SIZE;
    }
    else
    {
      return -1;
    }
  }
  return 0;
}


/**
  * @brief  This function writes a data buffer in flash (data are 32-bit aligned)
  * @param  FlashAddress: start address for writing data buffer
  * @param  Data: pointer on data buffer
  * @param  DataLength: length of data buffer (unit is 32-bit word)   
  * @retval None
  */
void FLASH_If_Write(__IO uint32_t* FlashAddress, uint32_t* Data ,uint16_t DataLength,uint32_t FlashEndAddr)
{
  uint32_t i = 0;
  
  for (i = 0; i < DataLength; i++)
  {
    if (*FlashAddress <= (FlashEndAddr-4))
    {
      if (fmc_word_program(*FlashAddress, *(uint32_t*)(Data + i)) == FMC_READY)
      {
        *FlashAddress += 4;
      }
      else
      {
        return;
      }
    }
    else
    {
      return;
    }
  }
}
