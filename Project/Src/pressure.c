/**
 * @file pressure.c
 *
 * @brief Pressure system main program body
 */

#include "pressure.h"
#include "I2C_LCD.h"
#include "menu.h"
#include "rotary.h"
#include "stm32f4xx_hal.h"

#include <math.h>
#include <stdint.h>
#include <stdio.h>

uint8_t userint_flg = 0;     /*!< User interrupt flag */
uint8_t userint_flg_lck = 0; /*!< User interrupt lock var */
uint8_t tim3_flg = 0;        /*!< Flag that indicates 100ms */
uint8_t tim3_wraps = 0;      /*!< Number of 100ms intervals */
float tim3_elapsed = 0.0f;   /*!< Amount of time elapsed during test */

void pressure_init (struct Pressure *pressure);
void pressure_cleanup (struct Pressure *pressure);
void pressure_uart_tx (struct Pressure *pressure);
void pressure_sensor_read (struct Pressure *pressure);
void pressure_ramp_v3 (struct Pressure *pressure, uint8_t dev, float target,
                       float perr);
void pressure_calib_static (struct Pressure *pressure);
void pressure_calib_dynam_step (struct Pressure *pressure);
void pressure_calib_dynam_ramp (struct Pressure *pressure);
void pressure_calib_dynam_sine (struct Pressure *pressure);

/**
 * @brief User interrupt callback
 *
 *        Sets the user interrupt flags once the encoder is pressed.
 *
 * @retval None
 */
void
HAL_GPIO_EXTI_Callback (uint16_t GPIO_Pin)
{
  if (GPIO_Pin == GPIO_PIN_8 && !userint_flg_lck)
    {
      userint_flg = 1;
      userint_flg_lck = 1;
    }
}

/**
 * @brief 100ms timer callback
 *
 *        Sets the 100ms timer flags and increments the total time elapsed
 *        by 100ms.
 *
 * @retval None
 */
void
HAL_TIM_PeriodElapsedCallback (TIM_HandleTypeDef *htim)
{
  tim3_flg = 1;
  tim3_elapsed += 0.1f;
}

/**
 * @brief Pressure system main
 *
 * @param huart Pointer to a HAL UART handle for data plotting
 * @param hadc Pointer to a HAL ADC handle for incoming reference sensor data
 * @param htim_enc Pointer to a HAL timer handle for rotary encoder
 * @param htim_upd Pointer to a HAL timer handle set for 100ms
 *
 * @retval None
 */
void
pressure_main (UART_HandleTypeDef *huart, ADC_HandleTypeDef *hadc,
               TIM_HandleTypeDef *htim_enc, _HandleTypeDef *htim_upd)
{
  /* Initializes struct containing handles to components, menu variables and
   * test parameters */
  struct Pressure pressure = { .val = 0.0f,
                               .target = 0.0f,
                               .per = 20.00f,
                               .ampl = 10.0f,
                               .offset = 10.0f,
                               .huart = huart,
                               .hadc = hadc,
                               .htim_enc = htim_enc,
                               .htim_upd = htim_upd,
                               .menu.output = 0};

  /* Initialization functions */
  pressure_init (&pressure);
  menu_sm_init ();
  menu_sm (&pressure);

  int8_t rotary_inpt; /* Holds rotary encoder directional/button status */

  /* Pressure main loop */
  while (1)
    {
      /* Reads and displays pressure data every 100ms */
      HAL_Delay (100);
      pressure_sensor_read (&pressure);

      /* Poll for rotary encoder and update LCD if there's any input */
      rotary_inpt = rotary_get_input ();
      if (rotary_inpt != 0)
        {
          menu_sm_setstate (&pressure, rotary_inpt);
          menu_sm (&pressure);
        }

      /* Begins the test if the menu state is set to output */
      if (pressure.menu.output)
        {
          /* Enable encoder interrupt */
          HAL_NVIC_EnableIRQ (EXTI9_5_IRQn);

          /* Reset test timer */
          tim3_elapsed = 0;
          pressure.tim3_elapsed = 0;

          /* Begins the specified test */
          switch (menu_get_waveform ())
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

          /* Disables encoder interrupt and resets interrupt flag so tank can
           * depressurize */
          HAL_NVIC_DisableIRQ (EXTI9_5_IRQn);
          userint_flg = 0;
          tim3_elapsed = 0;

          /* Depressurizes the tank and updates the sensor data + LCD every
           * 100ms */
          while (pressure.val >= 0.0000005f)
            {
              HAL_Delay (100);
              pressure_sensor_read (&pressure);
              HAL_GPIO_WritePin (GPIOB, GPIO_PIN_3, GPIO_PIN_SET);
            }
          HAL_GPIO_WritePin (GPIOB, GPIO_PIN_3, GPIO_PIN_RESET);

          /* Disables output and updates LCD */
          pressure.menu.output = 0;
          menu_sm_setstate (&pressure, 2);
          menu_sm (&pressure);
        }
    }

  pressure_cleanup (&pressure);
}

/**
 * @brief Ramps system to a target pressure using no error bounds
 *
 *        Mainly used to ramp to initial offsets
 *
 * @param pressure A pointer to a pressure struct
 * @param dev    The device to turn on to generate the ramp
 *                 1 : Compressor
 *                 2 : Exhaust valve
 * @param target Target pressure to ramp to
 *
 * @retval None
 */
void
pressure_ramp_noconstrain (struct Pressure *pressure, uint8_t dev,
                           float target)
{
  switch (dev)
    {
    case 1: /* Turns on the compressor, starts 100ms timer */
      HAL_TIM_Base_Start_IT (pressure->htim_upd);
      HAL_GPIO_WritePin (GPIOA, GPIO_PIN_5, GPIO_PIN_SET);

      while (pressure->val < target)
        {
          if (userint_flg)
            break;

          while (!tim3_flg)
            ;

          if ((pressure->val < 0.0000005f) && (pressure->val > -0.0000005f))
            pressure->val = 0;

          pressure_sensor_read (pressure);

          tim3_flg = 0;
        }

      HAL_GPIO_WritePin (GPIOA, GPIO_PIN_5, GPIO_PIN_RESET);
      break;

    case 2: /* Turns on the valve, starts 100ms timer */
      HAL_TIM_Base_Start_IT (pressure->htim_upd);
      HAL_GPIO_WritePin (GPIOB, GPIO_PIN_3, GPIO_PIN_SET);

      while (pressure->val > target)
        {
          if (userint_flg)
            break;

          while (!tim3_flg)
            ;

          if ((pressure->val < 0.0000005f) && (pressure->val > -0.0000005f))
            pressure->val = 0;

          pressure_sensor_read (pressure);

          tim3_wraps++;
          tim3_flg = 0;
        }

      HAL_GPIO_WritePin (GPIOB, GPIO_PIN_3, GPIO_PIN_RESET);
      break;
    }

  HAL_TIM_Base_Stop_IT (pressure->htim_upd);
}

/**
 * @brief Ramps system to a target pressure with error bounds
 *
 *        Essentially the same function as pressure_ramp_v3, except this
 *        function turns off the device once the system pressurizes to the specified
 *        target; there's no waiting for any duration between points.
 *
 *        Ultimately, this function was used by pressure_calib_dynam_ramp to
 *        generate a clean triangle wave.
 *
 * @param pressure A pointer to a pressure struct
 * @param dev    The device to turn on to generate the ramp
 *                 1 : Compressor
 *                 2 : Exhaust valve
 * @param target Target pressure to ramp to
 * @param perr   The amount of allowable error for the target. Values should be
 *               between 0.0 and 1.0.
 *
 * @retval None
 */
void
pressure_ramp_v4 (struct Pressure *pressure, uint8_t dev, float target,
                  float perr)
{
  if ((target < 0.0000005f) && (target > -0.0000005f))
    target = 0;

  /* Calculates max and min pressure targets based off target and error args */
  float b_mx = target + (perr * target);
  float b_mn = target - (perr * target);

  if ((pressure->val < 0.0000005f) && (pressure->val > -0.0000005f))
    pressure->val = 0;

  tim3_wraps = 0;

  switch (dev)
    {
    case 1: /* Turns on the compressor, starts 100ms timer */
      HAL_TIM_Base_Start_IT (pressure->htim_upd);
      HAL_GPIO_WritePin (GPIOA, GPIO_PIN_5, GPIO_PIN_SET);

      while (pressure->val <= target)
        {
          if (userint_flg)
            break;

          while (!tim3_flg)
            ;

          if ((pressure->val < 0.0000005f) && (pressure->val > -0.0000005f))
            pressure->val = 0;

          pressure->target = target;
          pressure_sensor_read (pressure);

          tim3_wraps++;
          tim3_flg = 0;
        }

      HAL_GPIO_WritePin (GPIOA, GPIO_PIN_5, GPIO_PIN_RESET);
      break;

    case 2: /* Turns on the valve, starts 100ms timer */
      HAL_TIM_Base_Start_IT (pressure->htim_upd);
      HAL_GPIO_WritePin (GPIOB, GPIO_PIN_3, GPIO_PIN_SET);

      while (pressure->val >= target)
        {
          if (userint_flg)
            break;

          while (!tim3_flg)
            ;

          if ((pressure->val < 0.0000005f) && (pressure->val > -0.0000005f))
            pressure->val = 0;

          pressure->target = target;
          pressure_sensor_read (pressure);

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

          pressure_sensor_read (pressure);

          tim3_wraps++;
          tim3_flg = 0;
        }
      break;
    }

  HAL_TIM_Base_Stop_IT (pressure->htim_upd);
}

/**
 * @brief Ramps system to a target pressure with error bounds
 *
 *        Turns off the compressor or valve when either the target pressure is
 *        met or 500ms has passed.
 *
 * @param pressure A pointer to a pressure struct
 * @param dev    The device to turn on to generate the ramp.
 *                 1 : Compressor
 *                 2 : Exhaust valve
 *                 default: wait for 500ms
 * @param target Target pressure to ramp to
 * @param perr   The amount of allowable error for the target. Values should be
 *               between 0.0 and 1.0.
 *
 * @retval None
 */
void
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
    case 1: /* Turns on the compressor, starts 100ms timer */
      HAL_TIM_Base_Start_IT (pressure->htim_upd);
      HAL_GPIO_WritePin (GPIOA, GPIO_PIN_5, GPIO_PIN_SET);

      while (tim3_wraps < 5)
        {
          if (userint_flg)
            break;

          while (!tim3_flg)
            ;

          if ((pressure->val < 0.0000005f) && (pressure->val > -0.0000005f))
            pressure->val = 0;

          pressure->target = target;
          pressure_sensor_read (pressure);

          if ((fabs (pressure->val) <= fabs (b_mx))
              && (fabs (pressure->val) >= fabs (b_mn)))
            HAL_GPIO_WritePin (GPIOA, GPIO_PIN_5, GPIO_PIN_RESET);

          tim3_wraps++;
          tim3_flg = 0;
        }

      HAL_GPIO_WritePin (GPIOA, GPIO_PIN_5, GPIO_PIN_RESET);
      break;

    case 2: /* Turns on the valve, starts 100ms timer */
      HAL_TIM_Base_Start_IT (pressure->htim_upd);
      HAL_GPIO_WritePin (GPIOB, GPIO_PIN_3, GPIO_PIN_SET);

      while (tim3_wraps < 5)
        {
          if (userint_flg)
            break;

          while (!tim3_flg)
            ;

          if ((pressure->val < 0.0000005f) && (pressure->val > -0.0000005f))
            pressure->val = 0;

          pressure->target = target;
          pressure_sensor_read (pressure);

          if ((fabs (pressure->val) <= fabs (b_mx))
              && (fabs (pressure->val) >= fabs (b_mn)))
            HAL_GPIO_WritePin (GPIOB, GPIO_PIN_3, GPIO_PIN_RESET);

          tim3_wraps++;
          tim3_flg = 0;
        }
      HAL_GPIO_WritePin (GPIOB, GPIO_PIN_3, GPIO_PIN_RESET);
      break;

    default: /* Waits and reads + displays pressure sensor data for 500ms */
      HAL_TIM_Base_Start_IT (pressure->htim_upd);

      while (tim3_wraps < 5)
        {
          while (!tim3_flg)
            ;

          pressure_sensor_read (pressure);

          tim3_wraps++;
          tim3_flg = 0;
        }
      break;
    }

  HAL_TIM_Base_Stop_IT (pressure->htim_upd);
}

/**
 * @brief Initializes components for the pressure system
 *
 * @param pressure A pointer to a pressure struct
 *
 * @retval None
 */
void
pressure_init (struct Pressure *pressure)
{
  HAL_NVIC_DisableIRQ (EXTI9_5_IRQn);
  I2C_LCD_Init (I2C_LCD_1);
  HAL_TIM_Encoder_Start_IT (pressure->htim_enc, TIM_CHANNEL_ALL);
}

/**
 * @brief Deinitializes components for the pressure system
 *
 * @param pressure A pointer to a pressure struct
 *
 * @retval None
 */
void
pressure_cleanup (struct Pressure *pressure)
{
  userint_flg = 0;

  HAL_ADC_Stop (pressure->hadc);
  HAL_TIM_Base_DeInit (pressure->htim_enc);
  HAL_TIM_Base_DeInit (pressure->htim_upd);
}

/**
 * @brief Transmits the current pressure read by the sensor out through UART
 *
 * @param pressure A pointer to a pressure struct
 *
 * @retval None
 */
void
pressure_uart_tx (struct Pressure *pressure)
{
  uint8_t str[35] = { '\0' };
  sprintf (str, "%.2f\r\n", pressure->val);
  HAL_UART_Transmit (pressure->huart, str, sizeof (str), 100);
}

/**
 * @brief Reads in the current pressure from the sensor using the ADC
 *
 *        Displays sensor data to LCD and transmits through UART to PC.
 *
 * @param pressure A pointer to a pressure struct
 *
 * @retval None
 */
void
pressure_sensor_read (struct Pressure *pressure)
{
  /* Reads in sensor data */
  HAL_ADC_Start (pressure->hadc);
  HAL_ADC_PollForConversion (pressure->hadc, 20);
  pressure->val = (HAL_ADC_GetValue (pressure->hadc) * 200) / 4096.0f;

  /* Transmits sensor data through UART, updates test duration and LCD */
  pressure_uart_tx (pressure);
  pressure->tim3_elapsed = tim3_elapsed;
  menu_sm (pressure);
}

/**
 * @brief Function that performs static calibration
 *
 *        Ramps to the specified offset under the .offset member of
 *        struct Pressure. Maintains pressure until user interrupt flag
 *        is set.
 *
 * @param pressure A pointer to a pressure struct
 *
 * @retval None
 */
void
pressure_calib_static (struct Pressure *pressure)
{
  /* Reset user interrupt flag */
  userint_flg = 0;
  userint_flg_lck = 0;

  /* Ramp to the offset */
  pressure_ramp_noconstrain (pressure, 1, pressure->offset);

  /* Display sensor data on LCD and UART every 100ms until user interrupts */
  HAL_TIM_Base_Start_IT (pressure->htim_upd);

  while (!userint_flg)
    {
      while (!tim3_flg)
        ;

      pressure_sensor_read (pressure);
      tim3_flg = 0;
    }

  HAL_TIM_Base_Stop_IT (pressure->htim_upd);
}

/**
 * @brief Function that performs dynamic step calibration
 *
 *        Forms a square wave by continuously ramping to the value in .ampl
 *        from .offset, and waiting for a duration specified by .per found
 *        under struct Pressure.
 *
 * @param pressure A pointer to a pressure struct
 *
 * @retval None
 */
void
pressure_calib_dynam_step (struct Pressure *pressure)
{
  userint_flg = 0;
  userint_flg_lck = 0;

  float sw = 0.8f; /* Switching time of the components in seconds */

  uint8_t N = pressure->per / sw;

  /* Generate linspaced time array */
  float ti[N];
  for (uint8_t i = 0; i < N; i++)
    ti[i] = (i * pressure->per) / N;

  /* Generate targets using a square wave function */
  float yi[N];
  for (uint32_t i = 0; i < N; i++)
    yi[i] = pressure->offset
            + ((pressure->ampl / 2)
               * pow (-1.0f, floor ((2 * ti[i]) / pressure->per)));

  /* Ramp to the initial offset */
  pressure_ramp_noconstrain (pressure, 1, pressure->offset);

  /* Loop through points until user interrupts test */
  while (1)
    {
      if (userint_flg)
        break;

      uint32_t i = 0;
      while (i < N / 2)
        {
          if (userint_flg)
            break;

          pressure_ramp_v3 (pressure, 1, yi[i], 0.1f);
          i++;
        }

      while (i < N)
        {
          if (userint_flg)
            break;

          pressure_ramp_v3 (pressure, 2, yi[i], 0.1f);
          i++;
        }
    }
}

/**
 * @brief Function that performs dynamic ramp calibration
 *
 *        Generates a triangle wave using the specified parameters
 *        held by the .per, .ampl, .offset members in struct Pressure.
 *
 *        To generate a cleaner ramp function, the sine function along with
 *        pressure_ramp_v4 are used instead of an actual ramp function with
 *        pressure_ramp_v3. The previous method still works, except it would
 *        take some work playing with parameters to generate a proper
 *        triangle wave. The previous method has been left commented in the
 *        code below.
 *
 * @param pressure A pointer to a pressure struct
 *
 * @bug Given the nature of pressure_ramp_v4, this function essentially ignores
 *      the .per member, effectively making the period parameter useless.
 *
 * @retval None
 */
void
pressure_calib_dynam_ramp (struct Pressure *pressure)
{
  userint_flg = 0;
  userint_flg_lck = 0;

  float sw = 0.5f;                /* Switching in sec */
  uint8_t N = pressure->per / sw; /* Number of points that make up the wave */

  /* Generate linspaced time array */
  float ti[N];
  for (uint8_t i = 0; i < N; i++)
    ti[i] = (i * pressure->per) / N;

  /* Generate targets */
  float yi[N];
  for (uint32_t i = 0; i < N; i++)
    yi[i] = pressure->offset
            + ((pressure->ampl / 2)
               * sin (2 * M_PI * (1 / pressure->per) * ti[i]));

  /* Previous ramp function:
   * yi[i] = ((((4 * half_ampl) / pressure->per)
   *           * fabs (fmod ((fmod ((ti[i] - (pressure->per / 4)), pressure->per)
   *           + pressure->per), pressure->per) - (pressure->per / 2))) - half_ampl)
   *           + pressure->offset;
   */

  /* Ramp to initial offset */
  pressure_ramp_noconstrain (pressure, 1, pressure->offset);

  /* Loop through targets until user interrupts */
  while (1)
    {
      if (userint_flg)
        break;

      uint32_t i = 0;
      while (i < N / 4)
        {
          if (userint_flg)
            break;

          /* pressure_ramp_v3(pressure, 1, yi[i], 0.1f); */
          pressure_ramp_v4 (pressure, 1, yi[i], 0.1f);
          i++;
        }

      while (i < 3 * (N / 4))
        {
          if (userint_flg)
            break;

          /* pressure_ramp_v3(pressure, 2, yi[i], 0.1f); */
          pressure_ramp_v4 (pressure, 2, yi[i], 0.1f);
          i++;
        }

      while (i < N)
        {
          if (userint_flg)
            break;

          /* pressure_ramp_v3(pressure, 1, yi[i], 0.1f); */
          pressure_ramp_v4 (pressure, 1, yi[i], 0.1f);
          i++;
        }
    }
}

/**
 * @brief Function that performs dynamic sine calibration
 *
 *        Generates a sine wave using the specified parameters
 *        held by the .per, .ampl, .offset members in struct
 *        Pressure.
 *
 * @param pressure A pointer to a pressure struct
 *
 * @retval None
 */
void
pressure_calib_dynam_sine (struct Pressure *pressure)
{
  userint_flg = 0;
  userint_flg_lck = 0;

  float sw = 0.8f;                /* Switching in sec */
  uint8_t N = pressure->per / sw; /* Number of points that make up the wave */

  /* Generate linspaced time array */
  float ti[N];
  for (uint8_t i = 0; i < N; i++)
    ti[i] = (i * pressure->per) / N;

  /* Generate targets */
  float yi[N];
  for (uint32_t i = 0; i < N; i++)
    yi[i] = pressure->offset
            + ((pressure->ampl / 2)
               * sin (2 * M_PI * (1 / pressure->per) * ti[i]));

  /* Ramp to initial offset */
  pressure_ramp_noconstrain (pressure, 1, pressure->offset);

  /* Loop through targets until user interrupts */
  while (1)
    {
      if (userint_flg)
        break;

      uint32_t i = 0;
      while (i <= N / 4)
        {
          if (userint_flg)
            break;

          pressure_ramp_v3 (pressure, 1, yi[i], 0.2f);
          i++;
        }

      while (i <= 3 * (N / 4))
        {
          if (userint_flg)
            break;

          pressure_ramp_v3 (pressure, 2, yi[i], 0.2f);
          i++;
        }

      while (i <= N)
        {
          if (userint_flg)
            break;

          pressure_ramp_v3 (pressure, 1, yi[i], 0.2f);
          i++;
        }
    }
}
