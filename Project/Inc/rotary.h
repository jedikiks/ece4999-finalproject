#ifndef ROTARY_H_
#define ROTARY_H_


#define ROTARY_DT_PIN GPIO_PIN_10
#define ROTARY_CLK_PIN GPIO_PIN_9
#define ROTARY_SW_PIN GPIO_PIN_15

unsigned char rotary_check (void);
int rotary_getcount (void);
void rotary_setcount (int count);

#endif // ROTARY_H_
