#ifndef MENU_H_
#define MENU_H_
#include <stdint.h>

//struct MenuItem
//{
//  const char **text;
//  int text_idx;
//  const char *units;
//  int value;
//  int min;
//  int max;
//  struct Menu *child;
//};
//
//struct Menu
//{
//  struct Menu *parent;
//  struct MenuItem **menuitem;
//};

void menu_init (void);
void menu_additem (struct MenuItem *item, const char **text, int text_idx,
                   const char *units, int value, int min, int max);
void menu_togglestate (void);
void menu_getopt (void);

#endif // MENU_H_
