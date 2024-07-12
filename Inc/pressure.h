#ifndef PRESSURE_H_
#define PRESSURE_H_

#include "main.h"

#define PRESSURE_REF_SENSOR_PIN GPIO_PIN_0
#define PRESSURE_COMPRESSOR_PIN GPIO_PIN_1
#define PRESSURE_EXHAUST_PIN GPIO_PIN_2

#define PRESSURE_SENSOR_RANGE 3.3f // Vdc

#define ADC_READ_TIME 100      // in us
#define ADC_RESOLUTION 4096.0f // 12 bit

void pressure_sensor_read (float target);
void pressure_calib_static (float target);
void pressure_calib_dynam_step (float target1, float target2);
void pressure_calib_dynam_ramp (float target1, float target2);

#endif // PRESSURE_H_
