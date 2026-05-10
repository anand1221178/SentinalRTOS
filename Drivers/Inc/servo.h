#ifndef __SERVO_H__
#define __SERVO_H__

#include "stm32f4xx.h"

void servo_init(void);
void servo_set_angle(uint16_t degrees);
void sweep_task(void);

#endif