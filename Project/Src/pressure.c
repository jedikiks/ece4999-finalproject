#include "pressure.h"
#include "I2C_LCD.h"
#include "menu.h"
#include "rotary.h"
#include "stm32f411xe.h"
#include "stm32f4xx_hal.h"
#include "stm32f4xx_hal_gpio.h"
#include "stm32f4xx_hal_tim.h"

#include <math.h>
#include <stdint.h>

uint8_t userint_flg = 0;     // user interrupt flag
uint8_t userint_flg_lck = 0; // user interrupt lck
uint8_t tim3_flg = 0;        // 100ms timer flag
uint8_t tim3_wraps = 0;
float tim3_elapsed = 0.0f;
uint32_t tim4_cnt = 0; // 1ms timer count

void pressure_uart_tx (struct Pressure *pressure);
void pressure_sensor_read (struct Pressure *pressure, float target);
uint8_t pressure_ramp_v3 (struct Pressure *pressure, uint8_t dev, float target,
                          float perr);
void pressure_calib_static (struct Pressure *pressure);
void pressure_calib_dynam_step (struct Pressure *pressure);
void pressure_calib_dynam_ramp (struct Pressure *pressure);
void pressure_calib_dynam_sine (struct Pressure *pressure);

void
HAL_GPIO_EXTI_Callback (uint16_t GPIO_Pin)
{
  if (GPIO_Pin == GPIO_PIN_8 && !userint_flg_lck)
    {
      userint_flg = 1;
      userint_flg_lck = 1;
    }
}

void
HAL_TIM_PeriodElapsedCallback (TIM_HandleTypeDef *htim)
{
  tim3_flg = 1;
  tim3_elapsed += 0.1f;
}

void
pressure_main (UART_HandleTypeDef *huart, ADC_HandleTypeDef *hadc,
               TIM_HandleTypeDef *htim_pwm, uint32_t comp_pwm_ch,
               uint32_t exhst_pwm_ch, TIM_HandleTypeDef *htim_upd)
{
  struct Pressure pressure = { .val = 0.0f,
                               .target = 0.0f,
                               .freq = 0.05f,
                               .ampl = 1.0f,
                               .offset = 0.0f,
                               .huart = huart,
                               .hadc = hadc,
                               .htim_pwm = htim_pwm,
                               .comp_pwm_ch = comp_pwm_ch,
                               .exhst_pwm_ch = exhst_pwm_ch,
                               .htim_upd = htim_upd,
                               .menu.output = -1,
                               .menu.prev_val = 0,
                               .menu.upd_flg = 0 };

  pressure_init (&pressure);
  menu_sm_init (&pressure);
  menu_sm (&pressure);

  int8_t rotary_inpt;
  while (1)
    {
      rotary_inpt = rotary_get_input ();

      if (rotary_inpt != 0)
        {
          pressure.menu.output = menu_sm_setstate (&pressure, rotary_inpt);
          menu_sm (&pressure);
        }

      if (pressure.menu.output != -1)
        {
          HAL_NVIC_EnableIRQ (EXTI9_5_IRQn);
          // FIXME: this sucks V
          tim3_elapsed = 0;
          pressure.tim3_elapsed = 0;

          switch (pressure.menu.output)
            {
            case 0:
              pressure_calib_static (&pressure);
              break;

            case 1:
              pressure_calib_dynam_step (&pressure);
              break;

            case 2:
              pressure_calib_dynam_ramp (&pressure);
              break;

            case 3:
              pressure_calib_dynam_sine (&pressure);
              break;

            default:
              break;
            }

          HAL_NVIC_DisableIRQ (EXTI9_5_IRQn);
          userint_flg = 0;

          tim3_elapsed = 0;

          while (pressure.val >= 0.0000005f)
            pressure_ramp_v3 (&pressure, 2, 0.0f, 0.10f);

          pressure.menu.output = -1;
          menu_sm (&pressure);
        }
    }

  pressure_cleanup (&pressure);
}

uint8_t
pressure_ramp_v3 (struct Pressure *pressure, uint8_t dev, float target,
                  float perr)
{
  uint8_t status = 0;

  if ((target < 0.0000005f) && (target > -0.0000005f))
    target = 0;

  float b_mx = target + (perr * target);
  float b_mn = target - (perr * target);

  if ((pressure->val < 0.0000005f) && (pressure->val > -0.0000005f))
    pressure->val = 0;

  if ((fabs (pressure->val) <= fabs (b_mx))
      && (fabs (pressure->val) >= fabs (b_mn)))
    {
      dev = 0;
      status = 1;
    }

  tim3_wraps = 0;

  switch (dev)
    {
    case 1:
      HAL_TIM_Base_Start_IT (pressure->htim_upd);
      HAL_GPIO_WritePin (GPIOA, GPIO_PIN_5, GPIO_PIN_SET);

      while (tim3_wraps < 5)
        {
          if (userint_flg)
            break;

          while (!tim3_flg)
            ;

          if (HAL_GPIO_ReadPin (GPIOA, GPIO_PIN_1) == GPIO_PIN_SET)
            pressure->val += 2.78 / 10;

          if ((pressure->val < 0.0000005f) && (pressure->val > -0.0000005f))
            pressure->val = 0;

          // Read in pressure value
          pressure->target = target;
          pressure_sensor_read (pressure, target);

          if ((fabs (pressure->val) <= fabs (b_mx))
              && (fabs (pressure->val) >= fabs (b_mn)))
            {
              HAL_GPIO_WritePin (GPIOA, GPIO_PIN_5, GPIO_PIN_RESET);
              status = 1;
            }

          tim3_wraps++;
          tim3_flg = 0;
        }

      HAL_GPIO_WritePin (GPIOA, GPIO_PIN_5, GPIO_PIN_RESET);
      break;

    case 2:

      HAL_TIM_Base_Start_IT (pressure->htim_upd);
      HAL_GPIO_WritePin (GPIOB, GPIO_PIN_3, GPIO_PIN_SET);

      while (tim3_wraps < 5)
        {
          if (userint_flg)
            break;

          while (!tim3_flg)
            ;

          if (HAL_GPIO_ReadPin (GPIOA, GPIO_PIN_4) == GPIO_PIN_SET)
            pressure->val -= 2.78 / 10;

          if ((pressure->val < 0.0000005f) && (pressure->val > -0.0000005f))
            pressure->val = 0;

          // Read in pressure value
          pressure->target = target;
          pressure_sensor_read (pressure, target);

          // if (pressure->val <= target)
          if ((fabs (pressure->val) <= fabs (b_mx))
              && (fabs (pressure->val) >= fabs (b_mn)))
            {
              HAL_GPIO_WritePin (GPIOB, GPIO_PIN_3, GPIO_PIN_RESET);
              status = 1;
            }

          tim3_wraps++;
          tim3_flg = 0;
        }
      HAL_GPIO_WritePin (GPIOB, GPIO_PIN_3, GPIO_PIN_RESET);
      break;

    default:
      HAL_TIM_Base_Start_IT (pressure->htim_upd);

      while (tim3_wraps < 5)
        {
          while (!tim3_flg)
            ;

          // Read in pressure value
          pressure_sensor_read (pressure, target);

          tim3_wraps++;
          tim3_flg = 0;
        }
      break;
    }

  HAL_TIM_Base_Stop_IT (pressure->htim_upd);

  return status;
}

void
pressure_init (struct Pressure *pressure)
{
  HAL_NVIC_DisableIRQ (EXTI9_5_IRQn);

  HAL_ADC_Start (pressure->hadc);
  I2C_LCD_Init (I2C_LCD_1);
}

void
pressure_cleanup (struct Pressure *pressure)
{
  userint_flg = 0;

  HAL_ADC_Stop (pressure->hadc);
  HAL_TIM_Base_DeInit (pressure->htim_pwm);
  HAL_TIM_Base_DeInit (pressure->htim_upd);
}

void
pressure_uart_tx (struct Pressure *pressure)
{
  uint8_t str[35] = { '\0' };
  sprintf (str, "%.2f\r\n", pressure->val);
  HAL_UART_Transmit (pressure->huart, str, sizeof (str), 100);
}

void
pressure_sensor_read (struct Pressure *pressure, float target)
{
  /*
  HAL_ADC_PollForConversion (pressure.hadc, HAL_MAX_DELAY);
  pressure.val = HAL_ADC_GetValue (pressure.hadc)
                  * (PRESSURE_SENSOR_RANGE / ADC_RESOLUTION);
 */
  pressure->menu.upd_flg = 1;
  pressure->tim3_elapsed = tim3_elapsed;
  pressure_uart_tx (pressure);
  menu_sm (pressure);
}

void
pressure_calib_static (struct Pressure *pressure)
{
  userint_flg = 0;
  userint_flg_lck = 0;

  // Ramp to initial offset
  while (!pressure_ramp_v3 (pressure, 1, pressure->offset, 0.1f))
    {
      if (userint_flg)
        break;
    }

  // Display results until user interrupt
  HAL_TIM_Base_Start_IT (pressure->htim_upd);

  while (!userint_flg)
    {
      while (!tim3_flg)
        ;

      // Read in pressure value
      pressure_sensor_read (pressure, pressure->offset);

      tim3_flg = 0;
    }

  HAL_TIM_Base_Stop_IT (pressure->htim_upd);
}

void
pressure_calib_dynam_step (struct Pressure *pressure)
{
  userint_flg = 0;
  userint_flg_lck = 0;
  /*
  float ampl = 2.0f;
  float offs = 4.0f;
  float freq = 0.1f;
  */
  float sw = 0.5f; // switching in sec

  uint8_t N = 1 / (pressure->freq * sw);

  // Get linspaced time array
  float ti[N];
  for (uint8_t i = 0; i < N; i++)
    ti[i] = (i * (1 / pressure->freq)) / N;

  // Use above info to gen sine points
  float yi[N];
  for (uint32_t i = 0; i < N; i++)
    yi[i] = pressure->offset
            + ((pressure->ampl / 2)
               * pow (-1.0f, floor ((2 * ti[i]) / (1 / pressure->freq))));

  // Ramp to initial offset
  while (!pressure_ramp_v3 (pressure, 1, pressure->offset, 0.1f))
    ;

  while (1)
    {
      for (long i = 0; i < N; i++)
        {
          while (i < N / 2)
            {
              pressure_ramp_v3 (pressure, 1, yi[i], 0.1f);
              i++;
            }

          while (i < N)
            {
              pressure_ramp_v3 (pressure, 2, yi[i], 0.1f);
              i++;
            }
        }
    }
}

void
pressure_calib_dynam_ramp (struct Pressure *pressure)
{
  userint_flg = 0;
  userint_flg_lck = 0;
  /*
  float ampl = 3.0f;
  float offs = 3.0f;
  float freq = 0.05f;
  float per = 1.0f / freq;
  */
  float sw = 0.5f; // switching in sec
  uint8_t N = 1 / (pressure->freq * sw);
  float per = 1.0f / pressure->freq;

  // Get linspaced time array
  float ti[N];
  for (uint8_t i = 0; i < N; i++)
    ti[i] = (i * (1 / pressure->freq)) / N;

  // Use above info to gen sine points
  float yi[N];
  for (uint32_t i = 0; i < N; i++)
    yi[i] = ((((4 * pressure->ampl) / per)
              * fabs (fmod ((fmod ((ti[i] - (per / 4)), per) + per), per)
                      - (per / 2)))
             - pressure->ampl);

  // TODO: Add offset
  //  Ramp to initial offset
  //  while (!pressure_ramp_v3 (pressure, 1, offs, 0.1f))
  //    ;

  while (1)
    {
      for (long i = 0; i < N; i++)
        {
          while (i <= N / 4)
            {
              pressure_ramp_v3 (pressure, 1, yi[i], 0.1f);
              i++;
            }

          while (i <= 3 * (N / 4))
            {
              pressure_ramp_v3 (pressure, 2, yi[i], 0.1f);
              i++;
            }

          while (i <= N)
            {
              pressure_ramp_v3 (pressure, 1, yi[i], 0.1f);
              i++;
            }
        }
    }
}

void
pressure_calib_dynam_sine (struct Pressure *pressure)
{
  userint_flg = 0;
  userint_flg_lck = 0;
  /*
  float ampl = 4.0f;
  float offs = 3.0f;
  float freq = 0.05f;
  */
  float sw = 0.5f; // switching in sec
  uint8_t N = 1 / (pressure->freq * sw);

  // Get linspaced time array
  float ti[N];
  for (uint8_t i = 0; i < N; i++)
    ti[i] = (i * (1 / pressure->freq)) / N;

  // Use above info to gen sine points
  float yi[N];
  for (uint32_t i = 0; i < N; i++)
    yi[i] = pressure->offset
            + ((pressure->ampl / 2) * sin (2 * M_PI * pressure->freq * ti[i]));

  // Ramp to initial offset
  while (!pressure_ramp_v3 (pressure, 1, pressure->offset, 0.1f))
    ;

  float current_rate;
  while (1)
    {
      for (long i = 1; i < N; i++)
        {
          while (i < N / 4)
            {
              pressure_ramp_v3 (pressure, 1, yi[i], 0.2f);
              i++;
            }

          while (i < 3 * (N / 4))
            {
              pressure_ramp_v3 (pressure, 2, yi[i], 0.2f);
              i++;
            }

          while (i < N)
            {
              pressure_ramp_v3 (pressure, 1, yi[i], 0.2f);
              i++;
            }
        }
    }
}
