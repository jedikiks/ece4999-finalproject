#ifndef PRESSURE_H_
#define PRESSURE_H_

#include "main.h"

#define PRESSURE_UART_TX_PIN GPIO_PIN_2
#define PRESSURE_UART_RX_PIN GPIO_PIN_3

#define PRESSURE_REF_SENSOR_PIN GPIO_PIN_0
#define PRESSURE_COMPRESSOR_PIN GPIO_PIN_1
#define PRESSURE_EXHAUST_PIN GPIO_PIN_2

#define PRESSURE_SENSOR_RANGE 3.3f // Vdc
#define ADC_READ_TIME 100          // us
#define ADC_RESOLUTION 4096.0f     // 12 bit

#define PRESSURE_PWM_FREQ 1000 // ms

struct Pressure
{
  float val;
  float val_last;
  float val_min;
  float val_max;
  const char *units;
  float freq;
  float ampl;
  float offset;
  UART_HandleTypeDef *huart;
  ADC_HandleTypeDef *hadc;
  TIM_HandleTypeDef *htim_pwm;
  TIM_HandleTypeDef *htim_upd;
  TIM_HandleTypeDef *htim_1ms;
  uint32_t tim_ch;
};

void pressure_main (UART_HandleTypeDef *huart, ADC_HandleTypeDef *hadc,
                    TIM_HandleTypeDef *htim_pwm, uint32_t tim_ch,
                    TIM_HandleTypeDef *htim_upd, TIM_HandleTypeDef *htim_1ms);
void pressure_init (void);
void pressure_cleanup (void);
void pressure_disp (void);
void pressure_uart_tx (void);
void pressure_sensor_read (void);
void pressure_calib_static (float target);
void pressure_calib_dynam_step (void);
void pressure_calib_dynam_ramp (void);
void pressure_calib_dynam_sine (void);

#endif // PRESSURE_H_
