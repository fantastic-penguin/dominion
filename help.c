  /* help.c -- help system (for now quite primitive) */

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
#include "cur_stuff.h"
#include <stdio.h>

extern Suser user;
extern char help_tag[];
WINDOW *helpw;
int help_win_len;

start_help_win()
{
  helpw = newwin(LINES-2, COLS, 0, 0);
  wrefresh(helpw);
  help_win_len = LINES-2;
}

show_help()
{
  char filename[100];
  int done = 0;
  
  statline2("[r]eference card, [l]ong help, or [i]nfo? ", "help");
  switch (getchar()) {
  case 'r':
    show_file(REF_CARD_FILE);
    break;
  case 'l':
    show_file(INFO_FILE);
    break;
  case 'i':			/* info browsing */
    strcpy(help_tag, "Top");
    online_info();
    break;
  default:
    break;
  }
  statline2("", "");
}

end_help_win()
{
  delwin(helpw);
  touchwin(stdscr); 
/*  touch_all_wins(); */
}

  /* a simple pager */
show_file(name)
     char name[];
{
  FILE *fp, *fopen();
  int lines, i;
  char helpline[200], menu_item[80];
  long pos = 0, old_pos = 0;

  start_help_win();		/* initialize the window we use here */
  statline2("", "");
  if ((fp = fopen(name, "r")) == NULL) {
    statline2("file was", name);
    statline("type spece to return", "cannot open file");
    get_space();
    return;
  }
  wclear(helpw);
  lines = 0;
  while (fgets(helpline, 180, fp) != NULL) {
    helpline[78] = '\0';
    mvwaddstr(helpw, lines, 0, helpline);
    wclrtoeol(helpw);
    wrefresh(helpw);
    ++lines;
    if (lines % (help_win_len) == 0) { /* next page? */
      wclrtobot(helpw);
      wrefresh(helpw);
      lines = 0;
      statline("type SPACE to continue, [q] to leave this file", name);
      /*      while (((c = getch()) != ' ') && (c != 'q'))
	      {}*/
      switch (getch()) {
      case 'q':			/* done with this file */
	fclose(fp);
	return;
	break;
      case 'f':			/* skip some lines */
	for (i = 0; i < 44 && fgets(helpline, 180, fp); ++i) {
	}
	break;
      case ' ':
	break;
      default:
	break;
      }
      wmove(helpw, 0, 0);
    }
  }
/*  statline("type space to continue", name);
  get_space();
*/
/*  wclrtobot(helpw);
  end_help_win();*/		/* close up the window */
  fclose(fp);
}

  /* runs the online curses info browser,
     using the global variable help_tag
   */
online_info()
{
  cinfo(INFO_FILE, help_tag);
  user.just_moved = 1;
  touchwin(stdscr);
  user.just_moved = 1;
}
