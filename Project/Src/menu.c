#include "menu.h"
#include "I2C_LCD.h"
#include "pressure.h"
#include "rotary.h"

#include <stdlib.h>
#include <string.h>

// unsigned char cursor_row = 0;

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

static const char *const waveforms[] = { "Const", "Step", "Ramp", "Sine" };
uint8_t waveform_idx = 0;

void menu_sm_printinfo (struct Pressure *pressure);
void menu_sm_println (const char *str, float val, uint8_t cursor_x,
                      uint8_t cursor_y);
void menu_sm_printstr (const char *str, const char *str_val, uint8_t cursor_x,
                       uint8_t cursor_y);

/*
enum MenuState
{
  MainState,
  OptState,
} menu_state;

struct Menu menu[2];

unsigned char scr = 0;
unsigned char current_opt = 0;
int last_cnt = 0;
int rotary_cnt;

void
menu_init (void)
{
  menu[0].menuitem = malloc (6 * sizeof (struct MenuItem *));
  for (int i = 0; i < 6; i++)
    menu[0].menuitem[i] = malloc (sizeof (struct MenuItem));

  menu[1].menuitem = malloc (sizeof (struct MenuItem *));
  menu[1].menuitem[0] = malloc (sizeof (struct MenuItem));

  menu_additem (menu[0].menuitem[0], opt_txt[0], 0, -1, -1, 0, 4);
  menu_additem (menu[0].menuitem[1], opt_txt[1], 0, "Hz", 4, 4, 10);
  menu_additem (menu[0].menuitem[2], opt_txt[2], 0, "pp", 5, 0, 10);
  menu_additem (menu[0].menuitem[3], opt_txt[3], 0, "psi", 50, 0, 100);
  menu_additem (menu[0].menuitem[4], opt_txt[4], 0, -1, -1, 0, 0);

  menu_getopt ();
}

void
menu_togglestate ()
{
  menu_state ^= 1;
  rotary_setcount (last_cnt);
  menu_getopt ();
}

void
menu_additem (struct MenuItem *item, const char **text, int text_idx,
              const char *units, int value, int min, int max)
{
  item->text = text;
  item->text_idx = text_idx;
  item->units = units;
  item->value = value;
  item->min = min;
  item->max = max;
}

void
menu_getopt (void)
{
  int status = 0;
  rotary_cnt = rotary_getcount ();

  switch (menu_state)
    {
    case MainState:
      if (scr == 0)
        {
          if (rotary_cnt > 3)
            {
              scr++;
              rotary_cnt = 0;
              rotary_setcount (0);
            }
          else if (rotary_cnt < 0)
            {
              rotary_cnt = 0;
              rotary_setcount (0);
            }
        }
      else
        {
          if (rotary_cnt > 0)
            {
              rotary_cnt = 0;
              rotary_setcount (0);
            }

          else if (rotary_cnt < 0)
            {
              scr--;
              rotary_cnt = 3;
              rotary_setcount (3);
            }
        }

      current_opt = 3 * scr + rotary_cnt;
      last_cnt = rotary_cnt;

      lcd_menu_draw (&menu[0], scr, 0, current_opt);
      break;
    case OptState:
      if (!strcmp (*menu[0].menuitem[current_opt + 1]->text, "Begin Test"))
        {
          // TODO: From here, people will complete tests based off of the
          // input.
          //   Maybe do a strcmp/check text index and jump to the necessary
          //   function based off the currently connected device.
          //  TODO: So in that case, you'll need to write 2 functions:
          //   1. One that prints sensor data to the lcd (like the code below)
          //   2. One that jumps to the test for the currently connected device
        }
      else
        {
          if (rotary_cnt > 0)
            {
              if (menu[0].menuitem[current_opt]->units == -1)
                {
                  if ((menu[0].menuitem[current_opt]->text_idx + 1)
                      < menu[0].menuitem[current_opt]->max)
                    {
                      menu[0].menuitem[current_opt]->text_idx++;
                    }
                }
              else
                {
                  if ((menu[0].menuitem[current_opt]->value + 1)
                      < menu[0].menuitem[current_opt]->max)
                    {
                      menu[0].menuitem[current_opt]->value++;
                    }
                }
              rotary_cnt = 0;
              rotary_setcount (0);
            }
          else if (rotary_cnt < 0)
            {
              if (menu[0].menuitem[current_opt]->units == -1)
                {
                  if ((menu[0].menuitem[current_opt]->text_idx - 1)
                      >= menu[0].menuitem[current_opt]->min)
                    menu[0].menuitem[current_opt]->text_idx--;
                }
              else
                {
                  if ((menu[0].menuitem[current_opt]->value - 1)
                      > menu[0].menuitem[current_opt]->min)
                    {
                      menu[0].menuitem[current_opt]->value--;
                    }
                }
              rotary_cnt = 0;
              rotary_setcount (0);
            }

          lcd_menu_draw (&menu[0], scr, 11, current_opt);
        }

      break;
    };
};
*/

/*
void
menu_sm_setstate (enum lcd_state state)
{
  switch (rotary_get_input ())
    {
    case -1:
      state > STATE_S1 ? state-- : STATE_S1;
      break;

    case 1:
      state < STATE_S4 ? state++ : STATE_S1;
      break;

    case 2:
      state = STATE_S5;
      break;

    default:
      break;
    }
}
*/

void
menu_sm_init (struct Pressure *pressure)
{
  pressure->menu.upd_flg = 1;
  I2C_LCD_CreateCustomChar (I2C_LCD_1, 0, lcd_char_arrow);
}

void
menu_sm_printinfo (struct Pressure *pressure)
{
  // Pressure value
  menu_sm_println ("Cur:  %.3f psi", pressure->val, 0, 0);

  // Deviation from target
  menu_sm_println (
      "Dev:  %.2f%%",
      ((pressure->val - pressure->target) / pressure->target) * 100, 0, 1);
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
      menu_sm_printstr ("<< Wave: %s>>", waveforms[waveform_idx], 0,
                        3); // Option
      I2C_LCD_SetCursor (I2C_LCD_1, 2, 3);
      I2C_LCD_PrintCustomChar (I2C_LCD_1, 0);
      break;

    case STATE_S0A:
      menu_sm_printinfo (pressure); // Info
      menu_sm_printstr ("<<Wave: %s>>",
                        waveforms[(uint8_t)pressure->menu.prev_val], 0,
                        3); // Option
      I2C_LCD_SetCursor (I2C_LCD_1, 7, 3);
      I2C_LCD_PrintCustomChar (I2C_LCD_1, 0);
      break;

    case STATE_S1:
      menu_sm_printinfo (pressure); // Info
      menu_sm_println ("<< Peri: %.2f sec>>", pressure->freq, 0,
                       3); // Option
      I2C_LCD_SetCursor (I2C_LCD_1, 2, 3);
      I2C_LCD_PrintCustomChar (I2C_LCD_1, 0);
      break;

    case STATE_S1A:
      menu_sm_printinfo (pressure);
      menu_sm_println ("<<Peri: %.2f sec>>", pressure->menu.prev_val, 0, 3);
      I2C_LCD_SetCursor (I2C_LCD_1, 7, 3);
      I2C_LCD_PrintCustomChar (I2C_LCD_1, 0);
      break;

    case STATE_S2:
      menu_sm_printinfo (pressure);
      menu_sm_println ("<< Ampl: %.2f sec>>", pressure->ampl, 0, 3);
      I2C_LCD_SetCursor (I2C_LCD_1, 2, 3);
      I2C_LCD_PrintCustomChar (I2C_LCD_1, 0);
      break;

    case STATE_S2A:
      menu_sm_printinfo (pressure);
      menu_sm_println ("<<Ampl: %.2f sec>>", pressure->menu.prev_val, 0, 3);
      I2C_LCD_SetCursor (I2C_LCD_1, 7, 3);
      I2C_LCD_PrintCustomChar (I2C_LCD_1, 0);
      break;

    case STATE_S3:
      menu_sm_printinfo (pressure);
      menu_sm_println ("<< Offs: %.2f sec>>", pressure->offset, 0, 3);
      I2C_LCD_SetCursor (I2C_LCD_1, 2, 3);
      I2C_LCD_PrintCustomChar (I2C_LCD_1, 0);
      break;

    case STATE_S3A:
      menu_sm_printinfo (pressure);
      menu_sm_println ("<<Offs: %.2f sec>>", pressure->menu.prev_val, 0, 3);
      I2C_LCD_SetCursor (I2C_LCD_1, 7, 3);
      I2C_LCD_PrintCustomChar (I2C_LCD_1, 0);
      break;

    case STATE_S4:
      menu_sm_printinfo (pressure);
      menu_sm_println ("<< Output: %.0f>>", pressure->menu.output, 0, 3);
      I2C_LCD_SetCursor (I2C_LCD_1, 2, 3);
      I2C_LCD_PrintCustomChar (I2C_LCD_1, 0);
      break;

    case STATE_S4A:
      menu_sm_printinfo (pressure);
      menu_sm_println ("<<Output: %.0f>>", pressure->menu.prev_val, 0, 3);
      I2C_LCD_SetCursor (I2C_LCD_1, 7, 3);
      I2C_LCD_PrintCustomChar (I2C_LCD_1, 0);
      break;

    default:
      break;
    }

  return status;
}
