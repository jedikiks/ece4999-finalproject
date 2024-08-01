#include "rotary.h"

int CLK_state, prev_CLK_state = 1, btn_state, last_btn_state = 1;
volatile int rotary_count = 0;

int
rotary_getcount (void)
{
  return rotary_count;
}

void
rotary_setcount (int count)
{
  rotary_count = count;
}

int8_t
rotary_get_input (void)
{
  int8_t status = 0;

  CLK_state = HAL_GPIO_ReadPin (GPIOA, ROTARY_CLK_PIN);

  if (CLK_state != prev_CLK_state && CLK_state == 1)
    {
      if (HAL_GPIO_ReadPin (GPIOA, ROTARY_DT_PIN) == 1)
        {
          rotary_count++;
          status = 1;
        }
      else
        {
          rotary_count--;
          status = -1;
        }
    }

  btn_state = HAL_GPIO_ReadPin (GPIOA, ROTARY_SW_PIN);

  if (last_btn_state == 0 && btn_state == 1)
    status = 2;

  last_btn_state = btn_state;
  prev_CLK_state = CLK_state;

  return status;
}
