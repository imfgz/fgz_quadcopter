/*
 * NRF24L01.c
 *
 *  Created on: 2014-12-5
 *      Author: FGZ
 */
#include "NRF24L01.h"
//----------------------------------���ļ�ȫ�ֱ���-------------------------------//
static uint8_t TX_ADR_VAL[TX_ADR_WIDTH]       = {0x34,0x43,0x28,0x37,0x51};//���͵�ֵַ
static uint8_t RX_ADR_VAL_P0[RX_ADR_WIDTH_P0] = {0x34,0x43,0x28,0x37,0x51};//���յ�ֵַ

static uint8_t sta;
//static int8_t rx_fail_count=0;

//-------------------------------------�ڲ�����---------------------------------//
static void Flush_TX(void);
static void Flush_RX(void);
static void Clear_Status(void);
static void Clear_All(void);
static uint8_t SPI_Read_FIFO_Width(void);

/*
 *   ���ܣ�GPIOģ��SPIЭ�飬����ͨ��MOSI�ߣ���byte�ֽ�������ӻ���ͬʱͨ��MISO�ߣ���ȡ�ӻ����ص����ݡ�
 *   ������byte:Ҫ���͵��ֽںͷ��͵�ͬʱ�յ����ֽ� (WRITE_REG+reg / COMMAND)
 */
static uint8_t SPI_RW(uint8_t byte)//ģ��SPI
{
        uint8_t i;
        for(i=0; i<8; i++)   // ѭ��8��
        {
                if(byte & 0x80)
                        MOSI_H;      //����ֽڵ�MSB=1
                else
                        MOSI_L;      //����ֽڵ�MSB=0 ������ֽ�ͨ��������MOSI�ߴ�MSBѭ�����
                byte <<= 1;      //byte�Ĵθ�λ�Ƶ�MSB�ȴ�����
                SCK_H;           //MSB�������ʹӻ�ͬʱ����
                byte |= MISO;    //��ȡ�ӻ���MOSI״̬�������ֽ�ͨ��MISO��LSBѭ����������
                SCK_L;           //MISO������λ�Ĵ���
        }
    return(byte);        //���ض������ֽ�
}

/*
 * 1��д�Ĵ�����ͬʱ���ؼĴ�����ʼֵ
 *    ��valueд��reg�Ĵ���
 *
 */
static uint8_t SPI_RW_Reg(uint8_t reg, uint8_t value)//д�Ĵ���
{
        uint8_t status;

        CSN_L;                //CNS=0��SPIͨ��ʹ��

        status = SPI_RW(reg); //���ͼĴ�����ַ��WRITE_REG+reg��
        SPI_RW(value);        //��valueд��reg�Ĵ�����

        CSN_H;                //CSN=1��SPIͨ��ʧ��

        return status;       //����״̬��
}

/*
 * 1�����Ĵ���
 *    ��reg�Ĵ�����ֵ
 */
static uint8_t SPI_Read_Reg(uint8_t reg)//���Ĵ���
{
        uint8_t reg_val;

        CSN_L;                 //CNS=0��SPIͨ��ʹ��

        reg_val = SPI_RW(reg); //���ͼĴ�����ַ��READ_REG+reg��
        reg_val = SPI_RW(0);   //���Ĵ���

        CSN_H;                 //CSN=1��SPIͨ��ʧ��

        return reg_val;       //���ؼĴ�������
}

/*
 * 2��дFIFO�����׵�ַΪreg�Ļ�����д����Ϊwidth������Buf
 */
static uint8_t SPI_Write_Buf(uint8_t reg, const uint8_t* Buf, uint8_t width)//дFIFO(����/��ַ)
{
        uint8_t status, i;

        CSN_L;                       //CNS=0��SPIͨ��ʹ��

        status = SPI_RW(reg);        //���ͼĴ�����ַ�����WRITE_REG+TX_ADDR��RX_ADDR_P0 / WR_TX_PLOAD��

        for(i=0; i < width; i++)     //width:TX_PLOAD_WIDTH��=RX_PLOAD_WIDTH_P0��
        {
                status = SPI_RW(Buf[i]); //��Fuf[i]���д��reg������
        }

        CSN_H;                       //CSN=1��SPIͨ��ʧ��
        return(status);             //����״̬��
}

/*
 * 2����FIFO�����׵�ַΪreg�Ļ������������Ϊwidth������Buf
 */
static uint8_t SPI_Read_Buf(uint8_t reg, uint8_t* Buf, uint8_t width)//��FIFO(����/��ַ)
{
        uint8_t status, i;

        CSN_L;                       //CNS=0��SPIͨ��ʹ��

        status = SPI_RW(reg);        //���ͼĴ�����ַ�����READ_REG+TX_ADDR��RX_ADDR_P0 / RD_RX_PLOAD��

        for(i=0; i < width; i++)     //width:RX_PLOAD_WIDTH_P0��=TX_PLOAD_WIDTH)
        {
                Buf[i] = SPI_RW(0);      //����ֽڶ�ȡreg������
        }

        CSN_H;                       //CSN=1��SPIͨ��ʧ��

        return(status);             //����״̬��
}

/*
 * 2��ʹ�ö�̬���س��ȹ��ܣ���ȡ����FIFO��
 *    �ӻ�������������Buf
 */
static uint8_t SPI_Read_PLOAD_DYNAMIC(uint8_t* Buf)//������FIFO
{
        uint8_t status, i;
        uint8_t width;

        width = SPI_Read_FIFO_Width();//��ȡ�������ݿ��

        CSN_L;                       //CNS=0��SPIͨ��ʹ��

        status = SPI_RW(RD_RX_PLOAD);//����RD_RX_PLOAD����

        for(i=0; i<width; i++)       //width:RX_PLOAD_WIDTH_P0��=TX_PLOAD_WIDTH)
        {
                Buf[i] = SPI_RW(0);      //����ֽڶ�ȡreg������
        }

        CSN_H;                       //CSN=1��SPIͨ��ʧ��

        return status;              //����״̬��
}

/*
 * ��RX_FIFO���ݵĿ��
 */
static uint8_t SPI_Read_FIFO_Width(void)
{
        uint8_t width;

        CSN_L;

        SPI_RW(RD_RX_PLOAD_WID);//����RD_RX_PLOAD_WID����

        width = SPI_RW(0);      //��ȡ���

        CSN_H;

        return width;
}

/*
 * ���TX_FIFO
 */
static void Flush_TX(void)
{
        CSN_L;
        SPI_RW(FLUSH_TX);      //���TX_FIFO
        CSN_H;
}

/*
 * ���RX_FIFO
 */
static void Flush_RX(void)
{
        CSN_L;
        SPI_RW(FLUSH_RX);      //���RX_FIFO
        CSN_H;
}

/*
 * �������״̬��־
 */
static void Clear_Status(void)
{
        SPI_RW_Reg(WRITE_REG + NRF_STATUS, 0xff);
}

/*
 * ���FIFO��״̬��־
 */
static void Clear_All(void)
{
        Flush_TX();
        Flush_RX();
        Clear_Status();
}

/*
 * �������ģʽ
 */
static void Power_Down(void)
{
        CE_L;
        SPI_RW_Reg(WRITE_REG + NRF_CONFIG,0x0d);  //PWR_UP=0
        CE_H;

        //delay_us(20);  //�ȴ�20us
}

//-------------------------------------�ⲿ����---------------------------------//

//--------------------------ģʽ����-----------------------------
//---------------------------------------------------------------
/*
 * NRF24L01��ʼ��
 */
void NRF_init(uint8_t mode)
{
        CE_L; //chip enable
        CSN_H;//spi disable
        SCK_L;//��ʼ��ʱ����

        if(mode == TX_MODE)
        {
                TX_Mode();
        }
        else
        {
                RX_Mode();
        }
}

/*
 * ����NRFΪ����ģʽ
 * �������ģʽ2��ֻҪTX_FIFO�ǿռ�����TX_Mode�Զ����з���
 */
void TX_Mode(void)
{
        //1��ģʽת��
        CE_L;                  //CE=0,ʹоƬ�������ģʽ1������WRITE_REG������

        //2���Ĵ�������
        //----------------------------------------
        Clear_All();           //���FIFO��״̬��־
        //----------------------------------------
        SPI_RW_Reg(WRITE_REG + EN_AA,     0x01);              //���ջ�P0���Զ�Ӧ����ʹ�ܣ�Ack��
        SPI_RW_Reg(WRITE_REG + EN_RXADDR, 0x01);              //���ջ�P0�Ľ��չ���ʹ��
        SPI_RW_Reg(WRITE_REG + SETUP_RETR,0x25);              //���ͻ��Զ��ط���ʱ750us���Զ��ط�5�� (250Kbps��������ʱ������40���ֽ���Ҫ320*1000/250000=1.28ms,�Զ��ط���ʱ����>250us�������ط�����̫�쵼�·��Ͷ˽��ղ���Ack�ź�)
        SPI_RW_Reg(WRITE_REG + RF_CH,       10);              //���ͻ���6�����ջ� ����Ƶ�� ���� frequency = 2400 + RF_CH [MHz]
        SPI_RW_Reg(WRITE_REG + RF_SETUP,  0x26);              //���ݴ�����250kbps�����书��0dBm
        if(DYNAMIC_PLOAD_LENGTH)  //ʹ�ö�̬���س��ȹ���
        {
                SPI_RW_Reg(WRITE_REG + FEATURE,   0x04);              //ʹ�ܶ�̬���س���
                SPI_RW_Reg(WRITE_REG + DYNPD,     0x01);                          //����DPL_P0
        }
        else                      //ʹ�þ�̬���س���
        {
                SPI_RW_Reg(WRITE_REG + RX_PW_P0, RX_PLOAD_WIDTH_P0);  //�趨��оƬRX_P0���� ��Ч���ݿ��=RX_PLOAD_WIDTH_P0
        }
        SPI_RW_Reg(WRITE_REG + SETUP_AW, RX_ADR_WIDTH_P0-2);  //�趨��оƬ6��RX���� ��ַ���=RX_ADR_WIDTH_P0
        SPI_Write_Buf(WRITE_REG + TX_ADDR,    TX_ADR_VAL,    TX_ADR_WIDTH);   //�趨��оƬ   TX����  ��ֵַ�����=TX_ADR_WIDTH��
        SPI_Write_Buf(WRITE_REG + RX_ADDR_P0, RX_ADR_VAL_P0, RX_ADR_WIDTH_P0);//�趨��оƬRX_P0����  ��ֵַ�����=RX_ADR_WIDTH_P0��
        //RX_ADDR_P0��Ҫ��Ϊ��ʹ��EN_AA
        SPI_RW_Reg(WRITE_REG + NRF_CONFIG,0x0e);              //�������жϣ�ʹ��CRC��16λCRCУ�飬PWR_UP=1��PRIM_RX=0����ģʽ
        //----------------------------------------

        //3��ģʽת��
        CE_H;//����ģʽ1 -> PRIM_RX=0��TX_FIFO�ա�CE=1 -> ����ģʽ2

        //CE=1������ģʽ2 -> TX_FIFO�ǿ� -> delay>130us -> TX_Mode  ( -> TX_FIFO�� -> ����ģʽ2)
}

/*
 * ����NRFΪ����ģʽ
 */
void RX_Mode(void)
{
        //1��ģʽת��
        CE_L;                  //CE=0,ʹоƬ�������ģʽ1������WRITE_REG������

        //2���Ĵ�������
        //----------------------------------------
        Clear_All();           //���FIFO��״̬��־
        //----------------------------------------
        SPI_RW_Reg(WRITE_REG + EN_AA,     0x01);              //���ջ�P0���Զ�Ӧ����ʹ�ܣ�Ack��
        SPI_RW_Reg(WRITE_REG + EN_RXADDR, 0x01);              //���ջ�P0�Ľ��չ���ʹ��
        SPI_RW_Reg(WRITE_REG + SETUP_RETR,0x25);              //���ͻ��Զ��ط���ʱ750us���Զ��ط�5�� (250Kbps��������ʱ������40���ֽ���Ҫ320*1000/250000=1.28ms,�Զ��ط���ʱ����>250us�������ط�����̫�쵼�·��Ͷ˽��ղ���Ack�ź�)
        SPI_RW_Reg(WRITE_REG + RF_CH,       10);              //���ͻ���6�����ջ� ����Ƶ�� ���� frequency = 2400 + RF_CH [MHz]
        SPI_RW_Reg(WRITE_REG + RF_SETUP,  0x26);              //���ݴ�����250kbps�����书��0dBm
        if(DYNAMIC_PLOAD_LENGTH)  //ʹ�ö�̬���س��ȹ���
        {
                SPI_RW_Reg(WRITE_REG + FEATURE,   0x04);              //ʹ�ܶ�̬���س���
                SPI_RW_Reg(WRITE_REG + DYNPD,     0x01);                          //����DPL_P0
        }
        else                      //ʹ�þ�̬���س���
        {
                SPI_RW_Reg(WRITE_REG + RX_PW_P0, RX_PLOAD_WIDTH_P0);  //�趨��оƬRX_P0���� ��Ч���ݿ��=RX_PLOAD_WIDTH_P0
        }
        SPI_RW_Reg(WRITE_REG + SETUP_AW, RX_ADR_WIDTH_P0-2);  //�趨��оƬ6��RX���� ��ַ���=RX_ADR_WIDTH_P0
        SPI_Write_Buf(WRITE_REG + TX_ADDR,    TX_ADR_VAL,    TX_ADR_WIDTH);   //�趨��оƬ   TX����  ��ֵַ�����=TX_ADR_WIDTH��
        SPI_Write_Buf(WRITE_REG + RX_ADDR_P0, RX_ADR_VAL_P0, RX_ADR_WIDTH_P0);//�趨��оƬRX_P0����  ��ֵַ�����=RX_ADR_WIDTH_P0��
        //RX_ADDR_P0��Ҫ��Ϊ��ʹ��EN_AA
        SPI_RW_Reg(WRITE_REG + NRF_CONFIG,0x0f);              //�������жϣ�ʹ��CRC��16λCRCУ�飬PWR_UP=1��PRIM_RX=1����ģʽ
        //----------------------------------------

        //3��ģʽת��
        CE_H; //����ģʽ1 -> PRIM_RX=1��CE=1 -> delay>130us -> RX_Mode
}

/*
 *��������NRFΪ����ģʽ�������ģʽ2
 */
void TX_Mode_FAST(void)
{
        //1��ģʽת��
        CE_L;                  //CE=0,ʹоƬ�������ģʽ1������WRITE_REG������

        //2���Ĵ�������
        //----------------------------------------
        Clear_All();           //���FIFO��״̬��־
        SPI_RW_Reg(WRITE_REG + NRF_CONFIG, 0x0e); //�������жϣ�ʹ��CRC��16λCRCУ�飬PWR_UP=1��PRIM_RX=0����ģʽ
        //----------------------------------------
        //3��ģʽת��
        CE_H;//����ģʽ1 -> PRIM_RX=0��TX_FIFO�ա�CE=1 -> ����ģʽ2

        //CE=1������ģʽ2 -> TX_FIFO�ǿ� -> delay>130us -> TX_Mode  ( -> TX_FIFO�� -> ����ģʽ2)
}

/*
 * ��������NRFΪ����ģʽ������RX_Mode
 */
void RX_Mode_FAST(void)
{
        //1��ģʽת��
        CE_L;                  //CE=0,ʹоƬ�������ģʽ1������WRITE_REG������

        //2���Ĵ�������
        //----------------------------------------
        Clear_All();           //���FIFO��״̬��־
        SPI_RW_Reg(WRITE_REG + NRF_CONFIG, 0x0f);//�������жϣ�ʹ��CRC��16λCRCУ�飬PWR_UP=1��PRIM_RX=1����ģʽ
        //----------------------------------------
        //3��ģʽת��
        CE_H; //����ģʽ1 -> PRIM_RX=1��CE=1 -> delay>130us -> RX_Mode
}
//---------------------------------------------------------------
//---------------------------------------------------------------

//----�����ա�������������TX���ACK�źţ�RX�жϽ��գ���������----
//---------------------------------------------------------------
/*
 * ���TX_FIFO���������ͣ��ȴ�Ack�ź�
 * ������Bufд��TX_FIFO�У����������䣬
 * ���TX_FIFO�������� -> �ȴ�Ack�ж� -> ��ȡ״̬�Ĵ��� -> �жϴ��� -> ���FIFO��״̬�Ĵ��� -> �ٴν������ģʽ2 �ȴ�����
 */
int8_t Tx_Packet(const uint8_t* Buf, uint8_t num)
{
        int8_t result=0;//�շ���������ͳɹ���1������ʧ�ܣ�-1 ���ճɹ���2 ����ʧ�ܣ�-2  ���ݰ�����0  ������3��

        //num = _constrain(num,1,32);//num�޷�

        //TX���Ѿ��� ����ģʽ2 ���ɽ���WRITE_REG������
        if(DYNAMIC_PLOAD_LENGTH)
        {
                SPI_Write_Buf(WR_TX_PLOAD, Buf, num);//���TX_FIFO��WR_TX_PLOAD -> TX_FIFO��Ч���ݻ�������дFIFO��
        }
        else
        {
                SPI_Write_Buf(WR_TX_PLOAD, Buf, TX_PLOAD_WIDTH);//���TX_FIFO��WR_TX_PLOAD -> TX_FIFO��Ч���ݻ�������дFIFO��
        }

        CE_H;             //����ģʽ2 -> TX_FIFO�ǿա�CE=1 -> 130us -> TX_Mode

        while(IRQ_READ);  //�ȴ�TX_DS�ж� (�յ����ն˵�Ack�ź�)���� MAX_RT�ж� (δ�յ����ն˵�Ack�ź�)

        sta = SPI_Read_Reg(READ_REG+NRF_STATUS); //��ȡ״̬�Ĵ���

        if(TX_DS) //�Զ����TX_FIFO������CE=1���ٴ� �������ģʽ2 ���ɽ���WRITE_REG������
        {
                Clear_Status();  //���״̬�Ĵ�������λIRQ

                result = 1;      //���ͳɹ�����1
        }
        else if(MAX_RT) //�����ط��ж�
        {

                Flush_TX(); //���TX_FIFO������CE=1���������ģʽ2���ſɽ���WRITE_REG������

                Clear_Status();    //���״̬�Ĵ���(MAX_RT)����λIRQ

                result = -1;       //����ʧ�ܷ���-1
        }
        else
        {
                Clear_All();      //���FIFO��״̬��־����λIRQ

                result =  3;     //��������3
        }

        return result;
}

/*
 * �ȴ�IRQ�����жϣ���RX_FIFO�е����ݶ�������Buf��
 * ��ѯIRQ_READ�ж� -> �������ģʽ1 -> ��ȡ״̬�Ĵ��� -> �ж��ж����� -> ��ȡRX_FIFO -> ���FIFO��״̬�Ĵ��� -> CE_Hϵͳ�ٴν���RX_Mode
 */
int8_t Rx_Packet(uint8_t* Buf)
{
        int8_t result=0;//�շ���������ͳɹ���1������ʧ�ܣ�-1 ���ճɹ���2 ����ʧ�ܣ�-2  ���ݰ�����0  ������3��

        while(IRQ_READ); //ȷ�Ͻ��յ���Ч���ݰ��󣬲���RX_DR�жϣ����Զ�����Ack�ź�
                           //NRF_STATUS �� RX_P_NO ��ʾ���ν����ж�����һ�����ջ�����

        CE_L;  //CE=0 �������ģʽ1

        sta = SPI_Read_Reg(READ_REG+NRF_STATUS); //��ȡ״̬�Ĵ���

        if(RX_DR) //�ж��Ƿ��ǽ����ж�
        {
                //��д����
                if(DYNAMIC_PLOAD_LENGTH)
                {
                        SPI_Read_PLOAD_DYNAMIC(Buf);
                }
                else
                {
                        SPI_Read_Buf(RD_RX_PLOAD, Buf, RX_PLOAD_WIDTH_P0);  //SPI��ȡRX_FIFO�е���Ч����
                }

                Clear_Status(); //���״̬�Ĵ�������λIRQ

                result = 2;     //���ճɹ�����2
        }
        else
        {
                Clear_All();     //���FIFO��״̬��־����λIRQ

                result =  3;     //��������3
        }

        CE_H;  //�ٴν���RX_Mode

        return result;
}

/*
 * ���TX_FIFO���������ͣ����Ack�ź�
 */
int8_t Tx_Packet_16(const int16_t* Buf,uint8_t num)//(int16)
{
        static int16_union dat;
        static uint8_t dat_8[32];
        uint8_t i=0;
        int8_t result; //�շ���������ͳɹ���1������ʧ�ܣ�-1 ���ճɹ���2 ����ʧ�ܣ�-2  ���ݰ�����0  ������3��

        for(i=0; i<num; i++) //�ֽڲ��
        {
                dat.dat_16 = Buf[i];
                dat_8[i*2] = dat.dat_8.dat1;
                dat_8[i*2+1] = dat.dat_8.dat2;
        }

        result = Tx_Packet(dat_8 ,2*num);  //�������ݰ���2*num<=TX_PLOAD_WIDTH��

        return result;
}

/*
 * �ȴ�IRQ�����жϣ���RX_FIFO�е����ݶ�������Buf��
 */
int8_t Rx_Packet_16(int16_t* Buf,uint8_t num)//(int16)
{
        static int16_union dat;
        static uint8_t dat_8[32];
        uint8_t i=0;
        int8_t result; //�շ���������ͳɹ���1������ʧ�ܣ�-1 ���ճɹ���2 ����ʧ�ܣ�-2  ���ݰ�����0  ������3��

        result = Rx_Packet(dat_8); //�������ݰ���<=RX_PLOAD_WIDTH_P0��

        for(i=0; i<num; i++) //�ֽڲ��
        {
                dat.dat_8.dat1 = dat_8[i*2];
                dat.dat_8.dat2 = dat_8[i*2+1];
                Buf[i] = dat.dat_16;
        }

        return result;
}
//---------------------------------------------------------------
//---------------------------------------------------------------

//---�����ա���һ�庯����TX���ACK�źţ�RX�жϽ��գ�����˫����---
//---------------------------------------------------------------
/*
 * ����ת���շ�ģʽ
 * ˫��ͨ�ŵ�TX���շ����������淢��TX_Buf�ͽ���BX_Buf
 */
int8_t Tx_Rx_Packet_TX_FAST(const uint8_t* TX_Buf, uint8_t TX_num, uint8_t* RX_Buf)
{
        int8_t result=0;//�շ���������ͳɹ���1������ʧ�ܣ�-1 ���ճɹ���2 ����ʧ�ܣ�-2  ���ݰ�����0  ������3��
        static uint8_t status = TX_MODE;//��ʼ��ΪTX��

        //TX_num = _constrain(TX_num,1,32);

        if(status == TX_MODE)
        {
                //TX���Ѿ��� ����ģʽ2 ���ɽ���WRITE_REG������
                if(DYNAMIC_PLOAD_LENGTH)
                {
                        SPI_Write_Buf(WR_TX_PLOAD, TX_Buf, TX_num);//���TX_FIFO��WR_TX_PLOAD -> TX_FIFO��Ч���ݻ�������дFIFO��
                }
                else
                {
                        SPI_Write_Buf(WR_TX_PLOAD, TX_Buf, TX_PLOAD_WIDTH);//���TX_FIFO��WR_TX_PLOAD -> TX_FIFO��Ч���ݻ�������дFIFO��
                }

                CE_H;             //����ģʽ2 -> TX_FIFO�ǿա�CE=1 -> 130us -> TX_Mode

                //TX��ɺ�P0���Զ�����Ϊ����ģʽ������Ack�źţ�Լ����RX_ADDR_P0=TX_ADDR
                while(IRQ_READ);  //�ȴ�TX_DS�ж� (�յ����ն˵�Ack�ź�)���� MAX_RT�ж� (δ�յ����ն˵�Ack�ź�)

                sta = SPI_Read_Reg(READ_REG+NRF_STATUS); //��ȡ״̬�Ĵ���

                if(TX_DS) //�Զ����TX_FIFO������CE=1���ٴ� �������ģʽ2 ���ɽ���WRITE_REG������
                {
                        Clear_Status(); //���״̬�Ĵ�������λIRQ
                        //--------------------------------------
                        RX_Mode_FAST();
                        status = RX_MODE; //���ͳɹ�����ʼ����
                        //--------------------------------------
                        result = 1;      //���ͳɹ�����1
                }
                else if(MAX_RT) //�����ط��ж�
                {
                        Flush_TX();; //���TX_FIFO������CE=1���������ģʽ2���ſɽ���WRITE_REG������

                        Clear_Status();                          //���״̬�Ĵ���(MAX_RT)����λIRQ
                        //--------------------------------------
                        RX_Mode_FAST();
                        status = RX_MODE;//1��TX�˷���ʧ�ܣ�ת��Ϊ���գ�����
                        //--------------------------------------
                        result = -1;       //����ʧ�ܷ���-1
                }
                else
                {
                        Clear_All();      //���FIFO��״̬��־����λIRQ

                        result =  3;      //��������3
                }
        }
        else  //RX_MODE
        {
                if(!IRQ_READ) //��ѯ����
                {

                        CE_L;  //CE=0 �������ģʽ1

                        sta = SPI_Read_Reg(READ_REG+NRF_STATUS); //��ȡ״̬�Ĵ���

                        if(RX_DR) //�ж��Ƿ��ǽ����ж�
                        {
                                //��д����
                                if(DYNAMIC_PLOAD_LENGTH)
                                {
                                        SPI_Read_PLOAD_DYNAMIC(RX_Buf);
                                }
                                else
                                {
                                        SPI_Read_Buf(RD_RX_PLOAD, RX_Buf, RX_PLOAD_WIDTH_P0);  //SPI��ȡRX_FIFO�е���Ч����
                                }

                                Clear_Status();                          //���״̬�Ĵ�������λIRQ
                                //--------------------------------------
                                TX_Mode_FAST();
                                status = TX_MODE;//���ճɹ�����ʼ���� ������ģʽ2��
                                //--------------------------------------
                                result = 2;   //���ճɹ�����2
                        }
                        else
                        {
                                Clear_All();                             //���FIFO��״̬��־����λIRQ

                                CE_H;  //�ٴν���RX_Mode

                                result =  3;     //��������3
                        }
                }
                else
                {
                        status = status;  //2��TX�˽���ʧ�ܣ��������գ�����

                        result =  -2;     //����ʧ�ܷ���-2
                }
        }
        return result;
}

/*
 * ����ת���շ�ģʽ
 * ˫��ͨ�ŵ�RX���շ����������淢��TX_Buf�ͽ���BX_Buf
 */
int8_t Tx_Rx_Packet_RX_FAST(const uint8_t* TX_Buf ,uint8_t TX_num, uint8_t* RX_Buf)
{
        int8_t result=0;//�շ���������ͳɹ���1������ʧ�ܣ�-1 ���ճɹ���2 ����ʧ�ܣ�-2  ���ݰ�����0  ������3��
        static uint8_t status = RX_MODE;//��ʼ��ΪRX��
        static uint8_t rx_failed=0;//����ʧ�ܼ���������ʧ�����ж�״̬��ʾ�����Բ��������������ˣ�

        //TX_num = _constrain(TX_num,1,32);

        rx_failed++;
        //rx_failed = _constrain(rx_failed,1,50);

        if(status == TX_MODE)
        {
                //TX���Ѿ��� ����ģʽ2 ���ɽ���WRITE_REG������
                if(DYNAMIC_PLOAD_LENGTH)
                {
                        SPI_Write_Buf(WR_TX_PLOAD, TX_Buf, TX_num);//���TX_FIFO��WR_TX_PLOAD -> TX_FIFO��Ч���ݻ�������дFIFO��
                }
                else
                {
                        SPI_Write_Buf(WR_TX_PLOAD, TX_Buf, TX_PLOAD_WIDTH);//���TX_FIFO��WR_TX_PLOAD -> TX_FIFO��Ч���ݻ�������дFIFO��
                }

                CE_H;             //����ģʽ2 -> TX_FIFO�ǿա�CE=1 -> 130us -> TX_Mode

                //TX��ɺ�P0���Զ�����Ϊ����ģʽ������Ack�źţ�Լ����RX_ADDR_P0=TX_ADDR
                while(IRQ_READ);  //�ȴ�TX_DS�ж� (�յ����ն˵�Ack�ź�)���� MAX_RT�ж� (δ�յ����ն˵�Ack�ź�)

                sta = SPI_Read_Reg(READ_REG+NRF_STATUS); //��ȡ״̬�Ĵ���

                if(TX_DS) //�Զ����TX_FIFO������CE=1���ٴ� �������ģʽ2 ���ɽ���WRITE_REG������
                {
                        Clear_Status();                          //���״̬�Ĵ�������λIRQ
                        //--------------------------------------
                        RX_Mode_FAST();
                        status = RX_MODE;
                        rx_failed = 0;  //���ͳɹ�����ʼ����
                        //--------------------------------------
                        result = 1;      //���ͳɹ�����1
                }
                else if(MAX_RT) //�����ط��ж�
                {
                        Flush_TX();; //���TX_FIFO������CE=1���������ģʽ2���ſɽ���WRITE_REG������

                        Clear_Status();      //���״̬�Ĵ���(MAX_RT)����λIRQ

                        status = status;     //3��RX�˷���ʧ�ܣ��������ͣ�����

                        result = -1;       //����ʧ�ܷ���-1
                }
                else
                {
                        Clear_All();      //���FIFO��״̬��־����λIRQ

                        result =  3;      //��������3
                }
        }
        else  //RX_MODE
        {
                if(!IRQ_READ) //��ѯ����
                {

                        CE_L;  //CE=0 �������ģʽ1���ɽ���WRITE_REG��SPI������

                        sta = SPI_Read_Reg(READ_REG+NRF_STATUS); //��ȡ״̬�Ĵ���

                        if(RX_DR) //�ж��Ƿ��ǽ����ж�
                        {
                                //��д����
                                if(DYNAMIC_PLOAD_LENGTH)
                                {
                                        SPI_Read_PLOAD_DYNAMIC(RX_Buf);
                                }
                                else
                                {
                                        SPI_Read_Buf(RD_RX_PLOAD, RX_Buf, RX_PLOAD_WIDTH_P0);  //SPI��ȡRX_FIFO�е���Ч����
                                }

                                Clear_Status();                          //���״̬�Ĵ�������λIRQ
                                //--------------------------------------
                                TX_Mode_FAST();
                                status = TX_MODE;
                                rx_failed=0;  //���ճɹ�����ʼ���ͣ�����ģʽ2��
                                //--------------------------------------
                                result = 2;   //���ճɹ�����2
                        }
                        else
                        {
                                Clear_All();  //���FIFO��״̬��־����λIRQ

                                CE_H;  //�ٴν���RX_Mode

                                result =  3;     //��������3
                        }
                }
                //--------------------------------------
                else if(rx_failed > 10)
                {
                        TX_Mode_FAST();
                        status = TX_MODE;  //4��RX�˽���ʧ�ܣ�ת��Ϊ���ͣ�����������ģʽ2��
                        rx_failed=0;

                        result =  -2;     //����ʧ�ܷ���-2
                }
                else
                {
                        status = status;  //״̬���䣬��������

                        result =  3;     //��������3
                }
                //--------------------------------------
        }
        return result;
}

/*
 * ˫��ͨ�ŵ�TX���շ�����������ת���շ�ģʽ
 */
int8_t Tx_Rx_Packet_TX_FAST_16(const int16_t* TX_Buf,uint8_t TX_num, int16_t* RX_Buf, uint8_t RX_num)//int16
{
        static uint8_t TX_dat_8[32];
        static uint8_t RX_dat_8[32];
        static int16_union dat;
        uint8_t i=0;
        int8_t result; //�շ���������ͳɹ���1������ʧ�ܣ�-1 ���ճɹ���2 ����ʧ�ܣ�-2  ���ݰ�����0  ������3��

        for(i=0; i<TX_num; i++) //�ֽڲ��
        {
                dat.dat_16 = TX_Buf[i];
                TX_dat_8[i*2] = dat.dat_8.dat1;
                TX_dat_8[i*2+1] = dat.dat_8.dat2;
        }

        result = Tx_Rx_Packet_TX_FAST(TX_dat_8, 2*TX_num, RX_dat_8); //�շ����ݰ���2*num<=TX_PLOAD_WIDTH��

        for(i=0; i<RX_num; i++) //�ֽڲ��
        {
                dat.dat_8.dat1 = RX_dat_8[i*2];
                dat.dat_8.dat2 = RX_dat_8[i*2+1];
                RX_Buf[i] = dat.dat_16;
        }

        return result;
}

/*
 * ˫��ͨ�ŵ�RX���շ�����������ת���շ�ģʽ
 */
int8_t Tx_Rx_Packet_RX_FAST_16(const int16_t* TX_Buf,uint8_t TX_num, int16_t* RX_Buf, uint8_t RX_num)//int16
{
        static uint8_t TX_dat_8[32];
        static uint8_t RX_dat_8[32];
        static int16_union dat;
        uint8_t i=0;
        int8_t result; //�շ���������ͳɹ���1������ʧ�ܣ�-1 ���ճɹ���2 ����ʧ�ܣ�-2  ���ݰ�����0  ������3��

        for(i=0; i<TX_num; i++) //�ֽڲ��
        {
                dat.dat_16 = TX_Buf[i];
                TX_dat_8[i*2] = dat.dat_8.dat1;
                TX_dat_8[i*2+1] = dat.dat_8.dat2;
        }

        result = Tx_Rx_Packet_RX_FAST(TX_dat_8, 2*TX_num, RX_dat_8); //�շ����ݰ���2*num<=TX_PLOAD_WIDTH��

        for(i=0; i<RX_num; i++) //�ֽڲ��
        {
                dat.dat_8.dat1 = RX_dat_8[i*2];
                dat.dat_8.dat2 = RX_dat_8[i*2+1];
                RX_Buf[i] = dat.dat_16;
        }

        return result;
}

//--------�ǳ��淢�ͺ����������Ack�źţ�(����˫��)--------------
//---------------------------------------------------------------
/*
 * ���TX_FIFO���������ͣ������Ack�ź�
 * ������Buf(uint8_t)д��TX_FIFO�У�����������
 */
void Tx_Packet_Noack(const uint8_t* Buf, uint8_t num)
{
        //num = _constrain(num,1,32);

        //TX���Ѿ��� ����ģʽ2 ���ɽ���WRITE_REG������
        if(DYNAMIC_PLOAD_LENGTH)
        {
                SPI_Write_Buf(WR_TX_PLOAD, Buf, num);//���TX_FIFO��WR_TX_PLOAD -> TX_FIFO��Ч���ݻ�������дFIFO��
        }
        else
        {
                SPI_Write_Buf(WR_TX_PLOAD, Buf, TX_PLOAD_WIDTH);//���TX_FIFO��WR_TX_PLOAD -> TX_FIFO��Ч���ݻ�������дFIFO��
        }

        CE_H;             //����ģʽ2 -> TX_FIFO�ǿա�CE=1 -> 130us -> TX_Mode
}

/*
 * ���TX_FIFO���������ͣ������Ack�ź�
 * ������Buf(int16)д��TX_FIFO�У�����������
 */
void Tx_Packet_Noack_16(const int16_t* Buf, uint8_t num)
{
        static int16_union dat;
        static uint8_t dat_8[32];
        uint8_t i=0;

        for(i=0; i<num; i++) //�ֽڲ��
        {
                dat.dat_16 = Buf[i];
                dat_8[i*2] = dat.dat_8.dat1;
                dat_8[i*2+1] = dat.dat_8.dat2;
        }

        Tx_Packet_Noack(dat_8 ,2*num);  //�������ݰ���2*num<=TX_PLOAD_WIDTH��
}
//---------------------------------------------------------------
//---------------------------------------------------------------


//�ǳ���IQR�жϴ�����(ֻ���ж��д��������ж�TX_DS��RX_DR��MAX_RT)��������
//---------------------------------------------------------------

/*
 * NRF�ж��źŴ���(TX��)
 */
int8_t NRF_IQR_hander_TX(void)
{
        int8_t result;//�շ���������ͳɹ���1������ʧ�ܣ�-1 ���ճɹ���2 ����ʧ�ܣ�-2  ���ݰ�����0  ������3��

        sta = SPI_Read_Reg(READ_REG+NRF_STATUS); //��ȡ״̬�Ĵ���

        if(TX_DS) //���ͳɹ��жϣ��Զ����TX_FIFO������CE=1���ٴ� �������ģʽ2 ���ɽ���WRITE_REG������
        {
                CE_L;            //�������ģʽ1
                Clear_All();     //���FIFO��״̬��־����λIRQ
                CE_H;            //�������ģʽ2(����ģʽ1 -> PRIM_RX=0��CE=1 -> ����ģʽ2 ���ɽ���WRITE_REG������)

                //--------------------------------------
                //...
                //--------------------------------------

                //g_ctrl_signal_check = 0;//���ͳɹ�����������

                result = 1;      //���ͳɹ�����1
        }
        else if(MAX_RT)  //����ط��ж�
        {
                //Flush_TX();; //���TX_FIFO������CE=1���������ģʽ2���ſɽ���WRITE_REG������

                CE_L;            //�������ģʽ1
                Clear_All();     //���FIFO��״̬��־����λIRQ
                CE_H;            //�������ģʽ2(����ģʽ1 -> PRIM_RX=0��CE=1 -> ����ģʽ2 ���ɽ���WRITE_REG������)

                //--------------------------------------
                //...
                //--------------------------------------

                result = -1;       //����ʧ�ܷ���-1
        } //���ͳɹ���ת��Ϊ����ģʽ
        else
        {
                CE_L;            //�������ģʽ1
                Clear_All();     //���FIFO��״̬��־����λIRQ
                CE_H;            //�������ģʽ2(����ģʽ1 -> PRIM_RX=0��CE=1 -> ����ģʽ2 ���ɽ���WRITE_REG������)


                result = 3;      //�����������3
        }
        return result;
}

/*
 * NRF�ж��źŴ���(RX��)���жϽ��պ���
 */
int8_t NRF_IQR_hander_RX(uint8_t* Buf)
{
        int8_t result;//�շ���������ͳɹ���1������ʧ�ܣ�-1 ���ճɹ���2 ����ʧ�ܣ�-2  ���ݰ�����0  ������3��

        sta = SPI_Read_Reg(READ_REG+NRF_STATUS); //��ȡ״̬�Ĵ���

        if(RX_DR) //�����ж�
        {
                if(DYNAMIC_PLOAD_LENGTH)//��RX_FIFO
                {
                        SPI_Read_PLOAD_DYNAMIC(Buf);
                }
                else
                {
                        SPI_Read_Buf(RD_RX_PLOAD, Buf, RX_PLOAD_WIDTH_P0);  //SPI��ȡRX_FIFO�е���Ч����
                }

                //Ack�źŷ�����ɺ�PRIM_RX=1 CE=1,��ʱϵͳ���Ǵ��ڽ���ģʽ

                CE_L;            //�������ģʽ1
                Clear_All();     //���FIFO��״̬��־����λIRQ
                CE_H;            //130us�󣬽������ģʽ

                //--------------------------------------
                //rx_fail_count=0;  //���ռ�ر�������
                //--------------------------------------
                result = 2;   //���ճɹ�����2
        }
        else
        {
                CE_L;            //�������ģʽ1
                Clear_All();     //���FIFO��״̬��־����λIRQ
                CE_H;            //130us�󣬽������ģʽ

                result = 3;     //�����������3
        }
        return result;
}

/*
 * NRF�ж��źŴ���(RX��)���жϽ���
 */
int8_t NRF_IQR_hander_RX_16(int16_t* Buf,uint8_t num)//(int16)
{
        static int16_union dat;
        static uint8_t dat_8[32];
        uint8_t i=0;
        int8_t result; //�շ���������ͳɹ���1������ʧ�ܣ�-1 ���ճɹ���2 ����ʧ�ܣ�-2  ���ݰ�����0  ������3��

        result = NRF_IQR_hander_RX(dat_8); //�������ݰ���2*num<=RX_PLOAD_WIDTH_P0��

        for(i=0; i<num; i++) //�ֽڲ��
        {
                dat.dat_8.dat1 = dat_8[i*2];
                dat.dat_8.dat2 = dat_8[i*2+1];
                Buf[i] = dat.dat_16;
        }
        return result;
}
//---------------------------------------------------------------
//---------------------------------------------------------------


//�ǳ���IQR�жϴ�����(���д��������ж�TX_DS��RX_DR��MAX_RT)��˫����
//---------------------------------------------------------------
/*
 * NRF�ж��źŴ���(TX��)
 * �жϽ��գ�ͨ�ųɹ�����ת��ģʽ
 */
int8_t TX_RX_NRF_IQR_hander_TX(uint8_t* Buf)
{
        int8_t result;//�շ���������ͳɹ���1������ʧ�ܣ�-1 ���ճɹ���2 ����ʧ�ܣ�-2  ���ݰ�����0  ������3��

        sta = SPI_Read_Reg(READ_REG+NRF_STATUS); //��ȡ״̬�Ĵ���

        if(TX_DS) //���ͳɹ��жϣ��Զ����TX_FIFO������CE=1���ٴ� �������ģʽ2 ���ɽ���WRITE_REG������
        {
                Power_Down();  //�� ����ģʽ2 ���� ����ģʽ

                //--------------------------------------
                RX_Mode_FAST();//�� ����ģʽ ���� ����ģʽ1 �ٽ��� ����ģʽ(���ͳɹ�����ʼ����)
                //--------------------------------------

                result = 1;      //���ͳɹ�����1
        }
        else if(MAX_RT)  //����ط��ж�
        {
                Flush_TX();; //���TX_FIFO������CE=1���������ģʽ2���ſɽ���WRITE_REG������

                Clear_Status();                          //���״̬�Ĵ���(MAX_RT)����λIRQ

                //--------------------------------------
                              //1��TX�˷���ʧ�ܣ��������ͣ�����
                //--------------------------------------

                result = -1;       //����ʧ�ܷ���-1
                //g_global[24]++;
        } //���ͳɹ���ת��Ϊ����ģʽ
        else if(RX_DR) //�����ж�
        {
                if(DYNAMIC_PLOAD_LENGTH) //��RX_FIFO
                {
                        SPI_Read_PLOAD_DYNAMIC(Buf);
                }
                else
                {
                        SPI_Read_Buf(RD_RX_PLOAD, Buf, RX_PLOAD_WIDTH_P0);  //SPI��ȡRX_FIFO�е���Ч����
                }

                //--------------------------------------
                //delay_us(500); //�ȴ�Ack�źŷ������
                //Ack�źŷ�����ɺ�PRIM_RX=1 CE=1,��ʱϵͳ���Ǵ��ڽ���ģʽ

                //rx_fail_count=0;  //���ռ�ر�������
                //--------------------------------------

                result = 2;   //���ճɹ�����2
        }
        else
        {
                Clear_All();                             //���FIFO��״̬��־����λIRQ

                result =  3;     //�����������3
        }

        return result;
}

/*
 * NRF�ж��źŴ���(RX��)
 */
int8_t TX_RX_NRF_IQR_hander_RX(uint8_t* Buf)
{
        int8_t result;//�շ���������ͳɹ���1������ʧ�ܣ�-1 ���ճɹ���2 ����ʧ�ܣ�-2  ���ݰ�����0  ������3��

        sta = SPI_Read_Reg(READ_REG+NRF_STATUS); //��ȡ״̬�Ĵ���

        if(TX_DS) //���ͳɹ��жϣ��Զ����TX_FIFO������CE=1���ٴ� �������ģʽ2 ���ɽ���WRITE_REG������
        {
                Power_Down();  //�� ����ģʽ2 ���� ����ģʽ

                //--------------------------------------
                RX_Mode_FAST();//�� ����ģʽ ���� ����ģʽ1 �ٽ��� ����ģʽ(���ͳɹ�����ʼ����)
                //--------------------------------------

                result = 1;      //���ͳɹ�����1
        }
        else if(MAX_RT)  //����ط��ж�
        {
                //Flush_TX();; //���TX_FIFO������CE=1���������ģʽ2���ſɽ���WRITE_REG������

                Power_Down(); //�� ����ģʽ2 ���� ����ģʽ

                //--------------------------------------
                RX_Mode_FAST();//�� ����ģʽ ���� ����ģʽ1 �ٽ��� ����ģʽ(2��RX�˷���ʧ�ܣ�ת����ģʽ������)
                //--------------------------------------

                result = -1;       //����ʧ�ܷ���-1
        }//���ͳɹ���ת��Ϊ����ģʽ
        else if(RX_DR) //�����ж�
        {
                if(DYNAMIC_PLOAD_LENGTH)//��RX_FIFO
                {
                        SPI_Read_PLOAD_DYNAMIC(Buf);
                }
                else
                {
                        SPI_Read_Buf(RD_RX_PLOAD, Buf, RX_PLOAD_WIDTH_P0);  //SPI��ȡRX_FIFO�е���Ч����
                }

                //--------------------------------------
                //delay_us(500); //�ȴ�Ack�źŷ������


                TX_Mode_FAST();//�������ģʽ2 (���ճɹ�����ʼ����)
                //Tx_Packet_Noack_16(g_RC_Back, 8); //RX�˽ӵ����ݷ���Ack�źź����Ϸ���һ�����ݰ���ȥ��TX�˽���(��������������Ҫ���ݸ�ң����)
                //delay_us(130);    //�ȴ�130us���뷢��ģʽ

                //rx_fail_count=0;  //���ռ�ر�������
                //--------------------------------------

                result = 2;   //���ճɹ�����2
        }
        else
        {
                Power_Down();  //�� ����ģʽ2 ���� ����ģʽ

                //--------------------------------------
                RX_Mode_FAST();//�� ����ģʽ ���� ����ģʽ1 �ٽ��� ����ģʽ(���ͳɹ�����ʼ����)
                //--------------------------------------

                result = 3;     //�����������3
        }
        return result;
}

/*
 * NRF�ж��źŴ���(TX��)
 * �жϽ��գ�ͨ�ųɹ�����ת��ģʽ
 */
int8_t TX_RX_NRF_IQR_hander_TX_16(int16_t* Buf,uint8_t num)//(int16)
{
        static int16_union dat;
        static uint8_t dat_8[32];
        uint8_t i=0;
        int8_t result; //�շ���������ͳɹ���1������ʧ�ܣ�-1 ���ճɹ���2 ����ʧ�ܣ�-2  ���ݰ�����0  ������3��

        result = TX_RX_NRF_IQR_hander_TX(dat_8); //�������ݰ���2*num<=RX_PLOAD_WIDTH_P0��

        for(i=0; i<num; i++) //�ֽڲ��
        {
                dat.dat_8.dat1 = dat_8[i*2];
                dat.dat_8.dat2 = dat_8[i*2+1];
                Buf[i] = dat.dat_16;
        }
        return result;
}

/*
 * NRF�ж��źŴ���(RX��)
 * �жϽ���(int16)��ͨ�ųɹ�����ת��ģʽ
 */
int8_t TX_RX_NRF_IQR_hander_RX_16(int16_t* Buf,uint8_t num)//(int16)
{
        static int16_union dat;
        static uint8_t dat_8[32];
        uint8_t i=0;
        int8_t result; //�շ���������ͳɹ���1������ʧ�ܣ�-1 ���ճɹ���2 ����ʧ�ܣ�-2  ���ݰ�����0  ������3��

        result = TX_RX_NRF_IQR_hander_RX(dat_8); //�������ݰ���2*num<=RX_PLOAD_WIDTH_P0��

        for(i=0; i<num; i++) //�ֽڲ��
        {
                dat.dat_8.dat1 = dat_8[i*2];
                dat.dat_8.dat2 = dat_8[i*2+1];
                Buf[i] = dat.dat_16;
        }
        return result;
}
//---------------------------------------------------------------
//---------------------------------------------------------------

#if 0
//-------------------IQR�����жϼ�⺯����˫����-----------------
//---------------------------------------------------------------
/*
 * ����ؽ�ʧ�ܼ�⣨ÿ���������ڶ�NRF�����жϽ���һ�μ�⣬��IRQ�ж��н����������㣩
 * ʧ�ܣ�����1
 * �ɹ�������0
 */
int8_t MAX_RR(void)
{
        const int8_t ARC = 50; //����ؽӴ���

        if(g_period_count == 1)
                rx_fail_count = ARC;//��ʼ��rx_fail_count�����������ֵΪ0������Ϊ��ʱ�Ѿ��ӵ�һ�������ˣ���Ȼ����

        rx_fail_count++;
        rx_fail_count = _constrain(rx_fail_count,1,100);

        if(rx_fail_count > ARC) //�ﵽ�����մ�����ȷ�Ͻ���ʧ����
                return 1;
        else
                return 0;
}
#endif
