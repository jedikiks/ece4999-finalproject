#include "pressure.h"
#include "stm32f411xe.h"
#include "stm32f4xx_hal.h"
#include "stm32f4xx_hal_tim.h"
#include <stdint.h>

uint8_t userint_flg = 0; // user interrupt flag

struct Pressure
{
  float val;
  float val_min;
  float val_max;
  const char *units;
  float freq;
  float ampl;
  float offset;
  UART_HandleTypeDef *huart;
  ADC_HandleTypeDef *hadc;
  TIM_HandleTypeDef *htim;
  uint32_t tim_ch;
};

uint32_t upd_dty (struct Pressure *pressure, float m_r);

void
HAL_GPIO_EXTI_Callback (uint16_t GPIO_Pin)
{
  if (GPIO_Pin == GPIO_PIN_13)
    userint_flg = 1;
}

uint32_t
upd_dty (struct Pressure *pressure, float m_r)
{
  pressure_sensor_read (pressure);
  float p_i = pressure->val;
  HAL_Delay (100);
  pressure_sensor_read (pressure);

  return m_r / ((pressure->val - p_i) / 0.1f);
}

void
pressure_main (UART_HandleTypeDef *huart, ADC_HandleTypeDef *hadc,
               TIM_HandleTypeDef *htim, uint32_t tim_ch)
{
  struct Pressure pressure = { .val = 0.0f,
                               .val_min = 0.0f,
                               .val_max = 150.0f,
                               .units = "psi",
                               .freq = 0.1f,
                               .ampl = 10.0f,
                               .offset = 0.0f,
                               .huart = huart,
                               .hadc = hadc,
                               .htim = htim,
                               .tim_ch = tim_ch };

  pressure_init (&pressure);

  // pressure_calib_static (&pressure, 10.0f);
  pressure_calib_dynam_ramp(&pressure, 10.0f);

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

  pressure_uart_tx (pressure);
  pressure->val += 0.0278;
  HAL_Delay (10);
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
pressure_calib_dynam_step (struct Pressure *pressure, float target1,
                           float target2)
{
  HAL_GPIO_WritePin (GPIOA, PRESSURE_COMPRESSOR_PIN, GPIO_PIN_SET);

  if (target1 > target2) // Step up
    {
      pressure_sensor_read (pressure);
      HAL_GPIO_WritePin (GPIOA, PRESSURE_COMPRESSOR_PIN, GPIO_PIN_RESET);
    }
  else // Step down
    {
      pressure_sensor_read (pressure);
      HAL_GPIO_WritePin (GPIOA, PRESSURE_COMPRESSOR_PIN, GPIO_PIN_RESET);
      HAL_GPIO_WritePin (GPIOA, PRESSURE_EXHAUST_PIN, GPIO_PIN_SET);

      pressure_sensor_read (pressure);
      HAL_GPIO_WritePin (GPIOA, PRESSURE_EXHAUST_PIN, GPIO_PIN_RESET);
    }
}

void
pressure_calib_dynam_ramp (struct Pressure *pressure, float target)
{
  // Start by assuming the rate of change is 2.78psi/sec. We'll know if
  // its slower after the first 100ms
  float m_r = (pressure->freq * 2 * pressure->ampl) * 0.1f;
  float m_c = 2.78f * 0.1f;
  TIM2->CCR1 = (uint32_t)(m_r / m_c);
  HAL_TIM_PWM_Start (pressure->htim, pressure->tim_ch);

  while (pressure->val < target)
  {
    TIM2->CCR1 = upd_dty(pressure, m_r);
    pressure_sensor_read(pressure);
  }

  HAL_TIM_PWM_Stop(pressure->htim, pressure->tim_ch);
}
