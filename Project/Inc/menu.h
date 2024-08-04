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

/*
 * @brief Initializes the menu driver for the pressure subsystem.
 *
 * Adds custom chars listed under menu.h to I2C_LCD_1.
 *
 * @retval none
 */
void menu_sm_init (void);

/*
** @brief State machine that controls the UI displayed on the LCD.
**
** @param pressure pointer to a Pressure struct
**
** @retval none
*/
void menu_sm (struct Pressure *pressure);

/*
** @brief Sets the state of the LCD state machine based off of rotary_inpt
**
** @param pressure pointer to a Pressure struct
**
** @retval The exit status of the function
**   1 - If
**
*/
void menu_sm_setstate (struct Pressure *pressure, int8_t rotary_inpt);

uint8_t menu_get_waveform (void);

#endif // MENU_H_
