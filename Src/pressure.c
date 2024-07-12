#include "pressure.h"

void
pressure_cleanup (ADC_HandleTypeDef *hadc)
{
  HAL_ADC_Stop (hadc);
}

void
pressure_init (ADC_HandleTypeDef *hadc)
{
  HAL_ADC_Start (hadc);
}

void
pressure_sensor_read (float target)
{
  do
    {
      HAL_ADC_PollForConversion (hadc, HAL_MAX_DELAY);
      pressure.val
          = HAL_ADC_GetValue (hadc) * (PRESSURE_SENSOR_RANGE / ADC_RESOLUTION);
      // update lcd here
    }
  while (pressure.val != target);
}

void
pressure_calib_static (float target)
{
  HAL_GPIO_WritePin (GPIOA, PRESSURE_COMPRESSOR_PIN, GPIO_PIN_SET);
  pressure_sensor_read (hadc, target);
  HAL_GPIO_WritePin (GPIOA, PRESSURE_COMPRESSOR_PIN, GPIO_PIN_RESET);
}

void
pressure_calib_dynam_step (float target1, float target2)
{
  HAL_GPIO_WritePin (GPIOA, PRESSURE_COMPRESSOR_PIN, GPIO_PIN_SET);

  if (target1 > target2) // Step up
    {
      pressure_sensor_read (hadc, target2);
      HAL_GPIO_WritePin (GPIOA, PRESSURE_COMPRESSOR_PIN, GPIO_PIN_RESET);
    }
  else // Step down
    {
      pressure_sensor_read (hadc, target1);
      HAL_GPIO_WritePin (GPIOA, PRESSURE_COMPRESSOR_PIN, GPIO_PIN_RESET);
      HAL_GPIO_WritePin (GPIOA, PRESSURE_EXHAUST_PIN, GPIO_PIN_SET);

      pressure_sensor_read (hadc, target2);
      HAL_GPIO_WritePin (GPIOA, PRESSURE_EXHAUST_PIN, GPIO_PIN_RESET);
    }
}

void
pressure_calib_dynam_ramp (float target1, float target2)
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

  while (pressure.val < max)
    pressure_calib_dynam_step (hadc, pressure.val, pressure.val + 5.0f);

  while (pressure.val > min)
    pressure_calib_dynam_step (hadc, target1, target1 - 5.0f);
}
