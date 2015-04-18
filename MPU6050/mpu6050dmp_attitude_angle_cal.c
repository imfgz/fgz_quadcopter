//-----------------------------------------------------------------
//�ļ����ƣ�mpu6050dmp_attitude_angle_cal.c
//���ܸ�Ҫ��mpu6050 dmp��ȡ��̬���ļ�
//�汾���£�2013-11-06
//-----------------------------------------------------------------

//------------------------------ ����Ԥ���� ------------------------------//
#include "mpu6050dmp_attitude_angle_cal.h"
#include "Delay.h"
//------------------------------ �ڲ����� ------------------------------//

//-----------------------------------------------------------------
//�������ƣ�inv_row_2_scale  inv_orientation_matrix_to_scalar
//���ܸ�Ҫ��These next two functions converts the orientation matrix (see gyro_orientation) to 
//          a scalar representation for use by the DMP.
//�������أ�unsigned short
//����˵����row
//    NOTE: These functions are borrowed from Invensense's MPL.
//-----------------------------------------------------------------
static  unsigned short inv_row_2_scale(const signed char *row)
{
    unsigned short b;
    if (row[0] > 0)b = 0;
    else if (row[0] < 0)b = 4;
    else if (row[1] > 0)b = 1;
    else if (row[1] < 0)b = 5;
    else if (row[2] > 0)b = 2;
    else if (row[2] < 0)b = 6;
    else b = 7;      // error
    return b;
}
static  unsigned short inv_orientation_matrix_to_scalar(const signed char *mtx)
{
    unsigned short scalar;
    scalar = inv_row_2_scale(mtx);
    scalar |= inv_row_2_scale(mtx + 3) << 3;
    scalar |= inv_row_2_scale(mtx + 6) << 6;
    return scalar;
}

//-----------------------------------------------------------------
//�������ƣ�run_self_test
//���ܸ�Ҫ��mpu�Լ캯��,if Test passed, We can trust the gyro data here, so let's push it down to the DMP.
//�������أ�void
//����˵����void
//-----------------------------------------------------------------
static void run_self_test(void)
{
    int result;
    long gyro[3], accel[3];
    result = mpu_run_self_test(gyro, accel);
    if (result == 0x7) 
	{
        float sens;
        unsigned short accel_sens;
        mpu_get_gyro_sens(&sens);
        gyro[0] = (long)(gyro[0] * sens);
        gyro[1] = (long)(gyro[1] * sens);
        gyro[2] = (long)(gyro[2] * sens);
        dmp_set_gyro_bias(gyro);
        mpu_get_accel_sens(&accel_sens);
        accel[0] *= accel_sens;
        accel[1] *= accel_sens;
        accel[2] *= accel_sens;
        dmp_set_accel_bias(accel);
    }
}

//------------------------------ �ⲿ�ӿں��� ------------------------------//

//-----------------------------------------------------------------
//�������ƣ�mpu6050dmp_init
//���ܸ�Ҫ��mpu6050��dmp��ʼ��
//�������أ�void
//����˵����void
//ע    �⣺DMP�����ݱ�������Ż������һ����̬���£�����һ��Ҫ��ʱ��ȡDMP����DMP�Ķ�ȡ����һ��ҪС��DMP��������
//          (DMP��ȡ����Ҫ����DMP�����������,�����������̬�ǽ����ͺ�,���ٶȼƺ������ǵ�ֵ�����ң�
//          ������������������ڶ�ȡ����ͬ���ݣ���������������,���Զ�ȡ����ֻҪ��֤����С�ڡ�����)��
//-----------------------------------------------------------------
#define DEFAULT_MPU_HZ  (100)  	//starting sampling rate.(ending sampling rate=200HZ)
#define DEFAULT_DMP_HZ  (100)   //dmp output rate(<=DMP_SAMPLE_RATE=200Hz)(Tdmp=5,10,20,40,50,100,200,500,1000ms)
void mpu6050dmp_init(void)
{
    // The sensors can be mounted onto the board in any orientation. The mounting matrix seen below tells the MPL how to rotate the raw data from the driver(s).
    // TODO: The following matrices refer to the configuration on an internal test board at Invensense. If needed, please modify the matrices to match the chip-to-body matrix for your particular set up.
    static signed char gyro_orientation[9] = {-1, 0, 0, 0, -1, 0, 0, 0, 1};
    static int result;
    delay_ms(1000);
    result = mpu_init();
    if(!result)
    {
        mpu_set_sensors(INV_XYZ_GYRO | INV_XYZ_ACCEL);		//turn specific sensors on
        mpu_configure_fifo(INV_XYZ_GYRO | INV_XYZ_ACCEL);	//set GYRO��ACCEL pushed to FIFO
        mpu_set_sample_rate(DEFAULT_MPU_HZ); 				//DIV=(1000/DEFAULT_MPU_HZ)-1
        dmp_load_motion_driver_firmware();					//load the DMP with this image.st.chip_cfg.dmp_sample_rate = DMP_SAMPLE_RATE=200;
        dmp_set_orientation(inv_orientation_matrix_to_scalar(gyro_orientation));//push gyro and accel orientation to the DMP
        dmp_enable_feature(DMP_FEATURE_6X_LP_QUAT | DMP_FEATURE_TAP | DMP_FEATURE_ANDROID_ORIENT | DMP_FEATURE_SEND_RAW_ACCEL | DMP_FEATURE_SEND_CAL_GYRO | DMP_FEATURE_GYRO_CAL);//enable DMP features.
        dmp_set_fifo_rate(DEFAULT_DMP_HZ); 					//set DMP output rate (0X6F = DMP_SAMPLE_RATE/DEFAULT_DMP_HZ-1)
        run_self_test();									//run_self_test
        mpu_set_dmp_state(1);				 				//enable/disable DMP support,  DIV=(1000/st.chip_cfg.dmp_sample_rate)-1=0x04 (��ending sampling rate=200HZ)
    }
}

void init_MPU6050(void)
{
    static signed char gyro_orientation[9] = {-1, 0, 0, 0, -1, 0, 0, 0, 1};

    delay_ms(1000);

    mpu_init();

    mpu_set_sensors(INV_XYZ_GYRO | INV_XYZ_ACCEL);

    mpu_configure_fifo(INV_XYZ_GYRO | INV_XYZ_ACCEL);

    mpu_set_sample_rate(DEFAULT_MPU_HZ);

    dmp_load_motion_driver_firmware();

    dmp_set_orientation(inv_orientation_matrix_to_scalar(gyro_orientation));

    dmp_enable_feature(DMP_FEATURE_6X_LP_QUAT | DMP_FEATURE_TAP |

        DMP_FEATURE_ANDROID_ORIENT | DMP_FEATURE_SEND_RAW_ACCEL | DMP_FEATURE_SEND_CAL_GYRO |

        DMP_FEATURE_GYRO_CAL);

    dmp_set_fifo_rate(DEFAULT_MPU_HZ);

    run_self_test();

    mpu_set_dmp_state(1);
}
//-----------------------------------------------------------------
//�������ƣ�attitude_angle_cal
//���ܸ�Ҫ��mpu6050��dmp��ȡ��̬��(Pitch,Roll,Yaw:�����ǣ�����ǣ�������)�����ؼ��ٶȼơ�������ֵ
//�������أ�void
//����˵����angle:ָ��������̬�ǵ��׵�ַ���Ŵ�10�����
//       gyro:ָ��������ٶȵ��׵�ַ ( gyro[0]:Ex(pitch);  gyro[1]:Ey(roll);  gyro[2]:Ez(yaw))(�޸�ͨ�˲�)
//          accel:ָ��������ٶȵ��׵�ַ(accel[0]:-x(pitch); accel[1]:-y(roll); accel[2]:-z(yaw))
//ע    �⣺dmp������������ֵ��һ���ʧ�棬���Ҫ��������ֵ���п��ƣ�����������Լ�������ȡ������ֵ��
//-----------------------------------------------------------------

#define q30  1073741824.0f
extern float pitch, roll, yaw;

void attitude_angle_cal(int16_t* angle, int16_t* gyro, int16_t* accel)
{
    static unsigned long sensor_timestamp;
    static short sensors;
    static unsigned char more;
    static long quat[4];
    static float q0 = 1.0f, q1 = 0.0f, q2 = 0.0f, q3 = 0.0f;
    //static float pitch,roll,yaw;
    //static float yaw_last;

    dmp_read_fifo(gyro, accel, quat, &sensor_timestamp, &sensors,&more);
    //Gyro and accel data are written to the FIFO by the DMP in chip frame and hardware units. This behavior is convenient because it
    //keeps the gyro and accel outputs of dmp_read_fifo and mpu_read_fifo consistent.
    //Unlike gyro and accel, quaternions are written to the FIFO in the body frame, q30. The orientation is set by the scalar passed
    //to dmp_set_orientation during initialization.
    if (sensors & INV_WXYZ_QUAT)
    {
        q0=quat[0] / q30;
        q1=quat[1] / q30;
        q2=quat[2] / q30;
        q3=quat[3] / q30;
        pitch = asin(-2 * q1 * q3 + 2 * q0 * q2) * 57.3f;                                                    //Pitch
        roll  = atan2(2 * q2 * q3 + 2 * q0 * q1, -2 * q1 * q1 - 2 * q2* q2 + 1) * 57.3f;                     //Roll
        yaw   = atan2(2 * (q1 * q2 + q0 * q3), q0 * q0 + q1 * q1 - q2 * q2 - q3 * q3) * 57.3f;               //Yaw

#ifdef SIDE_DOWN
        if(roll < 0)
        {
            roll +=180;
        }
        else
        {
            roll -=180;
        }
#endif
#if 0
        if(period_count == 1)
        {
                yaw_last = yaw;
        }//�궨

        if((yaw-yaw_last) > 180.0f)
        {
                yaw -= 360.0f;
        }
        else if((yaw-yaw_last) < -180.0f)
        {
                yaw += 360.0f;
        }//Ԥ����
        yaw -= yaw_last;//������λ�õ�ƫ���ǵ���Ϊ���ԭʼƫ����"yaw_last"��-180��~180�����ֵ��
#endif
        angle[0] = (int16_t)(pitch * 10.0f);
        angle[1] = (int16_t)(roll * 10.0f);
        angle[2] = (int16_t)(yaw * 10.0f);	//�Ŵ�10�����
    }
}

unsigned long nullVal = 0;
void get_AttitudeVal(short *gyroVal)
{
    static unsigned long sensor_timestamp;
    static short sensors;
    static unsigned char more;
    static long quat[4];
    static short gyro[3];
    static short accel[3];
    static float q0 = 1.0f, q1 = 0.0f, q2 = 0.0f, q3 = 0.0f;

    dmp_read_fifo(gyro, accel, quat, &sensor_timestamp, &sensors,&more);

    if (sensors & INV_WXYZ_QUAT)
    {

#if 0
        mpu_get_gyro_reg(gyroVal, &nullVal);
        sendLineX(MCU1, 0X1F, (float)gyroVal[1] / 16.4);
        sendLineX(MCU1, 0X2F, (float)gyro[1] / -16.4);
#endif

        q0=quat[0] / q30;
        q1=quat[1] / q30;
        q2=quat[2] / q30;
        q3=quat[3] / q30;

        pitch = asin(-2 * q1 * q3 + 2 * q0 * q2) * 57.3f;                                                    //Pitch
        roll  = atan2(2 * q2 * q3 + 2 * q0 * q1, -2 * q1 * q1 - 2 * q2 * q2 + 1) * 57.3f;                    //Roll
        yaw   = atan2(2 * (q1 * q2 + q0 * q3), q0 * q0 + q1 * q1 - q2 * q2 - q3 * q3) * 57.3f;               //Yaw


#ifdef SIDE_DOWN

        if(roll < 0)
        {
            roll += 180;
        }
        else
        {
            roll -= 180;
        }

#endif

#if 1
      mpu_get_gyro_reg(gyroVal, &nullVal);

      gyroVal[0] = -1 * gyroVal[0];
      gyroVal[1] = -1 * gyroVal[1];
      gyroVal[2] = -1 * gyroVal[2];
#endif

    }

#if 0
    mpu_get_gyro_reg(gyroVal, &nullVal);

    gyroVal[0] = -1 * gyroVal[0];
    gyroVal[1] = -1 * gyroVal[1];
    gyroVal[2] = -1 * gyroVal[2];
#endif
}
