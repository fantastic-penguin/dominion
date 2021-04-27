/* construct.c -- routines dealing with construction */

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
#include "army.h"
#include "costs.h"
#include "cur_stuff.h"
#include "functions.h"

#include <stdio.h>
#include <math.h>
#include <string.h>

extern Suser user;
extern Sworld world;
extern char help_tag[];
extern WINDOW * sectw;

extern int (*wrapx)(), (*wrapy)();

  /* construct a constructible object */
construct()
{
  WINDOW *cw;
  char c;
  int done=0;
  Snation *np = user.np;
  Ssector *sp =  &world.map[user.cursor.x][user.cursor.y];

  strcpy(help_tag, "Construction");

  if (user.xmode) {
    cw = NULL;
  } 
  else {
    cw = newwin(6, COLS/2+18, LINES-11, COLS/4-16);
  }

  while (!done) {
    if (cw != NULL) {
      statline("", "construct");
      mvwprintw(cw, 1, 1, "What type of object do you want to construct? ");
      mvwprintw(cw, 2, 1, " [f]ortification, [r]oads, [b]ubble");
      wmove(cw, 3, 1);
      wclrtobot(cw);
      box(cw, '|', '-');
      wrefresh(cw);
    }
    else {			/* expert mode */
      statline_prompt ("Construct: (f,r,b)", "construct");
    }

    switch (c = mygetch()) {
    case 'f':
      construct_fortification(np, sp, cw);
      break;
    case 'r':
      construct_roads(np, sp, cw);
      break;
    case 'b':
      construct_bubble(np, sp, cw);
      break;
    case ' ':
      done = 1;
      break;
    default:
      break;
    }
  }

  if (cw != NULL) {
    wrefresh(cw);
    delwin(cw);
  }

  touch_all_wins();
  statline2("", "");
  show_armies(sp);
  return 1;
}

  /* build fortification on a sector */
construct_fortification(np, sp, w)
     Snation *np;
     Ssector *sp;
     WINDOW *w;
{
  int fort_increase, ret, cost, cost_met;
  char s[EXECLEN];
  char c;

  if (sp->owner != user.id) {
    statline2_err("hit space to go on", "hey dude, this sector is not yours");
    return -1;
  }
  cost = (int) (pow(2.0, (double) sp->defense/10)*FORT_COST_MONEY);
  cost_met = (int) (pow(2.0, (double) sp->defense/10)*FORT_COST_METAL);

  if (w != NULL) {
    mvwprintw(w, 3, 1, "Current fort %d; cost for 10 more: %d sk., %d met.",
	      sp->defense, cost, cost_met);
    mvwprintw(w, 4, 1, "Build ten more (y/n)? ");
    box(w, '|', '-');
    wrefresh(w);
  }
  else {
    sprintf (s, "Current: %d, Cost +10 = %d sk., %d met. Build? ",
	     sp->defense, cost, cost_met);
    statline2 (s, "con_fort");
  }

  if ((c = getch()) != 'y' && c != 'Y') {
    statline2 ("", "");
    return -1;
  }
  if (w == NULL) {
    statline2 ("", "");
  }
  fort_increase = 10;
    /* now see if we can afford the construction */
  if (cost > user.np->money) {
    statline2_err("space to go on", "not enough money to construct");
    return -1;
  }
  if (cost_met > user.np->metal) {
    statline2_err("space to go on", "not enough metal to construct");
    return -1;
  }

  /* if we have reached this point, it means we can construct! */

  sp->defense += fort_increase;
  np->money -= cost;
  np->metal -= cost_met;
    /* now prepare the exec string */
  sprintf(s, "CFORT_SECTOR:%d:%d:%d\n", sp->loc.x, sp->loc.y, fort_increase);
  gen_exec(s);
  cmoney(np, -cost);
  cmetal(np, -cost_met);

  return 1;
}
  /* build roads on a sector */
construct_roads(np, sp, w)
     Snation *np;
     Ssector *sp;
     WINDOW *w;
{
  int roads_increase, ret, cost, cost_met;
  char s[EXECLEN];
  char c;

  if (sp->owner != user.id) {
    statline2_err("hit space to go on", "hey dude, this sector is not yours");
    return -1;
  }
  cost = (int) (pow(2.0, (double) sp->roads)*ROADS_COST_MONEY);
  cost_met = (int) (pow(2.0, (double) sp->roads)*ROADS_COST_METAL);

  if (w != NULL) {
    mvwprintw(w, 3, 1,
	      "Current roads %d; cost %d sk., %d met.", sp->roads, cost, cost_met);
    mvwprintw(w, 4, 1, "Build one more (y/n)? ");
    box(w, '|', '-');
    wrefresh(w);
  }
  else {
    sprintf (s, "Current: %d, Cost +1 = %d sk., %d met. Build? ",
	     sp->roads, cost, cost_met);
    statline2 (s, "con_roads");
  }
  if ((c = getch()) != 'y' && c != 'Y') {
    statline2 ("", "");
    return -1;
  }

  if (w == NULL) {
    statline2 ("", "");
  }

  roads_increase = 1;
    /* now see if we can afford the construction */
  if (roads_increase*cost > user.np->money) {
    statline2_err("space to go on", "not enough money to construct");
    return -1;
  }
  if (roads_increase*cost_met > user.np->metal) {
    statline2_err("space to go on", "not enough metal to construct");
    return -1;
  }

  /* if we have reached this point, it means we can construct! */

  sp->roads += roads_increase;
  np->money -= roads_increase*cost;
  np->metal -= roads_increase*cost_met;
    /* now prepare the exec string */
  sprintf(s, "CROADS_SECTOR:%d:%d:%d\n", sp->loc.x, sp->loc.y, roads_increase);
  gen_exec(s);
  cmoney(np, -roads_increase*cost);
  cmetal(np, -roads_increase*cost_met);

  return 1;
}

  /* build a bubble on a sector; allowing a
     race to live in a sector below (or above)
     water, when they normally could not.
   */
construct_bubble(np, sp, w)
     Snation *np;
     Ssector *sp;
     WINDOW *w;
{
  int ret, cost, cost_met;
  char s[EXECLEN];
  char c;

  if (sp->owner != user.id) {
    statline2_err("hit space to go on", "Hey dude, this sector is not yours");
    return -1;
  }
  if (has_bubble(sp)) {
    statline2_err("hit space to go on", "There already is a bubble!");
  }
  cost = BUBBLE_COST;
  cost_met = BUBBLE_COST_METAL;

  if (w != NULL) {
    mvwprintw(w, 3, 1, "Cost %d sk., %d met.", cost, cost_met);
    mvwprintw(w, 4, 1, "Go ahead (y/n)? ");
    box(w, '|', '-');
    wrefresh(w);
  }
  else {
    sprintf (s, "Cost = %d sk., %d met. Build? ",
	     cost, cost_met);
    statline2 (s, "con_bubble");
  }
  if ((c = getch()) != 'y' && c != 'Y') {
    statline2 ("", "");
    return -1;
  }
  if (w == NULL) {
    statline2 ("", "");
  }

    /* now see if we can afford the construction */
  if (cost > user.np->money) {
    beep();
    statline2_err("space to go on", "not enough money to construct");
    return -1;
  }
  if (cost_met > user.np->metal) {
    beep();
    statline2_err("space to go on", "not enough metal to construct");
    return -1;
  }

  /* if we have reached this point, it means we can construct! */

  sp->flags |= SF_BUBBLE;
  np->money -= cost;
  np->metal -= cost_met;
    /* now prepare the exec string */
  sprintf(s, "FLAG_SET_SECTOR:%d:%d:%d\n", sp->loc.x, sp->loc.y, SF_BUBBLE);
  gen_exec(s);
  cmoney(np, -cost);
  cmetal(np, -cost_met);

  return 1;
}

