  /* init.c -- initialize the keymap */

/*
 * Copyright (C) 1990 Free Software Foundation, Inc.
 * Written by the dominion project.
 *
 * This file is part of dominion.
 *
 * dominion is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as published
 * by the Free Software Foundation; either version 1, or (at your option)
 * any later version.
 *
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this software; see the file COPYING.  If not, write to
 * the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include "dominion.h"

extern Suser user;
extern void (*keymap[128])();

init_keymap()			/* assign keys to default functions */
{
  int i;
    /* now list all commands in keymap */
  void null_key(), bad_key(), quit(), up(), down(), right(), left(),
                 upright(), upleft(),jhome(),jpos(),
                 downright(), downleft(), jup(), jdown(), jright(), jleft(),
                 help(), redraw(), windows(), display_menu(), mail(),
                 report_menu(), army_menu(), zoom_sector(), trade_menu(),
                 wizardry_menu(), dump_map(), news(), construct(),
                 transport(), root_change_passwd(), root_edit(), options();

  for (i = 0; i < 128; ++i) {
    keymap[i] = bad_key;
  }
  if (user.id == 0) {
    keymap['E'] = root_edit;
  }
  keymap[' '] = keymap['\n'] = keymap['\r'] = null_key;
  keymap['q'] = keymap['Q'] = quit;
  keymap['?'] = help;
  keymap[''] = redraw;
  keymap['h'] = keymap['4'] = left;
  keymap['l'] = keymap['6'] = right;
  keymap['k'] = keymap['8'] = up;
  keymap['j'] = keymap['2'] = down;
  keymap['u'] = keymap['9'] = upright;
  keymap['y'] = keymap['7'] = upleft;
  keymap['n'] = keymap['3'] = downright;
  keymap['b'] = keymap['1'] = downleft;
  keymap['H'] = jleft;
  keymap['J'] = jdown;
  keymap['K'] = jup;
  keymap['L'] = jright;
  keymap['w'] = windows;
  keymap['d'] = display_menu;
  keymap['r'] = report_menu;
  keymap['a'] = army_menu;
  keymap['Z'] = zoom_sector;
  keymap['m'] = mail;
#ifdef TRADE_BOARD
  keymap['T'] = trade_menu;
#endif TRADE_BOARD
  keymap['W'] = wizardry_menu;
  keymap['F'] = dump_map;
  keymap['N'] = news;
  keymap['C'] = construct;
  keymap['t'] = transport;
  keymap['O'] = options;
  keymap['P'] = jhome;
  keymap['p'] = jpos;
}
