//-----------------------------------------------------------------
//�ļ����ƣ�mpu6050dmp_attitude_angle_cal.h
//���ܸ�Ҫ��mpu6050 dmp��ȡ��̬���ļ�
//�汾���£�2013-11-06
//-----------------------------------------------------------------
#ifndef _MPU6050DMP_ATTITUDE_ANGLE_CAL_H_
#define _MPU6050DMP_ATTITUDE_ANGLE_CAL_H_

//����ͷ�ļ�
#include "Common.h"
#include "inv_mpu.h"
#include "inv_mpu_dmp_motion_driver.h"
#include "My_UART.h"

//------------------------------ �ⲿ�ӿں��� ------------------------------//

//-----------------------------------------------------------------
//�������ƣ�mpu6050dmp_init
//���ܸ�Ҫ��mpu6050��dmp��ʼ��
//�������أ�void
//����˵����void
//-----------------------------------------------------------------

extern float pitch, roll, yaw;

void mpu6050dmp_init(void);

//-----------------------------------------------------------------
//��������:attitude_angle_cal
//���ܸ�Ҫ:mpu6050��dmp��ȡ��̬��(Pitch,Roll,Yaw:�����ǣ�����ǣ�������)�����ؼ��ٶȼơ�������ֵ
//��������:void
//����˵��:angle:ָ��������̬�ǵ��׵�ַ���Ŵ�10�����
//      gyro:ָ��������ٶȵ��׵�ַ (gyro[0] :y(roll); gyro[1] :x(pitch);gyro[2] :z(yaw))(�޸�ͨ�˲�)
//      accel:ָ��������ٶȵ��׵�ַ(accel[0]:x(pitch);accel[1]:y(roll); accel[2]:z(yaw))
//-----------------------------------------------------------------
void init_MPU6050(void);
void attitude_angle_cal(int16_t* angle, int16_t* gyro, int16_t* accel);
void get_AttitudeVal(short *gyroVal);

#endif
