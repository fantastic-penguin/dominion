  /* root.c -- special super-user commands */

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
#include "functions.h"

#include <stdio.h>
#include <string.h>

extern Sworld world;
extern Suser user;

root_edit()
{
  WINDOW *rew;
  char s[EXECLEN];
  int done = 0;

  rew = newwin(6, COLS-20, LINES/4, 10);
  touchwin(rew);
  while (!done) {
    werase(rew);
    sprintf(s, "Root Editing");
    wmove(rew, 1, (COLS/2-20-strlen(s))/2);
    wstandout(rew);
    waddstr(rew, s);
    wstandend(rew);
    mvwaddstr(rew, 3, 3, "Edit [s]ector or [n]ation? ");
    wrefresh(rew);
    switch (getch()) {
    case 's':
      root_edit_sector();
      break;
    case 'n':
      root_edit_nation();
      break;
    case ' ':
      done = 1;
      break;
    default:
      break;
    }
  }
  delwin(rew);
  touch_all_wins(stdscr);
}

root_edit_sector()
{
  WINDOW *resw;
  int pop_change, done = 0, new_owner, change;

  Ssector *sp = &world.map[user.cursor.x][user.cursor.y];

  resw = newwin(8, COLS-2, LINES-10, 1);
  touchwin(resw);

  while (!done) {
    werase(resw);
    mvwaddstr(resw, 5, 2, "Change: [p]opulation, [o]wner ");
    mvwaddstr(resw, 6, 2, "        [s]oil, [m]etal, [j]ewels, [a]ltitude");
/*    box(resw, 'R', 'R'); */
    wrefresh(resw);
    switch (getch()) {
    case 'p':
      mvwprintw(resw, 1, 1, "Sector (%d,%d), pop=%d, what population change? ",
		sp->loc.x, sp->loc.y, sp->n_people);
      wrefresh(resw);
      if (wget_number(resw, &pop_change) > 0) {
	sp->n_people += pop_change;
	cpeople_sector(sp, pop_change);
      }
      break;
    case 'o':
      mvwprintw(resw, 1, 1, "Sector (%d,%d), owner=%d, which new owner id? ",
		sp->loc.x, sp->loc.y, sp->owner);
      wrefresh(resw);
      if (wget_number(resw, &new_owner) > 0 && new_owner <= world.n_nations) {
	  /* 1. remove from old owner's list
	     2. add to new owner's list
	   */
	if (sp->owner != 0) {
	  subtsector(&world.nations[sp->owner], sp->loc.x, sp->loc.y);
	}
	sp->owner = new_owner;
	cowner_sector(sp, new_owner);
	if (new_owner != 0) {	/* nation 0 has no sector list */
	  addsector(&world.nations[new_owner], sp->loc.x, sp->loc.y);
	}
      }
      break;
    case 's':
      mvwprintw(resw, 1, 1, "Sector (%d,%d), soil=%d, what soil change? ",
		sp->loc.x, sp->loc.y, sp->soil);
      wrefresh(resw);
      if (wget_number(resw, &change) > 0) {
	sp->soil += change;
	csoil_sector(sp, change);
      }
      break;
    case 'm':
      mvwprintw(resw, 1, 1, "Sector (%d,%d), metal=%d, what metal change? ",
		sp->loc.x, sp->loc.y, sp->metal);
      wrefresh(resw);
      if (wget_number(resw, &change) > 0) {
	sp->metal += change;
	cmetal_sector(sp, change);
      }
      break;
    case 'j':
      mvwprintw(resw, 1, 1, "Sector (%d,%d), jewels=%d, what jewels change? ",
		sp->loc.x, sp->loc.y, sp->jewels);
      wrefresh(resw);
      if (wget_number(resw, &change) > 0) {
	sp->jewels += change;
	cjewels_sector(sp, change);
      }
      break;
    case 'a':
      mvwprintw(resw,1,1,"Sector (%d,%d), altitude=%d, what altitude change? ",
		sp->loc.x, sp->loc.y, sp->altitude);
      wrefresh(resw);
      if (wget_number(resw, &change) > 0) {
	sp->altitude += change;
	caltitude_sector(sp, change);
      }
      break;
    case ' ':
      done = 1;
      break;
    default:
      break;
    }
  }
  delwin(resw);
  touch_all_wins(stdscr);
  refresh();
}

  /* allows the game master to tinker with a nation */
root_edit_nation()
{
  WINDOW *renw;
  int done = 0, change, id;
  Snation *np;
  char name[NAMELEN], s[EXECLEN];
  char c;

  Ssector *sp = &world.map[user.cursor.x][user.cursor.y];

  renw = newwin(9, COLS-2, LINES-12, 1);
  touchwin(renw);
  mvwaddstr(renw, 1, 3, "Which nation (id) do you want to change? ");
  wrefresh(renw);
  if (wget_number(renw, &id) < 0) {
    return;
  }
  np = &world.nations[id];
  while (!done) {
    werase(renw);
    mvwprintw(renw, 1, 6, "Editing nation %s [%c]", np->name, np->mark);
    switch (np->npc_flag) {
    case NPC_NOMAIL:
      wprintw(renw, " [npc]     ");
      break;
    case NPC_MAIL:
      wprintw(renw, " [npc+mail]");
      break;
    case NOT_NPC:
    default:
      wprintw(renw, "           ");
      break;
    }
/*	      np->npc_flag ? "npc" : "not npc"); */
    mvwaddstr(renw, 4, 2,
	      "Change: [N]ame, [p]assword, [D]estroy, [t]oggle npc");
    mvwaddstr(renw, 5, 2, "        [s]heckles, [m]etal, [j]ewels, [f]ood");
    mvwaddstr(renw, 6, 2, "        [T]ech skill, [M]ag skill, [S]pell pts");
    mvwaddstr(renw, 7, 2, "        [d]iplomacy, mag. [o]rder, nation mar[k]");
    if (np->npc_flag) {
      mvwaddstr(renw, 8, 2, "        [a]ggressiveness");
/*      mvwaddstr(renw, 8, 2,
		"[a]ggressiveness, [e]xpansionism, [i]solationism");
 */
    }
/*    box(renw, 'R', 'R'); */
    wrefresh(renw);
    switch (getch()) {
    case 'N':
      mvwprintw(renw, 1, 1, "Give new name for nation: ");
      wrefresh(renw);
      if (wget_name(renw, name) > 0) {
	strcpy(np->name, name);
	sprintf(s, "NATION_NAME:%d:%s\n", np->id, name);
	gen_exec(s);
      }
      break;
    case 'p':
      change_passwd(np, renw);
      break;
    case 'D':
      root_destroy_nation(np, renw);
      break;
    case 't':			/* rotate values for the npc flag */
      np->npc_flag = (np->npc_flag + 1) % 3;
/*      np->npc_flag = !np->npc_flag; */
      if (np->npc_flag) {
	sprintf(s, "SET_NPC:%d\n", np->id);
      } else {
	sprintf(s, "CLEAR_NPC:%d\n", np->id);
      }
      gen_exec(s);
      break;
    case 's':
      mvwprintw(renw, 1, 1, "Nation's money=%d;  what change? ", np->money);
      wrefresh(renw);
      if (wget_number(renw, &change) > 0) {
	np->money += change;
	cmoney(np, change);
      }
      break;
    case 'm':
      mvwprintw(renw, 1, 1, "Nation's metal=%d;  what change? ", np->metal);
      wrefresh(renw);
      if (wget_number(renw, &change) > 0) {
	np->metal += change;
	cmetal(np, change);
      }
      break;
    case 'j':
      mvwprintw(renw, 1, 1, "Nation's jewels=%d;  what change? ", np->jewels);
      wrefresh(renw);
      if (wget_number(renw, &change) > 0) {
	np->jewels += change;
	cjewels(np, change);
      }
      break;
    case 'f':
      mvwprintw(renw, 1, 1, "Nation's food=%d;  what change? ", np->food);
      wrefresh(renw);
      if (wget_number(renw, &change) > 0) {
	np->food += change;
	cfood(np, change);
      }
      break;
    case 'T':
      mvwprintw(renw, 1, 1, "Nation's tech skill=%d;  what change? ",
		np->tech_skill);
      wrefresh(renw);
      if (wget_number(renw, &change) > 0) {
	np->tech_skill += change;
	ctech_skill(np, change);
      }
      break;
    case 'M':
      mvwprintw(renw, 1, 1, "Nation's mag skill=%d;  what change? ",
		np->mag_skill);
      wrefresh(renw);
      if (wget_number(renw, &change) > 0) {
	np->mag_skill += change;
	cmag_skill(np, change);
      }
      break;
    case 'S':
      mvwprintw(renw, 1, 1, "Nation's spell pts=%d;  what change? ",
		np->spell_pts);
      wrefresh(renw);
      if (wget_number(renw, &change) > 0) {
	np->spell_pts += change;
	cspell_pts(np, change);
      }
      break;
    case 'o':			/* change magic order */
      do {
	mvwprintw(renw, 1, 1, "Give new magic order (currently %s) ",
		  np->mag_order);
	wclrtoeol(renw);
	wrefresh(renw);
	wget_name(renw, name);
      } while (!is_good_order(name));
      strcpy(np->mag_order, name);
      sprintf(s, "NATION_ORDER:%d:%s\n", np->id, name);
      gen_exec(s);
      break;
    case 'k':			/* change nation mark */
      mvwprintw(renw, 1, 1, "Give new nation mark ");
      wclrtoeol(renw);
      wrefresh(renw);
      fflush(stdin);
      do {
	c = getchar();
	putchar(c);
      } while (!free_nation_mark(&world, c));
      np->mark = c;
      sprintf(s, "NATION_MARK:%d:%c\n", np->id, c);
      gen_exec(s);
      break;
    case 'd':
      diplo_report(np);
      break;
    case 'a':
      if (np->npc_flag) {
	set_aggressiveness(np, renw);
      }
      break;
    case ' ':
      done = 1;
      break;
    default:
      break;
    }
  }
  delwin(renw);
  touch_all_wins();
  refresh();
}

  /* here are some exec lines available only to the Gamemaster */
cowner_sector(sp, owner)
     Ssector *sp;
     int owner;
{
  char s[EXECLEN];

  sprintf(s, "COWNER_SECTOR:%d:%d:%d\n", sp->loc.x, sp->loc.y, owner);
  gen_exec(s);
}

csoil_sector(sp, change)
     Ssector *sp;
     int change;
{
  char s[EXECLEN];

  sprintf(s, "CSOIL_SECTOR:%d:%d:%d\n", sp->loc.x, sp->loc.y, change);
  gen_exec(s);
}

cmetal_sector(sp, change)
     Ssector *sp;
     int change;
{
  char s[EXECLEN];

  sprintf(s, "CMETAL_SECTOR:%d:%d:%d\n", sp->loc.x, sp->loc.y, change);
  gen_exec(s);
}

cjewels_sector(sp, change)
     Ssector *sp;
     int change;
{
  char s[EXECLEN];

  sprintf(s, "CJEWELS_SECTOR:%d:%d:%d\n", sp->loc.x, sp->loc.y, change);
  gen_exec(s);
}

caltitude_sector(sp, change)
     Ssector *sp;
     int change;
{
  char s[EXECLEN];

  sprintf(s, "CALTITUDE_SECTOR:%d:%d:%d\n", sp->loc.x, sp->loc.y, change);
  gen_exec(s);
}

  /* this is serious:  destroy a nation, interactively, as super user */
root_destroy_nation(np, renw)
     Snation *np;
     WINDOW *renw;
{
  char c;
  char s[EXECLEN];

  mvwaddstr(renw, 3, 4, "Are you quite sure?  This is serious!!! ");
  wrefresh(renw);
  if ((c = wgetch(renw)) == 'y' || c == 'Y') {
    mvwaddstr(renw, 4, 6, "Confirm again, dude: ");
    wrefresh(renw);
    if ((c = wgetch(renw)) == 'y' || c == 'Y') {
      destroy_nation(np->id);
      sprintf(s, "DESTROY:%d\n", np->id);
      gen_exec(s);
    }
  }
}

  /* allow the nation's aggressiveness to be modified.  we
     will make a new window so that this can be called from
     various different places (such as the root editing and
     the info report).
   */
set_aggressiveness(np, current_win)
     Snation *np;
     WINDOW *current_win;	/* for redrawing purposes */
{
  WINDOW *aggw;
  int aggr;
  char s[EXECLEN];

  aggw = newwin(8, 30, LINES-14, 10);
  touchwin(aggw);
  mvwprintw(aggw, 3, 3, "Current aggressiveness %d", np->npc_agg);
  mvwprintw(aggw, 4, 3, "Give new value: ");
  box(aggw, '|', '-');
  wrefresh(aggw);
  if (wget_number(aggw, &aggr) < 0) { /* if user types nothing or bad value */
    delwin(aggw);
    touchwin(current_win);
    return;
  }
    /* set the new value and make sure we don't overflow */
  np->npc_agg = aggr;
  np->npc_agg = min(np->npc_agg, 100);
  np->npc_agg = max(np->npc_agg, 0);
  sprintf(s, "NPC_PARAM:%d:%d:%d:%d\n", np->id, np->npc_agg,
	  np->npc_exp, np->npc_iso);
  gen_exec(s);
  delwin(aggw);
  touchwin(current_win);
  return;
}
