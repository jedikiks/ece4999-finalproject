#include "proj_main.h"
#include "rotary.h"

#include <stdlib.h>
#include <string.h>

unsigned char cursor_row = 0;

enum MenuState
{
  MainState,
  OptState,
} menu_state;

const char *opt_txt[][5] = {
  { "Wave         Const", "Wave         Step", "Wave         Ramp",
    "Wave         Sine" },
  { "Frequency", "Period" },
  { "Amplitude", "HighLevel" },
  { "Offset", "LowLevel" },
  { "Begin Test" },
};

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
