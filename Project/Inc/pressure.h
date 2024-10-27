/**
 * @file pressure.h
 * @brief Header for pressure.c
 *
 *       Contains pinouts, structs and prototypes for the pressure control
 *       system
 */

#ifndef PRESSURE_H_
#define PRESSURE_H_

#include "main.h"
#include <stdint.h>

/* Compressor, valve, sensor pinout */
#define PRESSURE_REF_SENSOR_PIN GPIO_PIN_0 /*!< A0  */
#define PRESSURE_COMPRESSOR_PIN GPIO_PIN_5 /*!< D13 */
#define PRESSURE_EXHAUST_PIN GPIO_PIN_3    /*!< D3  */

/* Rotary encoder pinout */
#define ROTARY_DT_PIN GPIO_PIN_10 /*!< D2 */
#define ROTARY_CLK_PIN GPIO_PIN_9 /*!< D8 */
#define ROTARY_SW_PIN GPIO_PIN_8  /*!< D7 */

/* LCD pinout */
#define LCD_MODULE_HANDLE hi2c2 /*!< D14 <= SDA
                                 *!< D6  <= SCL */

/* General info */
#define PRESSURE_SENSOR_RANGE 3.3f /*!< Input voltage in Vdc      */
#define ADC_READ_TIME 100          /*!< ADC conversion time in us */
#define ADC_RESOLUTION 4096.0f     /*!< 12 bit ADC resolution     */

/* Struct containing menu information */
struct Menu
{
  int8_t output; /* Flag for determining if a test is currently running */
};

/* Struct containing signal parameters, component handles and menu variables */
struct Pressure
{
  float val;                   /*!< Current pressure value */
  float per;                   /*!< Signal parameter: period */
  float ampl;                  /*!< Signal parameter: amplitude */
  float offset;                /*!< Signal parameter: offset */
  float target;                /*!< Current pressure target */
  float tim3_elapsed;          /*!< Amount of time elapsed */
  UART_HandleTypeDef *huart;   /*!< HAL UART handle */
  ADC_HandleTypeDef *hadc;     /*!< HAL ADC handle */
  TIM_HandleTypeDef *htim_enc; /*!< HAL TIM handle for rotary encoder */
  TIM_HandleTypeDef *htim_upd; /*!< HAL TIM handle for a 100ms timer */
  struct Menu menu;
};

void pressure_main (UART_HandleTypeDef *huart, ADC_HandleTypeDef *hadc,
                    TIM_HandleTypeDef *htim_enc, TIM_HandleTypeDef *htim_upd);

#endif // PRESSURE_H_
