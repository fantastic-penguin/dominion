  /* cur_stuff.c -- stuff that uses curses a lot */

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

/* pager(fname) - pages through a file                             */
/* mygetch() - runs getch(), and gives help if user types '?'      */
/* wget_name(w, name) - gets a name in a curses window             */
/* wget_string(w, str, len) - gets a string in a curses window     */
/* wget_number(w, p) - gets a number in a curses window            */

#define NCURSES_OPAQUE 0

#include "dominion.h"
#include "misc.h"
#include "cur_stuff.h"
#include "functions.h"
#include <stdio.h>
#include <string.h>

extern Suser user;
extern Sworld world;
extern int (*wrapx)(), (*wrapy)();
extern struct s_desig_map desig_map[];
extern struct s_altitude_map altitude_map[];
extern struct item_map terrains[];

WINDOW *sectw;
/* WINDOW *sectw, *armyw; */

/* statline and statline2 moved to misc.c */

  /* simple standalone pager, used for news and other stuff */
char pager(fname)
     char fname[];
{
  FILE *fp, *fopen();
  WINDOW *pagew;
  long page_lines = LINES-2, lines = 0;
  char line[200];
  char c;
  int i;

  if ((fp = fopen(fname, "r")) == NULL) {
    statline2_err("cannot open file", fname);
    return '\0';
  }
  pagew = newwin(LINES-2, COLS, 0, 0);
  touchwin(pagew);
  lines = 0;
  while (fgets(line, 180, fp) != NULL) {
    line[COLS-2] = '\0';	/* make sure it fits on screen */
    mvwaddstr(pagew, lines, 0, line);
    wclrtoeol(pagew);
    wrefresh(pagew);
    ++lines;
    if (lines % page_lines == 0) { /* next page? */
      wclrtobot(pagew);
      wrefresh(pagew);
      lines = 0;
      statline("SPACE to continue, [q] or [n] to leave this file", fname);
      switch(c = getch()) {
      case 'q':
      case 'n':
	fclose(fp);
	return c;
	break;
      case 'f':			/* skip 23 lines */
	for (i = 0; i < 23 && fgets(line, 180, fp); ++i) {
	}
	break;
      case ' ':
	break;
      default:
	break;
      }
      wmove(pagew, 0, 0);
    }
  }
  fclose(fp);
  wclrtobot(pagew);
  wrefresh(pagew);
  delwin(pagew);
  return ' ';
}

  /* draw the world map */
void draw_map()
{
  if (user.map_style == NORMAL_MAP) {
    draw_map_regular();
  } else {
    draw_map_compact();
  }
}

  /* draw the map with a space between adjacent sectors */
void draw_map_regular()
{
  char s[80];
  int x = user.cursor.x, y = user.cursor.y /*,
         xoff = user.center.x-(COLS-2)/4, yoff = user.center.y - (LINES-2)/2*/;
  int i, j, iw, jw, n;
  int mark;	/* what to draw in that sector, negative if highlight */
  Ssector *sp = &world.map[x][y];
  int visibility;

    /* clean up the space from the previous armies */
  for (i = 0; i <= 2*user.last_n_armies; ++i) {
    move(i, ARMYW_X);
    clrtoeol();
  }
  user.last_n_armies = sect_n_armies(sp);

  for (i = xoff(); (i < xoff()+(COLS-2)/2) && (i < xoff()+world.xmax); ++i) {
    for (j = yoff(); (j < yoff()+LINES-2) && (j < yoff()+world.ymax); ++j) {
      if (!is_under_sectw(2*(i-xoff()), j-yoff())) {
          /* now wrap the coordinates, so we handle the topology */
	move(j-yoff(), 2*(i-xoff()));	/* move does not want wrapping */
	iw = (*wrapx)(i,j);
	jw = (*wrapy)(i,j);
	visibility = user.visible_sectors[iw][jw];
	if (visibility > SEE_NOTHING && visibility != SEE_ARMIES) {
	  if ((mark = which_mark(iw, jw, &user)) < 0) {
	    standout();
	    addch(-mark);
	    standend();
	  } else {
	    addch(mark);
	  }
	} else {		/* if not visible, put a space */
	  addch(' ');
	}
      }
    }
  }
  show_armies(&world.map[x][y]);
  sprintf(s, "Nation %s; money %d; Thon %d;   type %c for help",
	  user.np->name, user.np->money, world.turn, user.help_char);
  statline(s, "draw_map_regular");
  show_sector(x, y);
  move((*wrapy)(x-xoff(),y-yoff()), 2*(*wrapx)(x-xoff(),y-yoff()));
  refresh();
}

void draw_map_compact()		/* compact drawing of map */
{
  char s[80];
  int x = user.cursor.x, y = user.cursor.y;
  int i, j, n;
  int mark;	/* what to draw in that sector, negative if highlight */
  Ssector *sp = &world.map[x][y];
  int visibility;

    /* clean up the space from the previous armie */
  for (i = 0; i <= 2*user.last_n_armies; ++i) {
    move(i, ARMYW_X);
    clrtoeol();
  }
  user.last_n_armies = sect_n_armies(sp);

  for (i = xoff_compact();
       (i < xoff_compact() + COLS-2) && (i < xoff_compact() + world.xmax); ++i) {
    for (j = yoff(); (j < yoff() + LINES-2) && (j < yoff() + world.ymax); ++j) {
      move(j-yoff(), i-xoff_compact());
      visibility = user.visible_sectors[(*wrapx)(i,j)][(*wrapy)(i,j)];
      if (visibility > SEE_NOTHING && visibility != SEE_ARMIES) {
	if ((mark = which_mark((*wrapx)(i,j), (*wrapy)(i,j), &user)) < 0) {
	  standout();
	  addch(-mark);
	  standend();
	} else {
	  addch(mark);
	}
      } else {			/* if not visible, put a space */
	addch(' ');
      }
    }
  }
  show_armies(&world.map[x][y]);
  sprintf(s, "Nation %s; money %d; Thon %d;   type %c for help",
	  user.np->name, user.np->money, world.turn, user.help_char);
  statline(s, "draw_map_compact");
  show_sector(user.cursor.x, user.cursor.y);
  move((*wrapy)(x-xoff_compact(),y-yoff()), (*wrapx)(x-xoff_compact(),y-yoff()));
  refresh();
}

void show_sector(x, y)		/* give info on the sector */
     int x, y;
{
  Ssector *sp = &world.map[x][y];
  char s[2*NAMELEN];
  int visibility = user.visible_sectors[x][y];

  if (user.show_sect_win && user.just_moved) {
    /* put the stuff in the special sector window */
  mvwprintw(sectw, 1, 1, "(%d,%d)", xrel(x,y,user.np->capital),
	    yrel(x,y,user.np->capital));

  wclrtobot(sectw);

    /* Show sector name if they can see the population of the sector! */
  if (visibility & SEE_POPULATION) {
    wprintw(sectw," %s", sp->name);
  }
  
    /* Shows owner if can see owner */
  wmove(sectw, 2, 1);
  if ((visibility & SEE_OWNER) && (sp->owner!=0)) {
    wprintw(sectw,"%s-", world.nations[sp->owner].name);
  }
  
    /* Shows designation if can see designation */
  if (visibility & SEE_DESIG) {
    if (sp->owner != 0 || sp->designation != D_NODESIG) {
      wprintw(sectw, "%s", desig_map[sp->designation].name);
      if (has_bubble(sp)) {
	waddstr(sectw, "/B");
      }
      if (has_hidden(sp)) {
	waddstr(sectw, "/H");
      }
      if (has_traded(sp)) {
	waddstr(sectw, "/T");
      }
      if (has_impenetrable(sp)) {
	waddstr(sectw, "/I");
      }
      if (has_hostile(sp)) {
	waddstr(sectw, "/h");
      }
    }
  }
  
  if (visibility & SEE_LAND_WATER) {
    mvwprintw(sectw, 3, 1, "%s ", terrains[sp->terrain - MIN_TERRAIN].name);
    wprintw(sectw, "%s", altitude_map[map_alt(sp->altitude)].name);
  }

    wmove(sectw, 4, 1);    
    if (visibility & SEE_POPULATION) {
      wprintw(sectw, "%d people", sp->n_people);
      if (sp->owner != 0) {	/* print race of owner, if owner is not 0 */
	wprintw(sectw, " (%c)",	world.nations[sp->owner].race.mark);
      }
    }
  
  if (visibility & SEE_RESOURCES) {
    mvwprintw(sectw, 5, 1, "metal %d", sp->metal);
    mvwprintw(sectw, 5, 13, "jewels %d", sp->jewels);
  }
  
  if (visibility & SEE_RESOURCES) {
    mvwprintw(sectw, 6, 2, "soil %d", sp->soil);
  }
  if (visibility & SEE_LAND_WATER) {
    mvwprintw(sectw, 6, 11, "movecost %d",
	      get_generic_move_cost(&world.nations[user.id],sp));
/*	      get_move_cost(&world.nations[user.id],sp)); */
  }

  box(sectw, '|', '-');
  } /* (for future optimization) */
  wrefresh(sectw);
}

void bad_key()			/* user typed an undefined key */
{
  statline("type space to go on", "bad_key");
  while (getch() != ' ') {
  }
}

void redraw()
{
/*  user.center = user.cursor; */
  clear();
  refresh();
  user.just_moved = 1;
}

void windows()			/* user gets to manage windows */
{
  WINDOW *winw;			/* for this screen only */
  Pt new_loc;			/* new location of the window */

  statline("", "windows");
  winw = newwin(6, 26, 2, 2);
  wstandout(winw);
  mvwprintw(winw, 1, 4, "you can choose: ");
  wstandend(winw);
  mvwprintw(winw, 2, 0, "m - [m]ove sector window");
  mvwprintw(winw, 3, 0, "h - [h]ide sector window");
  mvwprintw(winw, 4, 0, "s - [s]how sector window");
  move(4,0);
  box(winw, '|', '-');
  wrefresh(winw);
  move(4,0);
  switch(getch()) {
  case 'm':
    new_loc.x = sectw->_begx;
    new_loc.y = sectw->_begy;
    mvprintw(LINES-2, 0, "starting at (%d,%d)", new_loc.x, new_loc.y);
    refresh();
      /* absolute dragging */
    new_loc = drag_cursor(new_loc, DRAG_ABS, NULL, NULL);
    mvwin(sectw, new_loc.y, new_loc.x);
    break;
  case 'h':
    user.show_sect_win = 0;
    werase(sectw);
    wrefresh(sectw);
    touchwin(stdscr);
    break;
  case 's':
    if (!user.show_sect_win) {
      show_sector(user.cursor.x, user.cursor.y);
    }
    user.show_sect_win = 1;
    break;
  default:
    break;
  }
  delwin(winw);			/* done with it */
  touch_all_wins();
/*  fflush(stdin); */
}

void touch_all_wins()	/* make sure all permanent windows get touched */
{
  touchwin(stdscr);
/*  if (user.show_sect_win) {
    touchwin(sectw);
  }
*/
  user.just_moved = 1;
}

  /* this is used in general to track the user's movements */
Pt drag_cursor(pt, flags, comment, legal)
     Pt pt;
     int flags;
     char (*comment)();
     int (*legal)();
{
  char c;
  char s[100], comment_str[100];
  Pt old_pt;
  int done = 0;

  old_pt = pt;

  statline("move the cursor; type space when done", "drag_cursor");

  if ((flags == DRAG_REL) && (user.map_style == NORMAL_MAP)) {
    wrap(&pt);			/* regural map, relative drag */
    move(pt.y-yoff(), 2*(pt.x-xoff()));
  } else if (flags == DRAG_REL) { /* compact map, but still relative */
    wrap(&pt);
    move((*wrapy)(pt.x-xoff_compact(), pt.y-yoff()),
	 (*wrapx)(pt.x-xoff_compact(),pt.y-yoff()));
  } else {			/* absolute positions */
    move(pt.y, pt.x);
  }
  refresh();
  while (((c = getch()) != ' ') && !done) {
    old_pt = pt;
    switch (c) {
    case '?':
      online_info();
      break;
    case 'h':
    case '4':
      --pt.x;
      break;
    case 'j':
    case '2':
      ++pt.y;
      break;
    case 'k':
    case '8':  
      --pt.y;
      break;
    case 'l':
    case '6':  
      ++pt.x;
      break;
    case 'y':
    case '7':
      --pt.x;
      --pt.y;
      break;
    case 'u':
    case '9':
      ++pt.x;
      --pt.y;
      break;
    case 'b':
    case '1':
      --pt.x;
      ++pt.y;
      break;
    case 'n':
    case '3':
      ++pt.x;
      ++pt.y;
      break;
    default:
      continue;
    }
    if (legal != NULL) {	/* if there *is* a legal() func... */
      wrap(&pt);
      if (!((*legal)(pt, user.np, user.current_army))) {
	beep();			/* illegal */
	pt = old_pt;
	statline2("hit space", "invalid point");
	get_space();
      }
    }
    if (flags == DRAG_REL) {	/* cludge, and ugly, since we wrap later */
      wrap(&pt);
      sprintf(s, "(%d,%d)", xrel(pt.x,pt.y,user.np->capital),
	      yrel(pt.x,pt.y,user.np->capital));
    } else {
      sprintf(s, "(%d,%d)", pt.x, pt.y);
    }
    if (comment != NULL) {
      done = comment(comment_str);
      statline2(comment_str, s);
    } else {
      statline2("", s);
    }
    switch (flags) {
    case DRAG_REL:
      wrap(&pt);
      re_center(pt.x, pt.y);
      user.just_moved = 1;
      draw_map();
/*      show_sector(pt.x, pt.y); */
      if (user.map_style == NORMAL_MAP) {
	move((*wrapy)(pt.x-xoff(),pt.y-yoff()), 2*(*wrapx)(pt.x-xoff(),pt.y-yoff()));
      } else {
	move((*wrapy)(pt.x-xoff_compact(),pt.y-yoff()), (*wrapx)(pt.x-xoff_compact(),pt.y-yoff())); /* compact map */
      }
      break;
    case DRAG_ABS:
      move(pt.y, pt.x);
      break;
    default:
      break;
    }
    refresh();
  }
  if (flags == DRAG_REL) {
    sprintf(s, "the new point is (%d, %d)", xrel(pt.x,pt.y,user.np->capital),
	    yrel(pt.x,pt.y,user.np->capital));
  } else {
    sprintf(s, "the new point is (%d, %d)", pt.x, pt.y);
  }
  statline2(s, "");
  move(LINES-2, 0);
  clrtoeol();
  refresh();
  return pt;
}

  /* this routine sees if the screen needs re-centering, and
     re-centers it if necessary.  return 1 if re-centering was done.
     return 0 if there was no need to re-center.
   */
int re_center(x, y)
     int x, y;
{
  int change = 0;
  int width, height;

  height = LINES-3;
  width = (COLS-2)-2;

  if (user.map_style == NORMAL_MAP) {
    if ((*wrapx)(x - xoff(),0) >= width/2) { /* No Y coord to add! -KM */
      user.center.x = x;	/* too much to the right */
      change = 1;
    }
    if ((*wrapx)(x - xoff(),0) <= 0) { /* No Y coord to add! -KM */
      user.center.x = x;	/* too much to the left */
      change = 1;
    }
  } else {
    if ((*wrapx)(x - xoff_compact(),0) >= width) {
      user.center.x = x;	/* too much to the right */
      change = 1;
    }
    if ((*wrapx)(x - xoff_compact(),0) <= 0) {
      user.center.x = x;	/* too much to the left */
      change = 1;
    }
  }
  if ((*wrapy)(0,y - yoff()) >= height) {
    user.center.y = y;		/* too far down */
    change = 1;
  }
  if ((*wrapy)(0,y - yoff()) <= 0) {
    user.center.y = y;		/* too far up */
    change = 1;
  }
#ifdef hpux
  if (change) {
    redraw();
  }
#endif /* hpux */
  return change;
}

  /* see if these coordinates would appear under the sector window */
int is_under_sectw(x, y)
     int x, y;
{
  int xfirst, yfirst, xlast, ylast;

  if (!user.show_sect_win) {
    return 0;
  }
  xfirst = sectw->_begx;
  yfirst = sectw->_begy;
  xlast = xfirst + SECTW_SIZE_X;
  ylast = yfirst + SECTW_SIZE_Y;
  if (x < xfirst ||  x > xlast || y < yfirst || y > ylast) {
    return 0;
  }
  return 1;
}

  /* this puts the cursor in the right place */
void set_cursor()
{
  int x = user.cursor.x, y = user.cursor.y;

  if (user.map_style == NORMAL_MAP) {
    move((*wrapy)(x-xoff(),y-yoff()), 2*(*wrapx)(x-xoff(),y-yoff()));
  } else {
    move((*wrapy)(x-xoff_compact(),y-yoff()), (*wrapx)(x-xoff_compact(),y-yoff()));
  }
  refresh();
}

  /* replacement for getch():  first checks if it is a question
     mark, and if it is it calls online_info().
   */
int mygetch()
{
  int c;
  if ((c = getch()) == '?') {
    online_info();
  }
  return c;
}

  /* gets a string str of max length len; returns 0 on failure; 1 otherwise */
int wget_string (w, rets, len)
     WINDOW * w;
     char * rets;
     int len;
{
  char s [80];
  int pos, done;
  int x, y, i;
  int oldpos;		/* Used for ^W */
  noecho ();

  if (w == NULL) {
    w = stdscr;
  }

  pos = 0;
  done = 0;

  getyx (w, y, x);
  wrefresh (w);

  while (!done) {
    s [pos] = wgetch (stdscr);
    switch (s[pos]) {
    case '\n':
    case '\r':
      s [pos] = '\0';
      done = 1;
      break;
    case '\b':
    case DEL:
      if (pos > 0) {
	pos--;
	s[pos] = '\0';
	wmove (w, y, x + pos);
	waddch (w, ' ');
	wmove (w, y, x + pos);
      }
      break;
    case CTL('U'):
      wmove (w, y, x);
      for (i=0; i < pos; i++) {
	waddch (w, ' ');
      }
      wmove (w, y, x);
      pos = 0;
      s [pos] = '\0';
      break;
    case CTL('W'):
      oldpos = pos;
      while (pos != 0 && s[pos] == ' ') {
	pos --;
      }
      if (pos) {
	pos --;
      }
      while (pos != 0 && s[pos] != ' ') {
	pos --;
      }
      wmove (w, y, x + pos);
      while (oldpos != pos) {
	oldpos--;
	waddch (w, ' ');
      }      
      wmove (w, y, x + pos);
      break;
    default:
      waddch (w, s [pos]);
      pos++;
      break;
    }
    wrefresh (w);
  }

  if (pos == 0) {
    return 0;
  }
  s [len-1] = '\0';
  strcpy (rets, s); 
  return 1;
}

/* gets a number from window w; returns 1 if all OK; -1 otherwise */
int wget_number (w, num)
     WINDOW * w;
     int * num;
{
  char s [80];
  int pos, done;
  int x, y, i;
  noecho ();

  if (w == NULL) {
    w = stdscr;
  }

  pos = 0;
  done = 0;

  getyx (w, y, x);
  wrefresh (w);

  while (!done) {
    s [pos] = wgetch (stdscr);
    switch (s[pos]) {
    case '\n':
    case '\r':
      done = 1;
      break;
    case '\b':
    case DEL:
      if (pos > 0) {
	pos--;
	s[pos] = '\0';
	wmove (w, y, x + pos);
	waddch (w, ' ');
	wmove (w, y, x + pos);
      }
      break;
    case CTL('U'):
      wmove (w, y, x);
      for (i=0; i < pos; i++) {
	waddch (w, ' ');
      }
      wmove (w, y, x);
      pos = 0;
      s [pos] = '\0';
      break;
    default:
      waddch (w, s [pos]);
      pos++;
      break;
    }
    wrefresh (w);
  }
  if (pos == 0) {
    return -1;
  }
  if ((sscanf(s, "%d", num)) < 1) {
    return -1;
  }
  return 1;
}

int wget_name (w, name)
     WINDOW * w;
     char * name;
{
  char s[80];
  int pos, done;
  int x, y, i;
  int oldpos;		/* Used for ^W */
  noecho();

  if (w == NULL) {
    w = stdscr;
  }

  pos = 0;
  done = 0;

  getyx (w, y, x);
  wrefresh (w);

  while (!done) {
    s [pos] = wgetch (stdscr);
    switch (s[pos]) {
    case '\n':
    case '\r':
      s [pos] = '\0';
      done = 1;
      break;
    case '\b':
    case DEL:
      if (pos > 0) {
	pos--;
	s[pos] = '\0';
	wmove (w, y, x + pos);
	waddch (w, ' ');
	wmove (w, y, x + pos);
      }
      break;
    case CTL('U'):
      wmove (w, y, x);
      for (i=0; i < pos; i++) {
	waddch (w, ' ');
      }
      wmove (w, y, x);
      pos = 0;
      s [pos] = '\0';
      break;
    case CTL('W'):
      oldpos = pos;
      while (pos != 0 && s[pos] == ' ') {
	pos --;
      }
      if (pos) {
	pos --;
      }
      while (pos != 0 && s[pos] != ' ') {
	pos --;
      }
      wmove (w, y, x + pos);
      while (oldpos != pos) {
	oldpos--;
	waddch (w, ' ');
      }      
      wmove (w, y, x + pos);
      break;
    default:
      waddch (w, s [pos]);
      pos++;
      break;
    }
    wrefresh (w);
  }

  if (pos == 0) {
    return -1;
  }
  strcpy (name, s); 
  return 1;
}

#ifdef PMAX  /* Fix for the mvwprintw bug in pmax curses */
#undef mvwprintw
#include <varargs.h>

int mymvwprintw(va_alist)
     va_dcl
{
    va_list ap;
    reg WINDOW	*win;
    reg int		y, x;
    char		*fmt;
    char	buf[512];

    va_start(ap);
    win = va_arg(ap, WINDOW *);
    y = va_arg(ap, int);
    x = va_arg(ap, int);
    fmt = va_arg(ap, char *);
    
    if (wmove(win,y, x) != OK)
      return ERR;
    (void) vsprintf(buf, fmt, ap);
    va_end(ap);
    return waddstr(win, buf);
}
#endif /* PMAX */
