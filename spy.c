  /* spy.c -- things having to do with "other" nations and espionage */

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
#include "misc.h"
#include "functions.h"
#include <stdio.h>
#include <ctype.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>

  /* these macros represent the difficulty in gathering
     inteligence in various areas.
   */
#define SPY_POP 1
#define SPY_ECO 3
#define SPY_MAG 4
#define SPY_MIL 5
#define SPY_TECHNO 5
#define SPY_CAP 6

extern Sworld world;
extern Suser user;
extern char help_tag[];

  /* info on all nations in the world */
char nations_report()
{
  WINDOW *w;
  FILE *diplock;
  char s[200];
  Snation *np;
  char c;
  int i, done = 0, first_shown, n_shown, id;

  w = newwin(LINES-2, COLS, 0, 0); /* full screen */
  werase(w);
  touchwin(w);
  sprintf(s, "report on all nations");
  wmove(w, 0, (COLS-strlen(s))/2); /* make the string centered */
  wstandout(w);
  waddstr(w, s);
  wclrtoeol(w);
  wstandend(w);

    /* here go the guts */
  sprintf(s,"World size is: %dx%d, there are %d nations",
	    world.xmax, world.ymax, world.n_nations);
  wmove(w, 1, (COLS-strlen(s))/2); /* make the string centered */
  waddstr(w, s);
  mvwaddstr(w, 3, 0, " id");
  mvwaddstr(w, 4, 0, "---");
  mvwaddstr(w, 3, 5, "nation");
  mvwaddstr(w, 4, 5, "------");
  mvwaddstr(w, 3, 25, "mark");
  mvwaddstr(w, 4, 25, "----");
  mvwaddstr(w, 3, 31, "leader");
  mvwaddstr(w, 4, 31, "------");
  mvwaddstr(w, 3, 47, "race");
  mvwaddstr(w, 4, 47, "----");
  if (user.id == 0) {
    mvwaddstr(w, 3, 55, "money");
    mvwaddstr(w, 4, 55, "-----");
    mvwaddstr(w, 3, 65, "civil");
    mvwaddstr(w, 4, 65, "-----");
  }
    /* figure out which nations to show */
  first_shown = 1;
  n_shown = min(world.n_nations, LINES-10);
  while (!done) {
    strcpy(help_tag, "Nations Report");
    for (i = 0; i < n_shown && i+first_shown < world.n_nations; ++i) {
      np = &(world.nations[i+first_shown]);
      mvwprintw(w, i+5, 0, "%3d", np->id);
      wclrtoeol(w);
      mvwaddstr(w, i+5, 5, np->name);
      mvwaddch(w, i+5, 26, np->mark);
      sprintf(s,"%-12.12s",np->leader);
      mvwaddstr(w, i+5, 31, s);

      mvwaddstr(w, i+5, 47, np->race.name);

      if (!is_active_ntn(np)) {
	mvwaddstr(w, i+5, 60, "DESTROYED");
      } else if (user.id == 0) {
	mvwprintw(w, i+5, 55, "%d", np->money);
	mvwprintw(w, i+5, 65, "%d", get_n_civil(np));
      }
      if (np->npc_flag) {
	mvwaddstr(w, i+5, 75, "npc");
      }
    }
    wclrtobot(w);
    mvwaddstr(w, LINES-4, 4,
   "Options: [s]py on a nation, [<]/[,] previous screen, [>]/[.] next screen");
    mvwaddstr(w, LINES-3, 4,
	      "Reports: [b]udget, [p]roduction, [i]nfo, [d]iplomacy");
    wrefresh(w);
    statline("type space when done, or F to dump to a file", "nations_report");

    switch (c = getch()) {
    case '>':
    case '.':
      if (first_shown+n_shown < world.n_nations) {
	first_shown += n_shown;
      }
      break;
    case '<':
    case ',':
      if (first_shown > 1) {
	first_shown -= n_shown;
      }
      break;
    case 's':
      mvwaddstr(w, LINES-3, 4, "  Number of nation to spy on? ");
      wclrtoeol(w);
      if (wget_number(w, &id) > 0 && is_active_ntn(&world.nations[id])
	  && id != user.np->id) {
	if (user.id == 0) {	/* for the game master, give the total info */
	  (void) info_report(&world.nations[id]);
	} else {
	  spy_report(id);
	}
	touchwin(w);
      }
      break;
    case 'F':
      dump_current_screen(w, "nations_report");
      break;
    case ' ':
    case 'b':
    case 'p':
    case 'i':
    case 'd':
      done = 1;
      break;
    case '?':
      online_info();
      break;
    default:
      break;
    }
  }
  delwin(w);
  return c;
}

  /* allow a nation to spy on anther */
void spy_report(id)
     int id;
{
  WINDOW *spyw;
  Snation *spied_np = &world.nations[id], *spying_np = user.np;
  char s[EXECLEN];
  char c;
  int done = 0, bribe;		/* amount of jewels to get info */
  int x, y;			/* for capital locations */

  strcpy(help_tag, "Nations Report");
  spyw = newwin(18, 60, LINES-22, (COLS-60)/2);
  werase(spyw);
  touchwin(spyw);
  while (!done) {
    sprintf(s, "Espionage Report on nation %s", spied_np->name);
    wmove(spyw, 1, (60-strlen(s))/2);
    wstandout(spyw);
    waddstr(spyw, s);
    wclrtoeol(spyw);
    wstandend(spyw);
    /* put the guts between here and the wrefresh() */
    mvwaddstr(spyw, 3, 5, "Spy on: [p]opulation, [e]conomy");
    mvwaddstr(spyw, 4, 5, "        [m]ilitary, ma[g]ic, [C]apital location");
    mvwaddstr(spyw, 5, 5, "        [t]echology, [T]echnology theft");
    wclrtobot(spyw);
    box(spyw, '|', '-');
    wrefresh(spyw);
    statline("type space when done, or F to dump to a file", "spy_report");
    c = getch();
    if (strchr("pemgCt", c) != NULL) {
      mvwaddstr(spyw, 10, 1,
		"How many jewels do you want to pay in bribes? ");
      if (wget_number(spyw, &bribe) < 1 || bribe <= 0) {
	continue;
      }
      if (bribe > spying_np->jewels) {
	statline2_err("Hit space", "You don't have enough jewels");
	continue;
      }
    }
    switch (c) {
    case '?':
      online_info();
      break;
    case ' ':
      done = 1;
      break;
    case 'p':			/* info on their population */
      mvwprintw(spyw, 12, 6, "Population is %d",
		spy_figure(get_n_civil(spied_np), bribe,
		spying_np, spied_np, SPY_POP) );
      break;
    case 'e':			/* info on their military */
      mvwprintw(spyw, 12, 4, "Money: %d",
		spy_figure(spied_np->money, bribe,
		spying_np, spied_np, SPY_ECO) );
      mvwprintw(spyw, 12, 20, "Jewels: %d",
		spy_figure(spied_np->jewels, bribe,
		spying_np, spied_np, SPY_ECO) );
      mvwprintw(spyw, 12, 36, "Metal: %d",
		spy_figure(spied_np->metal, bribe,
		spying_np, spied_np, SPY_ECO) );
      mvwprintw(spyw, 13, 4, "Food: %d",
		spy_figure(spied_np->food, bribe,
		spying_np, spied_np, SPY_ECO) );
      mvwprintw(spyw, 13, 20, "Tax: %d",
		spy_figure(spied_np->taxes, bribe,
		spying_np, spied_np, SPY_ECO) );
      break;
    case 'C':
      x = spy_figure(spied_np->capital.x, bribe, spying_np, spied_np, SPY_CAP);
      y = spy_figure(spied_np->capital.y, bribe, spying_np, spied_np, SPY_CAP);
      x = xrel(x, y, spying_np->capital);
      y = yrel(x, y, spying_np->capital);
      mvwprintw(spyw, 13, 4, "Capital is at (%d, %d) ", x, y);
      break;
    case 't':			/* info on their technology */
      mvwprintw(spyw, 12, 4, "Techno skill: %d",
		spy_figure(spied_np->tech_skill, bribe,
		spying_np, spied_np, SPY_TECHNO) );
      break;
    case 'm':			/* info on their military */
      mvwprintw(spyw, 12, 4, "Soldiers: %d",
		spy_figure(get_n_soldiers(spied_np), bribe,
		spying_np, spied_np, SPY_MIL) );
      break;
    case 'g':			/* info on their magic */
      mvwprintw(spyw, 12, 4, "Magic skill: %d",
		spy_figure(spied_np->mag_skill, bribe,
		spying_np, spied_np, SPY_MAG) );
      mvwprintw(spyw, 13, 4, "Spell pts.: %d",
		spy_figure(spied_np->spell_pts, bribe,
		spying_np, spied_np, SPY_MAG) );
      break;
    }
        /* this section is common to all bribes */
    if (strchr("pemgCt", c) != NULL) {
      mvwaddstr(spyw, 14, 10, "Hit space");
      wrefresh(spyw);
      get_space();
      spying_np->jewels -= bribe;
      cjewels(spying_np, -bribe);
    }
  }
  delwin(spyw);
}

int spy_figure(n, expend, spying_np, spied_np, cost_fact)
     int n,			/* number we modify here */
       expend;			/* jewels spent on bribes */
     Snation *spying_np, *spied_np; /* nations involved */
     int cost_fact;		/* cost factor for type of info */
{
  double accuracy, error;
  int figure;			/* the figure we actually return */
  char s[400];

  if (spied_np->secrecy == 0) {
    spied_np->secrecy = 1;	/* avoid divide-by-zero */
  }
  accuracy =
    ((double) expend*spying_np->spy)/(cost_fact*spied_np->secrecy*100);
  
    /* now get the percent error */
  error = 100*( exp(-sqrt(accuracy)) + 1.0/(5.0+accuracy) );
  if (error > 100) {
    error = 100;
  }
/*  sprintf(s, "acc=%f,%%err=%f", accuracy, error); */
    /* now get the absolute error */
  error = 1 + n*error/100.0;
  figure = n + (RND() % ((int) error)) - (int) (error/2);

/*  statline2(s, ""); */

  return figure;
}
