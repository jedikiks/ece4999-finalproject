#include "menu.h"
#include "I2C_LCD.h"
#include "pressure.h"

#include <stdlib.h>
#include <string.h>

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

static enum lcd_state pressure_lcd_state = 0;
uint8_t waveform_idx = 0;

void menu_sm_printinfo (struct Pressure *pressure);
void menu_sm_println (const char *str, float val, uint8_t cursor_x,
                      uint8_t cursor_y);
void menu_sm_printstr (const char *str, const char *str_val, uint8_t cursor_x,
                       uint8_t cursor_y);

void
menu_sm_init ()
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

void
menu_sm_printinfo (struct Pressure *pressure)
{
  // Pressure value
  menu_sm_println ("Cur:  %.1f psi", pressure->val, 0, 0);

  // Deviation from target
  float devi = ((pressure->val - pressure->target) * 100) / pressure->target;
  if (isnan (devi))
    devi = 0.0f;

  if (!isinf(devi))
    menu_sm_println ("Dev:  %.1f%%", devi, 0, 1);
  else
    menu_sm_printstr("Dev:  %s", "--", 0, 1);

  // Time
  menu_sm_println ("Time: %.1f sec", pressure->tim3_elapsed, 0, 2);
}

void
menu_sm_printstr (const char *str, const char *str_val, uint8_t cursor_x,
                  uint8_t cursor_y)
{
  I2C_LCD_SetCursor (I2C_LCD_1, cursor_x, cursor_y);

  char buf[20] = { '\0' };
  snprintf (buf, 20, str, str_val);

  I2C_LCD_WriteString (I2C_LCD_1, buf);
}

void
menu_sm_println (const char *str, float val, uint8_t cursor_x,
                 uint8_t cursor_y)
{
  I2C_LCD_SetCursor (I2C_LCD_1, cursor_x, cursor_y);

  char buf[20] = { '\0' };
  snprintf (buf, 20, str, val);

  I2C_LCD_WriteString (I2C_LCD_1, buf);
}

uint8_t
menu_get_waveform (void)
{
  return waveform_idx;
}

void
menu_sm_setstate (struct Pressure *pressure, int8_t rotary_inpt)
{
  uint8_t status = 0;

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
          pressure->menu.prev_val = pressure->freq;
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
            pressure->menu.prev_val -= 0.10f;
          break;

        case 1:
          if (pressure->menu.prev_val < 0.50f)
            pressure->menu.prev_val += 0.10f;
          break;

        case 2:
          pressure_lcd_state = STATE_PER;
          pressure->freq = pressure->menu.prev_val;
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
          if (pressure->menu.prev_val < 100.0f)
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
          if (pressure->menu.prev_val < 50.0f)
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

        case 2: // Change in output
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

void
menu_sm (struct Pressure *pressure)
{
  I2C_LCD_Clear (I2C_LCD_1);

  switch (pressure_lcd_state)
    {
    case STATE_WAVE:
      menu_sm_printinfo (pressure); // Info
      menu_sm_printstr (" Wave: %s", waveforms[waveform_idx], 0,
                        3); // Option

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
      menu_sm_printinfo (pressure); // Info
      menu_sm_printstr ("Wave: %s",
                        waveforms[(uint8_t)pressure->menu.prev_val], 0,
                        3); // Option

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
      menu_sm_printinfo (pressure); // Info
      menu_sm_println (" Peri: %.2f sec", pressure->freq, 0,
                       3); // Option

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
