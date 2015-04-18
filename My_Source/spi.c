/*
 * spi.c
 *
 *  Created on: 2014-12-5
 *      Author: FGZ
 */
#include "spi.h"

uint8_t stopFlag;
uint8_t startFlag;
uint16_t delayTimes;

uint8_t g_key_decode = 255;
ideal_VAL Attitude;
/*
 * SPI�˿ڳ�ʼ�� ��׼����Ϊ:  MISO MOSI CLK
 * CSN CE IRQ Ϊ����оƬ���ƴ�������
 */
void PA7_int_hander(void);
void spi_gpio_init(uint8_t int_flag)
{
        ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);
        ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);

        //F0-----MISO    -----  INPUT
        ROM_GPIOPinTypeGPIOInput(GPIO_PORTF_BASE, GPIO_PIN_0);

        //F1��F2��F3��MOSI,CLK��CSN  -----  OUTPUT
        ROM_GPIOPinTypeGPIOOutput(GPIO_PORTF_BASE, GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3);

        //CE-----PA6                 -----  OUTPUT
        ROM_GPIOPinTypeGPIOOutput(GPIO_PORTA_BASE, GPIO_PIN_6);

        //IRQ-----PA7    -----  INPUT
        ROM_GPIOPinTypeGPIOInput(GPIO_PORTA_BASE, GPIO_PIN_7);

        if(int_flag == 1) //�������ⲿ�ж�
        {
                GPIOIntTypeSet(GPIO_PORTA_BASE,GPIO_PIN_7,GPIO_FALLING_EDGE);
                GPIOIntEnable(GPIO_PORTA_BASE,GPIO_INT_PIN_7); //��PA7�ⲿ�жϽ������ݶ�ȡ
                GPIOIntRegister(GPIO_PORTA_BASE, PA7_int_hander);//ָ���ⲿ�жϴ�����
        }
        else
        {
                GPIOIntDisable(GPIO_PORTA_BASE,GPIO_INT_PIN_7);
        }
}

float myYawVal;

/*
 * PA7�ⲿ�жϴ�����
 */
void PA7_int_hander(void)
{
        uint32_t int_status;
        int8_t     result=0;
        static int16_t NRF_recv_16[16];     //�жϽ��յ��Է�����������
        static int16_t key_coding;
        static float powerVal = 0;

        int_status = GPIOIntStatus(GPIO_PORTA_BASE, true);
        GPIOIntClear(GPIO_PORTA_BASE, int_status);

        result = NRF_IQR_hander_RX_16(NRF_recv_16,4);        //NRF24L01 IRQ �жϴ�����(����4ͨ��ң�����ݣ�
        //result = TX_RX_NRF_IQR_hander_RX_16(NRF_recv_16,4);//NRF24L01 IRQ �жϴ�����(ģʽת�������ݽ���)

        if(result == 2)//�����ж�(���ͳɹ������ط��ж�Ҳ������������)
        {
        #if 1
#if 1

                Attitude.ideal_Rol = (-1 * (float)NRF_recv_16[0] / 2) * cos(myYawVal / 180 * PI) - (-1 * (float)NRF_recv_16[1] / 2) * sin(myYawVal / 180 * PI);
                Attitude.ideal_Pit = (-1 * (float)NRF_recv_16[1] / 2) * cos(myYawVal / 180 * PI) + (-1 * (float)NRF_recv_16[0] / 2) * sin(myYawVal / 180 * PI);     //���(deg)
                //Attitude.ideal_Yaw = (float)NRF_recv_16[2] / 10;       //�Ƕ�

#else
                Attitude.ideal_Rol = -1 * (float)NRF_recv_16[0] / 2;
                Attitude.ideal_Pit = -1 * (float)NRF_recv_16[1] / 2;     //���(deg)
                //Attitude.ideal_Yaw = (float)NRF_recv_16[2] / 10;       //�Ƕ�
#endif

                key_coding = NRF_recv_16[3];                      //��������ֵthrottle_high

#if 1
                sendLineX(MCU1, 0X0F, Attitude.ideal_Rol);
                sendLineX(MCU1, 0X2F, Attitude.ideal_Pit);
                //sendLineX(MCU1, 0X6F, Attitude.ideal_Yaw);
#endif

        #else
                g_RC_Position.x = (float)NRF_recv_16[0]*0.02f;
                g_RC_Position.y = (float)NRF_recv_16[1]*0.02f;    //ˮƽλ��(m)
                g_RC_Position.yaw_rate = (float)NRF_recv_16[2]/10;//���ٶ�
                key_coding = NRF_recv_16[3];                        //��������ֵthrottle_high
        #endif

                switch(key_coding)                                                      //���ϰ�������ֵ(�԰�������ֵ���н���)
                {
                        case 100://�ػ�
                        {
                                g_key_decode = 0;
                                stopFlag = 1;
                                startFlag = 0;
                                powerVal = 0;
                                delayTimes = 0;
                                Attitude.Throttle_Hight = Motor_Closed;
                                clearIntegration(1);
                                break;
                        }
                        case 103://�ػ�
                        {
                                g_key_decode = 1;
                                stopFlag = 1;
                                startFlag = 0;
                                powerVal = 0;
                                delayTimes = 0;
                                Attitude.Throttle_Hight = Motor_Closed;
                                clearIntegration(1);
                                break;
                        }
                        case 101://������
                        {
                                g_key_decode = 2;
                                stopFlag = 0;
                                powerVal += 1;
                                if(powerVal >= 110)
                                {
                                  powerVal = 110;
                                }
#ifndef SIDE_DOWN
                                Attitude.Throttle_Hight = Throttle_START + powerVal * 15;
#else
                                Attitude.Throttle_Hight = Throttle_START + powerVal * 15;
#endif

                                break;
                        }
                        case 102://������
                        {
                                g_key_decode = 3;
                                stopFlag = 0;
                                powerVal = powerVal - 1;
                                if(powerVal < -50)
                                {
                                    powerVal = -50;
                                }
#ifndef SIDE_DOWN
                                Attitude.Throttle_Hight = Throttle_START + powerVal * 15;
#else
                                Attitude.Throttle_Hight = Throttle_START + powerVal * 15;
#endif
                                break;
                        }
                        case 104://����
                        {
                                if(startFlag != 1)
                                {
                                    g_key_decode = 50;
                                    startFlag = 1;
                                    stopFlag = 1;
                                }
                                break;
                        }
                }
#if 0
                sendLineX(MCU1, 0X3F, powerVal);
                sendLineX(MCU1, 0X4F, Attitude.Throttle_Hight);
#endif
        }
}
