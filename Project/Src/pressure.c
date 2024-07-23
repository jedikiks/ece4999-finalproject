#include "pressure.h"
#include "stm32f411xe.h"
#include "stm32f4xx_hal.h"
#include "stm32f4xx_hal_gpio.h"
#include "stm32f4xx_hal_tim.h"
#include <math.h>
#include <stdint.h>

uint8_t userint_flg = 0; // user interrupt flag
uint8_t tim3_flg = 0;    // send data every 100ms flag
uint8_t tim3_wraps = 0;

struct Pressure pressure = { .val = 0.0f,
                             .val_last = 0.0f,
                             .val_min = 0.0f,
                             .val_max = 150.0f,
                             .units = "psi",
                             .freq = 0.2f,
                             .ampl = 10.0f,
                             .offset = 0.0f };

void ramp_init (float m_r, float m_c);
void ramp_deinit (void);
void pressure_triwave (void);
void pressure_ramp (float target, float rate);

void
HAL_GPIO_EXTI_Callback (uint16_t GPIO_Pin)
{
  if (GPIO_Pin == GPIO_PIN_13)
    {
      pressure_ramp (0.0f, -2.78);
      userint_flg = 1;
    }
}

void
HAL_TIM_PeriodElapsedCallback (TIM_HandleTypeDef *htim_pwm)
{
  pressure_disp ();
  tim3_flg = 1;
}

void
pressure_main (UART_HandleTypeDef *huart, ADC_HandleTypeDef *hadc,
               TIM_HandleTypeDef *htim_pwm, uint32_t tim_ch,
               TIM_HandleTypeDef *htim_upd)
{

  pressure.huart = huart;
  pressure.hadc = hadc;
  pressure.htim_pwm = htim_pwm;
  pressure.tim_ch = tim_ch;
  pressure.htim_upd = htim_upd;

  HAL_ADC_Start (pressure.hadc);

  pressure_calib_static (10.0f);
  // pressure_triwave (&pressure);
  // pressure_calib_dynam_sine (&pressure);

  // TODO: make this a function (it's a square wave) V
  // while (1)
  //   {
  //     tim3_wraps = 0;
  //     pressure_calib_dynam_step (&pressure, 10.0f);
  //     tim3_wraps = 0;
  //     pressure_calib_dynam_step (&pressure, 0.0f);
  //   }

  // pressure_cleanup (&pressure);
}

void
pressure_disp (void)
{
  pressure_sensor_read ();
  pressure_uart_tx ();
}

void
pressure_cleanup (void)
{
  HAL_ADC_Stop (pressure.hadc);
}

void
pressure_uart_tx (void)
{
  uint8_t str[35] = { '\0' };
  sprintf (str, "%.2f\r\n", pressure.val);
  HAL_UART_Transmit (pressure.huart, str, sizeof (str), 100);
}

void
pressure_sensor_read (void)
{
  /*
  HAL_ADC_PollForConversion (pressure.hadc, HAL_MAX_DELAY);
  pressure.val = HAL_ADC_GetValue (pressure.hadc)
                  * (PRESSURE_SENSOR_RANGE / ADC_RESOLUTION);
 */
}

void
pressure_calib_static (float target)
{
  HAL_TIM_Base_Start_IT (pressure.htim_upd);
  // HAL_GPIO_WritePin (GPIOA, PRESSURE_COMPRESSOR_PIN, GPIO_PIN_SET);

  pressure_ramp (target, 2.78f);

  // Display results until user interrupt
  //HAL_TIM_Base_Start_IT (pressure.htim_upd);
  while (!userint_flg)
    ;
  //HAL_TIM_Base_Stop_IT (pressure.htim_upd);

  userint_flg = 0;
  HAL_TIM_Base_Stop_IT (pressure.htim_upd);
}

void
pressure_ramp (float target, float rate)
{
  float m_c = 2.78f;

  if (rate > 0)
    {
      TIM2->CCR1 = pressure.htim_pwm->Init.Period * (rate / m_c);
      HAL_TIM_PWM_Start (pressure.htim_pwm, pressure.tim_ch);
      HAL_TIM_Base_Start_IT (pressure.htim_upd);

      while (pressure.val < target)
        {
          while (!tim3_flg)
            ;

          // DEBUG: if pwm is on, inc/dec pressure val
          if (HAL_GPIO_ReadPin (GPIOA, GPIO_PIN_1) == GPIO_PIN_SET)
            pressure.val += m_c / 10;

          tim3_flg = 0;
        }
    }
  else
    {
      TIM2->CCR1 = pressure.htim_pwm->Init.Period * (fabs (rate) / m_c);
      HAL_TIM_PWM_Start (pressure.htim_pwm, pressure.tim_ch);
      HAL_TIM_Base_Start_IT (pressure.htim_upd);

      while (pressure.val > target)
        {
          while (!tim3_flg)
            ;

          // DEBUG: if pwm is on, inc/dec pressure val
          if (HAL_GPIO_ReadPin (GPIOA, GPIO_PIN_1) == GPIO_PIN_SET)
            pressure.val -= m_c / 10;

          tim3_flg = 0;
        }
    }

  HAL_TIM_Base_Stop_IT (pressure.htim_upd);
  HAL_TIM_PWM_Stop (pressure.htim_pwm, pressure.tim_ch);
}

void
pressure_calib_dynam_ramp (float target)
{
  float m_c = 2.78f;
  if (target > 0)
    {
      while (pressure.val < target)
        {
          if (tim3_flg == 1)
            {
              // TODO: remove sim stuff V
              if (HAL_GPIO_ReadPin (GPIOA, GPIO_PIN_1) == GPIO_PIN_SET)
                pressure.val += m_c / 10;

              pressure_sensor_read ();

              tim3_flg = 0;
              tim3_wraps++;
            }
        }
    }
  else
    {
      while (pressure.val > target)
        {
          if (tim3_flg == 1)
            {
              // TODO: remove sim stuff V
              if (HAL_GPIO_ReadPin (GPIOA, GPIO_PIN_1) == GPIO_PIN_SET)
                pressure.val -= m_c / 10;

              pressure_sensor_read ();

              tim3_flg = 0;
              tim3_wraps++;
            }
        }
    }
}

void
pressure_calib_dynam_step (float target)
{
  if (target > 0)
    {
      do
        {
          pressure_sensor_read ();
          pressure.val += 2.78 / 10;
          HAL_Delay (100);
        }
      while (pressure.val < target);
    }
  else
    {
      do
        {
          pressure_sensor_read ();
          pressure.val -= 2.78 / 10;
          HAL_Delay (100);
        }
      while (pressure.val > target);
    }

  HAL_TIM_Base_Start_IT (pressure.htim_upd);
  while (tim3_wraps < (10 * (1 / pressure.freq)))
    {
      pressure_sensor_read ();
      tim3_wraps++;
    }

  HAL_TIM_Base_Stop (pressure.htim_upd);
}

void
pressure_calib_dynam_sine (void)
{
  float ampl = 2.0f;
  float offs = 0.0f;
  float freq = 0.2f;
  float sw = 0.1f; // switching in sec

  // max N = 50?
  // Change rate of compressor from 100ms to 500ms
  uint8_t N = round (1 / (freq * sw));
  if (N > 50)
    N = 50;

  float ti[N];
  for (uint8_t i = 0; i < N; i++)
    ti[i] = i + (1 / freq);

  float yi[N];
  for (uint8_t i = 0; i < N; i++)
    yi[i] = offs + (ampl * sin (2 * M_PI * freq * ti[i]));
  yi[0] = 0;

  float yr[N];
  for (uint8_t i = 1; i < N; i++)
    yr[i] = (yi[i] - yi[i - 1]) / (ti[i] - ti[i - 1]);

  while (1)
    {
      for (long i = 0; i < N; i++)
        pressure_ramp (yi[i], yr[i]);
    }
}

void
pressure_triwave (void)
{
  float m_r = pressure.freq * pressure.ampl;
  float m_c = 2.78f;

  ramp_init (m_r, m_c);

  while (1)
    {
      tim3_wraps = 0;
      pressure_calib_dynam_ramp (10.0f);
      tim3_wraps = 0;
      pressure_calib_dynam_ramp (0.0f);
    }
}

void
ramp_init (float m_r, float m_c)
{
  TIM2->CCR1 = pressure.htim_pwm->Init.Period * (m_r / m_c);
  HAL_TIM_PWM_Start (pressure.htim_pwm, pressure.tim_ch);
  HAL_TIM_Base_Start_IT (pressure.htim_upd);

  tim3_wraps = 0;
}

void
ramp_deinit (void)
{
  HAL_TIM_Base_Stop (pressure.htim_upd);
  HAL_TIM_PWM_Stop (pressure.htim_pwm, pressure.tim_ch);
}
