/* misc.c -- routines for which a better place has yet to be found */
/*           or routines which are needed in many places           */

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


/* int interrupt() - handles interupts                             */
/* show_nation(np) - displays a nation's characteristics           */
/* free_nation_mark(wp,c) - tell us if the symbol is available     */
/* dom_getline(s,n) - read a line from stdin, remove \n                */
/* get_crypt_pass(prompt,s,w) - get password and encrypt           */
/* critical() - while writing, don't bug me!!                      */
/* noncritical() - normal operation                                */
/* which_mark(x,y,up) - determine which mark is to be shown for    */
/*           the sector in question based on display modes, etc.   */
/* addsector(np,x,y) - adds sector x,y to nation's list of owned   */
/*           sectors.  Also changes owner of sector to nation->id. */
/* subtsector(np,x,y) - deletes sector x,y from nation's list of   */
/*           owned sectors.                                        */
/* destroy_nation(np) - frees all memory used by that nation       */
/* get_n_cities(np) - return number of cities in nation            */
/* get_n_civil(np) - return number of civilians in nation          */
/* unique_name(name) - returns true if a nation with this name     */
/*           does not exist yet                                    */
/* free_army_id(np) - finds first free army id for a nation        */
/* get_space() - waits for the user to type a space                */
/* statline(s,s) - places information on the statline              */
/* statline2(s,s) - places information on statline 2               */
/* get_spirit_from_type(up,type) - returns a spirit pointer, given a type */
/* gen_exec(s) - puts the exec line into the exec list (and later file)   */
/* is_good_order(name) - TRUE if this is a valid name for a magic order   */
/* find_visible_sectors(visible_sectors) - updates visibility matrix      */

#include "dominion.h"
#include "misc.h"
#include "army.h"
#include "cur_stuff.h"
#include "functions.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <signal.h>

extern int (*wrapx)(), (*wrapy)();

extern Sworld world;
extern struct s_desig_map desig_map[];
extern struct s_altitude_map altitude_map[];
extern struct item_map terrains[];
extern struct item_map climates[];
extern Suser user;
int viewall;
extern int debug;

WINDOW *sectw;

/* handles interrupts */
int interrupt()
{
  printf("\r\ngot an interrupt.  quitting nicely...\r\n");
  cleanup();		/* cleanup depends on which program you are in */
  clean_exit();
  exit(0);
}

/* show a nation's characteristics */
void show_nation(np)
     Snation *np;		/* nation pointer */
{
  printf("\n\tname: %s (id = %d)\n", np->name, np->id);
  printf("\tleader: %s\n", np->leader);
  printf("\tpasswd: %s\n", np->passwd);
  printf("\tcapital is at (%d, %d)\n", np->capital.x, np->capital.y);
  printf("\trace is %s\n", np->race.name);
  printf("\tnation mark is %c\n", np->mark);
  printf("\t%d civilians; %d soldiers; %d armies.\n",
	 get_n_civil(np), get_n_soldiers(np), np->n_armies);
  printf("\tmagical order %s\n", np->mag_order);
}


	/* tell us if the symbol is available */
int free_nation_mark(wp, c)
     Sworld *wp;
     Symbol c;
{
  int i;

  if (!isprint(c)) {
    return 0;
  }
  for (i = 0; i < wp->n_nations; ++i) {
    if (wp->nations[i].mark == c) {
      return 0;
    }
  }
  return 1;
}

void dom_getline(s, n)			/* read a line from stdin, remove \n */
     char s[];
     int n;
{
  fgets(s, n, stdin);
  if (s[strlen(s)-1] == '\n') {	/* remove \n if it is there */
    s[strlen(s)-1] = '\0';
  }
}

  /* get a password and encrypt it.  if the parameter "default"
     has a string in it, use that instead of getting it from
     the terminal.  if "w" is not NULL, get the string from
     the window "w".
   */
void get_crypt_pass(prompt, pass, w, def_pass)
     char prompt[], pass[];
     WINDOW *w;
     char def_pass[];
{
  char *s1, s2[PASSLEN], *getpass(), *crypt();

    /* if there is already a password, it is passed in the string def_pass */
  if (def_pass != NULL && strlen(def_pass) > 0) {
    strcpy(s2, def_pass);
    s1 = crypt(s2, SALT);
    strcpy(pass, s1+2);
    return;
  }
    /* if no password was given us, get it from input */
  if (w == NULL) {
    s1 = getpass(prompt);
    strcpy(s2, s1);
    s1 = crypt(s2, SALT);
    strcpy(pass, s1 + 2);	/* final crypted pass. without salt */
  } else {
    wprintw(w, "%s", prompt);
    wrefresh(w);
    wscanw(w, "%s", s2);
    s1 = crypt(s2, SALT);
    strcpy(pass, s1+2);
  }
}

void cpass(np, pass)
     Snation *np;
     char pass[];
{
  char s[EXECLEN];
  sprintf(s, "CPASS:%d:%s\n", np->id, pass);
  gen_exec(s);
}

int which_mark(x, y, up)
     int x, y;
     Suser *up;
{
  int highlight = 1;		/* should we highlight? 1=no,-1=yes */
  int owner = world.map[x][y].owner, mark = '\0';
  Ssector *sectp = &world.map[x][y];
  int visibility = user.visible_sectors[x][y];
  int cost;
  Sarmy *ap, *get_army();
  struct armyid *alist;

    /* figure out what to draw */
    /* should find a clean way of dealing with above/under water */
  if (!user.underwater && sectp->altitude < SEA_LEVEL) {
    mark = '~';			/* if not underwater, water looks like ~ */
  }
  if (user.underwater && sectp->altitude >= SEA_LEVEL) {
    mark = '.';		/* if underwater, land looks like # */
  }

  if (!mark) {			/* only if mark is not yet set */
    switch (up->display) {
    case DESIGNATION:
      if (owner == up->id) {
	mark = desig_map[sectp->designation].mark;
      } else {
	if (world.nations[owner].mark == '-') {
	  mark = altitude_map[map_alt(sectp->altitude)].mark;
	} else {
	  mark = world.nations[owner].mark;
	}
      }
    break;
    case NATION_MARK:
/*    if (world.nations[owner].mark == '-' || !(visibility & SEE_OWNER)) {
      mark = altitude_map[map_alt(sectp->altitude)].mark;
      } else {
      mark = world.nations[owner].mark;
      }
   */
      mark = world.nations[owner].mark;
      break;
    case SOIL:
      if (visibility & SEE_RESOURCES) {
	mark = (sectp->soil <= 9) ? sectp->soil + '0' : '+';
      } else {
	mark = '?';
      }
      break;
    case METAL:
      if (visibility & SEE_RESOURCES) {
	mark = (sectp->metal <= 9) ? sectp->metal + '0' : '+';
      } else {
	mark = '?';
      }
      break;
    case JEWELS:
      if (visibility & SEE_RESOURCES) {
	mark = (sectp->jewels <= 9) ? sectp->jewels + '0' : '+';
      } else {
	mark = '?';
      }
      break;
    case ALTITUDE:
      if (visibility & SEE_LAND_WATER) {
/*      mark = (sectp->altitude <= 9) ? sectp->altitude + '0' : '+'; */
	if (sectp->altitude > 9) {
	  mark = '+';
	} else if (sectp->altitude >= 0) {
	  mark = sectp->altitude + '0';
	} else {
	  mark = (-1 * sectp->altitude) + '0';
	}
      } else {
	mark = '?';
      }
      break;
    case CLIMATE:
      if (visibility & SEE_LAND_WATER) {
	mark = climates[sectp->climate].mark;
      } else {
	mark = '?';
      }
      break;
    case POPULATION:
      if (visibility & SEE_POPULATION) {
	if (sectp->n_people < 950) {
	  mark =(sectp->n_people < 950) ? (sectp->n_people + 50)/100+'0' : '+';
	} else {
	  mark = 'I';
	}
      } else {
	mark = '?';
      }
      break;
    case ARMY_MOVECOST:
      if (visibility & SEE_LAND_WATER) {
	if ((ap=get_army (&world.nations[up->id], user.current_army))
	    == NULL) {
	  cost = get_generic_move_cost (&world.nations[up->id], sectp);
	} else {
	  cost = get_army_move_cost (&world.nations[up->id], sectp, ap);
	}
	mark = (cost <= 9) ? cost + '0' : '+';
      } else {
	mark = '?';
      }
      break;
    case MOVECOST:
      if (visibility & SEE_LAND_WATER) {
	cost = get_generic_move_cost(&world.nations[up->id], sectp);
	mark = (cost <= 9) ? cost + '0' : '+';
      } else {
	mark = '?';
      }
      break;
    case TERRAIN:
      if (visibility & SEE_LAND_WATER) {
	mark = terrains[sectp->terrain - MIN_TERRAIN].mark;
      } else {
	mark = '?';
      }
      break;
    default:			/* this should never happen */
      break;
    }
  }

    /* here set the highlighting; we know user owns sector */
  switch (up->highlight) {
  case H_OWNED:
    if (sectp->owner == up->id) {
      highlight = -1;		/* if user owns, highlight */
    }
    break;
  case H_ARMIES:
    if ((user.visible_sectors[sectp->loc.x][sectp->loc.y] & SEE_ARMIES)
	 && has_visible_army(sectp, &user)) {
      highlight = -1;
    } 
/*    if (sectp->alist != NULL
	&& (user.visible_sectors[sectp->loc.x][sectp->loc.y] & SEE_ARMIES)) {
      highlight = -1;
    } else {
      highlight = 1;
    }
*/
    break;
  case H_HOSTILE:
    if (has_hostile (sectp)) {
      highlight = -1;
    }
    break;
  case H_YOUR_ARMIES:
    if (sectp->alist != NULL
	&& (user.visible_sectors[sectp->loc.x][sectp->loc.y] & SEE_ARMIES)) {
      alist = sectp->alist;
      while (alist != NULL) {
	if (alist->owner == user.id) {
	  highlight = -1;
	  break;
	}
	alist = alist->next;
      }
    }
    break;
  case H_OTHER_ARMIES:
    if (sectp->alist != NULL
	&& (user.visible_sectors[sectp->loc.x][sectp->loc.y] & SEE_ARMIES)) {
      alist = sectp->alist;
      ap = get_army(&world.nations[alist->owner], alist->id);
      while (alist != NULL) {
	if (alist->owner != user.id && !is_hidden(ap)) {
	  highlight = -1;
	  break;
	}
	alist = alist->next;
      }
    }
    break;
  case H_MOVE_LEFT:
    if (sectp->alist != NULL
	&& (user.visible_sectors[sectp->loc.x][sectp->loc.y] & SEE_ARMIES)) {
      highlight = 1;
      alist = sectp->alist;
      while (alist != NULL) {
	if (alist->owner == user.id) {
	  ap = get_army(&world.nations[alist->owner], alist->id);
	  if (ap->mvpts > 0) {
	    highlight = -1;
	  }
	}
	alist = alist->next;
      }
    } else {
      highlight = 1;
    }
    break;
  case H_UNEMP:
    if ((sectp->n_people > n_workers(sectp)) && (sectp->owner == up->id)) {
      highlight = -1;
    }
    break;
  case H_NONE:
    highlight = 1;
    break;
  default:
    break;
  }

  return highlight*mark;	/* highlight can be +- 1 */
}

/********************************************************************/
/* adds sector x,y to nation's list of owned sectors. Also changes  */
/* owner of sector to nation->id.                                   */
/********************************************************************/
void addsector(np, x, y)
     Snation *np;
     int x, y;
{
  struct pt_list *sect;

  sect = (struct pt_list *) malloc(sizeof(struct pt_list));
  sect->pt.x = x;
  sect->pt.y = y;
  sect->next = np->ptlist;
  np->ptlist = sect;
  np->n_sects++;
  world.map[x][y].owner = np->id;
}


void subtsector(np, x, y)
     Snation *np;
     int x, y;
{
  struct pt_list *sect, *temp;

  sect = np->ptlist;
  if (sect->pt.x == x && sect->pt.y == y) {
    temp = sect;
    np->ptlist = sect->next;
    (np->n_sects)--;
    free(temp);
  } else {
    temp = sect;
    while (sect->pt.x != x || sect->pt.y != y && sect->next != NULL) {
      temp = sect;
      sect = sect->next;
    }
    if (sect->pt.x == x && sect->pt.y == y) {
      temp->next = sect->next;
      free(sect);
      (np->n_sects)--;
    } else printf("Error - deleting sector not in list!\n");
  }
  if (np->n_sects == 0) {
    np->ptlist = NULL;
  }
}

  /* Destroy a nation:  these things have to be done:
     1. Free up its point list, and return the sectors to owner 0
     2. Free up its army list, and also its army entires in the
        various sector army lists.  This can be done by disbanding
	them all. (?)
     3. Set the coordinates of its capital to (-1, -1), which alerts
        the program that this nation is no more.
   */
void destroy_nation(id)
     int id;
{
  struct pt_list *ptlist, *pt_tmp;
  Sarmy *army_list, *ap_tmp;
  Snation *np = &world.nations[id];
  Ssector *sp;
  struct army_id *sect_alist;
  char s[EXECLEN];

  if (np->id == 0) {
    return;			/* don't destroy nation 0, ever */
  }
  ptlist = np->ptlist;

  while (ptlist) {
      /* return all the sectors */
    world.map[ptlist->pt.x][ptlist->pt.y].owner = 0;
    world.map[ptlist->pt.x][ptlist->pt.y].designation = D_NODESIG;
    world.map[ptlist->pt.x][ptlist->pt.y].n_people = 0;
    pt_tmp = ptlist->next;
    free(ptlist);
    --np->n_sects;
    ptlist = pt_tmp;
  }
  np->ptlist = NULL;

  army_list = np->armies;
    /* delete armies while we still have armies in the nation */
  while (army_list) {
    ap_tmp = army_list->next;
    sp = &world.map[army_list->pos.x][army_list->pos.y];
    delete_army_sector(sp, army_list);
    delete_army_nation(np, army_list);
    /*    free(army_list); */
    /*    army_list = ap_tmp; */
    army_list = np->armies;
  }
  np->armies = NULL;

    /* set capital to (-1,-1):  this tells dominion that
       the parrot (i mean, nation) is no more
     */
  np->capital.x = -1;
  np->capital.y = -1;
}

  /* returns the number of cities in this nation */
int get_n_cities(np)
     Snation *np;
{
  Ssector *sp;
  struct pt_list *ptlist = np->ptlist;
  int n_cities = 0;

  while (ptlist != NULL) {
    sp = &world.map[ptlist->pt.x][ptlist->pt.y];
    if (sp->designation == D_CITY || sp->designation == D_CAPITAL) {
      ++n_cities;
    }
    ptlist = ptlist->next;
  }
  return n_cities;
}

  /* returns the number of civilians in this nation */
int get_n_civil(np)
     Snation *np;
{
  Ssector *sp;
  struct pt_list *ptlist = np->ptlist;
  int n_civil = 0;

  while (ptlist != NULL) {
    sp = &world.map[ptlist->pt.x][ptlist->pt.y];
    n_civil += sp->n_people;
    ptlist = ptlist->next;
  }
  return n_civil;
}

  /* returns the number of soldiers in this nation (includes spirits) */
int get_n_soldiers(np)
     Snation *np;
{
  Sarmy *armies = np->armies;
  int n_sold = 0;

  while (armies != NULL) {
    n_sold += armies->n_soldiers;
    armies = armies->next;
  }
  return n_sold;
}

  /* returns true if a nation with this name does not yet exist */
int unique_name(name)
     char name[];
{
  int i;
  for (i = 0; i < NATIONS; ++i) {
    if (strcmp(world.nations[i].name, name) == 0) {
      return 0;			/* found a conflicting name */
    }
  }
  return 1;			/* didn't find it any conflict */
}

  /* this is for when the user hits a harmless key, don't give error */
void null_key()
{
}

  /* returns the first free army id for a nation */
int free_army_id(np)
     Snation *np;
{
  Sarmy *ap = np->armies, *ap_prev = np->armies;
  int id;

  if (ap == NULL) {
    return 0;
  }
  if (ap->id > 0) {		/* if first slot is unused */
    return 0;
  }
  if (ap->next == NULL) {
    return 1;
  }
  
  while (ap != NULL) {
    if (ap->id > ap_prev->id+1) { /* a space!! */
      id = ap_prev->id + 1;
      return id;
    }
    ap_prev = ap;
    ap = ap->next;
  }
    /* if we have not yet found it, it must be the last one!! */
/*  beep(); refresh(); */
  id = ap_prev->id + 1;
  return id;
}

  /* this waits for the user to type a space */
void get_space()
{
  fflush(stdin);
  while (getch() != ' ') {
  }
}

#ifndef min
int min(a,b)
     int a,b;
{
  return (a < b) ? a : b;
}

int max(a,b)
     int a,b;
{
  return (a > b) ? a : b;
}
#endif

void statline(s1, s2)		/* print status line with s1 and s2 */
     char s1[], s2[];
{
	/* the stat line goes at the bottom of the screen */
  mvaddstr(LINES-1, 0, s1); /* first string at beginning */
  clrtoeol();
    /* second string at end of line */
  standout();
  mvaddstr(LINES-1, COLS-strlen(s2)-2, s2);
  standend();
  refresh();
}

  /* statline2 is like statline, but prints on the second-last line.
     statline2 should be used for instructions while a command is being run,
        (like a move_army).
   */
void statline2(s1, s2)
     char s1[], s2[];
{
	/* the stat line goes at the bottom of the screen */
  mvprintw(LINES-2, 0, "%s", s1); /* first string at beginning */
  clrtoeol();
	/* second string at end of line */
  standout();
  mvprintw(LINES-2, COLS-strlen(s2)-2, "%s", s2);
  standend();
  refresh();
}

  /* this runs a statline, and then moves to just
     after s1, for a prompt.  used a lot in xmode
   */
void statline_prompt(s1, s2)
     char s1[], s2[];
{
  statline(s1, s2);
  move(LINES-1, strlen(s1));
  refresh();
}

  /* this runs a statline2, and then moves to just
     after s1, for a prompt.  used a lot in xmode
   */
void statline2_prompt(s1, s2)
     char s1[], s2[];
{
  statline2(s1, s2);
  move(LINES-2, strlen(s1));
  refresh();
}

  /* runs a statline2, waits for a space to be typed,
     then cleans the statline2 and returns
   */
void statline2_err(s1, s2)
     char s1[], s2[];
{
  statline2(s1, s2);
  get_space();
  statline2("", "");
}

	/* curses interface */
void init_screen()
{
  printf("initializing screen...\r\n");
  initscr();
  savetty();
/*  nonl(); */
  cbreak();
  noecho();
  clear();
  /* OK, now the stdscr is made, must make a couple other windows */
  sectw = newwin(SECTW_SIZE_Y, SECTW_SIZE_X, SECTW_Y, SECTW_X);
  /*  armyw = newwin(ARMYW_SIZE_Y, ARMYW_SIZE_X, ARMYW_Y, ARMYW_X); */
  /* move the point to the user's capital (to start) */
  if (user.map_style == NORMAL_MAP) {
    move(user.cursor.y, user.cursor.x);
  } else {
    move(2*user.cursor.y, user.cursor.x);
  }
  /* refresh(); */
}

  /* returns a pointer to the spirit pointer of that given type */
struct spirit_type *get_spirit_type(up, type)
     Suser *up;
     char type[];
{
  int i;
  extern struct spirit_type *spirit_types;

/*  printf("user.n_spirit_types = %d\n", up->n_spirit_types); */
  for (i = 0; i < up->n_spirit_types; ++i) {
/* printf("type=%s, spirit_types[%d].type = %s\n", type, i, spirit_types[i].type); */
    if (strcmp(type, spirit_types[i].type) == 0) {
      return &(spirit_types[i]);
    }
  }
  return NULL;
}

  /* add to the user's exec_list; write it to file if full.
     If passed string is NULL, DON'T generate exec, and
     write out the whole thing anyway.
   */
void gen_exec(s)
     char *s;
{
  FILE *fp, *fopen();
  char exec_file[NAMELEN];
  int i;
  
  sprintf(exec_file, "exec/exec%d", user.id);  

    /* first add the string s to the user's exec_lines;
       special case if string is empty or NULL:
       if empty, do nothing;
       if NULL, write out to file.
     */
  if ((s != NULL) && (strlen(s) == 0)) {
    return;
  }
  if (s != NULL) {
    strcpy(user.exec_lines[user.n_execs], s);
    ++user.n_execs;
  }
  if ((user.n_execs >= N_EXECS) || (s == NULL) ) {
    if ((fp = fopen(exec_file, "a")) == NULL) {
      printf("cannot open your exec file, this is serious\n");
      clean_exit();
      exit(1);
    }
    critical();
    for (i = 0; i < user.n_execs; ++i) {
      fprintf(fp, "%s", user.exec_lines[i]);
      /*      printf("debug: writing to exec file: %s", user.exec_lines[i]); */
    }
    fclose(fp);
    noncritical();
    user.n_execs = 0;		/* reset count */
  }
}


/* This just displays the title/intro screen */
void intro(wp, np)
     Sworld *wp;
     Snation *np;
{
  FILE *mail;
  char txt[200];

  sprintf(txt, "mail/mail.%d", np->id);
  mail = fopen(txt, "r");
  strcpy(txt, "Dominion");
  mvprintw((LINES-12)/2, (COLS-strlen(txt))/2, txt);
  sprintf(txt, "Version %s", VERSION);
  mvprintw((LINES-10)/2, (COLS-strlen(txt))/2, txt);
  standout();
  strcpy(txt, "Copyright (c) 1990, Free Software Foundation");
  mvprintw((LINES-6)/2, (COLS-strlen(txt))/2, txt);
  standend();
  sprintf(txt, "Thon %d", wp->turn);
  mvprintw((LINES-2)/2, (COLS-strlen(txt))/2, txt);
  sprintf(txt, "%d nations", wp->n_nations);
  mvprintw(LINES/2, (COLS-strlen(txt))/2, txt);
  sprintf(txt, "world size is %d across, %d down", wp->xmax, wp->ymax);
  mvprintw((LINES-2)/2, (COLS-strlen(txt))/2, txt);
  standout();
  if (mail) {
    strcpy(txt, "You have mail!");
    mvprintw((LINES+6)/2, (COLS-strlen(txt))/2, txt);
    fclose(mail);
  }
  strcpy(txt, "Press any key to begin");
  mvprintw((LINES+10)/2, (COLS-strlen(txt))/2, txt);
  standend();
  refresh();
  getch();
  clear();
}

  /* check the mag_Orders file, and see if this is a valid magic order */
int is_good_order(name)
     char name[];
{
  int good = 0;
  FILE *fp, *fopen();
  char line[200];
  int i, n_orders;

  if ((fp = fopen(MAG_ORDERS, "r")) == NULL) {
    printf("cannot find file %s.  this is bad.\n", MAG_ORDERS);
    clean_exit();
    exit(1);
  }
  while (fgets(line, 200, fp) != NULL) {
    /*    printf("%s", line); */
    if (line[0] != '#') {
      sscanf(line, "%d", &n_orders);
      break;			/* we got the number of orders */
    }
  }
  /*  printf("there are %d magical orders\n", n_orders); */
  for (i = 0; i < n_orders; ) {
    fgets(line, NAMELEN, fp);
    if (line[strlen(line)-1] == '\n') {
      line[strlen(line)-1] = '\0';
    }
    if (line[0] != '#') {
      if (strncmp(line, name, NAMELEN) == 0) {
	good = 1;
      }
      ++i;
    }
  }
  return good;
}

  /* returns true if there is at least one
     non-hidden army on this sector.
   */
int has_visible_army(sp, up)
     Ssector *sp;
     Suser *up;
{
  struct armyid *alist = sp->alist;
  Sarmy *ap, *get_army();
  int found = 0;

  while (alist) {
    ap = get_army(&world.nations[alist->owner], alist->id);
    if (!is_hidden(ap)) {
      found = 1;
      break;
    }
    alist = alist->next;
  }
  return found;
}

  /* this routine goes through the entire map and figures out
     which sectors are visible by the user.
   */
void find_visible_sectors(visible_sectors)
     int **visible_sectors;
{
  int x, y, i, j;
  struct pt_list *plist;
  Sarmy *ap;
  Ssector *sp;

  for (i = 0; i < world.xmax; ++i) {
    for (j = 0; j < world.ymax; ++j) {
      visible_sectors[i][j] = viewall ? SEE_ALL : SEE_NOTHING;
    }
  }
  for (plist = user.np->ptlist; plist != NULL; plist = plist->next) {
    x = plist->pt.x;
    y = plist->pt.y;
    visible_sectors[x][y] = SEE_ALL;
    for (i = x-LAND_SIGHT; i <= x+LAND_SIGHT; ++i) {
      for (j = y-LAND_SIGHT; j <= y+LAND_SIGHT; ++j) {
	sp = &world.map[(*wrapx)(i,j)][(*wrapy)(i,j)];
	if (has_hidden(sp) && sp->owner != user.id) {
	  visible_sectors[x][y] |= SEE_ARMIES;
	} else {
	  visible_sectors[(*wrapx)(i,j)][(*wrapy)(i,j)] |=
	    (SEE_LAND_WATER | SEE_OWNER | SEE_DESIG | SEE_POPULATION);
	}
	if (world.map[(*wrapx)(i,j)][(*wrapy)(i,j)].owner == 0) {
	  visible_sectors[(*wrapx)(i,j)][(*wrapy)(i,j)] |= SEE_RESOURCES;
	}
      }
    }
  }
  for (ap = user.np->armies; ap != NULL; ap = ap->next) {
    x = ap->pos.x;
    y = ap->pos.y;
    sp = &world.map[x][y];
    if (has_hidden(sp) && sp->owner != user.id) {
      visible_sectors[x][y] = SEE_ARMIES;
    } else {
      visible_sectors[x][y] = SEE_ALL;
    }
    for (i = x-ARMY_SIGHT; i <= x+ARMY_SIGHT; ++i) {
      for (j = y-ARMY_SIGHT; j <= y+ARMY_SIGHT; ++j) {
	sp = &world.map[(*wrapx)(i,j)][(*wrapy)(i,j)];
	if (!has_hidden(sp)) {
	  visible_sectors[(*wrapx)(i,j)][(*wrapy)(i,j)] |=
	    (SEE_LAND_WATER | SEE_OWNER | SEE_DESIG |
	     SEE_POPULATION | SEE_ARMIES);
	}
	if (world.map[(*wrapx)(i,j)][(*wrapy)(i,j)].owner == 0) {
	  visible_sectors[(*wrapx)(i,j)][(*wrapy)(i,j)] |= SEE_RESOURCES;
	}
      }
    }
  }
}

/* Returns a string that formats the arguments */

char * contents (money, metal, jewels, food, people, army, title, sp)

int money, metal, jewels, food, people, army, sp;
Pt * title;
{
  char * rcontents;
  char tmps [60];

  if ((rcontents = (char *)malloc (sizeof (char) * 100)) == NULL) {
    clean_exit();
    exit (-1);
  }

  sprintf (rcontents, "");

  if (money > 0) {
    sprintf (tmps, "/%d sk.", money);
    strcat (rcontents, tmps);
  }
  if (metal > 0) {
    sprintf (tmps, "/%d met", metal);
    strcat (rcontents, tmps);
  }
  if (jewels > 0) {
    sprintf (tmps, "/%d jwl", jewels);
    strcat (rcontents, tmps);
  }
  if (food > 0) {
    sprintf (tmps, "/%d food", food);
    strcat (rcontents, tmps);
  }
  if (people > 0) {
    sprintf (tmps, "/%d peop", people);
    strcat (rcontents, tmps);
  }
  if (army >= 0) {
    sprintf (tmps, "/army %d", army);
    strcat (rcontents, tmps);
  }
  if (title && title->x != -1) {
    sprintf (tmps, "/sect %d,%d",
	     xrel (title->x, title->y, user.np->capital),
	     yrel (title->x, title->y, user.np->capital));
    strcat (rcontents, tmps);
  }
  if (sp > 0) {
    sprintf (tmps, "/%d sp", sp);
    strcat (rcontents, tmps);
  }

  if (rcontents [0] == '/') {
    strcpy (rcontents, rcontents + 1);
  }

  return rcontents;
}

int univ_intel (np)

Snation * np;
{
  double sqrt ();
  int ret;

  if (get_n_civil (np)) {
    ret = (int) (100 * get_n_students (np) / get_n_civil (np));
  }
  else { ret = 0; }

  return ret;
}

  
/*
  if (get_n_civil (np)) {
    ret = (int) sqrt ((double)(get_n_students (np) / 5));
    if (ret > 100) {
      ret = 100;
    }
    return ret;
  }
  return 0;
*/

int priestliness (np)

Snation * np;
{
  double sqrt ();
  int ret;

  if (get_n_civil (np)) {
    ret = (int) (100 * get_n_priests (np) / get_n_civil (np));
  }
  else { ret = 0; }

  return ret;
}
/*  if (get_n_civil (np)) {
    ret = (int) sqrt ((double)(get_n_priests (np) / 5));
    if (ret > 100) {
      ret = 100;
    }
    return ret;
    }
  return 0;
*/
