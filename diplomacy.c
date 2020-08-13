/* diplomacy.c - screen-oriented diplomacy routines */

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
#include <stdio.h>
#include <ctype.h>

extern Sworld world;
extern Suser user;
extern char *dip_status[];

char show_to_screen();

char diplo_report(nation)
     Snation *nation;
{
  Sdiplo **diplo_matrix, **initial_diplo; /* structure containing all info */
  Sdiplo **allocate_diplo();
  FILE   *dfp;           /* file containing all info */
  int i;
  char c;

/*  statline2_err("diplo was not locked", "hit space"); */
    /* read in the current diplomacy matrix, and also the
       initial state (to see if the user wants to change
       their status too much)
     */
  diplo_matrix = allocate_diplo(world.n_nations);
  initial_diplo = allocate_diplo(world.n_nations);
  read_initial_diplo(initial_diplo, world.n_nations);
    /* if reading in the file fails, we must re-create it */
  if (read_in_diplo(diplo_matrix, world.n_nations) == -1) {
    statline2_err("Hit space to go on", "have to re-build diplo file");
    init_diplo(1);
    for (i = 2; i <= world.n_nations; ++i) {
      Sdiplo **dm_old, **dm_new;
      dm_old = allocate_diplo(i-1);
      dm_new = allocate_diplo(i);
      read_in_diplo(dm_old, i-1);
      increase_diplo(dm_old, dm_new, i-1, &world.nations[i-1]);
      dump_diplo(nation, dm_new, i);
      free_diplo(dm_old, i-1);
      free_diplo(dm_new, i);
    }
    update_diplo();		/* update it, since it was reset */
    read_in_diplo(diplo_matrix, world.n_nations);
  }

  c = show_to_screen(diplo_matrix, initial_diplo, nation);
  free_diplo(diplo_matrix, world.n_nations);
  return c;
}

char show_to_screen(dm, initial_dm, nation)
  Sdiplo **dm, **initial_dm;
  Snation *nation;
{
/* this function will dynamically keep the window updated with 
   the diplomacy information.
 */

  WINDOW *dipw;          /* the diplomacy window */
  int i, j, done=0, neigh_id=0, first_shown, n_shown;
  char c;

  statline("hit space when done", "diplomacy report");

  dipw = newwin(LINES-2, COLS, 0, 0);  /* full screen window */
  werase(dipw);
  touchwin(dipw);
  first_shown = 1;
  n_shown = min(world.n_nations, LINES-9);
  while (!done) {
    wmove(dipw, 0, COLS/2-13);
    wstandout(dipw);
    wprintw(dipw, "Diplomacy for nation %s", nation->name);
    wstandend(dipw);

  mvwprintw(dipw,1,3,"                                        TO  YOU        TO YOU  ");
  mvwprintw(dipw,2,3," NATION                   BY YOU         (now)       (at start)");
  mvwprintw(dipw,3,3," ======                   ======         ======      ==========");
    wclrtobot(dipw);

    i=0;
    for (j = 0; j < n_shown &&  j+first_shown < world.n_nations; ++j) {
        /* j+first_shown is for nation ids, i is for lines printed */
      wmove(dipw, 4+j, 3);
      wclrtoeol(dipw);
      ++i;
      if (is_active_ntn(&world.nations[j+first_shown])) {
	mvwprintw(dipw, 4+j, 3, "%2d. %-21s%c%-12s  %c%-12s  %-12s",
		  world.nations[j+first_shown].id,
		  world.nations[j+first_shown].name,
		  dm[nation->id][j+first_shown].status ==
	          initial_dm[nation->id][j+first_shown].status ? ' ' : '*',
		  dip_status[ dm[nation->id][j+first_shown].status ],
		  dm[j+first_shown][nation->id].status ==
		  initial_dm[j+first_shown][nation->id].status ? ' ' : '*',
		  dip_status[dm[j+first_shown][nation->id].status ],
		  dip_status[ initial_dm[j+first_shown][nation->id].status ]);
      } else {
	mvwprintw(dipw, 4+j, 3, "%2d. %-21s   ** DESTROYED **",
		  world.nations[j+first_shown].id,
		  world.nations[j+first_shown].name);
      }
    }
    mvwprintw(dipw, LINES-4, 4,
      "[c]hange diplomacy status, [<]/[,] previous page, [>]/[.] next page");  
    mvwprintw(dipw, LINES-3, 4,
      "    Other reports: [i]nfo, [b]udget, [p]roduction, [n]ations ");
    wrefresh(dipw);    /* update the window, always before input */

    switch (c = getch()) {
    case ' ':
    case 'i':
    case 'b':
    case 'p':
    case 'n':
      done = 1;
      break;
    case '.':
    case '>':
      if (first_shown + n_shown < world.n_nations) {
	first_shown += n_shown;
      }
      break;
    case ',':
    case '<':
      if (first_shown > 1) {
	first_shown -= n_shown;
      }
      break;
    case 'c':
        /* allow to change status */
      mvwprintw(dipw, LINES-3, 3, "TO WHAT NATION: ");
      wclrtoeol(dipw);
      wget_number(dipw, &neigh_id);
      if (!(have_met(dm, nation->id, neigh_id)) && user.id != 0) {
	mvwprintw(dipw, LINES-4, 3, "Have not met that nation yet!!!");
	mvwprintw(dipw, LINES-3, 3, "--- hit space to continue ---");
	wrefresh(dipw);    /* update the window, always before input */
	get_space();
	wrefresh(dipw);    /* update the window, always before input */
      } else if (diplo_is_locked()) {
	statline2_err("Sorry, someone writing diplomacy file.", "hit space");
      } else {
	change_dip_status(dm, initial_dm, nation->id, neigh_id);
	dump_diplo(nation, dm, world.n_nations);
	touchwin(dipw);
	wrefresh(dipw);    /* update the window, always before input */
      }
      break;
    default:
      bad_key();
      break;
    } /* end of switch (c) statement */
  }   /* end of while(done ==0) loop */
  
  delwin(dipw);
/*  touch_all_wins();
  refresh();
 */
  return c;
}

change_dip_status(dm, initial_dm, n1, n2)
     Sdiplo **dm, **initial_dm;
     int n1, n2;
{
  char cc;
  int i,               /* for loops... */
  New_St,              /* input for new status */
  ind1=0, ind2=0;      /* indices of nations in array... */
  WINDOW *chng_dipw;
  
  chng_dipw = newwin((LINES-3), (COLS/2), 3, (COLS/2));
  touchwin(chng_dipw);
  mvwprintw(chng_dipw, 0,3, "NEW STATUS");
  mvwprintw(chng_dipw, 1,3, "==========");
  for (i=JIHAD; i<= TREATY; i++) {
    mvwprintw(chng_dipw, i,3, "[%c]. %s", dip_status[i][0], dip_status[i] );  
  }      
  mvwprintw(chng_dipw, i+1, 3, "Enter new status: ");
  wrefresh(chng_dipw);    /* update the window, always before input */
/*  wget_number(chng_dipw, &New_St); */

  cc = getch();			/* we want the first char of a status */
  if (cc > 'Z') {
    cc -= ('a'-'A');		/* go to upper case */
  }

  for (New_St = JIHAD; New_St <= TREATY; New_St++) {
    if (dip_status[New_St][0] == cc)
      break;
  }
  
/*  New_St++;   /* to compensate for numbers shown on screen */
  if ((New_St < JIHAD) || (New_St > TREATY)) {
    mvwprintw(chng_dipw, i+2, 3, "Illegal Input!");
    mvwprintw(chng_dipw, i+3, 3,"--- hit space to continue ---");
    wrefresh(chng_dipw);    /* update the window, always before input */
    get_space();
  } else if (user.id != 0 && abs(New_St - initial_dm[n1][n2].status) > 2) {
    mvwprintw(chng_dipw, i+2, 3, "Can't change so much!!");
    mvwprintw(chng_dipw, i+3, 3,"--- hit space to continue ---");
    wrefresh(chng_dipw);    /* update the window, always before input */
    get_space();
  } else {
    /* make the change in status!!! */
    while(n1 != dm[ind1++][0].self_id)
      ;
    ind1--;
    while(n2 != dm[ind1][ind2++].neighbor_id)
      ;
    ind2--;
    dm[ind1][ind2].status = New_St;

    mvwprintw(chng_dipw, i+2, 3, "The change has been made.");
    mvwprintw(chng_dipw, i+3, 3,"--- hit space to continue ---");
    wrefresh(chng_dipw);    /* update the window, always before input */
    get_space();
  }

  wrefresh(chng_dipw);     /* update the window, always before input */
  
  delwin(chng_dipw);
/*  touch_all_wins();
  refresh();
 */
}

