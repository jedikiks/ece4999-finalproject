#include "pressure.h"
#include "stm32f4xx_hal.h"

uint8_t userint_flg = 0; // user interrupt flag

struct Pressure
{
  float val;
  float val_min;
  float val_max;
  const char *units;
  UART_HandleTypeDef *huart;
  ADC_HandleTypeDef *hadc;
};

void
HAL_GPIO_EXTI_Callback (uint16_t GPIO_Pin)
{
  if (GPIO_Pin == GPIO_PIN_13)
    userint_flg = 1;
}

void
pressure_main (UART_HandleTypeDef *huart, ADC_HandleTypeDef *hadc)
{
  struct Pressure pressure = { .val = 0.0f,
                               .val_min = 0.0f,
                               .val_max = 150.0f,
                               .units = "psi",
                               .huart = huart,
                               .hadc = hadc };

  pressure_init (&pressure);

  pressure_calib_static (&pressure, 10.0f);

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
      //FIXME: remove this when testing irl V
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
pressure_calib_dynam_ramp (struct Pressure *pressure, float target1,
                           float target2)
{
  uint8_t min, max;
  if (target1 < target2)
    {
      min = target1;
      max = target2;
    }
  else
    {
      min = target1;
      max = target2;
    }

  while (pressure->val < max)
    pressure_calib_dynam_step (pressure, pressure->val, pressure->val + 5.0f);

  while (pressure->val > min)
    pressure_calib_dynam_step (pressure, target1, target1 - 5.0f);
}
