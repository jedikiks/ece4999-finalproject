#include "pressure.h"
#include "stm32f411xe.h"
#include "stm32f4xx_hal.h"
#include "stm32f4xx_hal_gpio.h"
#include "stm32f4xx_hal_tim.h"
#include <stdint.h>

uint8_t userint_flg = 0; // user interrupt flag
uint8_t tim3_flg = 0;    // send data every 100ms flag
uint8_t tim3_wraps = 0;

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
  uint32_t tim_ch;
};

uint32_t update_dty (struct Pressure *pressure, float m_r);

void
HAL_GPIO_EXTI_Callback (uint16_t GPIO_Pin)
{
  if (GPIO_Pin == GPIO_PIN_13)
    userint_flg = 1;
}

void
HAL_TIM_PeriodElapsedCallback (TIM_HandleTypeDef *htim_pwm)
{
  tim3_flg = 1;
}

void
pressure_main (UART_HandleTypeDef *huart, ADC_HandleTypeDef *hadc,
               TIM_HandleTypeDef *htim_pwm, uint32_t tim_ch,
               TIM_HandleTypeDef *htim_upd)
{
  struct Pressure pressure = { .val = 0.0f,
                               .val_last = 0.0f,
                               .val_min = 0.0f,
                               .val_max = 150.0f,
                               .units = "psi",
                               .freq = 0.2f,
                               .ampl = 10.0f,
                               .offset = 0.0f,
                               .huart = huart,
                               .hadc = hadc,
                               .htim_pwm = htim_pwm,
                               .tim_ch = tim_ch,
                               .htim_upd = htim_upd };

  pressure_init (&pressure);

  // pressure_calib_static (&pressure, 10.0f);
  pressure_calib_dynam_ramp (&pressure, 10.0f);

  pressure_cleanup (&pressure);

  // while (1)
  //   {
  //     pressure.val += 2.78;
  //     pressure_uart_tx (&pressure);
  //     HAL_Delay (1000);
  //   }
}

void
pressure_decomp (struct Pressure *pressure)
{
  // TODO: switch on solenoid valve here V

  // TODO: remove this V
  while (pressure->val >= 0.1)
    {
      pressure->val -= 0.0278;
      pressure_uart_tx (pressure);
      HAL_Delay (10);
    }

  // TODO: switch off solenoid valve here V
}

void
pressure_init (struct Pressure *pressure)
{
  HAL_ADC_Start (pressure->hadc);
}

void
pressure_cleanup (struct Pressure *pressure)
{
  HAL_ADC_Stop (pressure->hadc);
}

void
pressure_uart_tx (struct Pressure *pressure)
{
  uint8_t str[35] = { '\0' };
  sprintf (str, "%.2f\r\n", pressure->val);
  HAL_UART_Transmit (pressure->huart, str, sizeof (str), 100);
}

void
pressure_sensor_read (struct Pressure *pressure)
{
  /*
   * FIXME: uncomment this for irl test V
  HAL_ADC_PollForConversion (pressure->hadc, HAL_MAX_DELAY);
  pressure->val = HAL_ADC_GetValue (pressure->hadc)
                  * (PRESSURE_SENSOR_RANGE / ADC_RESOLUTION);
  pressure_uart_tx (pressure);
   */

  pressure->val_last = pressure->val;

  if (HAL_GPIO_ReadPin (GPIOA, GPIO_PIN_1)
      == GPIO_PIN_SET) // simulates if compressor's on
    pressure->val += 2.78 / 10;

  pressure_uart_tx (pressure);
}

void
pressure_calib_static (struct Pressure *pressure, float target)
{
  HAL_GPIO_WritePin (GPIOA, PRESSURE_COMPRESSOR_PIN, GPIO_PIN_SET);

  do
    pressure_sensor_read (pressure);
  while (pressure->val <= (target - 0.1f));

  HAL_GPIO_WritePin (GPIOA, PRESSURE_COMPRESSOR_PIN, GPIO_PIN_RESET);

  while (!userint_flg)
    {
      // FIXME: remove this when testing irl V
      pressure_uart_tx (pressure);
      HAL_Delay (10);
    }

  userint_flg = 0;
  pressure_decomp (pressure);
}

void
pressure_calib_dynam_ramp (struct Pressure *pressure, float target)
{
  // Start by assuming the rate of change is 2.78psi/sec. We'll know if
  // its slower after the first 100ms
  float m_r = pressure->freq * pressure->ampl;
  float m_c = 2.78f;
  TIM2->CCR1 = pressure->htim_pwm->Init.Period * (m_r / m_c);
  HAL_TIM_PWM_Start (pressure->htim_pwm, pressure->tim_ch);
  HAL_TIM_Base_Start_IT (pressure->htim_upd);

  while (tim3_wraps < (10 * (1 / pressure->freq)))
    {
      if (tim3_flg == 1)
        {
          pressure_sensor_read (pressure);
          tim3_flg = 0;
          tim3_wraps++;
        }
    }

  userint_flg = 0;
  HAL_TIM_Base_Stop (pressure->htim_upd);
  HAL_TIM_PWM_Stop (pressure->htim_pwm, pressure->tim_ch);
  // pressure_decomp (pressure);
}
