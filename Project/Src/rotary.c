#include "rotary.h"
#include "stm32f4xx_hal_gpio.h"

int CLK_state;
int prev_CLK_state = 1;
int btn_state = 0;
int last_btn_state = GPIO_PIN_RESET;
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
  unsigned char status = 0;

  CLK_state = HAL_GPIO_ReadPin (GPIOA, ROTARY_CLK_PIN);

  if (CLK_state != prev_CLK_state && CLK_state == 1)
    {
      if (HAL_GPIO_ReadPin (GPIOA, ROTARY_DT_PIN) == 1)
        status = -1;
      else
        status = 1;
    }

  btn_state = HAL_GPIO_ReadPin (GPIOA, ROTARY_SW_PIN);

  if (last_btn_state == 0 && btn_state == 1)
    status = 2;

  last_btn_state = btn_state;
  prev_CLK_state = CLK_state;

  return status;
}

// int8_t
// rotary_get_input (void)
//{
//   int8_t status = 0;
//
//   CLK_state = HAL_GPIO_ReadPin (GPIOA, ROTARY_CLK_PIN);
//   if (CLK_state != prev_CLK_state && CLK_state == GPIO_PIN_SET)
//     {
//       if (HAL_GPIO_ReadPin (GPIOA, ROTARY_DT_PIN) == GPIO_PIN_SET)
//         {
//           rotary_count--;
//           status = -1;
//         }
//       else if (HAL_GPIO_ReadPin (GPIOA, ROTARY_DT_PIN) == GPIO_PIN_RESET)
//         {
//           rotary_count++;
//           status = 1;
//         }
//     }
//   prev_CLK_state = CLK_state;
//
//   btn_state = HAL_GPIO_ReadPin (GPIOA, ROTARY_SW_PIN);
//   if (last_btn_state == GPIO_PIN_SET && btn_state == GPIO_PIN_RESET)
//     status = 2;
//   last_btn_state = btn_state;
//
//   return status;
// }
