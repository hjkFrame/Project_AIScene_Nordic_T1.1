#include "EmbeddedFlash.h"
#include "string.h"
#include "gd32f20x_fmc.h"
//#include "includes.h"
#include "systick.h"

const INT8U		VERSION[8]			= {'k','i','n','g','l','u','m','i'};
const INT8U		LEDID_DF[10]			= {'0','0','0','0','6','8','9','0','0','0'};

#define Base_Addr 0x0870f800   //��255ҳ  bank1  ��С��Kb
#define EEP_MAX 256     //256�ֽڵĴ洢�ռ�   һ�������ramѹ��  ��һ�������flash����ʱ��

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

//1�ɹ���0ʧ��
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
��  ��  ����Flash_Read_HalfWord
��       �ܣ�flash���ֶ�ȡ
�����������Ե�ַ 0-128
���ز�������������ֵ
˵      ����
***********************************************************************/
unsigned short int Flash_Read_HalfWord(unsigned  faddr)
{
        return *(unsigned short int*)(faddr*2+FLASH_BASE);//���������ַ�е�ֵ
}
/***********************************************************************
��  ��  ����Flash_Read
��       �ܣ���ȡָ�����ȵ�����
�����������Ե�ַ 0-128   ��ȡʹ�õ����� 16λ   Ҫ��ȡ�ĳ���
���ز�����
˵      ����
***********************************************************************/
void Flash_Read(unsigned int addr,unsigned short int *redata,unsigned short int renum)
{
          unsigned short int i;
          for(i = 0;i<renum;i++)
          {
                 *(redata+i*2) = Flash_Read_HalfWord(addr+i);//��ʼ��
          }
}
/***********************************************************************
��  �� ����Flash_Write_Byte
��      �ܣ�flash����д
�����������Ե�ַ 0-128��16λ������ָ��    д�������
���ز�����1�ɹ�  2��ַ�������÷�Χ
˵      ����
***********************************************************************/
unsigned char Flash_Write_Byte(INT32U uaddr,INT32U *data16,unsigned short int datanum)
{
          unsigned short int i;
          unsigned char Nclear = 0;
          unsigned short int tempdata[EEP_MAX/2];
          if(uaddr>EEP_MAX/2)return 2;//���볬�����÷�Χ

          fmc_bank0_unlock();//Flash����
          fmc_flag_clear(FMC_FLAG_BANK0_END | FMC_FLAG_BANK0_WPERR | FMC_FLAG_BANK0_PGERR);
 
          for(i = 0;i<datanum;i++)
          {
              if(Flash_Read_HalfWord(uaddr+i)!=0xffff)Nclear = 1;//��Ҫ���flash
          }
          if(Nclear == 1)
          {
							for(i = 0;i<EEP_MAX/2;i++)//��������
							{
									tempdata[i]=Flash_Read_HalfWord(uaddr+i);
							}
							for(i = 0;i<datanum;i++)
							{
									tempdata[uaddr+i] = *(data16+i*2);//д�����ݵ���������
							}
							fmc_page_erase(Base_Addr);
							for(i = 0;i<EEP_MAX/2;i++)
							{
									fmc_halfword_program(Base_Addr+i*2,tempdata[i]);//��������д��
							}
          }
          else
          {
                for(i = 0;i<datanum;i++)
                {
                         fmc_halfword_program(Base_Addr+(uaddr+i)*2,*(data16+i*2));//д��
                }                                
          }
          fmc_bank0_lock();//Flash����
          return 1;
}





//��ȡ�ƶ���ַ����  32λ����
//addr :ָ����ȡ�ĵ�ַ  ������4 �ı���
//����ֵ����Ӧ������
uint32_t GD32FMC_ReadWord(uint32_t addr)
{
	return *(vu32*)addr;
}

uint32_t GD32FMC_ReadHalfWord(uint32_t addr)
{
	return *(vu16*)addr;
}


//������д��
//WriteAddr:��ʼ��ַ
//pBuffer:����ָ��
//NumToWrite:��   32λ����
void GD32FMC_Write_Nocheck(uint32_t WriteAddr,uint32_t *pBuffer,uint16_t NumToWrite)
{
	uint16_t i;
	for(i=0;i<NumToWrite;i++)
	{
		  fmc_word_program(WriteAddr,pBuffer[i]);
		  WriteAddr+=4;//��ַ����4   32λ���ݣ��֣�	
	}
	
}


//������д��
//WriteAddr:��ʼ��ַ
//pBuffer:����ָ��
//NumToWrite:��   32λ����
void GD32FMC_Half_Write_Nocheck(uint32_t WriteAddr,uint16_t *pBuffer,uint16_t NumToWrite)
{
	uint16_t i;
	for(i=0;i<NumToWrite;i++)
	{
		  fmc_halfword_program(WriteAddr,pBuffer[i]);
		  WriteAddr+=2;//��ַ����4   32λ���ݣ��֣�	
	}
	
}

//WriteAddr:��ʼ��ַ   4�ı���
//pBuffer:����ָ��
//NumToWrite:Ҫд�����ݵĸ���


void GD32FMC_Write(uint32_t WriteAddr,uint32_t *pBuffer ,uint16_t NumToWrite)
{
	uint32_t secpos;//
	uint16_t secoff;//
	uint16_t secremain;//
	uint16_t i;
	uint32_t offaddr;//
	if(WriteAddr<GD32_FMC_BASE||(WriteAddr>=(GD32_FMC_BASE+512*GD32_BANK0_SECTOR_SIZE))) return;//����ַ�ĺϷ���
	fmc_bank0_unlock();//Flash����
	offaddr=WriteAddr-GD32_FMC_BASE;//ʵ��ƫ�Ƶ�ַ
	secpos=offaddr/GD32_BANK0_SECTOR_SIZE;//������ַ
	secoff=(offaddr%GD32_BANK0_SECTOR_SIZE)/4;//
	secremain=GD32_BANK0_SECTOR_SIZE/4-secoff;//����ʣ��ռ�Ĵ�С
	if(NumToWrite<=secremain)  secremain=NumToWrite;//�����ڸ������ķ�Χ   
	
	while(1)
	{
		GD32FMC_Read(secpos*GD32_BANK0_SECTOR_SIZE+GD32_FMC_BASE,GD32_BANK1_BUF,GD32_BANK0_SECTOR_SIZE/4);//��ȡ������������
	  for(i=0;i<secremain;i++)//
		{
		 if(GD32_BANK1_BUF[secoff+i]!=0XFFFFFFFF) break;//
		}
		if(i<secremain)//��Ҫ����
		{

			fmc_page_erase(secpos*GD32_BANK0_SECTOR_SIZE+GD32_FMC_BASE);//��������
			for(i=0;i<secremain;i++)//����Ҫд�������
			{
			  GD32_BANK1_BUF[secoff+i]=pBuffer[i];
			}
			GD32FMC_Write_Nocheck(secpos*GD32_BANK0_SECTOR_SIZE+GD32_FMC_BASE,GD32_BANK1_BUF,GD32_BANK0_SECTOR_SIZE/4);//д�����ݵ�����
		}
		else
			GD32FMC_Write_Nocheck(WriteAddr,pBuffer,secremain);//
		if(NumToWrite==secremain) break;
		else
		{
		  secpos++;
			secoff=0;
			pBuffer+=secremain;//ָ��ƫ��
			WriteAddr+=secremain;//дָ��ƫ��
			NumToWrite-=secremain;//
			
			if(NumToWrite>(GD32_BANK0_SECTOR_SIZE/4))  secremain=GD32_BANK0_SECTOR_SIZE/4;//
			else
				secremain=NumToWrite;//
		}
		fmc_bank0_lock();//Flash����
	}
}


uint8_t GD32FMC_Half_Write(uint32_t WriteAddr,uint16_t *pBuffer ,uint16_t NumToWrite)
{
	uint32_t secpos;//
	uint16_t secoff;//
	uint16_t secremain;//
	uint16_t i;
	uint32_t offaddr;//
	if(WriteAddr<GD32_FMC_BASE||(WriteAddr>=(GD32_FMC_BASE+512*GD32_BANK0_SECTOR_SIZE))) //����ַ�ĺϷ���
		return 0;
	fmc_bank0_unlock();//Flash����
	offaddr=WriteAddr-GD32_FMC_BASE;//ʵ��ƫ�Ƶ�ַ
	secpos=offaddr/GD32_BANK0_SECTOR_SIZE;//������ַ
	secoff=(offaddr%GD32_BANK0_SECTOR_SIZE)/2;//
	secremain=GD32_BANK0_SECTOR_SIZE/2-secoff;//����ʣ��ռ�Ĵ�С
	if(NumToWrite<=secremain)  
		secremain=NumToWrite;//�����ڸ������ķ�Χ   
	while(1)
	{
		GD32FMC_Half_Read(secpos*GD32_BANK0_SECTOR_SIZE+GD32_FMC_BASE,GD32_BANK0_BUF,GD32_BANK0_SECTOR_SIZE/2);//��ȡ������������
	  for(i=0;i<secremain;i++)//
		{
		 if(GD32_BANK0_BUF[secoff+i]!=0XFFFF) break;//
		}
		if(i<secremain)//��Ҫ����
		{

			fmc_page_erase(secpos*GD32_BANK0_SECTOR_SIZE+GD32_FMC_BASE);//��������
			for(i=0;i<secremain;i++)//����Ҫд�������
			{
			  GD32_BANK0_BUF[secoff+i]=pBuffer[i];
			}
			GD32FMC_Half_Write_Nocheck(secpos*GD32_BANK0_SECTOR_SIZE+GD32_FMC_BASE,GD32_BANK0_BUF,GD32_BANK0_SECTOR_SIZE/2);//д�����ݵ�����
		}
		else
			GD32FMC_Half_Write_Nocheck(WriteAddr,pBuffer,secremain);//
		if(NumToWrite==secremain) break;
		else
		{
		  secpos++;
			secoff=0;
			pBuffer+=secremain;//ָ��ƫ��
			WriteAddr+=secremain;//дָ��ƫ��
			NumToWrite-=secremain;//
			
			if(NumToWrite>(GD32_BANK0_SECTOR_SIZE/2))  
				secremain=GD32_BANK0_SECTOR_SIZE/2;//
			else
				secremain=NumToWrite;//
		}
		fmc_bank0_lock();//Flash����
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

	if(WriteAddr<(GD32_FMC_BASE+512*GD32_FMC_SIZE)||(WriteAddr>=(FLASH_DOWNLOAD_APP1_ADDR+512*GD32_FMC_SIZE))) //����ַ�ĺϷ���
		return 0;
	fmc_bank1_unlock();//Flash����
	offaddr=WriteAddr-GD32_FMC_BASE;//ʵ��ƫ�Ƶ�ַ
	secpos=offaddr/GD32_BANK1_SECTOR_SIZE;//������ַ
	secoff=(offaddr%GD32_BANK1_SECTOR_SIZE)/4;//
	secremain=GD32_BANK1_SECTOR_SIZE/4-secoff;//����ʣ��ռ�Ĵ�С
	if(NumToWrite<=secremain)
		secremain = NumToWrite;
	while(1)
	{
		GD32FMC_Read(secpos*GD32_BANK1_SECTOR_SIZE+GD32_FMC_BASE,GD32_BANK1_BUF,GD32_BANK1_SECTOR_SIZE/4);//��ȡ������������
	  for(i=0;i<secremain;i++)
		{
		 if(GD32_BANK1_BUF[secoff+i]!=0XFFFFFFFF) break;//
		}
		if(i<secremain)//��Ҫ����
		{
			fmc_page_erase(secpos*GD32_BANK1_SECTOR_SIZE+GD32_FMC_BASE);//��������
			for(i=0;i<secremain;i++)//����Ҫд�������
			{
			  GD32_BANK1_BUF[secoff+i]=pBuffer[i];
			}
			GD32FMC_Write_Nocheck(secpos*GD32_BANK1_SECTOR_SIZE+GD32_FMC_BASE,GD32_BANK1_BUF,GD32_BANK1_SECTOR_SIZE/4);//д�����ݵ�����
		}
		else
			GD32FMC_Write_Nocheck(WriteAddr,pBuffer,secremain);//
		if(NumToWrite == secremain)
			break;
		else
		{
		  secpos++;
			secoff=0;
			pBuffer+=secremain;//ָ��ƫ��
			WriteAddr+=secremain;//дָ��ƫ��
			NumToWrite-=secremain;
			if(NumToWrite>(GD32_BANK1_SECTOR_SIZE/4))  
				secremain=GD32_BANK1_SECTOR_SIZE/4;//
			else
				secremain=NumToWrite;

		}
		fmc_bank1_lock();//Flash����
	}
	return 1;
}

//���ƶ���ַ�����ƶ�����������
//ReadAddr :��ʼ��ַ
//pBuffer:
//NumToRead:

void GD32FMC_Read(uint32_t ReadAddr,uint32_t *pBuffer,uint16_t NumToRead)
{
	uint16_t i;
	for(i=0;i<NumToRead;i++)
	{
	  pBuffer[i]=GD32FMC_ReadWord(ReadAddr);
		ReadAddr+=4;//ƫ��4���ֽ�
	}
}


void GD32FMC_Half_Read(uint32_t ReadAddr,uint16_t *pBuffer,uint16_t NumToRead)
{
	uint16_t i;
	for(i=0;i<NumToRead;i++)
	{
	  pBuffer[i]=GD32FMC_ReadHalfWord(ReadAddr);
		ReadAddr+=2;//ƫ��2���ֽ�
	}
}


void GD32FLASH_Erase(uint32_t WriteAddr,uint16_t NumToWrite)
{
  uint32_t secpos;	   //������ַ
	uint16_t secoff;	   //������ƫ�Ƶ�ַ(16λ�ּ���)
	uint16_t secremain; //������ʣ���ַ(16λ�ּ���)	   
 	uint16_t i;    
	uint32_t offaddr;   //ȥ��0X08000000��ĵ�ַ
	if(WriteAddr<GD32_FMC_BASE||(WriteAddr>=(GD32_FMC_BASE+512*GD32_BANK0_SECTOR_SIZE)))return;//�Ƿ���ַ
	fmc_bank0_unlock();						//����
	
	offaddr=WriteAddr-GD32_FMC_BASE;		//ʵ��ƫ�Ƶ�ַ.
	secpos=offaddr/GD32_BANK0_SECTOR_SIZE;			//������ַ  
	secoff=(offaddr%GD32_BANK0_SECTOR_SIZE)/2;		//�������ڵ�ƫ��(2���ֽ�Ϊ������λ.)
	secremain=GD32_BANK0_SECTOR_SIZE/2-secoff;		//����ʣ��ռ��С 
  
	if(NumToWrite<=secremain)
		secremain=NumToWrite;//�����ڸ�������Χ
	
	while(1) 
	{	
		GD32FMC_Half_Read(secpos*GD32_FMC_SIZE+GD32_FMC_BASE,GD32_BANK0_BUF,GD32_BANK0_SECTOR_SIZE/2);//������������������
		for(i=0;i<secremain;i++)//У������
		{
			if(GD32_BANK0_BUF[secoff+i]!=0XFFFF)
				break;//��Ҫ����  	  
		}
		if(i<secremain)//��Ҫ����
		{
			fmc_page_erase(secpos*GD32_FMC_SIZE+GD32_FMC_BASE);
				//printf("eraser the page successful\r\n");//�����������
		}
		if(NumToWrite==secremain)
			break;//д�������
		else//д��δ����
		{
			secpos++;				//������ַ��1
			secoff=0;				//ƫ��λ��Ϊ0 	 
			WriteAddr+=secremain;	//д��ַƫ��	   
		  NumToWrite-=secremain;	//�ֽ�(16λ)���ݼ�
			if(NumToWrite>(GD32_BANK0_SECTOR_SIZE/2))
				secremain=GD32_BANK0_SECTOR_SIZE/2;//��һ����������д����
			else 
				secremain=NumToWrite;//��һ����������д����
		}	 
	};	
	fmc_bank0_lock();//����
}



uint32_t WriteFirmToEmbeddedFlashBank1(uint32_t WriteAddr,uint32_t *pBuffer,uint32_t len)
{
	
	if(WriteAddr<(GD32_FMC_BASE+512*GD32_FMC_SIZE)||(WriteAddr>=(FLASH_DOWNLOAD_APP1_ADDR+512*GD32_FMC_SIZE))) //����ַ�ĺϷ���
		return 0;
	else if(WriteAddr<FLASH_DOWNLOAD_APP2_ADDR){
		fmc_bank1_unlock();						//����
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
		fmc_bank1_lock();//����

		return 1;
	}
  else{
	  fmc_bank1_unlock();						//����
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
		fmc_bank1_lock();//����
		
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
