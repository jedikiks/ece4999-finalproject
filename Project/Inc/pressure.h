#ifndef PRESSURE_H_
#define PRESSURE_H_

#include "main.h"
#include <stdint.h>

#define PRESSURE_UART_TX_PIN GPIO_PIN_2
#define PRESSURE_UART_RX_PIN GPIO_PIN_3

#define PRESSURE_REF_SENSOR_PIN GPIO_PIN_0
#define PRESSURE_COMPRESSOR_PIN GPIO_PIN_1
#define PRESSURE_EXHAUST_PIN GPIO_PIN_2

#define PRESSURE_SENSOR_RANGE 3.3f // Vdc
#define ADC_READ_TIME 100          // us
#define ADC_RESOLUTION 4096.0f     // 12 bit

#define PRESSURE_PWM_FREQ 1000 // ms

struct Menu
{
  float prev_val;
  int8_t output;
};

struct Pressure
{
  float val;
  float per;
  float ampl;
  float offset;
  float target;
  float tim3_elapsed;
  UART_HandleTypeDef *huart;
  ADC_HandleTypeDef *hadc;
  TIM_HandleTypeDef *htim_pwm;
  TIM_HandleTypeDef *htim_upd;
  uint32_t comp_pwm_ch;
  uint32_t exhst_pwm_ch;
  struct Menu menu;
};

void pressure_main (UART_HandleTypeDef *huart, ADC_HandleTypeDef *hadc,
                    TIM_HandleTypeDef *htim_pwm, uint32_t comp_pwm_ch,
                    uint32_t exhst_pwm_ch, TIM_HandleTypeDef *htim_upd);

#endif // PRESSURE_H_
