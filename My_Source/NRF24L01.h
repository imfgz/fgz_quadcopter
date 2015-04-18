/*
 * NRF24L01.h
 *
 *  Created on: 2014-12-5
 *      Author: FGZ
 */

#ifndef NRF24L01_H_
#define NRF24L01_H_

#include "Common.h"
#include "spi.h"

typedef union
{
    int16_t dat_16;
    struct
    {
            uint8_t dat1;//��λ
            uint8_t dat2;//��λ
    }dat_8;
}int16_union,*int16_union_ptr;

//-----------------------------------------------------------------
#define TX_ADR_WIDTH      5  //���͵�ַУ������(=SETUP_AW)
#define TX_PLOAD_WIDTH    16 //������Ч���ݿ��  (=RX_PW_P0)

#define RX_ADR_WIDTH_P0   5  //���յ�ַУ������(=SETUP_AW)
#define RX_PLOAD_WIDTH_P0 16 //������Ч���ݿ��  (=RX_PW_P0)

#define TX_MODE 1
#define RX_MODE 0

#define RX_DR   (sta & 0x40) >> 6
#define TX_DS   (sta & 0x20) >> 5
#define MAX_RT (sta & 0x10) >> 4

#define DYNAMIC_PLOAD_LENGTH   1 //1:DYNAMIC packet length, 0:STATIC
//-----------------------------------------------------------------

//-----------------------------------------------------------------
// nRF24L01) ���� �궨��
#define READ_REG           0x00  // 1�����Ĵ�������
#define WRITE_REG          0x20  // 1��д�Ĵ������ֻ���ڵ���ʹ���ģʽ�¿ɲ����������

#define RD_RX_PLOAD_WID    0x60  // 2����RX_FIFO���ݵĿ��(������>32������Flush RX_FIFO)��FEATURE��DYNPD��ع���Ҫʹ�ܣ�
#define RD_RX_PLOAD        0x61  // 2����RX_FIFO����
#define WR_TX_PLOAD        0xA0  // 2��дTX_FIFO����
#define WR_TX_PLOAD_NOACK  0xB0  // 2��дTX_FIFO���NO_ACK=1�����ն˲���ACK�źţ�
#define WR_ACK_PLOAD       0xA8  //    RX��дACK PAYLOAD ����

#define FLUSH_TX           0xE1  // 3����TX_FIFO����
#define FLUSH_RX           0xE2  // 3����RX_FIFO����

#define REUSE_TX_PL        0xE3  // �ظ�װ����������

#define NOP                0xFF  // �ղ�������
//-----------------------------------------------------------------

//-----------------------------------------------------------------
// nRF24L01) �Ĵ�����ַ �궨��
#define NRF_CONFIG  0x00  //���ƼĴ������жϿ��أ�CRCʹ�ܣ�PWR_UP��PRIM_RX��

#define EN_AA       0x01  //6�����ջ��� �Զ�Ӧ�� ���� �ֱ�ʹ�ܣ�ACKʹ�ܣ�

#define EN_RXADDR   0x02  //6�����ջ��� ���չ��� �ֱ�ʹ��

#define SETUP_AW    0x03  //6�����ջ���Լ�� ��ַУ���� ��� ��ͬ���ã�3-5��

#define SETUP_RETR  0x04  //���ͻ� �Զ��ط� �������ã��ط���ʱ���ط�������

#define RF_CH       0x05  //���ͻ���6�����ջ� ����Ƶ�� ��ͬ���� frequency = 2400 + RF_CH [MHz]

#define RF_SETUP    0x06  //��Ƶ���ƼĴ������������ʣ����书�ʣ�

#define NRF_STATUS  0x07  //״̬�Ĵ��� ��RX_DR,TX_DS,MAX_RT,RX_P_NO,TX_FULL��

#define OBSERVE_TX  0x08  //���ͼ��Ĵ��������ݰ���ʧ���������ط���������

#define CD          0x09  //�ز����

#define RX_ADDR_P0  0x0A  //6�����ջ��� ��ַУ���� ����(ADR_WIDTH��)��=TX_ADDR��
#define RX_ADDR_P1  0x0B  //
#define RX_ADDR_P2  0x0C  //
#define RX_ADDR_P3  0x0D  //
#define RX_ADDR_P4  0x0E  //
#define RX_ADDR_P5  0x0F  //

#define TX_ADDR     0x10  //Ŀ����ջ��� ��ַУ���� ����(ADR_WIDTH��)��=RX_ADDR_P0��

#define RX_PW_P0    0x11  //6�����ջ���Ԥ�� ��Ч���� ��� ����
#define RX_PW_P1    0x12  //
#define RX_PW_P2    0x13  //
#define RX_PW_P3    0x14  //
#define RX_PW_P4    0x15  //
#define RX_PW_P5    0x16  //

#define FIFO_STATUS 0x17  //FIFO״̬�Ĵ��� (TX_FULL,TX_EMPTY,RX_FULL,RX_EMPTY)

#define DYNPD       0x1C  //6�����ջ��� ��̬���س��ȹ��� �ֱ�ʹ�� ���ſ���ͨ�� RD_RX_PL_WID �����ȡ�����յ������ݰ�����Ч���ݵĿ�ȣ�
#define FEATURE     0x1D  //�������ƼĴ���(��̬���س��ȹ�����ʹ��(RD_RX_PLOAD_WID�������)��ʹ��Ӧ���źŸ��ع���(WR_ACK_PAYLOAD�������)��ʹ��WR_TX_PAYLOAD_NOACK ����)
//-----------------------------------------------------------------
#if 1
//�ⲿ����
//-----------------------------------------------------------------
//--------------------------ģʽ����-----------------------------
extern void NRF_init(uint8_t mode);//NRF��ʼ��
extern void TX_Mode(void);//ת������ģʽ
extern void RX_Mode(void);//ת������ģʽ
extern void TX_Mode_FAST(void);//����ת��Ϊ����ģʽ
extern void RX_Mode_FAST(void);//����ת��Ϊ����ģʽ

//----�����ա�������������TX���ACK�źţ�RX�жϽ��գ���������----
extern int8_t Tx_Packet(const uint8_t* Buf, uint8_t num);
extern int8_t Rx_Packet(uint8_t* Buf);
extern int8_t Tx_Packet_16(const int16_t* Buf,uint8_t num);
extern int8_t Rx_Packet_16(int16_t* Buf,uint8_t num);

//---�����ա���һ�庯����TX���ACK�źţ�RX�жϽ��գ�����˫����---
extern int8_t Tx_Rx_Packet_TX_FAST(const uint8_t* TX_Buf, uint8_t TX_num, uint8_t* RX_Buf);
extern int8_t Tx_Rx_Packet_RX_FAST(const uint8_t* TX_Buf ,uint8_t TX_num, uint8_t* RX_Buf);
extern int8_t Tx_Rx_Packet_TX_FAST_16(const int16_t* TX_Buf,uint8_t TX_num, int16_t* RX_Buf, uint8_t RX_num);//int16
extern int8_t Tx_Rx_Packet_RX_FAST_16(const int16_t* TX_Buf,uint8_t TX_num, int16_t* RX_Buf, uint8_t RX_num);//int16;

//--------�ǳ��淢�ͺ����������Ack�źţ�(����˫��)--------------
extern void Tx_Packet_Noack(const uint8_t* Buf, uint8_t num);
extern void Tx_Packet_Noack_16(const int16_t* Buf, uint8_t num);

//�ǳ���IQR�жϴ�����(ֻ���ж��д��������ж�TX_DS��RX_DR��MAX_RT)��������
extern int8_t NRF_IQR_hander_TX(void);
extern int8_t NRF_IQR_hander_RX(uint8_t* Buf);
extern int8_t NRF_IQR_hander_RX_16(int16_t* Buf,uint8_t num);

//�ǳ���IQR�жϴ�����(���д��������ж�TX_DS��RX_DR��MAX_RT)��˫����
extern int8_t TX_RX_NRF_IQR_hander_TX(uint8_t* Buf);
extern int8_t TX_RX_NRF_IQR_hander_RX(uint8_t* Buf);
extern int8_t TX_RX_NRF_IQR_hander_TX_16(int16_t* Buf,uint8_t num);
extern int8_t TX_RX_NRF_IQR_hander_RX_16(int16_t* Buf,uint8_t num);

//-------------------IQR�����жϼ�⺯����˫����-----------------
extern int8_t MAX_RR(void);
//-----------------------------------------------------------------
#endif

#endif /* NRF24L01_H_ */
