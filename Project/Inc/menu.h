#ifndef MENU_H_
#define MENU_H_

#include "pressure.h"
#include <stdint.h>

// struct MenuItem
//{
//   const char **text;
//   int text_idx;
//   const char *units;
//   int value;
//   int min;
//   int max;
//   struct Menu *child;
// };
//
// struct Menu
//{
//   struct Menu *parent;
//   struct MenuItem **menuitem;
// };

static unsigned char lcd_char_arrow[8] = {
  0b00000, //
  0b00100, //
  0b00010, //
  0b11111, //
  0b00010, //
  0b00100, //
  0b00000, //
  0b00000  //
};

void menu_init (void);
void menu_additem (struct MenuItem *item, const char **text, int text_idx,
                   const char *units, int value, int min, int max);
void menu_togglestate (void);
void menu_getopt (void);

void menu_sm_init (struct Pressure *pressure);
uint8_t menu_sm (struct Pressure *pressure);
uint8_t menu_sm_setstate (struct Pressure *pressure, int8_t rotary_inpt);

#endif // MENU_H_
