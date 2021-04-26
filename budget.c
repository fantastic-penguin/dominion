/* budget.c -- modify budget, etc. */

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
#include <string.h>

extern Sworld world;
extern Suser user;
extern WINDOW *sectw;
extern char *libdir;
extern char help_tag[];
#ifdef BSD
 extern char current_dir[];
#else /* BSD */
 extern char *current_dir;
#endif

#define ALL_CUR_PER (np->cur_tech_r_d + np->cur_mag_r_d + np->cur_spy_r_d)
#define ALL_NEXT_PER (np->mag_r_d + np->spy_r_d + np->charity + np->tech_r_d)

/* char budget_report (Snation *); */

/* Internal functions, should NOT be called outside of this file */

/* do_tech_budget (WINDOW *, Snation *); */
/* do_magic_budget (WINDOW *, Snation *); */
/* void storage_budget (Snation *); */

/* a nation's budget report */

char budget_report(np)
     Snation *np;
{
  WINDOW *w;
  char s[EXECLEN];
  char c, spych;
  int new_value, done = 0, percent_expend, ret, got=0, bad_number;
  int semidone = 0, tmp;
  FILE *diplock;

  strcpy(help_tag, "Budget Report");
  strcpy(s, "");

  w = newwin(LINES-2, COLS, 0, 0); /* full screen */

  touchwin(w);
  while (!done) {
    semidone = 0;
    bad_number = 0;
    statline("type space when done, or F to dump to a file", "budget_report");
    statline2("","");

    draw_budget_screen (w, np);

    got = 0;
    do {
      c = getch();
    } while (strchr(" stTMSFipndL?", c) == NULL && c != CTL('L'));
    switch (c) {
    case ' ':
    case 'i':
    case 'p':
    case 'n':
    case 'd':
      done = 1;
      break;
    case 's':
      storage_budget (np);
      draw_budget_screen (w, np);
      semidone = 1;
      break;
    case 'T':
      do_tech_budget (w, np);
      draw_budget_screen (w, np);
      semidone = 1;
      break;
    case 'M':
      do_magic_budget (w, np);
      draw_budget_screen (w, np);
      semidone = 1;
      break;
    case CTL('L'):
      wclear (w);
      draw_budget_screen (w, np);
      semidone = 1;
      break;
    case 'F':
      /* should also give thon in default file name */
      dump_current_screen(w, "budget_report");
      break;
    case '?':
      online_info();
      break;
    default:
      break;
    }
    
    if (!done && !semidone) {
      /* here prepare for getting the new value */
      wmove(w, LINES-3, 0);
      wclrtoeol(w);
      mvwprintw(w, LINES-3, 0, "      give new value: ");

      ret = wget_number(w, &new_value);
      if (ret > 0) {	/* only do this if the user actually gave a number */
	if (new_value < 0 || new_value > 100) {
	  statline2_err ("Bad value for a percentage (hit space)", "");
	} else {
	  strcpy(s, "");

	  switch (c) {
	  case 't':
	    np->taxes = new_value;
	    sprintf(s, "TAXRATE:%d\n", np->taxes);
	    break;
/* Charity doesn't work right now... */
/*	  case 'c':
	    np->charity = n;
	    sprintf(s, "CHARITY:%d\n", n);
	    break;
*/
	  case 'S':
	    if (next_thon_money (np) >= 0) {
	      np->spy_r_d = new_value;
	      sprintf (s, "SPYR&Dmoney:%d\n", new_value);
	    }
	    else bad_number = 1;
	    break;
	  default:
	    bad_number = 1;
	    break;
	  }
	  if (bad_number) {
	    statline2_err ("hit space", "bad number");
	  }	    
	  else gen_exec(s);
	}
      }
    }
  }
  delwin(w);

  return c;
}

/* Does the sub-budget for magic */
void do_magic_budget (w, np)
WINDOW * w;
Snation * np;
{
  char ch;
  int new_value;
  char exec [EXECLEN];
  int bad_number = 0;
  int old_value_tmp;

  wmove (w, LINES-3, 0);
  wclrtoeol (w);
  mvwprintw (w, LINES-3, 0, "Change %% of [M]oney or [j]ewels invested?");
  wrefresh (w);
  ch = getch ();

  if (strchr ("Mj", ch) == NULL) { ; }
  else {
    mvwprintw (w, LINES-3, 0, " enter new value (as %%): ");
    wclrtoeol (w);
    wrefresh (w);
    
    if (wget_number (w, &new_value) <= 0) {
      bad_number = 1;
    }
     
    if (new_value > 100 || new_value < 0) {
      bad_number = 1;
    }

    /* What I do next is simple - calc_expend uses np->mag_r_d(_jewels)
       to calculate jewels, so I save the old value of np->mag_r_d
       and then copy the new value in, and if the new value is too
       large, then copy the old value back in. This way, you can change
       calc_expend however you want to, and it will work */    

    switch (ch) {
    case 'j':
      old_value_tmp = np->mag_r_d_jewels; /* Do the old swaperoo */
      np->mag_r_d_jewels = new_value;

      if (next_thon_jewels(np) >= 0 && !bad_number) {
	sprintf (exec, "MAGR&Djewels:%d\n", np->mag_r_d_jewels);
	gen_exec (exec);
      }
      else { bad_number = 1; np->mag_r_d_jewels = old_value_tmp; }
      break;
    case 'M':
      old_value_tmp = np->mag_r_d; /* Do the old swaperoo! */
      np->mag_r_d = new_value;

      if (next_thon_money(np) >= 0 && !bad_number &&
	  ALL_NEXT_PER <= 100) {	  
	sprintf (exec, "MAGR&Dmoney:%d\n", np->mag_r_d);
	gen_exec (exec);
	break;
      }
      else { bad_number = 1; np->mag_r_d = old_value_tmp; }
      break;
    default:
      break;
    }
    if (bad_number) {  /* Print an error message if it was a bad number */
      statline2_err ("hit space", "bad number");
    }
  }
}

void do_tech_budget (w, np)
WINDOW * w;
Snation * np;
{
  char ch;
  int new_value;
  char exec [EXECLEN];
  int bad_number = 0;
  int old_value_tmp;

  wmove (w, LINES-3, 0);
  wclrtoeol (w);
  mvwprintw (w, LINES-3, 0, "Change %% of [M]oney or [m]etal invested?");
  wrefresh (w);
  ch = getch ();

  if (strchr ("Mm", ch) == NULL) { ; }
  else {
    mvwprintw (w, LINES-3, 0, " enter new value (as %%): ");
    wclrtoeol (w);
    wrefresh (w);

    if (wget_number (w, &new_value) <= 0) {
      bad_number = 1;
    }
    
    if (new_value > 100 || new_value < 0) {
      bad_number = 1;
    }

    /* What I do next is simple - calc_expend uses np->mag_r_d(_jewels)
       to calculate jewels, so I save the old value of np->mag_r_d
       and then copy the new value in, and if the new value is too
       large, then copy the old value back in. This way, you can change
       calc_expend however you want to, and it will work */    

    switch (ch) {
    case 'm':
      old_value_tmp = np->tech_r_d_metal; /* Do the old swaperoo */
      np->tech_r_d_metal = new_value;

      if (next_thon_metal(np) >= 0 && !bad_number) {
	sprintf (exec, "TECHR&Dmetal:%d\n", np->tech_r_d_metal);
	gen_exec (exec);
      }
      else { bad_number = 1; np->tech_r_d_metal = old_value_tmp; }
      break;
    case 'M':
      old_value_tmp = np->tech_r_d; /* Do the old swaperoo! */
      np->tech_r_d = new_value;

      if (next_thon_money(np) >= 0 && !bad_number &&
	  ALL_NEXT_PER <= 100) {
	sprintf (exec, "TECHR&Dmoney:%d\n", np->tech_r_d);
	gen_exec (exec);
	break;
      }
      else { bad_number = 1; np->tech_r_d = old_value_tmp; }
      break;
    default:
      break;
    }
    if (bad_number) {  /* Print an error message if it was a bad number */
      statline2_err ("hit space", "bad number");
    }
  }
}

/* */
void draw_budget_screen (w, np)
WINDOW * w;
Snation * np;
{
  char s [EXECLEN];

  sprintf(s, "Budget Report for Nation %s", np->name);

  wmove(w, 0, (COLS-strlen(s))/2); /* make the string centered */
  wstandout(w);
  waddstr (w, s);
  wclrtoeol(w);
  wstandend(w);

      /* now a tedious list of stuff */
  wclrtobot(w);
  mvwprintw(w, 2, 0, "  Item");
  mvwprintw(w, 3, 0, "--------");
  mvwprintw(w, 2, COLS/5-2, "This Thon (%d)", world.turn);
  mvwprintw(w, 3, COLS/5-2, "---------", world.turn);
  mvwprintw(w, 2, (2*COLS)/5-3, "Revenue");
  mvwprintw(w, 3, (2*COLS)/5-3, "-------");
  mvwprintw(w, 2, (3*COLS)/5, "Expenditure");
  mvwprintw(w, 3, (3*COLS)/5, "-----------");
  mvwprintw(w, 2, (4*COLS)/5, "Next Thon (%d)", world.turn + 1);
  mvwprintw(w, 3, (4*COLS)/5, "---------", world.turn + 1);
  mvwprintw(w, 4, 0, "Treasury");
  mvwprintw(w, 5, 0, "Metal");
  mvwprintw(w, 6, 0, "Jewels");
  mvwprintw(w, 7, 0, "Food");
  /* this turn */
  mvwprintw(w, 4, COLS/5-2, "%d", user.init_money);
  mvwprintw(w, 5, COLS/5-2, "%d", user.init_metal);
  mvwprintw(w, 6, COLS/5-2, "%d", user.init_jewels);
  mvwprintw(w, 7, COLS/5-2, "%d", user.init_food);

  /* revenue */
  mvwprintw(w, 4, (2*COLS)/5-3, "%d (%d%% tax)        ",
	    calc_revenue(np), np->taxes);
  mvwprintw(w, 5, (2*COLS)/5-3, "%d   ", calc_metal(np));
  mvwprintw(w, 6, (2*COLS)/5-3, "%d   ", calc_jewels(np));
  mvwprintw(w, 7, (2*COLS)/5-3, "%d   ", calc_food(np));

  /* expenses */
  mvwprintw(w, 4, (3*COLS)/5, "%d    ",
	    calc_expend(np) + user.init_money - np->money +
	    (cur_expend(np)));
  mvwprintw(w, 5, (3*COLS)/5, "%d    ",
	    calc_expend_metal(np) + user.init_metal - np->metal
	    + cur_expend_metal(np));
  mvwprintw(w, 6, (3*COLS)/5, "%d    ",
	    calc_expend_jewels(np) + user.init_jewels - np->jewels
	    + cur_expend_jewels(np));
  mvwprintw(w, 7, (3*COLS)/5, "%d    ",
	    calc_expend_food(np) + user.init_food - np->food);

  /* next turn */
  mvwprintw(w, 4, (4*COLS)/5, "%d    ",
	    next_thon_money(np));
  mvwprintw(w, 5, (4*COLS)/5, "%d    ",
	    next_thon_metal(np));
  mvwprintw(w, 6, (4*COLS)/5, "%d    ",
	    next_thon_jewels(np));
  mvwprintw(w, 7, (4*COLS)/5, "%d    ",
	    np->food + calc_food(np) - calc_expend_food(np));


  /* now for the modifiable stuff */
  strcpy(s, "**Breakdown of Expenditures**");
  mvwaddstr(w, 9, (COLS-strlen(s))/2, s);
  
  mvwprintw(w, 11, 0, "MONEY:");
  mvwprintw(w, 11, 8, "Charity:     %3d%% -> %d      ",
	    np->charity, (np->charity*calc_revenue(np))/100);
  mvwprintw(w, 12, 8, "Tech R&D:    %3d%% -> %d      ",
	    np->tech_r_d, ((np->tech_r_d*calc_revenue(np)) / 100));
  mvwprintw(w, 13, 8, "Magic R&D:   %3d%% -> %d      ",
	    np->mag_r_d, np->mag_r_d*calc_revenue(np)/100);
  mvwprintw(w, 14, 8, "Spy R&D:     %3d%% -> %d      ",
	    np->spy_r_d, (np->spy_r_d * calc_revenue (np)) / 100);
  mvwprintw(w, 15, 8, "Storage use: %3d%% -> %d",
	    (np->cur_tech_r_d + np->cur_mag_r_d + np->cur_spy_r_d),
	    (np->cur_tech_r_d + np->cur_mag_r_d + np->cur_spy_r_d) *
	    np->money / 100);
  mvwprintw(w, 16, 8, "Military maint.:     %d      ", military_maint(np));
  mvwprintw(w, 17, 8, "Other:               %d      ",
	    user.init_money + non_profit_maint(np) - np->money);

  mvwprintw(w, 11, COLS/2, "METAL:");
  mvwprintw(w, 11, COLS/2+8, "Tech R&D:    %3d%% -> %d    ",
	    np->tech_r_d_metal, ((np->tech_r_d_metal*calc_metal(np))/100));
  mvwprintw(w, 12, COLS/2+8, "Storage use: %3d%% -> %d",
	    np->cur_tech_r_d_metal,
	    (np->cur_tech_r_d_metal * np->metal) / 100);	    
  mvwprintw(w, 13, COLS/2+8, "Other:               %d    ",
	    user.init_metal - np->metal + military_maint_metal(np));
  
  mvwprintw(w, 15, COLS/2, "JEWELS:");
  mvwprintw(w, 15, COLS/2+8, "Magic R&D:   %3d%% -> %d    ",
	    np->mag_r_d_jewels, ((np->mag_r_d_jewels*calc_jewels(np))/100));
  mvwprintw(w, 16, COLS/2+8, "Storage use: %3d%% -> %d",
	    np->cur_mag_r_d_jewels,
	    (np->cur_mag_r_d_jewels * np->jewels) / 100);
  mvwprintw(w, 17, COLS/2+8, "Other:               %d    ",
	    user.init_jewels - np->jewels + military_maint_jewels(np));
  
  /* now the part where the user gets to decide HOW TO SPEND the money! */
  mvwaddstr(w, LINES-5, 0, "Choose which parameter to modify:");
  mvwaddstr(w, LINES-4, 8,
	    "[t]ax rate, [T]ech R&D, [M]agic R&D, [S]py R&D, [s]torage");
  mvwaddstr(w, LINES-3, 0,
	    "See report: [i]nfo, [p]roduction, [n]ations, [d]iplomacy");
  wclrtobot(w);
  wrefresh(w);

}

/* Storage report, budget */
void storage_budget (np)
Snation * np;
{
  WINDOW * w;
  char s [EXECLEN];
  char ch;
  int bad_number, new_value;

  w = newwin (11, 70, 8, 5);

  while (1) {
    bad_number = 0;
    draw_storage_budget (w, np);

    do { ch = getch (); } while (strchr (" TMS", ch) == NULL);

    if (ch == ' ') return;

    switch (ch) {
    case 'S':
      wmove (w, 9, 1);
      wclrtoeol (w);
      box (w, '|', '-');
      mvwprintw (w, 9, 2, "enter new value (percent): ");
      wrefresh (w);
      if (wget_number (w, &new_value) <= 0) {
	bad_number = 1;
      }

      if (new_value < 0 || new_value > 100) bad_number = 1;

      if (!bad_number && next_thon_money(np) >= 0 &&
       ((np->cur_tech_r_d + np->cur_mag_r_d + new_value) <= 100)) {
         
	sprintf (s, "CUR_SPYR&Dmoney:%d\n", new_value);
	np->cur_spy_r_d = new_value;
	gen_exec (s);
      }
      else bad_number = 1;
      break;
    case 'T':
      wmove (w, 9, 1);
      wclrtoeol (w);
      box (w, '|', '-');
      mvwprintw (w, 9, 3, "Change [M]oney or [m]etal? ");
      wrefresh (w);

      do { ch = getch (); } while (strchr (" Mm", ch) == NULL);

      if (ch != 'M' && ch != 'm') { break; }
      wmove (w, 9, 1);
      wclrtoeol (w);
      box (w, '|', '-');
      mvwprintw (w, 9, 3, "enter new value (percentage): ");
      wrefresh (w);
      if (wget_number (w, &new_value) <= 0) {
	bad_number = 1;
      }
      wmove (w, 0, 0);
      //TODO: Next line was original; how did this work? Above line is guess...
      //wmove (w);
      wclrtoeol (w);
      box (w, '|', '-');

      if (new_value < 0 || new_value > 100) bad_number = 1;

      switch (ch) {
      case 'M':
	if (next_thon_money(np) >= 0 && !bad_number &&
          ((new_value + np->cur_mag_r_d + np->cur_spy_r_d) <= 100)) {

	  np->cur_tech_r_d = new_value;
	  sprintf (s, "CUR_TECHR&Dmoney:%d\n", new_value);
	  gen_exec (s);
	}
	else bad_number = 1;
	break;
      case 'm':
	if (!bad_number && next_thon_metal(np) >= 0) {
	  np->cur_tech_r_d_metal = new_value;
	  sprintf (s, "CUR_TECHR&Dmetal:%d\n", new_value);
	  gen_exec (s);
	  break;
	}
	else bad_number = 1;
	break;
      default:
	break;
      }
      break;
    case 'M':
      wmove (w, 9, 1);
      wclrtoeol (w);
      box (w, '|', '-');
      mvwprintw (w, 9, 3, "Change [M]oney or [j]ewels? ");
      wrefresh (w);

      do { ch = getch (); } while (strchr (" Mj", ch) == NULL);

      if (ch != 'M' && ch != 'j') { break; }
      wmove (w, 9, 1);
      wclrtoeol (w);
      box (w, '|', '-');
      mvwprintw (w, 9, 3, "enter new value (percentage): ");
      wrefresh (w);
      if (wget_number (w, &new_value) <= 0) {
	bad_number = 1;
      }
      wmove (w, 0, 0);
      //TODO: Next line was original; how did this work? Above line is guess...
      //wmove (w);
      wclrtoeol (w);
      box (w, '|', '-');

      if (new_value < 0 || new_value > 100) bad_number = 1;

      switch (ch) {
      case 'M':
	if (next_thon_money(np) && !bad_number &&
    ((np->cur_tech_r_d + new_value + np->cur_spy_r_d) <= 100)) { 

	  np->cur_mag_r_d = new_value;
	  sprintf (s, "CUR_MAGR&Dmoney:%d\n", new_value);
	  gen_exec (s);
	}
	else bad_number = 1;
	break;
      case 'j':
	if (!bad_number && next_thon_jewels(np) >= 0) {
	  np->cur_mag_r_d_jewels = new_value;
	  sprintf (s, "CUR_MAGR&Djewels:%d\n", new_value);
	  gen_exec (s);
	  break;
	}
	else bad_number = 1;
	break;
      default:
	break;
      }
      break;
    default:
      break;
    }
    if (bad_number) {
      statline2_err ("hit space", "bad number");
    }
    werase (w);
  }
}

void draw_storage_budget (w, np)

WINDOW * w;
Snation * np;
{
  char s [EXECLEN];

  box (w, '|', '-');

  sprintf (s, "Storage Usage Report");
  mvwprintw (w, 1, 35 - (strlen (s)) / 2, "%s", s);
  mvwprintw (w, 2, 35 - (strlen (s)) / 2, "--------------------");

  mvwprintw (w, 4, 2, "MONEY:");
  mvwprintw (w, 5, 7, "Tech R&D:   %3d%% -> %d",
	     np->cur_tech_r_d, (np->cur_tech_r_d * np->money / 100));
  mvwprintw (w, 6, 7, "Magic R&D:  %3d%% -> %d",
	     np->cur_mag_r_d, (np->cur_mag_r_d * np->money / 100));
  mvwprintw (w, 7, 7, "Spy R&D:    %3d%% -> %d",
	     np->cur_spy_r_d, (np->cur_spy_r_d * np->money / 100));

  mvwprintw (w, 4, 36, "METAL:");
  mvwprintw (w, 5, 41, "Tech R&D:   %3d%% -> %d",
	     np->cur_tech_r_d_metal, (np->cur_tech_r_d_metal * np->metal
				       / 100));
  mvwprintw (w, 6, 36, "JEWELS:");
  mvwprintw (w, 7, 41, "Magic R&D:  %3d%% -> %d",
	     np->cur_mag_r_d_jewels, (np->cur_mag_r_d_jewels * np->jewels
				       / 100));

  mvwprintw (w, 9, 12, "Modify: [M]agic R&D, [T]ech R&D, [S]py R&D");

  wrefresh (w);
}
