#include "pressure.h"
#include "stm32f411xe.h"
#include "stm32f4xx_hal.h"
#include "stm32f4xx_hal_gpio.h"
#include "stm32f4xx_hal_tim.h"
#include <math.h>
#include <stdint.h>

uint8_t userint_flg = 0; // user interrupt flag
uint8_t tim3_flg = 0;    // 100ms timer flag

struct Pressure pressure = { .val = 0.0f,
                             .val_last = 0.0f,
                             .val_min = 0.0f,
                             .val_max = 150.0f,
                             .units = "psi",
                             .freq = 0.2f,
                             .ampl = 2.0f,
                             .offset = 0.0f };

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
pressure_lcd_draw (void)
{
  // pressure +/- uncert
  // current time
  /*
  ** new timer counts to 1 ms
   */
  I2C_LCD_Clear (I2C_LCD_1);
  I2C_LCD_SetCursor (I2C_LCD_1, 0, 0);

  uint8_t str[35] = { '\0' };
  snprintf (buffer, 3, "%.2f", menu->menuitem[current_opt + i]->value);
  I2C_LCD_WriteString (I2C_LCD_1, "str");
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

  pressure_init ();

  pressure_calib_static (10.0f);
  // pressure_calib_dynam_sine ();
  // pressure_calib_dynam_ramp ();

  pressure_cleanup ();
}

void
pressure_disp (void)
{
  pressure_sensor_read ();
  pressure_uart_tx ();
}

void
pressure_init (void)
{
  HAL_ADC_Start (pressure.hadc);
}

void
pressure_cleanup (void)
{
  HAL_ADC_Stop (pressure.hadc);
  HAL_TIM_Base_DeInit (pressure.htim_pwm);
  HAL_TIM_Base_DeInit (pressure.htim_upd);
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
  while (!userint_flg)
    ;

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
          if (userint_flg)
            break;

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
          if (userint_flg)
            break;

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
pressure_calib_dynam_step (void)
{
  // Ramp to initial offset
  pressure_ramp (pressure.offset, 2.78f);

  while (!userint_flg)
    {
      // Ramp to +ampl
      pressure_ramp (pressure.ampl, 2.78f);

      // Delay
      HAL_TIM_Base_Start_IT (pressure.htim_upd);
      for (uint32_t i = 0; i < (((1 / pressure.freq) - (2 * 2.78f)) / 2) / 100;
           i++)
        {
          while (!tim3_flg)
            ;
        }
      HAL_TIM_Base_Stop_IT (pressure.htim_upd);

      // Ramp to -ampl
      pressure_ramp (pressure.ampl, -2.78f);
    }

  userint_flg = 0;
}

void
pressure_calib_dynam_ramp (void)
{
  // Ramp to initial offset
  pressure_ramp (pressure.offset, 2.78f);

  // Ramp from ampl until user int
  while (!userint_flg)
    {
      pressure_ramp (pressure.ampl, (1 / pressure.freq) / 2);
      pressure_ramp (pressure.ampl, -1 * ((1 / pressure.freq) / 2));
    }

  userint_flg = 0;
}

void
pressure_calib_dynam_sine (void)
{
  // Ramp to initial offset
  pressure_ramp (pressure.offset, 2.78f);

  // Get number of samples. Based off the input freq x switching speed
  // max N = 50?
  // Change rate of compressor from 100ms to 500ms
  float sw = 0.1f; // switching in sec
  uint32_t N = round (1 / (pressure.freq * sw));
  if (N > 50)
    N = 50;

  // Get linspaced time array
  float ti[N];
  for (uint32_t i = 0; i < N; i++)
    ti[i] = i + (1 / pressure.freq);

  // Use above info to gen sine points
  float yi[N];
  for (uint32_t i = 0; i < N; i++)
    yi[i] = pressure.offs
            + (pressure.ampl * sin (2 * M_PI * pressure.freq * ti[i]));
  yi[0] = 0;

  // Gen array of rates between 2 points
  float yr[N];
  for (uint32_t i = 1; i < N; i++)
    yr[i] = (yi[i] - yi[i - 1]) / (ti[i] - ti[i - 1]);

  // Sine loop
  while (!userint_flg)
    {
      for (uint32_t i = 0; i < N; i++)
        pressure_ramp (yi[i], yr[i]);
    }

  userint_flg = 0;
}
