 /* reports.c -- all things from the 'r' menu */

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
extern WINDOW *sectw;
extern char *libdir;
extern char help_tag[];
#ifdef BSD
 extern char current_dir[];
#else /* BSD */
 extern char *current_dir;
#endif
extern int ruid, euid;

  /* each report returns a char */
char info_report(), production_report(), nations_report();


extern char diplo_report (), budget_report ();

report_menu()		/* this gives a menu of possible report forms */
{
  WINDOW *repw;
  char c, exit_c = ' ';
  FILE *fp, *fopen(), *diplock;
  int done = 0;
  char filename[80];

  strcpy(help_tag, "Reports");

  if (user.xmode) {
    repw = NULL;
  } else {
    repw = newwin(5, 60, LINES-7, 10);
  }
  while (!done) {
    if (exit_c == ' ') {
      touch_all_wins();

      if (user.xmode) {
	statline_prompt("Report: (i,b,p,n,d,v)", "report_menu");
      } else {
	statline("choose the form you want (space to exit)", "report_menu");
	wmove(repw, 1, 1);
	waddstr(repw, "your nation:       [i]nfo [b]udget [p]roduction");
	wmove(repw, 2, 1);
	waddstr(repw, "the whole world:   ");
	waddstr(repw, "[n]ations [d]iplomacy");
	wmove(repw, 3, 1);
	waddstr(repw,
		"  or type [v] to view a report that was saved to a file");
	wclrtobot(repw);
	box(repw, '|', '-');
	wrefresh(repw);
      }
 
    }

    if (exit_c != ' ') 
      c = exit_c;
    else
      do { c = getch (); } while (strchr (" ibpndv", c) == NULL);

    switch (c) {
    case ' ':
      done = 1;
      break;
    case 'v':
#ifdef NO_FILE_ACCESS
      statline2_err("Hit space", "File access is disabled");
#else
      if (user.xmode) {
	statline_prompt("file you want to view: ", "reports_menu");
      } else {
	wmove (repw, 3, 4);
	wclrtoeol (repw);
	mvwaddstr(repw, 3, 4, "give file name you want to view: ");
	box (repw, '|', '-');
	wrefresh(repw);
      }
      if (wget_name(repw, filename) > 0) {
	/*   erase();	  /* now erase the screen so we can view the report */
	werase(sectw);
	if (!user.xmode) {
	  werase(repw);
	}
	/*      wrefresh(repw); */
	/*      scr_restore(filename); */

	if (my_scr_restore(filename) != -1) {
	/*      doupdate(); */
	}
	if (!user.xmode) {
	  wclrtobot(repw);
	}
      }
#endif NO_FILE_ACCESS
      break;
    case 'i':
      exit_c = info_report(user.np);
      break;
    case 'b':
      exit_c = budget_report(user.np);
      break;
    case 'p':
      exit_c = production_report(user.np);
      break;
    case 'n':
      exit_c = nations_report();
      break;
    case 'd':
      exit_c = diplo_report(user.np);
      break;
    case '?':
      online_info();
      break;
    }
    if (!user.xmode) {
      statline("choose the form you want (space to exit)", "report_menu");
      touchwin(repw);
    }
  }
  if (!user.xmode) {
    delwin(repw);
  }
  touchwin(stdscr);
  user.just_moved = 1;
}

  /* info about a specific nation */
char info_report(np)
     Snation *np;
{
  WINDOW *w;
  FILE *diplock;
  char s[80];
  char c;
  int done = 0;

  strcpy(help_tag, "Information Report");
  w = newwin(LINES-2, COLS, 0, 0); /* full screen */

  touchwin(w);
    /* now a tedious list of stuff */
  while (!done) {
    draw_info_screen(w, np);

    statline("type space when done, or F to dump to a file", "info_report");

    switch (c = getch()) {
    case CTL('L'):
      wclear (w);
      draw_info_screen (w, np);
      break;
    case 'P':
      change_passwd(np, w);
      break;
    case 'l':
      change_leader(np, w);
      break;
    case 't':			/* rotate values for the npc flag */
      np->npc_flag = (np->npc_flag + 1) % 3;
      if (np->npc_flag) {
	sprintf(s, "SET_NPC:%d\n", np->id);
      } else {
	sprintf(s, "CLEAR_NPC:%d\n", np->id);
      }
      gen_exec(s);
      break;
    case 'a':			/* set the aggressiveness paramter */
      if (np->npc_flag) {
	set_aggressiveness(np, w);
      }
      break;
    case 'F':
      dump_current_screen(w, "info_report");
      break;
    case ' ':
    case 'b':			/* go to other reports */
    case 'p':
    case 'n':
    case 'd':
      done = 1;
      break;
    case '?':
      online_info();
      break;
    default:
      break;
    }
    wrefresh(w);
    statline2("", "");
  }
  delwin(w);
/*  touch_all_wins(); */
/*  refresh(); */
  return c;
}

/* */
draw_info_screen (w, np)

WINDOW * w;
Snation * np;
{
  char s [EXECLEN];

  sprintf(s, "Info Report for Nation %s", np->name);
  wmove(w, 0, (COLS-strlen(s))/2 - 4); /* make the string centered */
  wstandout(w);
  waddstr(w, s);
  wstandend(w);
  mvwprintw(w, 1, COLS/3+7, "Thon %d", world.turn);
  mvwprintw(w, 3, 0, "Nation:  %s", np->name);
  switch(np->npc_flag) {
  case NPC_NOMAIL:
    mvwaddstr(w, 2, 8, " [npc]     ");
    break;
  case NPC_MAIL:
    mvwaddstr(w, 2, 8, " [npc+mail]");
    break;
  case NOT_NPC:
  default:
    mvwaddstr(w, 2, 8, "           ");
    break;
  }
/*
  if (!np->npc_flag) {
    waddstr(w, "           ");
  } else if (np->npc_flag == 1) {
/*    waddstr(w, " [npc: aggress. %d]", np->npc_agg); */
/*    waddstr(w, " [npc]");
  } else {			/* for npc_flag == 2 */
/*    waddstr(w, " [npc+mail]");
  }
*/
  mvwprintw(w, 4, 0, "Id:      %d", np->id);
  mvwprintw(w, 5, 0, "Leader:  %s    ", np->leader);
  mvwprintw(w, 6, 0, "Capital: (%d,%d)",
	    xrel(np->capital.x,np->capital.y,user.np->capital),
	    yrel(np->capital.x,np->capital.y,user.np->capital));
  mvwprintw(w, 7, 0, "Race:    %s", np->race.name);
  mvwprintw(w, 8, 0, "Mark:    %c", np->mark);
  mvwprintw(w, 3, COLS/3, "Sectors:  %d", np->n_sects);
  mvwprintw(w, 4, COLS/3, "Treasury: %d", np->money);
  mvwprintw(w, 5, COLS/3, "Jewels:   %d", np->jewels);
  mvwprintw(w, 6, COLS/3, "Metal:    %d", np->metal);
  mvwprintw(w, 7, COLS/3, "Food:     %d", np->food);
  mvwprintw(w,  3, (2*COLS)/3, "Birth rate:  %d%%", np->race.repro);
  mvwprintw(w,  4, (2*COLS)/3, "Mortality:   %d%%", np->race.mortality);
  mvwprintw(w,  5, (2*COLS)/3, "Strength:    %d", np->race.strength);
  mvwprintw(w,  6, (2*COLS)/3, "Intel: %d + %d = %d", np->race.intel,
	    univ_intel(np), np->race.intel + univ_intel(np));
  mvwprintw(w,  7, (2*COLS)/3, "Magic Apt: %d + %d = %d", np->race.mag_apt,
	    priestliness(np), np->race.mag_apt + priestliness(np));
  mvwprintw(w,  8, (2*COLS)/3, "Speed:       %d", np->race.speed);
  mvwprintw(w,  9, (2*COLS)/3, "Stealth:     %d", np->race.stealth);

  mvwprintw(w, 11, (2*COLS)/3, "Civilians:     %d", get_n_civil(np));
  mvwprintw(w, 12, (2*COLS)/3, "Soldiers:      %d", get_n_soldiers(np));
  mvwprintw(w, 13, (2*COLS)/3, "Armies:        %d", np->n_armies);
  mvwprintw(w, 14, (2*COLS)/3, "Attack bonus:  %d%%", np->attack);
  mvwprintw(w, 15, (2*COLS)/3, "Defense bonus: %d%%", np->defense);
  mvwprintw(w, 16, (2*COLS)/3, "Move points:   %d", basic_move_rate(np));

  mvwprintw(w, 10, 0, "Initiated to the magical order %s", np->mag_order);
  mvwprintw(w, 11, 0, "Magic skill:       %d", np->mag_skill);
  mvwprintw(w, 12, 0, "Spell points:      %d (%d in maint)", np->spell_pts,
	    military_maint_spell_pts(np));
  mvwprintw(w, 13, 0, "Technology skill:  %d", np->tech_skill);
  mvwprintw(w, 14, 0, "Farming skill:     %d", np->farm_skill);
  mvwprintw(w, 15, 0, "Mining skill:      %d", np->mine_skill);
  mvwprintw(w, 16, 0, "Spy skill:         %d", np->spy);
  mvwprintw(w, 17, 0, "Secrecy:           %d", np->secrecy);

  mvwprintw(w, LINES-5, 4,
	    "Options: [P]assd change, [l]eader change, [t]oggle npc");
  if (np->npc_flag) {
    waddstr(w, ", [a]ggr.");
  } else {
    wclrtoeol(w);
  }
  mvwprintw(w, LINES-4, 4,
	    "Reports: [b]udget, [p]roduction, [n]ations, [d]iplomacy");
  wclrtobot(w);
  wrefresh(w);
}

  /* a nation's production report:  this gives detail on where
     all the income comes from (mines, cities...) and so on
   */
char production_report(np)
     Snation *np;
{
  WINDOW *w;
  FILE *diplock;
  char s[EXECLEN];
  char c;
  int n, done = 0, percent_expend, ret;

  strcpy(help_tag, "Production Report");

  w = newwin(LINES-2, COLS, 0, 0); /* full screen */
  statline2 ("", "");
  touchwin(w);

  while (!done) {
    draw_production_screen (w, np);

    statline("type space when done, or F to dump to a file",
	     "production_report");

    switch (c = getch()) {
    case CTL('L'):
      wclear (w);
      draw_production_screen (w, np);
      break;
    case ' ':
      done = 1;
      break;
    case 'F':
      dump_current_screen(w, "production_report");
      break;
    case 'b':
    case 'i':
    case 'n':
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
/*  touch_all_wins(); */
/*  refresh(); */
  return c;
}

draw_production_screen (w, np)

WINDOW * w;
Snation * np;
{
  char s [EXECLEN];

  int emp, emp_met, emp_jwl, emp_farm, emp_serv, n_civil;

  emp = get_employed(np);
  emp_met = get_emp_met(np);
  emp_jwl = get_emp_jwl(np);
  emp_farm = get_emp_farm(np);
  emp_serv = get_emp_serv(np);
  n_civil = get_n_civil(np);
  
  sprintf(s, "Production Report for Nation %s", np->name);
  wmove(w, 0, (COLS-strlen(s))/2 - 4); /* make the string centered */
  wstandout(w);
  waddstr(w, s);
  wstandend(w);
  wclrtoeol(w);
  /* now a tedious list of stuff */
  mvwprintw(w, 2, 0, "  Job");
  mvwprintw(w, 3, 0, "--------");
  mvwprintw(w, 2, COLS/4-5, "# employed (unemp.)");
  mvwprintw(w, 3, COLS/4-5, "-------------------", world.turn);
  mvwprintw(w, 2, (2*COLS)/4-3, "Revenue");
  mvwprintw(w, 3, (2*COLS)/4-3, "-------");
  mvwprintw(w, 2, (3*COLS)/4-5, "prod/person");
  mvwprintw(w, 3, (3*COLS)/4-5, "-----------");
  mvwprintw(w, 4, 0, "General");
  mvwprintw(w, 5, 0, "Metal miners");
  mvwprintw(w, 6, 0, "Jewel miners");
  mvwprintw(w, 7, 0, "Farmers");
  mvwprintw(w, 8, 0, "Services");
  /* number of people */
  mvwprintw(w, 4, COLS/4-2, "%6d (%d)", emp, get_unemployed(np));
  mvwprintw(w, 5, COLS/4-2, "%6d (%d)", emp_met, get_unemp_met(np));
  mvwprintw(w, 6, COLS/4-2, "%6d (%d)", emp_jwl, get_unemp_jwl(np));
  mvwprintw(w, 7, COLS/4-2, "%6d (%d)", emp_farm, get_unemp_farm(np));
  mvwprintw(w, 8, COLS/4-2, "%6d (%d)", emp_serv, get_unemp_serv(np));
  
  /* revenue */
  mvwprintw(w, 4, (2*COLS)/4-3, "%7d (%d%% tax)        ",
	    calc_revenue(np), np->taxes);
  mvwprintw(w, 5, (2*COLS)/4-3, "%7d   ", calc_metal(np));
  mvwprintw(w, 6, (2*COLS)/4-3, "%7d   ", calc_jewels(np));
  mvwprintw(w, 7, (2*COLS)/4-3, "%7d   ", calc_food(np));
  mvwprintw(w, 8, (2*COLS)/4-3, "%7d   ", calc_serv_revenue(np));
  
  /* revenue/person */
  mvwprintw(w, 4, (3*COLS)/4-3, "%5.2f   sk.",
	    emp ? calc_revenue(np)/(1.0*emp) : 0, np->taxes);
  mvwprintw(w, 5, (3*COLS)/4-3,"%5.2f   met.",
	    emp_met ? calc_metal(np)/(1.0*emp_met) : 0.0);
  mvwprintw(w, 6, (3*COLS)/4-3,"%5.2f   jwl.",
	    emp_jwl ? calc_jewels(np)/(1.0*emp_jwl) : 0.0);
  mvwprintw(w, 7, (3*COLS)/4-3,"%5.2f   food",
	    emp_farm ? calc_food(np)/(1.0*emp_farm) : 0.0);
  mvwprintw(w, 8, (3*COLS)/4-3,"%5.2f   sk.",
	    emp_serv ? calc_serv_revenue(np)/(1.0*emp_serv) : 0.0);
  
  /* other statistics */
  mvwprintw(w, 10, COLS/5, "Total people: %d  ",
	      n_civil + get_n_soldiers(np));
    mvwprintw(w, 11, COLS/5, "Civilians:    %d  ", n_civil);
    mvwprintw(w, 12, COLS/5, "Employed:     %d  ", get_employed(np));
    mvwprintw(w, 13, COLS/5, "Unemployed:   %d  ", get_unemployed(np));
    mvwprintw(w, 14, COLS/5, "Soldiers:     %d  ", get_n_soldiers(np));
    mvwprintw(w, 15, COLS/5, "Unemp. rate:  %d%%  ", n_civil == 0 ? 0 :
	      (100*get_unemployed(np))/n_civil);

    mvwprintw(w, 20, 4, "Reports: [b]udget, [i]info, [n]ations, [d]iplomacy");
    wclrtobot(w);
    wrefresh(w);
}


  /* this prompts for a file name and then dumps
     the current screen to that file
   */
dump_current_screen(w, def_filename)
     WINDOW *w;
     char def_filename[];
{
  char filename[80], s[80];

#ifdef NO_FILE_ACCESS
  statline2_err("Hit space", "File access disabled");
#else  
  sprintf(s, "give file name (default = \"%s%d\")", def_filename, world.turn);
  statline2(s, "dump_current_screen");
  move(LINES-3, 0);
  clrtoeol();
  move(LINES-3, COLS/3);
  addstr("> ");
  echo();
  if (scanw("%18s", filename) < 1) {
    strcpy(filename, def_filename);
    sprintf(filename, "%s%d",  def_filename, world.turn);
  }
  noecho();
  move(LINES-3, COLS/3);
  clrtoeol();
  statline2("", "");
  my_scr_dump(w, filename);
/*  scr_dump(filename); /* the curses scr_dump() isn't good enough */
#endif
}

  /* the curses routine is not very nice, since it
     can only be read-in by curses.  hence this.
   */
my_scr_dump(w, fname)
     WINDOW *w;
     char fname[];
{
  int x, y;
  char c;
  FILE *fp, *fopen();

#ifdef UID_SECURITY
  if (fork() == 0) {		/* child has fork() == 0 */
      /* first change back to the user's current directory */
    setuid(ruid);
    chdir(current_dir);
/*    printf("\r\nnow I am %d, changed to %s\n\r", ruid, current_dir); */
      /* dilemma:  should we overwrite or append? */
    if ((fp = fopen(fname, "w")) == NULL) {
      exit(1);
    }
/*    printf("\r\n writing out the file %s in %s\n\r", fname, current_dir); */
    for (y = 0; y < LINES; ++y) {
      for (x = 0; x < COLS; ++x) {
	wmove(w, y, x);
	c=winch(w);
	fputc(c, fp);
      }
      fputc('\n', fp);
    }
    fclose(fp);
    exit(0);
  }
  wait(0);
#else /* UID_SECURITY */
    /* dilemma:  should we overwrite or append? */
  if ((fp = fopen(fname, "w")) == NULL) {
    return -1;
  }
  for (y = 0; y < LINES; ++y) {
    for (x = 0; x < COLS; ++x) {
      wmove(w, y, x);
      c=winch(w);
      fputc(c, fp);
    }
    fputc('\n', fp);
  }
  fclose(fp);
#endif /* UID_SECURITY */
  return 1;
}

  /* this restores a dump made with our custom screen dump routine */
my_scr_restore(fname)
     char fname[];
{
  int x, y;
  char c;
  FILE *fp, *fopen();
  WINDOW *restw;


#ifdef UID_SECURITY
  if (fork() == 0) {
    setuid(ruid);
    chdir(current_dir);
    if ((fp = fopen(fname, "r")) == NULL) {
      exit(1);
    }
    restw = newwin(LINES, COLS, 0, 0);
    werase(restw);
    touchwin(restw);
    for (y = 0; y < LINES; ++y) {
      for (x = 0; x < COLS; ++x) {
	c = fgetc(fp);
	if ((c == '\n') || (c == '\r')) {
	  --x;
	  continue;		/* wow!!! a use for continue!!! */
	}
	mvwaddch(restw, y, x, c);
      }
      wclrtoeol(restw);
    }
    wclrtobot(restw);
    wrefresh(restw);
    fclose(fp);
    statline2_err("type space to go on", "view_saved_report");
    delwin(restw);
    exit(0);
  }				/* we close the "if (fork() == 0) {" */
  wait(0);
#else /* UID_SECURITY */
  if ((fp = fopen(fname, "r")) == NULL) {
    return -1;
  }
  restw = newwin(COLS, LINES, 0, 0);

  for (y = 0; y < LINES; ++y) {
    for (x = 0; x < COLS; ++x) {
      c = fgetc(fp);
      if ((c == '\n') || (c == '\r')) {
	--x;
	continue;		/* wow!!! a use for continue!!! */
      }
      mvwaddch(restw, y, x, c);
    }
    wclrtoeol(restw);
  }
  wclrtobot(restw);
  fclose(fp);
  delwin(restw);
  statline2_err("type space to go on", "view_saved_report");
#endif /* UID_SECURITY */
  touch_all_wins();
  refresh();
  user.just_moved = 1;
  return 1;
}

  /* let a nation change its password */
change_passwd(np, w)
     Snation *np;
     WINDOW *w;
{
  char old_p[NAMELEN], try1[NAMELEN], try2[NAMELEN];

  wmove(w, w->_maxy-3, 0);
  wclrtobot(w);
  wmove(w, w->_maxy-3, 6);
  wrefresh(w);
  /* if it is NOT the Gamemaster, then we have to make sure
     this user knows the old password.  Gamemaster, instead,
     can change passwds without knowing the old ones.
   */
  if (user.id != 0) {
    get_crypt_pass("Old password: ", old_p, w, NULL);
    wmove(w, w->_maxy-3, 6);
    wrefresh(w);
    if (strcmp(old_p, np->passwd) != 0) {
      statline2_err("Hit space", "sorry");
      wmove(w, w->_maxy-4, 0);
      wclrtobot(w);
      wrefresh(w);
      return -1;
    }
  }
  get_crypt_pass("New password:  ", try1, w, NULL);
  wmove(w, w->_maxy-3, 6);
  wrefresh(w);
  get_crypt_pass("Type it again: ", try2, w, NULL);
  while (strcmp(try1, try2) != 0) {
    mvwprintw(w, 20, 6, "They don't match, try again");
    wmove(w, w->_maxy-3, 6);
    wrefresh(w);
    get_crypt_pass("New password:  ", try1, w, NULL);
    wmove(w, w->_maxy-3, 6);
    wrefresh(w);
    get_crypt_pass("Type it again: ", try2, w, NULL);
  }
  strcpy(np->passwd, try1);
  cpass(np, try1);		/* generate the exec line */
  wmove(w, w->_maxy-4, 0);
  wclrtobot(w);
}

  /* allow a user to change their leader */
change_leader(np, w)
     Snation *np;
     WINDOW *w;
{
  char name[NAMELEN], s[EXECLEN];

  mvwprintw(w, 21, 6, "What is the new name of your leader? ");
  if (wget_name(w, name) > 0) {
    strcpy(np->leader, name);
    sprintf(s, "NATION_LEADER:%d:%s\n", np->id, name);
    gen_exec(s);
  }
}
