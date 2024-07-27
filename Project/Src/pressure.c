#include "pressure.h"
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
uint32_t tim4_cnt = 0; // 1ms timer count

void ramp_init (struct Pressure *pressure, float m_r, float m_c);
void ramp_deinit (struct Pressure *pressure);
void pressure_triwave (struct Pressure *pressure);
void pressure_ramp (struct Pressure *pressure, float target, float rate);
void pressure_ramp_v2 (struct Pressure *pressure, float target, float rate);

void
HAL_GPIO_EXTI_Callback (uint16_t GPIO_Pin)
{
  if (GPIO_Pin == GPIO_PIN_13 && !userint_flg_lck)
    {
      userint_flg = 1;
      userint_flg_lck = 1;
    }
}

void
HAL_TIM_PeriodElapsedCallback (TIM_HandleTypeDef *htim)
{
  tim3_flg = 1;
}

void
pressure_lcd_draw (void)
{
  // pressure +/- uncert
  // current time
  /*
  ** new timer counts to 1 ms
  */
  /*
  I2C_LCD_Clear (I2C_LCD_1);
  I2C_LCD_SetCursor (I2C_LCD_1, 0, 0);

  uint8_t buf[35] = { '\0' };
  snprintf (buf, 35, "%.2f", menu->menuitem[current_opt + i]->value);
  I2C_LCD_WriteString (I2C_LCD_1, "str");
  */
}

void
pressure_main (UART_HandleTypeDef *huart, ADC_HandleTypeDef *hadc,
               TIM_HandleTypeDef *htim_pwm, uint32_t comp_pwm_ch,
               uint32_t exhst_pwm_ch, TIM_HandleTypeDef *htim_upd)
{
  struct Pressure pressure = { .val = 0.0f,
                               .val_last = 0.0f,
                               .val_min = 0.0f,
                               .val_max = 150.0f,
                               .units = "psi",
                               .freq = 0.05f,
                               .ampl = 1.0f,
                               .offset = 0.0f,
                               .huart = huart,
                               .hadc = hadc,
                               .htim_pwm = htim_pwm,
                               .comp_pwm_ch = comp_pwm_ch,
                               .exhst_pwm_ch = exhst_pwm_ch,
                               .htim_upd = htim_upd };

  pressure_init (&pressure);

  pressure_calib_static (&pressure, 10.0f);
  // pressure_calib_dynam_sine (&pressure);
  // pressure_calib_dynam_step (&pressure);

  pressure_cleanup (&pressure);
}

void
pressure_init (struct Pressure *pressure)
{
  HAL_ADC_Start (pressure->hadc);
}

void
pressure_cleanup (struct Pressure *pressure)
{
  userint_flg = 0;

  while (pressure->val >= 0.0000005f)
    pressure_ramp_v2 (pressure, 0.0f, -2.78);

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
pressure_sensor_read (struct Pressure *pressure)
{
  /*
  HAL_ADC_PollForConversion (pressure.hadc, HAL_MAX_DELAY);
  pressure.val = HAL_ADC_GetValue (pressure.hadc)
                  * (PRESSURE_SENSOR_RANGE / ADC_RESOLUTION);
 */
  pressure_uart_tx (pressure);
}

void
pressure_calib_static (struct Pressure *pressure, float target)
{
  while (pressure->val < target)
    {
      if (userint_flg)
        break;

      pressure_ramp_v2 (pressure, target, 2.78f);
    }

  // Display results until user interrupt
  HAL_TIM_Base_Start_IT (pressure->htim_upd);

  while (!userint_flg)
    {
      while (!tim3_flg)
        ;

      // Read in pressure value
      pressure_sensor_read (pressure);

      tim3_flg = 0;
    }

  HAL_TIM_Base_Stop_IT (pressure->htim_upd);
}

void
pressure_ramp_v2 (struct Pressure *pressure, float target, float rate)
{
  float m_c = 2.78f;
  tim3_wraps = 0;

  if (rate > 0.0f)
    {
      TIM2->CCR1 = pressure->htim_pwm->Init.Period * (rate / m_c);
      HAL_TIM_PWM_Start (pressure->htim_pwm, pressure->comp_pwm_ch);
      HAL_TIM_Base_Start_IT (pressure->htim_upd);

      while (tim3_wraps < 5)
        {
          if (userint_flg)
            break;

          if (pressure->val >= target)
            HAL_TIM_PWM_Stop (pressure->htim_pwm, pressure->comp_pwm_ch);

          while (!tim3_flg)
            ;

          // DEBUG: if pwm is on, inc/dec pressure val
          if (HAL_GPIO_ReadPin (GPIOA, GPIO_PIN_1) == GPIO_PIN_SET)
            pressure->val += m_c / 10;

          // Read in pressure value
          pressure_sensor_read (pressure);

          tim3_wraps++;
          tim3_flg = 0;
        }

      HAL_TIM_PWM_Stop (pressure->htim_pwm, pressure->comp_pwm_ch);
    }
  else if (rate < 0.0f)
    {
      TIM2->CCR2 = pressure->htim_pwm->Init.Period * (fabs (rate) / m_c);
      HAL_TIM_PWM_Start (pressure->htim_pwm, pressure->exhst_pwm_ch);
      HAL_TIM_Base_Start_IT (pressure->htim_upd);

      while (tim3_wraps < 5)
        {
          if (userint_flg)
            break;

          if (pressure->val <= target)
            HAL_TIM_PWM_Stop (pressure->htim_pwm, pressure->exhst_pwm_ch);

          while (!tim3_flg)
            ;

          // DEBUG: if pwm is on, inc/dec pressure val
          if (HAL_GPIO_ReadPin (GPIOA, GPIO_PIN_4) == GPIO_PIN_SET)
            pressure->val -= m_c / 10;

          // Read in pressure value
          pressure_sensor_read (pressure);

          tim3_wraps++;
          tim3_flg = 0;
        }

      HAL_TIM_PWM_Stop (pressure->htim_pwm, pressure->exhst_pwm_ch);
    }
  else
    {
      HAL_TIM_Base_Start_IT (pressure->htim_upd);

      while (tim3_wraps < 5)
        {
          if (userint_flg)
            break;

          while (!tim3_flg)
            ;

          // Read in pressure value
          pressure_sensor_read (pressure);

          tim3_wraps++;
          tim3_flg = 0;
        }
    }

  HAL_TIM_Base_Stop_IT (pressure->htim_upd);
}

void
pressure_ramp (struct Pressure *pressure, float target, float rate)
{
  float m_c = 2.78f;
  tim3_wraps = 0;

  if (rate > 0.0f)
    {
      TIM2->CCR1 = pressure->htim_pwm->Init.Period * (rate / m_c);
      HAL_TIM_Base_Start_IT (pressure->htim_upd);

      while (tim3_wraps < 5)
        {
          if (userint_flg)
            break;

          while (!tim3_flg)
            ;

          // DEBUG: if pwm is on, inc/dec pressure val
          if (HAL_GPIO_ReadPin (GPIOA, GPIO_PIN_1) == GPIO_PIN_SET)
            pressure->val += m_c / 10;

          // Read in pressure value
          pressure_sensor_read (pressure);

          tim3_wraps++;
          tim3_flg = 0;
        }
    }
  else if (rate < 0.0f)
    {
      TIM2->CCR2 = pressure->htim_pwm->Init.Period * (fabs (rate) / m_c);
      HAL_TIM_Base_Start_IT (pressure->htim_upd);

      while (tim3_wraps < 5)
        {
          if (userint_flg)
            break;

          while (!tim3_flg)
            ;

          // DEBUG: if pwm is on, inc/dec pressure val
          if (HAL_GPIO_ReadPin (GPIOA, GPIO_PIN_4) == GPIO_PIN_SET)
            pressure->val -= m_c / 10;

          // Read in pressure value
          pressure_sensor_read (pressure);

          tim3_wraps++;
          tim3_flg = 0;
        }
    }
  else
    {
      HAL_TIM_Base_Start_IT (pressure->htim_upd);

      while (tim3_wraps < 5)
        {
          while (!tim3_flg)
            ;

          // Read in pressure value
          pressure_sensor_read (pressure);

          tim3_wraps++;
          tim3_flg = 0;
        }
    }

  HAL_TIM_Base_Stop_IT (pressure->htim_upd);
}

void
pressure_calib_dynam_step (struct Pressure *pressure)
{
  float ampl = 2.0f;
  float offs = 4.0f;
  float freq = 0.1f;
  float sw = 0.5f; // switching in sec
  uint8_t N = 1 / (freq * sw);

  // Get linspaced time array
  float ti[N];
  for (uint8_t i = 0; i < N; i++)
    ti[i] = (i * (1 / freq)) / N;

  // Use above info to gen sine points
  float yi[N];
  for (uint32_t i = 0; i < N; i++)
    yi[i]
        = offs + ((ampl / 2) * pow (-1.0f, floor ((2 * ti[i]) / (1 / freq))));

  // Ramp to initial offset
  while (pressure->val < offs)
    pressure_ramp_v2 (pressure, offs, 2.78f);

  float current_rate;
  while (1)
    {
      for (long i = 1; i < N; i++)
        {
          while (i < N / 2)
            {
              current_rate = (yi[i] - pressure->val) / (ti[i] - ti[i - 1]);
              if (current_rate > 0.0000005f)
                {
                  pressure_ramp_v2 (pressure, yi[i], current_rate);
                }
              else
                pressure_ramp_v2 (pressure, yi[i], 0.0f);

              i++;
            }

          while (i < N)
            {
              current_rate = (yi[i] - pressure->val) / (ti[i] - ti[i - 1]);
              if (current_rate < 0.0000005f)
                {
                  pressure_ramp_v2 (pressure, yi[i], current_rate);
                }
              else
                pressure_ramp_v2 (pressure, yi[i], 0.0f);

              i++;
            }
        }
    }
}

void
pressure_calib_dynam_ramp (struct Pressure *pressure)
{
  // Ramp to initial offset
  pressure_ramp (pressure, pressure->offset, 2.78f);

  // Ramp from ampl until user int
  while (!userint_flg)
    {
      pressure_ramp (pressure, pressure->ampl, (1 / pressure->freq) / 2);
      pressure_ramp (pressure, pressure->ampl,
                     -1 * ((1 / pressure->freq) / 2));
    }

  userint_flg = 0;
}

// FIXME: you dont stop the pwm after the 4th section
void
pressure_calib_dynam_sine (struct Pressure *pressure)
{
  float ampl = 2.0f;
  float offs = 0.0f;
  float freq = 0.05f;
  float sw = 0.5f; // switching in sec
  uint8_t N = 1 / (freq * sw);

  // Get linspaced time array
  float ti[N];
  for (uint8_t i = 0; i < N; i++)
    ti[i] = (i * (1 / freq)) / N;

  // Use above info to gen sine points
  float yi[N];
  for (uint32_t i = 0; i < N; i++)
    yi[i] = offs + ((ampl / 2) * sin (2 * M_PI * freq * ti[i]));

  float current_rate;
  while (1)
    {
      for (long i = 1; i < N; i++)
        {
          while (i < N / 4)
            {
              current_rate = (yi[i] - pressure->val) / (ti[i] - ti[i - 1]);
              if (current_rate > 0.0000005f)
                {
                  pressure_ramp_v2 (pressure, yi[i], current_rate);
                }
              else
                pressure_ramp_v2 (pressure, yi[i], 0.0f);

              i++;
            }

          while (i < 3 * (N / 4))
            {
              current_rate = (yi[i] - pressure->val) / (ti[i] - ti[i - 1]);
              if (current_rate < 0.0000005f)
                {
                  pressure_ramp_v2 (pressure, yi[i], current_rate);
                }
              else
                pressure_ramp_v2 (pressure, yi[i], 0.0f);

              i++;
            }

          while (i < N)
            {
              current_rate = (yi[i] - pressure->val) / (ti[i] - ti[i - 1]);
              if (current_rate > 0.0000005f)
                {
                  pressure_ramp_v2 (pressure, yi[i], current_rate);
                }
              else
                pressure_ramp_v2 (pressure, yi[i], 0.0f);

              i++;
            }
        }
    }
}

// void
// pressure_triwave (struct Pressure *pressure)
//{
//   float m_r = pressure->freq * pressure->ampl;
//   float m_c = 2.78f;
//
//   ramp_init (pressure, m_r, m_c);
//
//   while (1)
//     {
//       tim3_wraps = 0;
//       pressure_calib_dynam_ramp (pressure, 10.0f);
//       tim3_wraps = 0;
//       pressure_calib_dynam_ramp (pressure, 0.0f);
//     }
// }
//
// void
// ramp_init (struct Pressure *pressure, float m_r, float m_c)
//{
//   TIM2->CCR1 = pressure->htim_pwm->Init.Period * (m_r / m_c);
//   HAL_TIM_PWM_Start (pressure->htim_pwm, pressure->comp_pwm_ch);
//   HAL_TIM_Base_Start_IT (pressure->htim_upd);
//
//   tim3_wraps = 0;
// }
//
// void
// ramp_deinit (struct Pressure *pressure)
//{
//   HAL_TIM_Base_Stop (pressure->htim_upd);
//   HAL_TIM_PWM_Stop (pressure->htim_pwm, pressure->comp_pwm_ch);
//>>>>>>> feat/sinewave
// }
