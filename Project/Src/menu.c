#include "menu.h"
#include "I2C_LCD.h"
#include "pressure.h"

#include <stdlib.h>
#include <string.h>

enum lcd_state
{
  STATE_S0,
  STATE_S0A,
  STATE_S1,
  STATE_S1A,
  STATE_S2,
  STATE_S2A,
  STATE_S3,
  STATE_S3A,
  STATE_S4,
  STATE_S4A
};

static enum lcd_state pressure_lcd_state = 0;
uint8_t waveform_idx = 0;

void menu_sm_printinfo (struct Pressure *pressure);
void menu_sm_println (const char *str, float val, uint8_t cursor_x,
                      uint8_t cursor_y);
void menu_sm_printstr (const char *str, const char *str_val, uint8_t cursor_x,
                       uint8_t cursor_y);

void
menu_sm_init (struct Pressure *pressure)
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

int8_t
menu_sm_setstate (struct Pressure *pressure, int8_t rotary_inpt)
{
  uint8_t status = 0;

  switch (pressure_lcd_state)
    {
    case STATE_S0:
      switch (rotary_inpt)
        {
        case -1:
          pressure_lcd_state = STATE_S4;
          break;

        case 1:
          if (waveform_idx != 0)
            pressure_lcd_state = STATE_S1;
          else
            pressure_lcd_state = STATE_S3;
          break;

        case 2:
          pressure_lcd_state = STATE_S0A;
          pressure->menu.prev_val = waveform_idx;
          break;

        default:
          break;
        }
      break;

    case STATE_S0A:
      switch (rotary_inpt)
        {
        case -1:
          pressure->menu.prev_val -= 1;
          break;

        case 1:
          pressure->menu.prev_val += 1;
          break;

        case 2:
          pressure_lcd_state = STATE_S0;
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

    case STATE_S1:
      switch (rotary_inpt)
        {
        case -1:
          pressure_lcd_state = STATE_S0;
          break;

        case 1:
          pressure_lcd_state = STATE_S2;
          break;

        case 2:
          pressure_lcd_state = STATE_S1A;
          pressure->menu.prev_val = pressure->freq;
          break;

        default:
          break;
        }
      break;

    case STATE_S1A:
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
          pressure_lcd_state = STATE_S1;
          pressure->freq = pressure->menu.prev_val;
          break;

        default:
          break;
        }
      break;

    case STATE_S2:
      switch (rotary_inpt)
        {
        case -1:
          pressure_lcd_state = STATE_S1;
          break;

        case 1:
          pressure_lcd_state = STATE_S3;
          break;

        case 2:
          pressure_lcd_state = STATE_S2A;
          pressure->menu.prev_val = pressure->ampl;
          break;

        default:
          break;
        }
      break;

    case STATE_S2A:
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
          pressure_lcd_state = STATE_S2;
          pressure->ampl = pressure->menu.prev_val;
          break;

        default:
          break;
        }
      break;

    case STATE_S3:
      switch (rotary_inpt)
        {
        case -1:
          if (waveform_idx != 0)
            pressure_lcd_state = STATE_S2;
          else
            pressure_lcd_state = STATE_S0;
          break;

        case 1:
          pressure_lcd_state = STATE_S4;
          break;

        case 2:
          pressure_lcd_state = STATE_S3A;
          pressure->menu.prev_val = pressure->offset;
          break;

        default:
          break;
        }
      break;

    case STATE_S3A:
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
          pressure_lcd_state = STATE_S3;
          pressure->offset = pressure->menu.prev_val;
          break;

        default:
          break;
        }
      break;

    case STATE_S4:
      switch (rotary_inpt)
        {
        case -1:
          pressure_lcd_state = STATE_S3;
          break;

        case 1:
          pressure_lcd_state = STATE_S0;
          break;

        case 2: // Change in output
          if (pressure->menu.output <= 0)
            pressure->menu.output = 1;

          pressure_lcd_state = STATE_S4A;
          status = 1;
          break;

        default:
          break;
        }
      break;

    case STATE_S4A:
      switch (rotary_inpt)
        {
        case 2:
          if (pressure->menu.output >= 1)
            pressure->menu.output = -1;

          pressure_lcd_state = STATE_S4;
          status = 1;
          break;

        default:
          break;
        }
      break;

    default:
      break;
    }

  if (status == 1)
    return waveform_idx;
  else
    return -1;
}

uint8_t
menu_sm (struct Pressure *pressure)
{
  int8_t status = 0;

  I2C_LCD_Clear (I2C_LCD_1);

  switch (pressure_lcd_state)
    {
    case STATE_S0:
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

    case STATE_S0A:
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

    case STATE_S1:
      menu_sm_printinfo (pressure); // Info
      menu_sm_println (" Peri: %.2f sec", pressure->freq, 0,
                       3); // Option

      I2C_LCD_SetCursor (I2C_LCD_1, 19, 3);
      I2C_LCD_PrintCustomChar (I2C_LCD_1, 5);

      I2C_LCD_SetCursor (I2C_LCD_1, 0, 3);
      I2C_LCD_PrintCustomChar (I2C_LCD_1, 0);
      break;

    case STATE_S1A:
      menu_sm_printinfo (pressure);
      menu_sm_println ("Peri: %.2f sec", pressure->menu.prev_val, 0, 3);

      I2C_LCD_SetCursor (I2C_LCD_1, 19, 3);
      I2C_LCD_PrintCustomChar (I2C_LCD_1, 5);

      I2C_LCD_SetCursor (I2C_LCD_1, 5, 3);
      I2C_LCD_PrintCustomChar (I2C_LCD_1, 0);
      break;

    case STATE_S2:
      menu_sm_printinfo (pressure);
      menu_sm_println (" Ampl: %.2f pp", pressure->ampl, 0, 3);

      I2C_LCD_SetCursor (I2C_LCD_1, 19, 3);
      I2C_LCD_PrintCustomChar (I2C_LCD_1, 6);

      I2C_LCD_SetCursor (I2C_LCD_1, 0, 3);
      I2C_LCD_PrintCustomChar (I2C_LCD_1, 0);
      break;

    case STATE_S2A:
      menu_sm_printinfo (pressure);
      menu_sm_println ("Ampl: %.2f pp", pressure->menu.prev_val, 0, 3);

      I2C_LCD_SetCursor (I2C_LCD_1, 19, 3);
      I2C_LCD_PrintCustomChar (I2C_LCD_1, 6);

      I2C_LCD_SetCursor (I2C_LCD_1, 5, 3);
      I2C_LCD_PrintCustomChar (I2C_LCD_1, 0);
      break;

    case STATE_S3:
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

    case STATE_S3A:
      menu_sm_printinfo (pressure);
      menu_sm_println ("Offs: %.2f psi", pressure->menu.prev_val, 0, 3);
      I2C_LCD_SetCursor (I2C_LCD_1, 5, 3);
      I2C_LCD_PrintCustomChar (I2C_LCD_1, 0);
      break;

    case STATE_S4:
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

    case STATE_S4A:
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

  return status;
}
