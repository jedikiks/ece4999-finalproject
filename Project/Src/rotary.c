/**
 * @file rotary.c
 *
 * @brief Rotary encoder program body
 */

#include "rotary.h"
#include "stm32f4xx_hal_gpio.h"
#include <stdint.h>

uint8_t btn_state
    = GPIO_PIN_RESET; /*!< Holds the button's current state, for debouncing */
uint8_t last_btn_state
    = GPIO_PIN_SET; /*!< Holds the button's last state, for debouncing */

volatile int16_t rotary_count = 0; /*!< Current encoder count */
volatile int16_t dir = 0;          /*!< Direction of last encoder turn */

/**
 * @brief Rotary encoder button's interrupt callback
 *
 *        Modifies dir once rotary_count is a high enough value.
 *        If dir = -1 : counter clockwise movement
 *               =  1 : clockwise movement
 *
 * @param htim HAL timer handle for the rotary encoder
 *
 * @retval None
 */
void
HAL_TIM_IC_CaptureCallback (TIM_HandleTypeDef *htim)
{
  rotary_count = __HAL_TIM_GET_COUNTER (htim);
  if (rotary_count >= 1)
    {
      dir = 1;
      __HAL_TIM_SET_COUNTER (htim, 0);
    }
  else if (rotary_count <= -1)
    {
      dir = -1;
      __HAL_TIM_SET_COUNTER (htim, 0);
    }
}

/**
 * @brief Returns rotary encoder movement status
 *
 *        Prioritizes button presses above encoder movement.
 *
 * @retval int8_t -1 : Counter clockwise movement
 *                 0 : No change
 *                 1 : Clockwise movement
 *                 2 : Button press
 */
int8_t
rotary_get_input (void)
{
  int8_t status = 0;

  /* Checks for encoder movement */
  status = dir;
  dir = 0;

  /* Checks for a button press */
  btn_state = HAL_GPIO_ReadPin (GPIOA, ROTARY_SW_PIN);
  if (last_btn_state == GPIO_PIN_RESET && btn_state == GPIO_PIN_SET)
    status = 2;
  last_btn_state = btn_state;

  return status;
}
