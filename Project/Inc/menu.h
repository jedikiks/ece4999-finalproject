#ifndef MENU_H_
#define MENU_H_

#include "pressure.h"

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

static unsigned char lcd_char_scr_3rd_1_3[8] = {
  0b00011, //
  0b00011, //
  0b00011, //
  0b00000, //
  0b00000, //
  0b00000, //
  0b00000, //
  0b00000  //
};

static unsigned char lcd_char_scr_3rd_2_3[8] = {
  0b00000, //
  0b00000, //
  0b00011, //
  0b00011, //
  0b00011, //
  0b00000, //
  0b00000, //
  0b00000  //
};

static unsigned char lcd_char_scr_3rd_3_3[8] = {
  0b00000, //
  0b00000, //
  0b00000, //
  0b00000, //
  0b00000, //
  0b00011, //
  0b00011, //
  0b00011  //
};


static unsigned char lcd_char_scr_qt_1_4[8] = {
  0b00011, //
  0b00011, //
  0b00000, //
  0b00000, //
  0b00000, //
  0b00000, //
  0b00000, //
  0b00000  //
};

static unsigned char lcd_char_scr_qt_2_4[8] = {
  0b00000, //
  0b00000, //
  0b00011, //
  0b00011, //
  0b00000, //
  0b00000, //
  0b00000, //
  0b00000  //
};

static unsigned char lcd_char_scr_qt_3_4[8] = {
  0b00000, //
  0b00000, //
  0b00000, //
  0b00000, //
  0b00011, //
  0b00011, //
  0b00000, //
  0b00000  //
};

static unsigned char lcd_char_scr_qt_4_4[8] = {
  0b00000, //
  0b00000, //
  0b00000, //
  0b00000, //
  0b00000, //
  0b00000, //
  0b00011, //
  0b00011  //
};

static const char *const waveforms[] = { "Const", "Step", "Ramp", "Sine" };

void menu_sm_init (struct Pressure *pressure);
uint8_t menu_sm (struct Pressure *pressure);
int8_t menu_sm_setstate (struct Pressure *pressure, int8_t rotary_inpt);

#endif // MENU_H_
