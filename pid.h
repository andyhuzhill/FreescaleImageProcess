#ifndef PID_H
#define PID_H

#ifdef __cplusplus
extern "C" {
#endif


typedef struct
{
  float desired;      //< 设置要达到的值
  float error;        //< 误差
  float prevError;    //< 上一次的误差
  float integ;        //< 积分和
  float deriv;        //< 微分
  float kp;           //< 比例系数
  float ki;           //< 积分系数
  float kd;           //< 微分系数
  float iLimit;       //< 积分限
} PidObject;

float UpdatePID(PidObject *pid, const float measured);

void pidInit(PidObject *pid, const float desired, const float kp,
        const float ki, const float kd);

void pidSetKp(PidObject *pid, const float kp);

void pidSetKi(PidObject *pid, const float ki);

void pidSetKd(PidObject *pid, const float kd);

#ifdef __cplusplus
}
#endif

#endif // PID_H


