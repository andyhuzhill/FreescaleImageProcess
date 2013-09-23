

#include "pid.h"


#define DEFAULT_PID_INTEGRATION_LIMIT  5000.0

PidObject pidSteer;
PidObject pidMotor;

void pidInit(PidObject *pid, const float desired, const float kp,
        const float ki, const float kd)
{
    pid->error = 0;
    pid->prevError = 0;
    pid->integ = 0;
    pid->deriv = 0;
    pid->desired = desired;
    pid->kp = kp;
    pid->ki = ki;
    pid->kd = kd;

    pid->iLimit = DEFAULT_PID_INTEGRATION_LIMIT;
}

void pidSetKp(PidObject *pid, const float kp)
{
    pid->kp = kp;
}

void pidSetKi(PidObject *pid, const float ki)
{
    pid->ki = ki;
}

void pidSetKd(PidObject *pid, const float kd)
{
    pid->kd = kd;
}

float UpdatePID(PidObject *pid, const float measured)
{
    float output;

    pid->error = pid->desired - measured;       //计算误差

    pid->integ += pid->error;                   //计算积分

    if(pid->integ > pid->iLimit) {               //避免积分和超过限制
        pid->integ = pid->iLimit;
    } else if (pid->integ < - pid->iLimit) {
        pid->integ = -pid->iLimit;
    }

    pid->deriv = (pid->error - pid->prevError); //计算微分

    //计算PID输出
    output = pid->kp * pid->error + pid->ki*pid->integ + pid->kd *pid->deriv;

    pid->prevError = pid->error;

    return output;
}

