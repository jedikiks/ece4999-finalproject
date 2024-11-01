/**
 * @file menu.c
 *
 * @brief LCD menu program body
 */

#include "menu.h"
#include "I2C_LCD.h"
#include "pressure.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* LCD states */
enum lcd_state
{
  STATE_WAVE,
  STATE_WAVE_SETVAL,
  STATE_PER,
  STATE_PER_SETVAL,
  STATE_AMPL,
  STATE_AMPL_SETVAL,
  STATE_OFFS,
  STATE_OFFS_SETVAL,
  STATE_OUTPUT,
  STATE_OUTPUT_SETVAL
};

static enum lcd_state pressure_lcd_state
    = 0;                  /* Holds the state of the LCD state machine */
uint8_t waveform_idx = 2; /* Indexes waveform strings located in menu.h */

void menu_sm_printinfo (struct Pressure *pressure);
void menu_sm_println (const char *str, float val, uint8_t cursor_x,
                      uint8_t cursor_y);
void menu_sm_printstr (const char *str, const char *str_val, uint8_t cursor_x,
                       uint8_t cursor_y);

/**
 * @brief Initializes the menu driver
 *
 *        Adds custom chars listed under menu.h to I2C_LCD_1.
 *
 * @retval None
 */
void
menu_sm_init (void)
{
  I2C_LCD_CreateCustomChar (I2C_LCD_1, 0, lcd_char_arrow);
  I2C_LCD_CreateCustomChar (I2C_LCD_1, 1, lcd_char_scr_3rd_1_3);
  I2C_LCD_CreateCustomChar (I2C_LCD_1, 2, lcd_char_scr_3rd_2_3);
  I2C_LCD_CreateCustomChar (I2C_LCD_1, 3, lcd_char_scr_3rd_3_3);
  I2C_LCD_CreateCustomChar (I2C_LCD_1, 4, lcd_char_scr_qt_1_4);
  I2C_LCD_CreateCustomChar (I2C_LCD_1, 5, lcd_char_scr_qt_2_4);
  I2C_LCD_CreateCustomChar (I2C_LCD_1, 6, lcd_char_scr_qt_3_4);
  I2C_LCD_CreateCustomChar (I2C_LCD_1, 7, lcd_char_scr_qt_4_4);
}

/**
 * @brief Prints test data to the LCD
 *
 *        Includes current pressure read by the sensor, percent error
 *        between the current and target pressure, and the duration
 *        of the test.
 *
 * @param pressure Pointer to a pressure struct
 *
 * @retval None
 */
void
menu_sm_printinfo (struct Pressure *pressure)
{
  /* Pressure value */
  menu_sm_println ("Cur:  %.1f psi", pressure->val, 0, 0);

  /* Deviation from target */
  float devi = ((pressure->val - pressure->target) * 100) / pressure->target;
  if (isnan (devi))
    devi = 0.0f;

  if (pressure_lcd_state == STATE_OUTPUT_SETVAL)
    {
      if (!isinf (devi))
        menu_sm_println ("Dev:  %.1f%%", devi, 0, 1);

      /* Time */
      menu_sm_println ("Time: %.1f sec", pressure->tim3_elapsed, 0, 2);
    }
}

/**
 * @brief Prints a string with an optional second string to the LCD
 *
 *        (0, 0) is the top left of the LCD, (20, 4) is the bottom right.
 *
 * @param str First part of the string
 * @param str_val Optional second part of the string
 * @param cursor_x Cursor's horizontal position on the LCD starting from 0
 * @param cursor_y Cursor's vertical position on the LCD starting from 0
 *
 * @retval None
 */
void
menu_sm_printstr (const char *str, const char *str_val, uint8_t cursor_x,
                  uint8_t cursor_y)
{
  I2C_LCD_SetCursor (I2C_LCD_1, cursor_x, cursor_y);

  char buf[20] = { '\0' };
  snprintf (buf, 20, str, str_val);

  I2C_LCD_WriteString (I2C_LCD_1, buf);
}

/**
 * @brief Prints a string with an optional floating point value to the LCD
 *
 *        (0, 0) is the top left of the LCD, (20, 4) is the bottom right.
 *
 * @param str First part of the string
 * @param val Optional floating point value inserted into the resulting string
 * @param cursor_x Cursor's horizontal position on the LCD starting from 0
 * @param cursor_y Cursor's vertical position on the LCD starting from 0
 *
 * @retval None
 */
void
menu_sm_println (const char *str, float val, uint8_t cursor_x,
                 uint8_t cursor_y)
{
  I2C_LCD_SetCursor (I2C_LCD_1, cursor_x, cursor_y);

  char buf[20] = { '\0' };
  snprintf (buf, 20, str, val);

  I2C_LCD_WriteString (I2C_LCD_1, buf);
}

/**
 * @brief Sets the state of the LCD state machine using input from the rotary
 * encoder
 *
 * @param pressure Pointer to a pressure struct.
 * @param rotary_inpt Rotary encoder movement. -1 : Counter clockwise movement
 *                                              0 : No change
 *                                              1 : Clockwise movement
 *                                              2 : Button press
 *
 * @retval None
 */
void
menu_sm_setstate (struct Pressure *pressure, int8_t rotary_inpt)
{
  switch (pressure_lcd_state)
    {
    case STATE_WAVE:
      switch (rotary_inpt)
        {
        case -1:
          pressure_lcd_state = STATE_OUTPUT;
          break;

        case 1:
          if (waveform_idx != 0)
            pressure_lcd_state = STATE_PER;
          else
            pressure_lcd_state = STATE_OFFS;
          break;

        case 2:
          pressure_lcd_state = STATE_WAVE_SETVAL;
          pressure->menu.prev_val = waveform_idx;
          break;

        default:
          break;
        }
      break;

    case STATE_WAVE_SETVAL:
      switch (rotary_inpt)
        {
        case -1:
          pressure->menu.prev_val -= 1;
          break;

        case 1:
          pressure->menu.prev_val += 1;
          break;

        case 2:
          pressure_lcd_state = STATE_WAVE;
          waveform_idx = pressure->menu.prev_val;
          break;

        default:
          break;
        }

      if (pressure->menu.prev_val > 3)
        pressure->menu.prev_val = 0;
      else if (pressure->menu.prev_val < 0)
        pressure->menu.prev_val = 3;
      break;

    case STATE_PER:
      switch (rotary_inpt)
        {
        case -1:
          pressure_lcd_state = STATE_WAVE;
          break;

        case 1:
          pressure_lcd_state = STATE_AMPL;
          break;

        case 2:
          pressure_lcd_state = STATE_PER_SETVAL;
          pressure->menu.prev_val = pressure->per;
          break;

        default:
          break;
        }
      break;

    case STATE_PER_SETVAL:
      switch (rotary_inpt)
        {
        case -1:
          if (pressure->menu.prev_val > 0.0f)
            pressure->menu.prev_val -= 1.0f;
          break;

        case 1:
          if (pressure->menu.prev_val < 150.0f)
            pressure->menu.prev_val += 1.0f;
          break;

        case 2:
          pressure_lcd_state = STATE_PER;
          pressure->per = pressure->menu.prev_val;
          break;

        default:
          break;
        }
      break;

    case STATE_AMPL:
      switch (rotary_inpt)
        {
        case -1:
          pressure_lcd_state = STATE_PER;
          break;

        case 1:
          pressure_lcd_state = STATE_OFFS;
          break;

        case 2:
          pressure_lcd_state = STATE_AMPL_SETVAL;
          pressure->menu.prev_val = pressure->ampl;
          break;

        default:
          break;
        }
      break;

    case STATE_AMPL_SETVAL:
      switch (rotary_inpt)
        {
        case -1:
          if (pressure->menu.prev_val > 0.0f)
            pressure->menu.prev_val -= 1.00f;
          break;

        case 1:
          if (pressure->menu.prev_val < 150.0f)
            pressure->menu.prev_val += 1.00f;
          break;

        case 2:
          pressure_lcd_state = STATE_AMPL;
          pressure->ampl = pressure->menu.prev_val;
          break;

        default:
          break;
        }
      break;

    case STATE_OFFS:
      switch (rotary_inpt)
        {
        case -1:
          if (waveform_idx != 0)
            pressure_lcd_state = STATE_AMPL;
          else
            pressure_lcd_state = STATE_WAVE;
          break;

        case 1:
          pressure_lcd_state = STATE_OUTPUT;
          break;

        case 2:
          pressure_lcd_state = STATE_OFFS_SETVAL;
          pressure->menu.prev_val = pressure->offset;
          break;

        default:
          break;
        }
      break;

    case STATE_OFFS_SETVAL:
      switch (rotary_inpt)
        {
        case -1:
          if (pressure->menu.prev_val > 0.0f)
            pressure->menu.prev_val -= 1.00f;
          break;

        case 1:
          if (pressure->menu.prev_val < 150.0f)
            pressure->menu.prev_val += 1.00f;
          break;

        case 2:
          pressure_lcd_state = STATE_OFFS;
          pressure->offset = pressure->menu.prev_val;
          break;

        default:
          break;
        }
      break;

    case STATE_OUTPUT:
      switch (rotary_inpt)
        {
        case -1:
          pressure_lcd_state = STATE_OFFS;
          break;

        case 1:
          pressure_lcd_state = STATE_WAVE;
          break;

        case 2: /* Change in output */
          if (pressure->menu.output <= 0)
            pressure->menu.output = 1;

          pressure_lcd_state = STATE_OUTPUT_SETVAL;
          break;

        default:
          break;
        }
      break;

    case STATE_OUTPUT_SETVAL:
      switch (rotary_inpt)
        {
        case 2:
          if (pressure->menu.output >= 1)
            pressure->menu.output = 0;

          pressure_lcd_state = STATE_OUTPUT;
          break;

        default:
          break;
        }
      break;

    default:
      break;
    }
}

/**
 * @brief State machine that controls the UI displayed on the LCD
 *
 * @param pressure Pointer to a pressure struct
 *
 * @retval None
 */
void
menu_sm (struct Pressure *pressure)
{
  I2C_LCD_Clear (I2C_LCD_1);

  switch (pressure_lcd_state)
    {
    case STATE_WAVE:
      menu_sm_printinfo (pressure); /* Info */
      menu_sm_printstr (" Wave: %s", waveforms[waveform_idx], 0,
                        3); /* Option */

      if (waveform_idx == 0)
        {
          I2C_LCD_SetCursor (I2C_LCD_1, 19, 3);
          I2C_LCD_PrintCustomChar (I2C_LCD_1, 1);
        }
      else
        {
          I2C_LCD_SetCursor (I2C_LCD_1, 19, 3);
          I2C_LCD_PrintCustomChar (I2C_LCD_1, 4);
        }

      I2C_LCD_SetCursor (I2C_LCD_1, 0, 3);
      I2C_LCD_PrintCustomChar (I2C_LCD_1, 0);
      break;

    case STATE_WAVE_SETVAL:
      menu_sm_printinfo (pressure);
      menu_sm_printstr ("Wave: %s",
                        waveforms[(uint8_t)pressure->menu.prev_val], 0, 3);

      if (waveform_idx == 0)
        {
          I2C_LCD_SetCursor (I2C_LCD_1, 19, 3);
          I2C_LCD_PrintCustomChar (I2C_LCD_1, 1);
        }
      else
        {
          I2C_LCD_SetCursor (I2C_LCD_1, 19, 3);
          I2C_LCD_PrintCustomChar (I2C_LCD_1, 4);
        }

      I2C_LCD_SetCursor (I2C_LCD_1, 5, 3);
      I2C_LCD_PrintCustomChar (I2C_LCD_1, 0);
      break;

    case STATE_PER:
      menu_sm_printinfo (pressure);
      menu_sm_println (" Peri: %.2f sec", pressure->per, 0, 3);

      I2C_LCD_SetCursor (I2C_LCD_1, 19, 3);
      I2C_LCD_PrintCustomChar (I2C_LCD_1, 5);

      I2C_LCD_SetCursor (I2C_LCD_1, 0, 3);
      I2C_LCD_PrintCustomChar (I2C_LCD_1, 0);
      break;

    case STATE_PER_SETVAL:
      menu_sm_printinfo (pressure);
      menu_sm_println ("Peri: %.2f sec", pressure->menu.prev_val, 0, 3);

      I2C_LCD_SetCursor (I2C_LCD_1, 19, 3);
      I2C_LCD_PrintCustomChar (I2C_LCD_1, 5);

      I2C_LCD_SetCursor (I2C_LCD_1, 5, 3);
      I2C_LCD_PrintCustomChar (I2C_LCD_1, 0);
      break;

    case STATE_AMPL:
      menu_sm_printinfo (pressure);
      menu_sm_println (" Ampl: %.2f pp", pressure->ampl, 0, 3);

      I2C_LCD_SetCursor (I2C_LCD_1, 19, 3);
      I2C_LCD_PrintCustomChar (I2C_LCD_1, 6);

      I2C_LCD_SetCursor (I2C_LCD_1, 0, 3);
      I2C_LCD_PrintCustomChar (I2C_LCD_1, 0);
      break;

    case STATE_AMPL_SETVAL:
      menu_sm_printinfo (pressure);
      menu_sm_println ("Ampl: %.2f pp", pressure->menu.prev_val, 0, 3);

      I2C_LCD_SetCursor (I2C_LCD_1, 19, 3);
      I2C_LCD_PrintCustomChar (I2C_LCD_1, 6);

      I2C_LCD_SetCursor (I2C_LCD_1, 5, 3);
      I2C_LCD_PrintCustomChar (I2C_LCD_1, 0);
      break;

    case STATE_OFFS:
      menu_sm_printinfo (pressure);
      menu_sm_println (" Offs: %.2f psi", pressure->offset, 0, 3);

      if (waveform_idx == 0)
        {
          I2C_LCD_SetCursor (I2C_LCD_1, 19, 3);
          I2C_LCD_PrintCustomChar (I2C_LCD_1, 1);
        }
      else
        {
          I2C_LCD_SetCursor (I2C_LCD_1, 19, 3);
          I2C_LCD_PrintCustomChar (I2C_LCD_1, 6);
        }

      I2C_LCD_SetCursor (I2C_LCD_1, 0, 3);
      I2C_LCD_PrintCustomChar (I2C_LCD_1, 0);
      break;

    case STATE_OFFS_SETVAL:
      menu_sm_printinfo (pressure);
      menu_sm_println ("Offs: %.2f psi", pressure->menu.prev_val, 0, 3);
      I2C_LCD_SetCursor (I2C_LCD_1, 5, 3);
      I2C_LCD_PrintCustomChar (I2C_LCD_1, 0);
      break;

    case STATE_OUTPUT:
      menu_sm_printinfo (pressure);
      I2C_LCD_SetCursor (I2C_LCD_1, 0, 3);
      I2C_LCD_WriteString (I2C_LCD_1, " Press to begin ");

      if (waveform_idx == 0)
        {
          I2C_LCD_SetCursor (I2C_LCD_1, 19, 3);
          I2C_LCD_PrintCustomChar (I2C_LCD_1, 3);
        }
      else
        {
          I2C_LCD_SetCursor (I2C_LCD_1, 19, 3);
          I2C_LCD_PrintCustomChar (I2C_LCD_1, 7);
        }

      I2C_LCD_SetCursor (I2C_LCD_1, 0, 3);
      I2C_LCD_PrintCustomChar (I2C_LCD_1, 0);

      break;

    case STATE_OUTPUT_SETVAL:
      menu_sm_printinfo (pressure);
      I2C_LCD_SetCursor (I2C_LCD_1, 0, 3);
      I2C_LCD_WriteString (I2C_LCD_1, " Press to abort");

      if (waveform_idx == 0)
        {
          I2C_LCD_SetCursor (I2C_LCD_1, 19, 3);
          I2C_LCD_PrintCustomChar (I2C_LCD_1, 3);
        }
      else
        {
          I2C_LCD_SetCursor (I2C_LCD_1, 19, 3);
          I2C_LCD_PrintCustomChar (I2C_LCD_1, 7);
        }

      I2C_LCD_SetCursor (I2C_LCD_1, 0, 3);
      I2C_LCD_PrintCustomChar (I2C_LCD_1, 0);
      break;

    default:
      break;
    }
}
