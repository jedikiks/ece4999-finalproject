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

struct Pressure;

void pressure_main (UART_HandleTypeDef *huart, ADC_HandleTypeDef *hadc,
                    TIM_HandleTypeDef *htim_pwm, uint32_t tim_ch,
                    TIM_HandleTypeDef *htim_upd);
void pressure_init (struct Pressure *pressure);
void pressure_decomp (struct Pressure *pressure);
void pressure_cleanup (struct Pressure *pressure);
void pressure_uart_tx (struct Pressure *pressure);
void pressure_sensor_read (struct Pressure *pressure);
void pressure_calib_static (struct Pressure *pressure, float target);
void pressure_calib_dynam_step (struct Pressure *pressure, float target1,
                                float target2);
void pressure_calib_dynam_ramp (struct Pressure *pressure, float target);

#endif // PRESSURE_H_
