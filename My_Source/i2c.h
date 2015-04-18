//-----------------------------------------------------------------
//�ļ����ƣ�i2c.h
//���ܸ�Ҫ��ģ��i2c�ײ����������ļ�
//�汾���£�2013-10-17
//-----------------------------------------------------------------

//------------------------------ ����Ԥ���� ------------------------------//
#ifndef _I2C_H_
#define _I2C_H_  

//����ͷ�ļ�
#include "Common.h"             //�����������Ͷ���

//----------------------------------------------------------------------//
//��������
//----------------------------------------------------------------------//
#define TM4C123GH6PM_1
#define TM4C123GH6PM
//������MCS51оƬ
#if defined MCS_51

sbit SCL = P2^0;                //IICʱ�����Ŷ���
sbit SDA = P2^1;                //IIC�������Ŷ���

#define SCL_L (SCL=0)           //SCL�õ�
#define SCL_H (SCL=1)           //SCL�ø�
#define SDA_L (SDA=0)           //SDA�õ�
#define SDA_H (SDA=1)           //SDA�ø�

//������XS128оƬ
#elif defined XS_128 

#define SCL PTH_PTH0  			//IICʱ�����Ŷ���
#define SDA PTH_PTH1		   	//IIC�������Ŷ���

#define SCL_L (SCL=0)           //SCL�õ�
#define SCL_H (SCL=1)           //SCL�ø�
#define SDA_L (SDA=0)           //SDA�õ�
#define SDA_H (SDA=1)           //SDA�ø�

#define SDA_I (DDRH_DDRH1=0)    //����SDAΪ����ģʽ
#define SDA_O (DDRH_DDRH1=1)    //����SDAΪ���ģʽ

//������STM32оƬ
#elif defined STM32_F4XX

#if 1 //X�ʹ�����(PD2,PD3)
#define SCL_L    GPIOD->BSRRH=GPIO_Pin_2   //SCL�õ�     SCL:PD2
#define SCL_H    GPIOD->BSRRL=GPIO_Pin_2   //SCL�ø�
#define SDA_L    GPIOD->BSRRH=GPIO_Pin_3   //SDA�õ�     SDA:PD3
#define SDA_H    GPIOD->BSRRL=GPIO_Pin_3   //SDA�ø�

#define SDA_I	GPIOD->MODER|=GPIO_Mode_IN<<4; //����SDAΪ����ģʽ�����ܣ���GPIOD->MODER�ĵ�4λ�ó�GPIO_Mode_IN�������ģʽ��
#define SDA_O   GPIOD->MODER|=GPIO_Mode_OUT<<4;//����SDAΪ���ģʽ�����ܣ���GPIOD->MODER�ĵ�4λ�ó�GPIO_Mode_OUT�������ģʽ��

#define SDA    ((GPIOD->IDR&GPIO_Pin_3)!=0)?1:0 //��ȡSDA��ֵ

#else //ʮ���ʹ�����(PB11,PB13)
#define SCL_L    GPIOB->BSRRH=GPIO_Pin_11   //SCL�õ�     SCL:PB11
#define SCL_H    GPIOB->BSRRL=GPIO_Pin_11   //SCL�ø�
#define SDA_L    GPIOB->BSRRH=GPIO_Pin_13   //SDA�õ�     SDA:PB13
#define SDA_H    GPIOB->BSRRL=GPIO_Pin_13   //SDA�ø�

#define SDA_I	GPIOB->MODER|=GPIO_Mode_IN<<14; //����SDAΪ����ģʽ�����ܣ���GPIOA->MODER�ĵ�14λ�ó�GPIO_Mode_IN�������ģʽ��
#define SDA_O   GPIOB->MODER|=GPIO_Mode_OUT<<14;//����SDAΪ���ģʽ�����ܣ���GPIOA->MODER�ĵ�14λ�ó�GPIO_Mode_OUT�������ģʽ��

#define SDA    ((GPIOB->IDR&GPIO_Pin_13)!=0)?1:0 //��ȡSDA��ֵ
#endif

//������TM4C123GH6PMоƬ
#elif defined TM4C123GH6PM_1

#define SCL_L    ROM_GPIOPinWrite(GPIO_PORTE_BASE, GPIO_PIN_4, 0x00)	//SCL�õ�     SCL:PE4
#define SCL_H    ROM_GPIOPinWrite(GPIO_PORTE_BASE, GPIO_PIN_4, 0x10)   	//SCL�ø�
#define SDA_L    ROM_GPIOPinWrite(GPIO_PORTE_BASE, GPIO_PIN_5, 0x00)   	//SDA�õ�     SDA:PE5
#define SDA_H    ROM_GPIOPinWrite(GPIO_PORTE_BASE, GPIO_PIN_5, 0x20)   	//SDA�ø�

#define SDA_I	ROM_GPIOPinTypeGPIOInput(GPIO_PORTE_BASE, GPIO_PIN_5)	//����SDAΪ����ģʽ
#define SDA_O   ROM_GPIOPinTypeGPIOOutput(GPIO_PORTE_BASE, GPIO_PIN_5)	//����SDAΪ���ģʽ

#define SDA    (ROM_GPIOPinRead(GPIO_PORTE_BASE, GPIO_PIN_5)!=0)?1:0 	//��ȡSDA��ֵ

#endif

//------------------------------ �ⲿ�ӿں������� ------------------------------//

extern void I2C_start(void);
extern void I2C_stop(void);
extern void I2C_ack(void);
extern void I2C_no_ack(void);
extern void I2C_check_ack(void);
extern void I2C_send_one_char(uint8_t c);
extern void I2C_recv_one_char(uint8_t *c);

//-----------------------------------------------------------------
//�������ƣ�i2c_init
//���ܸ�Ҫ��i2c��ʼ������ʼ��ģ��i2c��IO��
//�������أ�void
//����˵������
//-----------------------------------------------------------------
extern void i2c_init(void);

//-----------------------------------------------------------------
//�������ƣ�I2C_send_str
//���ܸ�Ҫ�����мĴ�����ַ��������һ���ַ�����               
//          ���������ߵ����͵�ַ���Ĵ�����ַ,�������ݣ��������ߵ�ȫ���̡�
//          �ӻ���ַslave���Ĵ�����ַreg,���͵�������sָ������ݣ�����num���ֽڡ�
//�������أ�void
//����˵����slave:�ӻ���ַ
//          reg  :�Ĵ�����ַ
//          s    :�����ַ����׵�ַ
//          num  :�ֽڸ���
//-----------------------------------------------------------------
extern void  I2C_send_str(uint8_t slave,uint8_t reg,uint8_t *s,uint8_t num);

//-----------------------------------------------------------------
//�������ƣ�I2C_recv_str
//���ܸ�Ҫ�����мĴ�����ַ��������һ���ַ�����         
//          ���������ߵ����͵�ַ���Ĵ�����ַ,�������ݣ��������ߵ�ȫ���̡�
//          �ӻ���ַslave���Ĵ�����ַreg,���յ����ݷ���sָ��Ĵ洢��������num���ֽڡ�
//�������أ�void
//����˵����slave:�ӻ���ַ
//          reg  :�Ĵ�����ַ
//          s    :�����ַ����׵�ַ
//          num  :�ֽڸ���
//-----------------------------------------------------------------
extern void  I2C_recv_str(uint8_t slave,uint8_t reg,uint8_t *s,uint8_t num);

//������������˳��
extern int8_t i2cwrite(uint8_t addr, uint8_t reg, uint8_t len, uint8_t * data);
extern int8_t i2cread(uint8_t addr, uint8_t reg, uint8_t len, uint8_t * buf);

#endif

