/**
 * @file rotary.h
 *
 * @brief Rotary encoder header
 *
 *        Contains function prototypes.
 */

#ifndef ROTARY_H_
#define ROTARY_H_

#include "pressure.h"

int8_t rotary_get_input (void);
int rotary_getcount (void);
void rotary_setcount (int count);

#endif // ROTARY_H_
