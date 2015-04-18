//-----------------------------------------------------------------
//�ļ����ƣ�i2c.c
//���ܸ�Ҫ��ģ��i2c�ײ����������ļ�
//�汾���£�2013-12-15
//
//I2Cͨ��Э�飺������ʼ�źţ�ֹͣ�ź��⣬SDA��SCL_Hʱ��ά�ֲ��䣬ֻ������SCL_Lʱ���䡣���Է�������ʱ��SDAӦ����SCL_Lʱ�ı�״̬����������ʱ��Ӧ����SCL_Hʱ��ȡSDA��Ч״̬��
//   ��ʼ�źţ�SCL_Hʱ��SDA�����½��ء�
//   ֹͣ�źţ�SCL_Hʱ��SDA���������ء�
//   Ӧ���źţ�ԭ��I2Cÿ����1���ֽ�(�ȴ�MSB,�ٴ�LSB)��Ҫ��һ��Ӧ��λ�Ĵ��䣡Ӧ��λSCLʼ����������������SDA���ɽ��շ�������
//             Ӧ���ź�Э�飺��9��SCL_Hʱ���SDA��SDA_L--ack;SDA_H--no_ack�����ͷ����յ���no_ackʱ����ֹ���͡�
//   ����ʱ��д��start>slave_address_w>A>data>A...>data>NA>stop
//             ����start>slave_address_r>A>DATA>a...>DATA>na>stop  (Сд��������������������д�������ɴӻ�����)
//-----------------------------------------------------------------
#include "i2c.h"
#include "Delay.h"
//------------------------------ �ڲ����� ------------------------------//

//��������

//----------------------------------------------------------------------//
#if defined MCS_51  //������MCS51оƬ���������� SDA �����������
//----------------------------------------------------------------------//

//-----------------------------------------------------------------
//�������ƣ�I2C_start
//���ܸ�Ҫ����������,SCL_Hʱ��SDA�����½��ء�
//�������أ�void
//����˵������
//-----------------------------------------------------------------
static void I2C_start(void)                         
{
    SDA_H;          
    delay_1us();
    SCL_H;
    delay_5us();   
    SDA_L;          
    delay_5us();          
    SCL_L;          
    delay_2us();
}

//-----------------------------------------------------------------
//�������ƣ�I2C_stop
//���ܸ�Ҫ����������,SCL_Hʱ��SDA���������ء�
//�������أ�void
//����˵������
//-----------------------------------------------------------------
static void I2C_stop(void)   
{
    SDA_L;         
    delay_1us();   
    SCL_H;         
    delay_5us();
    SDA_H;         
    delay_4us();
}


//-----------------------------------------------------------------
//�������ƣ�I2C_ack
//���ܸ�Ҫ����������Ӧ��λ��SCL_Hʱ������SDA_L״̬
//�������أ�void
//����˵������
//-----------------------------------------------------------------
static void I2C_ack(void)     
{ 
    SDA_L;     
    delay_1us();      
    SCL_H;
    delay_5us(); 
    SCL_L;              
    delay_2us();    
}


//-----------------------------------------------------------------
//�������ƣ�I2C_no_ack
//���ܸ�Ҫ���������ͷ�Ӧλ��SCL_Hʱ������SDA_H״̬
//�������أ�void
//����˵������
//-----------------------------------------------------------------
static void I2C_no_ack(void)   
{ 
    SDA_H;
    delay_1us();     
    SCL_H;
    delay_5us(); 
    SCL_L;               
    delay_2us();   
}

//-----------------------------------------------------------------
//�������ƣ�I2C_check_ack
//���ܸ�Ҫ���������մӻ�Ӧ��λ��SCL_Hʱ��ȡSDA�����SDA_L����յ�Ӧ��λ��������յ���Ӧ��λ��
//          �����Ӧ��������Ҫ����I2C����
//�������أ�void
//����˵������
//-----------------------------------------------------------------
static void I2C_check_ack(void)
{    
    delay_3us();
    SCL_L;
    delay_3us();
	
    SDA_H;
    delay_3us();
    SCL_H;
	delay_5us(); 	
	
	if(SDA==1)
    {   
        I2C_stop();  //SDA!=0��δ���յ�Ӧ��λ����������
    }  
    else  
    {
        SCL_L;       //SDA==0�����յ�Ӧ��λ������ʱ����
    }   	 
}

//-----------------------------------------------------------------
//�������ƣ�I2C_send_one_char
//���ܸ�Ҫ������һ���ֽڡ�
//          ������c���ͳ�ȥ,�����ǵ�ַ,Ҳ����������,������ɺ�����Ӧ��λ
//�������أ�void
//����˵����c :�����ֽ�
//-----------------------------------------------------------------
static void I2C_send_one_char(uint8_t c)
{
    uint8_t i;

    for (i=0; i<8; i++)         //8λ������
    {
        if(c&0x80)
          SDA_H;                
        else
          SDA_L;                //�����ݿ�
        
        c <<= 1;                //�Ƴ����ݵ����λ
        
        SCL_H;                  
        delay_3us();
        SCL_L;                  
        delay_3us();            //����SCL���壬��SCL_Lʱ�ı�SDA״̬��
    } 
}

//-----------------------------------------------------------------
//�������ƣ�I2C_recv_one_char
//���ܸ�Ҫ������һ���ֽڡ�
//          �������մ�����������һ���ֽڡ�������ɺ�����Ӧ��λ
//�������أ���
//����˵����c :�����ֽڴ�ŵĵ�ַ
//-----------------------------------------------------------------
static void I2C_recv_one_char(uint8_t *c)
{
    uint8_t i;
	uint8_t retc = 0;

    SDA_H;             

    for(i=0;i<8;i++)
    {
         delay_1us();          
         SCL_L;       
         delay_5us();
         SCL_H;       
         delay_2us();     //����SCL���壬��SCL_Hʱ��ȡSDA״̬��
		
         retc=retc<<1;
         if(SDA==1)
         retc=retc+1;     //������λ,���յ�����λ����retc��
         delay_2us(); 
    } 
    SCL_L;

    *c = retc;	
}

//----------------------------------------------------------------------//
#elif defined XS_128 || defined STM32_F4XX || defined TM4C123GH6PM //������XS128оƬ����Ҫ���� SDA �����������
//----------------------------------------------------------------------//

//-----------------------------------------------------------------
//�������ƣ�I2C_start
//���ܸ�Ҫ����������,SCL_Hʱ��SDA�����½��ء�
//�������أ�void
//����˵������
//-----------------------------------------------------------------
 void I2C_start(void)                     
{
    SDA_O;

    SDA_H;          
    delay_1us();
    SCL_H;
    delay_5us();    
    SDA_L;          
    delay_5us();     
    SCL_L;         
    delay_2us();
}

//-----------------------------------------------------------------
//�������ƣ�I2C_stop
//���ܸ�Ҫ����������,SCL_Hʱ��SDA���������ء�
//�������أ�void
//����˵������
//-----------------------------------------------------------------
 void I2C_stop(void)  
{
    SDA_O;

    SDA_L;         
    delay_1us();   
    SCL_H;         
    delay_5us();
    SDA_H;
    delay_4us();
}


//-----------------------------------------------------------------
//�������ƣ�I2C_ack
//���ܸ�Ҫ����������Ӧ��λ��SCL_Hʱ������SDA_L״̬
//�������أ�void
//����˵������
//-----------------------------------------------------------------
 void I2C_ack(void)     
{ 
    SDA_O;

    SDA_L;     
    delay_1us();      
    SCL_H;
    delay_5us(); 
    SCL_L;               
    delay_2us();    
}

//-----------------------------------------------------------------
//�������ƣ�I2C_no_ack
//���ܸ�Ҫ���������ͷ�Ӧλ��SCL_Hʱ������SDA_H״̬
//�������أ�void
//����˵������
//-----------------------------------------------------------------
 void I2C_no_ack(void)   
{ 
    SDA_O;

    SDA_H;
    delay_1us();     
    SCL_H;
    delay_5us(); 
    SCL_L;                
    delay_2us();   
}

//-----------------------------------------------------------------
//�������ƣ�I2C_check_ack
//���ܸ�Ҫ���������մӻ�Ӧ��λ��SCL_Hʱ��ȡSDA�����SDA_L����յ�Ӧ��λ��������յ���Ӧ��λ��
//          �����Ӧ��������Ҫ����I2C����
//�������أ�void
//����˵������
//-----------------------------------------------------------------
 void I2C_check_ack(void)
{   
    SDA_O;
    
    delay_3us();
    SCL_L;
    delay_3us();
    SDA_I;           //����Ϊ����ģʽ
    
    SDA_H;
    delay_3us();
    SCL_H;
    delay_5us();
    
    if(SDA==1)
    {   
        I2C_stop();  //SDA!=0��δ���յ�Ӧ��λ����������
    }  
    else  
    {
        SCL_L;       //SDA==0�����յ�Ӧ��λ������ʱ����
    }   
}

//-----------------------------------------------------------------
//�������ƣ�I2C_send_one_char
//���ܸ�Ҫ������һ���ֽڡ�
//          ������c���ͳ�ȥ,�����ǵ�ַ,Ҳ����������,������ɺ�����Ӧ��λ
//�������أ�void
//����˵����c :�����ֽ�
//-----------------------------------------------------------------
 void I2C_send_one_char(uint8_t c)
{
    uint8_t i;
    
    SDA_O;
    
    for (i=0; i<8; i++)        //8λ������
    {
        if(c&0x80)
          SDA_H;               
        else
          SDA_L;               //�����ݿ�
        
        c <<= 1;               //�Ƴ����ݵ����λ
        
        SCL_H;                
        delay_3us();
        SCL_L;                 //����SCL���壬��SCL_Lʱ�ı�SDA״̬��
        delay_3us();
    } 
}

//-----------------------------------------------------------------
//�������ƣ�I2C_recv_one_char
//���ܸ�Ҫ������һ���ֽڡ�
//          �������մ�����������һ���ֽڡ�������ɺ�����Ӧ��λ
//�������أ���
//����˵����c :�����ֽڴ�ŵĵ�ַ
//-----------------------------------------------------------------
 void I2C_recv_one_char(uint8_t *c)
{
    uint8_t i;
	uint8_t retc = 0;

    SDA_O;

    SDA_H;             

    SDA_I;            

    for(i=0;i<8;i++)
    {
         delay_1us();          
         SCL_L;       
         delay_5us();
         SCL_H;       
         delay_2us();		//����SCL���壬��SCL_Hʱ��ȡSDA״̬��
		
         retc=retc<<1;
         if(SDA==1)
         retc=retc+1;       //������λ,���յ�����λ����retc��
         delay_2us(); 
    } 
    SCL_L; 
	
	*c = retc;
}

#endif


//------------------------------ �ⲿ�ӿں��� ------------------------------//

//-----------------------------------------------------------------
//�������ƣ�i2c_init
//���ܸ�Ҫ��i2c��ʼ������ʼ��ģ��i2c��IO��
//�������أ�void
//����˵������
//-----------------------------------------------------------------
void i2c_init(void)
{
	#ifdef MCS_51
	
    #elif defined XS_128
	
        DDRH_DDRH0 = 1;
        DDRH_DDRH1 = 1;
	
    #elif defined STM32_F4XX
		#if 1 //X�ʹ�����(PD2,PD3)
		GPIO_InitTypeDef  GPIO_InitStructure;
		RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOD, ENABLE);  //ʹ��ʱ��
		GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2 |GPIO_Pin_3;  //ģ��i2c��PD2��PD3�ڣ�
		GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;          //��ʼ��Ϊ���ģʽ
		GPIO_InitStructure.GPIO_OType = GPIO_OType_OD;         //��©���
		GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;     //�˿�����100MHz
		GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;       //��������
		GPIO_Init(GPIOD, &GPIO_InitStructure);
		#else //ʮ���ʹ�����(PB11,PB13)
		GPIO_InitTypeDef  GPIO_InitStructure;
		RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE);  //ʹ��ʱ��
		GPIO_InitStructure.GPIO_Pin = GPIO_Pin_11 |GPIO_Pin_13;//ģ��i2c��PB11��PB13�ڣ�
		GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;          //��ʼ��Ϊ���ģʽ
		GPIO_InitStructure.GPIO_OType = GPIO_OType_OD;         //��©���
		GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;     //�˿�����100MHz
		GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;       //��������
		GPIO_Init(GPIOB, &GPIO_InitStructure);
		#endif

		#elif defined TM4C123GH6PM_1

		ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOE);		//ʹ��PORTE
		ROM_GPIOPinTypeGPIOOutput(GPIO_PORTE_BASE, GPIO_PIN_4 | GPIO_PIN_5);//ģ��i2c��PE4��PE5��,��ʼ��Ϊ���ģʽ

    #endif	
}

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
/*
void I2C_send_str(uint8_t slave,uint8_t reg,uint8_t *s,uint8_t num)
{
   uint8_t i;
   
   for(i=0;i<num;i++)
   { 
     I2C_start();                   //��������
     
     I2C_send_one_char(slave);      //����������ַ(д)
     I2C_check_ack();
     I2C_send_one_char(reg);        //���������ӵ�ַ
     I2C_check_ack();
     I2C_send_one_char(*s);         //�������� 
     I2C_check_ack();
        
     I2C_stop();                    //��������  
     
	   delay_ms(1);                 //������ʱ�ȴ�оƬ�ڲ��Զ������������  
	   s++;
	   reg++;                       //�Ĵ�����ַ����������
   }
} */

void I2C_send_str(uint8_t slave,uint8_t reg,uint8_t *s,uint8_t num)
{
    int i;

    I2C_start();                    //��ʼ�ź�

    I2C_send_one_char(slave);       //�����豸��ַ(д)
    I2C_check_ack();
    I2C_send_one_char(reg);         //�ڲ��Ĵ�����ַ
    I2C_check_ack();

    for (i=0;i<num;i++)
    {  
     I2C_send_one_char(s[i]);       //�ڲ��Ĵ�������
     I2C_check_ack();               //�Ĵ�����ַ����������
    }

    I2C_stop();                     //����ֹͣ�ź�
}  

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
void I2C_recv_str(uint8_t slave,uint8_t reg,uint8_t *s,uint8_t num)
{
     I2C_start();                    //��ʼ�ź�

     I2C_send_one_char(slave);       //�����豸��ַ(д)
     I2C_check_ack();
     I2C_send_one_char(reg);         //���ʹ洢��Ԫ��ַ����0��ʼ
     I2C_check_ack();
     	
     I2C_start();                    //��ʼ�źţ���ʾ���淢���Ǵӻ���ַ�������ǵ������ֽڣ�����
     
     I2C_send_one_char(slave+1);     //�����豸��ַ(��)
     I2C_check_ack();
     
     while(num) 
     {   
         I2C_recv_one_char(s);     //�����Ĵ�������
         if (num == 1)
            I2C_no_ack();          //��Ӧ��ֹͣ����
         else
            I2C_ack();             //��Ӧ�𣬼�������
         s++;
         num--;
     }
     
     I2C_stop();                     //ֹͣ�ź�
} 


//������������˳��
int8_t i2cwrite(uint8_t addr, uint8_t reg, uint8_t len, uint8_t * data)
{
	I2C_send_str(addr,reg,data,len);
	return 0;
}
int8_t i2cread(uint8_t addr, uint8_t reg, uint8_t len, uint8_t * buf)
{
	I2C_recv_str(addr,reg,buf,len);
	return 0;
}

