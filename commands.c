  /* commands.c -- various commands in dominion */

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
#include "army.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

	/* many commands are kept in this file, except for those
	   that depend on the graphics being used.
	 */

extern Suser user;
extern Sworld world;
extern struct s_desig_map desig_map[];
extern struct s_altitude_map altitude_map[];
extern struct item_map climates[];
extern struct item_map terrains[];
extern WINDOW *sectw;
extern char help_tag[];
extern int (*wrapx)(), (*wrapy)();
extern char *getenv();
extern char libdir[], *current_dir;

void quit()				/* cleanup and quit */
{
  clear();
  cleanup();
  clean_exit();
  exit(0);
}

void help()
{
  start_help_win();
  show_help();
  end_help_win();
}

  /* this routine sets certain parameters that must be
     set when the cursor has been moved
   */
void just_moved()
{
  user.just_moved = 1;
}

void up()				/* move the cursor; should look at topology */
{
  --user.cursor.y;
  just_moved();
}
void jup()
{
  user.cursor.y -= 8;
  just_moved();
}
void down()
{
  ++user.cursor.y;
  just_moved();
}
void jdown()
{
  user.cursor.y += 8;
  just_moved();
}
void right()
{
  ++user.cursor.x;
  just_moved();
}
void jright()
{
  user.cursor.x += 8;
  just_moved();
}
void left()
{
  --user.cursor.x;
  just_moved();
}
void jleft()
{
  user.cursor.x -= 8;
  just_moved();
}
void upright()
{
  up(); right();
  just_moved();
}
void upleft()
{
  up(); left();
  just_moved();
}
void downright()
{
  down(); right();
  just_moved();
}
void downleft()
{
  down(); left();
  just_moved();
}
void jhome()
{
  user.cursor.x = user.np->capital.x;
  user.cursor.y = user.np->capital.y;
  user.center = user.cursor;
  just_moved();
}

void jpos()
{
  char s[100];
  int x,y;

  statline2("  x postion to jump to ? : " , "position");
  move (LINES-2, strlen("  x position to jump to ? : "));
  if ( wget_number(stdscr, &x) < 1) {
    statline2("", "");
    return;
  }
  statline2("  y postion to jump to ? : " , "position");
  move (LINES-2, strlen("  y position to jump to ? : "));
  if ( wget_number(stdscr, &y) < 1) {
    statline2("", "");
    return;
  }
  user.cursor.x = (*wrapx)(user.np->capital.x + x,user.np->capital.y + y);
  user.cursor.y = (*wrapy)(user.np->capital.x + x,user.np->capital.y + y);
  user.center = user.cursor;
  just_moved();
  statline2("", "");
}

#define ZX 70
#define ZY 18

  /* zoom-in on a sector */
void zoom_sector()
{
  WINDOW *zoomw = NULL;
  char s[EXECLEN];
  char c;
  int x = user.cursor.x, y = user.cursor.y;
  Ssector *sp = &world.map[x][y];
  Snation *np;
  int done = 0, old_desig = sp->designation;
  int new_desig = sp->designation, quantity, fraction, total;
  int visibility = user.visible_sectors[x][y];

  strcpy(help_tag, "Designations");
  if (user.id != 0) {
    np = user.np;
  } else {
    np = &world.nations[sp->owner]; /* for super user */
  }
  if (!user.xmode) {
    zoomw = newwin(ZY, ZX, 4, 5);
  }
  while (!done) {
    if (zoomw) {
      statline("type space when done", "zoom_sector");

      if (strlen(sp->name) > 0 && (visibility & SEE_POPULATION)) {
	sprintf(s, "Detailed Evaluation of %s", sp->name);
      } else {
	sprintf(s, "Detailed Evaluation of Sector (%d,%d)",
		xrel(x, y, np->capital), yrel(x, y, np->capital));
      }
      wmove(zoomw, 1, 1);
      wclrtoeol(zoomw);
      wmove(zoomw, 1, (70-strlen(s))/2); /* make the string centered */
      wstandout(zoomw);
      waddstr(zoomw, s);
      wclrtoeol(zoomw);
      wstandend(zoomw);

      if (sp->owner != 0 && (visibility & SEE_OWNER)) {
	mvwprintw(zoomw, 2, 2, "     Owner: %s",world.nations[sp->owner].name);
      } else {
	mvwprintw(zoomw, 2, 2, "     Owner: None");
      }
      if (strlen(sp->name) > 0 || !(visibility & SEE_POPULATION)) {
	mvwprintw(zoomw, 2, 1+ZX/2, "Location: (%d,%d)",xrel(x,y,np->capital),
		  yrel(x,y,np->capital));
      }
      wclrtoeol(zoomw);

      mvwprintw(zoomw, 3, 2, " Geography: ");
      if (visibility & SEE_LAND_WATER) {
	wprintw(zoomw, "%s ", terrains[sp->terrain - MIN_TERRAIN].name);
	if (sp->altitude > SEA_LEVEL) {
	  wprintw(zoomw, "%s", altitude_map[map_alt(sp->altitude)].name);
	}
      } else {
	wprintw(zoomw, "Unknown");
      }
      wclrtoeol(zoomw);

      mvwprintw(zoomw, 4, 2, "   Climate: ");
      if (visibility & SEE_LAND_WATER) {
	wprintw(zoomw, "%s", climates[sp->climate].name);
      } else {
	wprintw(zoomw, "Unknown");
      }
      wclrtoeol(zoomw);

      mvwprintw(zoomw, 5, 2, " Resources: ");
      if (visibility & SEE_RESOURCES) {
	wprintw(zoomw, "soil %d, metal %d, jewels %d", sp->soil, sp->metal,
		sp->jewels);
      } else {
	wprintw(zoomw, "Unknown");
      }
      wclrtoeol(zoomw);

      mvwprintw(zoomw, 6, 2, "Population: ");
      if (sp->owner != 0) {
	if (visibility & SEE_POPULATION) {
	  wprintw(zoomw, "%d people (%c); %d employed; %d%% unemp.",
		  sp->n_people, world.nations[sp->owner].race.mark,
		  n_workers(sp), sp->n_people ?
		  (100*(sp->n_people - n_workers(sp)))/sp->n_people : 0);
	} else {
	  wprintw(zoomw, "Unknown people");
	}
	if ((visibility & SEE_ARMIES) && sp->alist != NULL) {
	  wprintw(zoomw, "; %d armies", sect_n_armies(sp));
	}
      } else {
	wprintw(zoomw, "None");
      }
      wclrtoeol(zoomw);

      mvwprintw(zoomw, 7, 2, "  Movecost: ");
      if (visibility & SEE_LAND_WATER) {
	wprintw(zoomw, "%d", get_generic_move_cost(np,sp));
/*	wprintw(zoomw, "%d", get_move_cost(np,sp)); */
      } else {
	wprintw(zoomw, "Unknown");
      }
      wclrtoeol(zoomw);
      mvwprintw(zoomw, 7, 1+ZX/4, "Roads: %d", sp->roads);
      mvwprintw(zoomw, 7, 1+2*ZX/4, "Defense: %d", sp->defense);
      mvwprintw(zoomw, 7, 1+3*ZX/4, " %s", has_bubble(sp) ? "Bubble" : "");

      if (sp->owner == np->id || user.id == 0) {
	mvwprintw(zoomw, 9, 2, "Sector Economy:");
	wclrtoeol(zoomw);

	mvwprintw(zoomw, 10, 2, "       Designation: %s (%c)",
		  desig_map[old_desig].name,
		  desig_map[old_desig].mark);
	wclrtoeol(zoomw);

	mvwprintw(zoomw, 11, 2, " Trial Designation: %s (%c)",
		  desig_map[new_desig].name,
		  desig_map[new_desig].mark);
	wclrtoeol(zoomw);

	  /* now tell the user how much revenue this sector generates */
	quantity=(desig_map[new_desig].revenue*np->taxes*n_workers(sp))/100;
	if (calc_revenue(np) > 0) {
	  fraction = (100*quantity)/calc_revenue(np);
	} else {
	  fraction = 100;
	}

	mvwprintw(zoomw, 12, 2, "Per capita revenue: %d    ",
		  desig_map[new_desig].revenue);
	mvwprintw(zoomw, 13, 2,
		  "   Taxes collected: %d, %d%% of total income (%d)    ",
		  quantity, fraction, calc_revenue(np));
	wclrtoeol(zoomw);

	switch (desig_map[new_desig].mark) {
	case 'm':
	  strcpy(s, "metal");
	  total = calc_metal(np);
	  quantity = sector_metal(sp);
	  break;
	case 'j':
	  strcpy(s, "jewels");
	  total = calc_jewels(np);
	  quantity = sector_jewels(sp);
	  break;
	case 'f':
	  strcpy(s, "food");
	  total = calc_food(np);
	  quantity = sector_food(sp);
	  break;
	default:
	  strcpy(s, "");
	  quantity = -1;
	  fraction = 0;
	  break;
	}
	fraction = (total == 0) ? 100 : ((quantity * 100) / total);

	wmove(zoomw, 14, 2);
	if (quantity != -1) {
	  /* watch out for division by zero if there are zero workers */
	  wprintw(zoomw,
               "Production of %s: %d; %d%% of total (%d); %5.2f per capita   ",
	  s, quantity, fraction, total,
	  ((double) quantity)/((double) (n_workers(sp) ? n_workers(sp) : 1)));
	}
	wclrtoeol(zoomw);
      }

      mvwaddstr(zoomw, 16, 1,
		"Options: [N]ame sector, [t]rial redesignate, [r]edesignate");
      box(zoomw, '|', '-');
      wrefresh(zoomw);
    } else {			/* else we must be in e[x]pert mode */
      if (visibility & SEE_POPULATION) {
	sprintf(s, "[N],[r],pop=%d,emp=%d,roads=%d,defense=%d", sp->n_people,
		n_workers(sp), sp->roads, sp->defense);
      } else {
	sprintf(s, "[N],[r],pop=?,emp=?,roads%d,defense%d",
		sp->roads, sp->defense);
      }
      statline_prompt(s, "sector zoom");
    }
    switch(c = mygetch()) {
    case ' ':
      done = 1;
      break;
    case 'N':
      name_sector(zoomw);
      break;
    case 't':			/* redesignate without paying and saving */
      if (zoomw) {			/* not in expert mode */
	new_desig = redesignate(np, zoomw, 0);
	sp->designation = new_desig;
      }
      break;
    case 'r':			/* make a redesignation for sure, and pay */
      new_desig = redesignate(np, zoomw, 2);
      old_desig = new_desig;	/* permanent change!! */
      sp->designation = new_desig;
      break;
    default:
      break;
    }
  }
  if (zoomw) {
    delwin(zoomw);
    touch_all_wins();
    refresh();
  }
  sp->designation = old_desig;	/* make sure "trial" changes are temporary */
}

  /* change the name of a sector */
void name_sector(w)
     WINDOW *w;
{
  char name[NAMELEN];
  int x = user.cursor.x, y = user.cursor.y;
  Ssector *sp = &world.map[x][y];
  char s[EXECLEN];
  int ret;			/* for return values */

  if (w) {
    statline("name this sector", "name_sector");
  }
  if (sp->owner != user.id && user.id != 0) {
    statline2_err("hit space", "sector not yours");
  } else {
    if (w) {
      mvwaddstr(w, ZY-3, 2, "Enter name for the sector: ");
      wclrtoeol(w);
      wrefresh(w);
    } else {
      statline_prompt("Name this sector: ", "");
    }
    ret = wget_name(w, name);
    if (ret > 0) {
      strcpy(sp->name, name);
        /* now generate the exec instruction */
      sprintf(s, "SNAME:%d:%d:%s\n", x, y, name);
      gen_exec(s);
    }
  }
  if (w) {
    wmove(w, ZY-3, 2);
    wclrtoeol(w);
  }
  user.just_moved = 1;
}

  /* menu that allows you to redesignate a sector */
int redesignate(np, w, confirm)
     Snation * np;
     WINDOW *w;
     int confirm;
{
  char name[NAMELEN];
  int x = user.cursor.x, y = user.cursor.y;
  Ssector *sp = &world.map[x][y];
  char s[EXECLEN];
  char c;
  int i, new_desig = sp->designation;

  if (w) {
    statline("choose a designation", "redesignate");
  }
  if (sp->owner != user.id && user.id != 0) {
    statline2_err("type space to continue", "sector not yours");
      /* a user cannot remove their capital, but Gamemaster can */
  } else if (user.id != 0 && sp->designation == D_CAPITAL) {
    statline2_err("type space to continue", "you cannot be without a capital");
  } else {
    if (w) {
      wmove(w, ZY-5, 1);
      wclrtoeol(w);
      wmove(w, ZY-5, 1);
      for (i = 0; i < D_MAX_DESIG; ++i) {
	if (i % 4 == 0) {
	  wmove(w, ZY-5 + i/4, 1);
	  wclrtoeol(w);
	  wmove(w, ZY-5 + i/4, 1);
	}
	wprintw(w, "[%c]-%s  ", desig_map[i].mark, desig_map[i].name);
      }
      box(w,'|','-');
      wrefresh(w);
    } else {			/* expert mode */
      statline_prompt("Give new designation: ", "");
    }
    c = getch();		/* get the new designation */

    for (i = 0; i < D_MAX_DESIG; ++i) {
      if (desig_map[i].mark == c) {
	new_desig = i;
	break;
      }
    }
    /* make sure that user's capital is moved (if it is confirmed) */
    if ((new_desig == D_CAPITAL) && confirm) {
      move_capital(&world.nations[sp->owner], sp);
    }
  }
  if (w) {
    wmove(w, ZY-5, 0);
    wclrtobot(w);
    statline2("", "");
  }
  if (new_desig==D_CITY && sp->n_people < desig_map[new_desig].min_employed) {
    sprintf(s, "need %d people for a city", desig_map[new_desig].min_employed);
    statline2_err("type space to continue", s);
    new_desig = sp->designation;
    confirm = 0;
  }

  if (np->money < desig_map [new_desig].price && confirm && !(user.id == 0)) {
    confirm = 0;
    new_desig = sp->designation;
    statline2_err ("space to continue", "not enough money");
  }

  /* only make the actual change if the confirm flag is set */
  if ((new_desig != sp->designation) && confirm) {
    sp->designation = new_desig;
      /* now generate the exec instructions */
    sprintf(s, "DESIG_SECTOR:%d:%d:%d\n", x, y, new_desig);
    gen_exec(s);
    user.np->money -= desig_map[i].price;
    cmoney(user.np, -desig_map[new_desig].price);
  }
  user.just_moved = 1;
  return new_desig;
}

  /* a couple of little routines that generate simple
     exec lines, and are used all the time
   */
void cpeople_sector(sp, p)
     Ssector *sp;
     int p;
{
  char s[EXECLEN];

  sprintf(s, "CPEOPLE_SECTOR:%d:%d:%d\n", sp->loc.x, sp->loc.y, p);
  gen_exec(s);
}

void cmoney(np, m)
     Snation *np;
     int m;
{
  char s[EXECLEN];
  sprintf(s, "CMONEY:%d:%d\n", np->id, m);
  gen_exec(s);
}

void cmetal(np, m)
     Snation *np;
     int m;
{
  char s[EXECLEN];
  sprintf(s, "CMETAL:%d:%d\n", np->id, m);
  gen_exec(s);
}

void cjewels(np, j)
     Snation *np;
     int j;
{
  char s[EXECLEN];
  sprintf(s, "CJEWELS:%d:%d\n", np->id, j);
  gen_exec(s);
}

void cspell_pts(np, pts)
     Snation *np;
     int pts;
{
  char s[EXECLEN];

  sprintf(s, "CSPELL_PTS:%d:%d\n", np->id, pts);
  gen_exec(s);
}

void cfood(np, f)
     Snation *np;
     int f;
{
  char s[EXECLEN];
  sprintf(s, "CFOOD:%d:%d\n", np->id, f);
  gen_exec(s);
}

void ctech_skill(np, change)
     Snation *np;
     int change;
{
  char s[EXECLEN];
  sprintf(s, "CTECH_SKILL:%d:%d\n", np->id, change);
  gen_exec(s);
}

void cmag_skill(np, change)
     Snation *np;
     int change;
{
  char s[EXECLEN];
  sprintf(s, "CMAG_SKILL:%d:%d\n", np->id, change);
  gen_exec(s);
}

  /* this ensures that the user has only one capital,
     when he moves his over.
   */
void move_capital(np, sp)
     Snation *np;
     Ssector *sp;
{
  char s[EXECLEN];
    /* first make the old capital be a simple city */
  if (user.id != 0) {
    world.map[np->capital.x][np->capital.y].designation = D_CITY;
    sprintf(s,"DESIG_SECTOR:%d:%d:%d\n", np->capital.x, np->capital.y, D_CITY);
    gen_exec(s);
      /* now make this sector be the new capital */
  }
  np->capital = sp->loc;
}

  /* dumps the map visible on the screen to a file */
void dump_map()
{
  char s[80], filename[NAMELEN];
  sprintf(s, "give a file name (default = \"%s\")", "map");
  statline(s, "dump_map");
  move(LINES-2, COLS/3);
  addstr("> ");
  echo();
  if (wget_string(NULL,filename,18) <= 0) {
    strcpy(filename, "map");
  }
  noecho();
  move(LINES-2, COLS/3);
  clrtoeol();
  my_scr_dump(stdscr, filename);
}

  /* mail menu */
void mail()
{
  char *mail_prog;

  statline("do you want to (r)ead mail or (w)rite mail", "mail");
  switch (getch()) {
  case 'r':
    refresh();
    mail_read(user.id);
    clear(); refresh();
    user.just_moved = 1;
    break;
  case 'w':
    mail_write();
    user.just_moved = 1;
    break;
  default:
    break;
  }
}

/* This function takes care of players' sending mail to other nations.
 It does all the curses printing and scaning here (That's why it's a bit
 messy) and calls functions from mail for editing and sending and lock foo. */

void mail_write()
{
  char s[EXECLEN];
  int ret, id, done = 0;
  char r_name[NAMELEN];
  int c;
  char tmp_fname[PATHLEN];
  char subject[100];
  char sender[NAMELEN*2+4],receiver[NAMELEN*2+4];
  
  clear();
  statline("Sending mail","mail_write");
  mvaddstr(1,0,"Enter name of recipient (Nation name) ");
  ret = wget_name(stdscr,r_name);
  if (ret > 0) {
    id=get_nation_id(r_name);
    if (id>=0) {
      if (has_mail_lock(id)) {
	mvaddstr(4,0,"The recipient's mailbox is active right now.");
	mvaddstr(5,0,"Try sending you mail in a moment.");
	refresh();
	statline2_err("Press space to return","write_mail");
	clear();
      } else {
	strcpy(tmp_fname, "/usr/tmp/domedXXXXXX");
	if (mktemp(tmp_fname) == NULL) {
	  fprintf(stderr,"Error getting temp file name\n");
	  fflush(stderr);
	  return;
	}
	mvaddstr(4,0,"Subject: ");
	refresh();
	wget_name(stdscr, subject);
	cleanup();
	chdir("/usr/tmp");
	edit(tmp_fname);
	chdir(current_dir);
	chdir(libdir);
	{
	  initscr();
	  savetty();
	  nonl();
	  cbreak();
	  noecho();
	  clear();
	}
	mvaddstr(2, 0, "Choices: S)end mail or A)bort sending ");
	refresh();
	done = 0;
	while(!done) {
	  switch(getch()){
	  case 'S':
	  case 's':
/*	    lock_mail(id); */
	    mvaddstr(3, 9, "Sending Mail...");
	      /* Check if mail is being used by/for this user right now */
	    if (has_mail_lock(id)) {
	      printf("Found a lock file for receiver %d\n", id);
	      fflush(stdout);
	      refresh();
	      sleep(2);
	      if(has_mail_lock(id)) {
		printf("Found a lock file for receiver %d\n", id);
		fflush(stdout);
		refresh();
		return;
	      }
	    }
	    lock_mail(id);
	    refresh();
	    mail_send(tmp_fname, user.id, id, subject);
	    unlock_mail(id);
	    mvaddstr(3, LINES, "done.");
	    refresh();
	    done = 1;
	    break;
	  case 'A':
	    mvaddstr(3, LINES-1, "OK. Aborting...");
	    refresh();
	    done = 1;
	    unlock_mail(id);
	    break;
	  }
	  unlink(tmp_fname);
	}
	cleanup();
	init_screen();
      } /* else */
    } /* id >= 0 */
    else {			/* else we got a bad nation name */
      mvaddstr(7,0,"Invalid Nation Name");
      statline2_err("Press <SPACE> to return", "Bad Nation Name");
      clear();
      refresh();
    } /* else */
  } /* ret > 0 */
  clear(); refresh();
} /* mail_write */
