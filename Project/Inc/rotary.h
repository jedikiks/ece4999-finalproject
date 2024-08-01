#ifndef ROTARY_H_
#define ROTARY_H_

#include "main.h"

#define ROTARY_DT_PIN GPIO_PIN_10 // D2
#define ROTARY_CLK_PIN GPIO_PIN_9 // D9
#define ROTARY_SW_PIN GPIO_PIN_8  // D8

int8_t rotary_get_input (void);
int rotary_getcount (void);
void rotary_setcount (int count);

#endif // ROTARY_H_
