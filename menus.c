  /* menus.c -- various menus for dominion */

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

#include <stdio.h>
#include <ctype.h>
#ifdef SYSV
# include <string.h>
#else
# include <strings.h>
#endif /* SYSV */

#include "dominion.h"
#include "misc.h"
#include "army.h"
#include "functions.h"

extern Sworld world;
extern Suser user;
  /* info about all spirits */
extern struct spirit_type *spirit_types;
extern char help_tag[];

  /* asks the user what the display should look like */
void display_menu()
{
  WINDOW *dispw;
  char c;

  strcpy(help_tag, "Display options");
  if (user.xmode) {
    statline("[r,c,n,d,p,A,w,M,t,T,s,m,j,h,o,a,y,O,l,u,-,C,W]", "display_menu");
  } else {
    statline("choose an option (space to exit)", "display menu");
    dispw = newwin(9, COLS-4, LINES-11, 2);
    wmove(dispw, 1, 1);
    waddstr(dispw, "Map Style:         [r]egular, [c]ompact");
    wmove(dispw, 2, 1);
    waddstr(dispw, "Display Options:   ");
    waddstr(dispw, "[n]ation mark, [d]esignation, [p]opulation,");
    wmove(dispw, 3, 1);
    waddstr(dispw, "                   ");
    waddstr(dispw, "[A]ltitude, [w] Climate, army [M]ovecost, [t]errain,");
    wmove(dispw, 4, 1);
    waddstr(dispw, "                   ");
    waddstr(dispw, "[s]oil, [m]etal, [j]ewels, [T]errain move cost");
    wmove(dispw, 5, 1);
    waddstr(dispw, "Highlight Options: ");
    waddstr(dispw, "[o]wnership, [a]rmies, [y]our armies, [O]ther armies,");
    wmove(dispw, 6, 1);
    waddstr(dispw, "                   ");
    waddstr(dispw, "[l] move left, [h]ostile, [u]nemployment, [-]none");
    wmove(dispw, 7, 1);
    waddstr(dispw, "Other Things:      ");
    waddstr(dispw, "[C]enter screen at cursor, [W]ater/underwater toggle");
    box(dispw, '|', '-');
    wrefresh(dispw);
  }
  switch (c = mygetch()) {
  case ' ':
    break;
  case 'h':
    user.highlight = H_HOSTILE;
    break;
  case 'M':
    user.display = ARMY_MOVECOST;
    break;
  case 'r':
    user.map_style = NORMAL_MAP; /* map style */
    wclear(stdscr);
    break;
  case 'c':
    user.map_style = COMPACT_MAP;
    wclear(stdscr);
    break;
  case 'd':
    user.display = DESIGNATION; /* display */
    break;
  case 'n':
    user.display = NATION_MARK;
    break;
  case 's':
    user.display = SOIL;
    break;
  case 'm':
    user.display = METAL;
    break;
  case 'j':
    user.display = JEWELS;
    break;
  case 'A':
    user.display = ALTITUDE;
    break;
  case 'w':
    user.display = CLIMATE;
    break;
  case 'p':
    user.display = POPULATION;
    break;
  case 'T':
    user.display = MOVECOST;
    break;
  case 't':
    user.display = TERRAIN;
    break;
  case 'o':
    user.highlight = H_OWNED;	/* highlighting */
    break;
  case 'a':
    user.highlight = H_ARMIES;
    break;
  case 'y':
    user.highlight = H_YOUR_ARMIES;
    break;
  case 'O':
    user.highlight = H_OTHER_ARMIES;
    break;
  case 'l':
    user.highlight = H_MOVE_LEFT;
    break;
  case 'u':
    user.highlight = H_UNEMP;
    break;
  case '-':			/* no highlighting */
    user.highlight = H_NONE;
    break;
  case 'C':			/* center screen around current cursor */
    user.center = user.cursor;
#ifdef hpux			/* HP curses is baaad */
    redraw();
#endif /* hpux */
    break;
  case 'W':			/* above/below water toggle */
    user.underwater = !user.underwater;
    break;
  default:
    bad_key();
    break;
  }
  if (!user.xmode) {
    delwin(dispw);
  }
  touchwin(stdscr);
  user.just_moved = 1;
}

  /* wizardry commands */
void wizardry_menu()
{
  WINDOW *wizw;
  char c;
  int done = 0;
  char s[PATHLEN];
  Snation *np = user.np;
  Ssector *sp = &world.map[user.cursor.x][user.cursor.y];

  if (user.xmode) {
    wizw = (WINDOW *) NULL;
  } else {
    wizw = newwin(6, COLS-4, LINES-8, 2);
  }
    /* now see if we can automatically select the first mage
       on this sector.
     */
  if (first_sect_mage_id(user.np, sp) != -1) {
    user.current_army = first_sect_mage_id(user.np, sp);
    show_armies(sp);
  }
  while (!done) {
    strcpy(help_tag, "Wizardry");
    if (user.xmode) {
      sprintf(s, "%d spell pts. [l,c,s,h,i]", np->spell_pts);
      statline(s, "wizardry_menu");
    } else {
      statline("Choose a wizardry command (space to leave)","wizardry_menu");
      mvwaddstr(wizw, 1, 1,
	"  [l]ist spells and spirits, [c]ast a spell, [s]ummon a spirit");
      mvwaddstr(wizw, 2, 1, "  list [h]anging spells, [i]nitiate a mage");
      mvwprintw(wizw, 3, 1, "    You have %d spell points", np->spell_pts);
      box(wizw, '|', '-');
      wrefresh(wizw);
    }
    switch (c = getch()) {
    case ' ':
      done = 1;
      break;
    case 'l':
      if (!user.xmode) {
	wmove(wizw, 2, 1);
	wprintw(wizw,
		"  listing spells and spirits for nation %s (order of %s)",
		user.np->name, user.np->mag_order);
      }
      list_spells(&user);
      list_spirits(&user);
      statline2_err("Type space to get back", "spells and spirits list");
      touchwin(stdscr);
      refresh();
      fflush(stdin);
      user.just_moved = 1;
      break;
    case 's':
      if (!user.xmode) {
	list_spirits(&user);
      }
      summon(&user, wizw);
      break;
    case 'c':
      if (!user.xmode) {
	list_spells(&user);
      }
      cast_spell(&user, wizw);
      break;
    case 'i':
      initiate_mage(&user, wizw);
      break;
    case 'h':
      show_hanging_spells(&user);
      break;
    case '?':
      online_info();
      break;
    default:
      break;
    }
    statline2("", "");
    if (!user.xmode) {
      wmove(wizw, 2, 1);
      wclrtobot(wizw);
    }
  }
  if (!user.xmode) {
    touchwin(stdscr);
    delwin(wizw);
    user.just_moved = 1;
  }
}

  /* conjures a spirit */
void summon(up, w)
     Suser *up;
     WINDOW *w;
{
  char type[NAMELEN], name[NAMELEN];
  int type_index;
  Sspirit *spiritp = user.spirit_list;
  Sarmy *ap, *get_army();
  Ssector *sp = &world.map[up->cursor.x][up->cursor.y];

  strcpy(help_tag, "Spirit types");

  if ((ap = get_army(up->np, up->current_army)) == NULL  ||  !is_mage(ap) ) {
    statline2_err("only mages can summon. hit space.", "summon");
    return;
  }
  if (sp->owner != up->np->id) {
    statline2_err("must summon in your own land", "summon");
    return;
  }
  if (w) {
    mvwprintw(w, 2, 1, "Which type of spirit do you wish to summon? ");
  } else {
    statline_prompt("Which type? ", "summon");
  }
  wget_name(w, type);
  if ((type_index = spirit_type_index(type)) < 0) {
    statline2_err("couldn't find a spirit of that type. hit space.", "summon");
    return;
  }
    /* get the spirit pointer from the list of available spirits */
  while (spiritp && (strcmp(type, spiritp->type) != 0)) {
    spiritp = spiritp->next;
  }
  if (spiritp == NULL) {
    statline2_err("That is not available to you. hit space.", "summon");
    return;
  }
    /* now see if they can afford it */
  if (spiritp->cost > up->np->spell_pts) {
    statline2_err("you do not have enough spell points. hit space.", "summon");
    return;
  }
  if (w) {
    mvwprintw(w, 3, 1, "Give a name to your spirit (default %s: ", type);
  } else {
    statline("Give name: ", "summon");
    move(LINES-1, strlen("Give name: "));
  }
  if (wget_name(w, name) <= 0) {
    sprintf(name, "%s", type);
  }
  exec_summon(type_index, name);
  statline2_err("spirit has been summoned, hit space to get back", "summon");
}

  /* initiates a mage */
void initiate_mage(up, w)
     Suser *up;
     WINDOW *w;
{
  char c;
  char name[NAMELEN], s[NAMELEN];
  Ssector *sp;
  char def_name [NAMELEN];

  strcpy(help_tag, "Mages");
  sp = &world.map[user.cursor.x][user.cursor.y];
  if ((sp->designation != D_CITY && sp->designation != D_UNIVERSITY
      && sp->designation != D_TEMPLE && sp->designation != D_CAPITAL)
      || (sp->owner != user.id)) {
    statline2_err("must initiate in *your* city/univ/temple. hit space.",
	      "initiate_mage");
    return;
  }
  name[0] = '\0';
  if (w) {
    mvwprintw(w, 3, 1,
	      "initiation costs %d jewels.  go ahead (y/n)? ",
	      INITIATION_JEWELS);
    wrefresh(w);
  } else {
    sprintf(s, "costs %d jewels.  go ahead (y/n)? ", INITIATION_JEWELS);
    statline(s, "initiate");
    move(LINES-1, strlen(s));
  }
  c = getchar();
  if (c == '?') {
    online_info();
  }
  if (c != 'y' && c != 'y') {
    return;
  }
  if (up->np->jewels < INITIATION_JEWELS) {
    statline2_err("You do not have enough jewels. hit space.","initiate_mage");
    return;
  }
  if (next_thon_jewels (up->np) - MAGE_JEWELS_MAINT < 0) {
    statline2_err ("You would have negative jewels next thon.  hit space.",
		   "initiate_mage");
    return;
  }

  sprintf (def_name, "Mage");
  sprintf (s, "Name (default \"%s\"): ", def_name);

  if (w) {
    mvwprintw(w, 4, 1,"Give a name to your new mage (default \"%s\"): ",
	      def_name);
  } else {
    statline(s, "initiate");
    move(LINES-1,strlen (s));
  }

  // TODO wget_name in cur_stuff.c only returns 1 or -1...  ???
  // ORIG: if ((wget_name(w, name)) == NULL) {
  if ((wget_name(w, name)) == -1) {
    strcpy (name, def_name);
  }
    /* if we got this far, it means that the initiation is valid */
  exec_initiate(name);
  statline2_err("mage has been initiated, hit space to get back",
		"initiate_mage");
}

  /* list a user's spells */
void list_spells(up)
     Suser *up;
{
  Sspell *spells = up->spell_list;
  WINDOW *listw;
  int i, n_spells = 0;

  while (spells != NULL) {
    spells = spells->next;
    ++n_spells;
  }
  listw = newwin(n_spells+3, NAMELEN, 2, 37);
  box(listw, '|', '-');
  wstandout(listw);
  mvwaddstr(listw, 0, 2, "Spell");
  wstandend(listw);
  waddstr(listw, "(cost,time)");
  spells = up->spell_list;
  for (i = 0; i < n_spells; ++i) {
    mvwprintw(listw, i+2, 1, "%s(%d,%d)",
	      spells->name, spells->cost, spells->duration);
    spells = spells->next;
  }
  wrefresh(listw);
  delwin(listw);
}

  /* list a user's spirits */
void list_spirits(up)
     Suser *up;
{
  Sspirit *spirits = up->spirit_list;
  int n_spirits = 0, i;
  WINDOW *listw;

  while (spirits != NULL) {
    spirits = spirits->next;
    ++n_spirits;
  }
  listw = newwin(n_spirits+3, NAMELEN, 2, 7);
  box(listw, '|', '-');
  wstandout(listw);
  mvwaddstr(listw, 0, 4, "Spirit");
  wstandend(listw);
  waddstr(listw, "(cost)");
  spirits = up->spirit_list;
    /* run through the list and print them out */
  for (i = 0; i < n_spirits; ++i) {
    if (spirit_type_index(spirits->type) >= 0) {
      mvwprintw(listw, i+2, 1, "%s(%d)", spirits->type, spirits->cost);
    }
    spirits = spirits->next;
  }
  wrefresh(listw);
  delwin(listw);
}

void cast_spell(up, w)
     Suser *up;
     WINDOW *w;
{
  Sspell *slist = up->spell_list;
  char name[NAMELEN], s[NAMELEN];
  Sarmy *ap, *get_army();
  int cost;

  if ((ap = get_army(up->np, up->current_army)) == NULL  ||  !is_mage(ap) ) {
    statline2_err("only mages can cast spells. hit space.", "cast_spell");
    return;
  }
  if (w) {
    mvwprintw(w, 4, 1, "which spell do you want to cast? ");
  } else {
    statline_prompt("Which spell? ", "cast_spell");
  }
  wget_name(w, name);
  while (slist != NULL) {
    if (strcmp(slist->name, name) == 0) {
      break;			/* found it */
    }
    slist = slist->next;
  }
  if (slist == NULL) {
    statline2_err("no spell by that name. hit space.", "cast_spell");
    return;
  }
  if (slist->cost > 0 && slist->cost > up->np->spell_pts) {
    statline2_err("you do not have enough spell points. hit space.",
		  "cast_spell");
    return;
  }
    /* if we got this far, it means that the spell is valid */
  cost = exec_spell(slist, w);
  show_armies(&world.map[up->cursor.x][up->cursor.y]);
  if (cost > 0) {
    statline2_err("spell has been cast, hit space to get back", "cast_spell");
  }
}

  /* this sets up the spirit as a new army */
void exec_summon(type_index, name)
     int type_index;
     char name[];
{
  Sarmy army, make_army();
  Ssector *sp = &world.map[user.cursor.x][user.cursor.y];
  char s[EXECLEN];

  army = make_army(spirit_types[type_index].type, name,
		   spirit_types[type_index].size, A_DEFEND,
		   user.np->id, sp->loc);
  army.id = free_army_id(user.np);
   /* spirits cost spell points to draft *and* maintain (set in make_army) */
  army.next = NULL;
  army.flags = spirit_types[type_index].flags;
    /* now insert it into the list */
  ++user.np->n_armies;
  if (user.np->armies == NULL) { /* special case:  empty list */
    user.np->armies = (Sarmy *) malloc(sizeof(Sarmy));
    *(user.np->armies) = army;
    user.np->armies->next = NULL;
  } else {
    insert_army_nation(user.np, &army, -1);
  }
  insert_army_sector(sp, &army);
  user.np->spell_pts -= spirit_types[type_index].spell_pts_draft;
  cspell_pts(user.np, -spirit_types[type_index].spell_pts_draft);
    /* now prepare the exec string for making the army and costs */
  sprintf(s, "AMAKE:%d:%d:%d:%d:%s:%s\n", army.id, army.n_soldiers,
	  army.pos.x, army.pos.y, army.type, army.name);
  gen_exec(s);
    /* now a last detail:  the "current_army" variable
       must be set (like in draft_army())
     */
  if (user.current_army == -1) {
    user.current_army = first_sect_army(sp);
  }
  show_armies(sp);
}

  /* this sets up the mage as a new army */
void exec_initiate(name)
     char name[];
{
  Sarmy army, make_army();
  Ssector *sp = &world.map[user.cursor.x][user.cursor.y];
  char s[EXECLEN];

  army = make_army("Mage", name, 1, A_DEFEND,
		   user.np->id, sp->loc);
    /* now give the mage maintainance costs */
  army.jewel_maint = 1000;
/*  army.spell_pts_maint = 1; */
  army.id = free_army_id(user.np);
  army.flags |= AF_WIZARD;
  army.next = NULL;
    /* now insert it into the list */
  ++user.np->n_armies;
  if (user.np->armies == NULL) { /* special case:  empty list */
    user.np->armies = (Sarmy *) malloc(sizeof(Sarmy));
    *(user.np->armies) = army;
    user.np->armies->next = NULL;
  } else {
    insert_army_nation(user.np, &army, -1);
  }
  insert_army_sector(sp, &army);
  user.np->jewels -= INITIATION_JEWELS;
  cjewels(user.np, -INITIATION_JEWELS);
    /* now prepare the exec string for making the army and costs */
  sprintf(s, "AMAKE:%d:%d:%d:%d:%s:%s\n", army.id, army.n_soldiers,
	  army.pos.x, army.pos.y, army.type, army.name);
  gen_exec(s);
  aflag_set(&army, AF_WIZARD);
/*  sprintf(s, "%d:%d", army.id, AF_WIZARD); */
/*  aflag_set(&army, AF_NOFIGHT); */
    /* now a last detail:  the "current_army" var must be set
       (like in draft_army())
     */
  if (user.current_army == -1) {
    user.current_army = first_sect_army(sp);
  }
  show_armies(sp);
}

  /* make a window to show user her/his hanging spells */
void show_hanging_spells(up)
     Suser *up;
{
  WINDOW *sw;
  int done = 0, i;
  Sh_spell *h_list;		/* list of hanging spells for this user */

  strcpy(help_tag, "Spells");
  statline("Hit space to get back", "show_hanging_spells");
  sw = newwin(20, 30, 2, 2);
  while (!done) {
    mvwprintw(sw, 1, 1, "     Spell       thons");
    wclrtobot(sw);
    if (up->id == 0) { waddstr(sw, " (nat)"); }
    mvwprintw(sw, 2, 1, "   --------       --- ");
    if (up->id == 0) { waddstr(sw, "  --- "); }
    for (i = 0, h_list = up->h_spells; h_list != NULL;
	 ++i,h_list = h_list->next) {
      mvwprintw(sw, 3+i, 1, " %2d %-12.12s  %2d", i, h_list->name,
		h_list->thons_left);
      if (up->id == 0) { wprintw(sw, "    %3d", h_list->nat_id); }
      wclrtoeol(sw);
    }
    mvwaddstr(sw, 18, 1, " [z]oom, [d]elete ");
    wclrtoeol(sw);
    box(sw, '|', '-');
    wrefresh(sw);
    switch(getch()) {
    case 'z':
      zoom_on_h_spell(up->h_spells, sw);
      break;
    case 'd':
      zoom_del_h_spell(up->h_spells, sw);
      break;

    case ' ':
      done = 1;
      break;
    case '?':
      online_info();
      break;
    default:
      break;
    }
  }
  delwin(sw);
  touch_all_wins();
}

  /* allows the user to focus on a specific spell */
int zoom_on_h_spell(h_list, sw)
     Sh_spell *h_list;
     WINDOW *sw;
{
  WINDOW *zoomw;
  int n, i;
  char s[EXECLEN];

    /* first ask the user which spell s/he wants to see */
  mvwaddstr(sw, 18, 1, " On which spell? ");
  wclrtoeol(sw);
  box(sw, '|', '-');
  wrefresh(sw);
  if (wget_number(sw, &n) < 0 || n < 0) {
    return -1;
  }
  for (i = 0; h_list && i < n; ++i) {
    h_list = h_list->next;
  }
  if (h_list == NULL) {
    return -1;
  }
    /* then zoom in on it */
  zoomw = newwin(h_list->n_lines/2+3, 30, 4, 25);
  mvwprintw(zoomw, 1, 3, " Spell <%s>  ", h_list->name);
  for (i = 0; i < h_list->n_lines; i += 2) {
      /* now insert the hiding of coordinates */
    strcpy(s, h_list->lines[i]);
    fix_sector_line(h_list->lines[i], s);
    mvwprintw(zoomw, i/2+2, 1, " %s", s);
  }
  box(zoomw, '|', '-');
  wrefresh(zoomw);
  statline2_err("Hit space to return", "zoom_on_h_spell");
  delwin(zoomw);
  return 1;
}

  /* fix the exec line if it has references to a sector by
     absolute coordinates.  put the "censored" line into s.
     this is quite a hack, what?
   */
void fix_sector_line(line, s)
     char line[], s[];
{
  int xabs, yabs, x_rel, y_rel, arg;
  char *s2, s3[EXECLEN];	/* for temporary work */

    /* if there is no '_', then just return */
  if ((s2 = strrchr(line, '_')) == NULL) {
    strcpy(s, line);
    return;
  }
    /* the convention is, if the exec line ends with "_SECTOR"
       then the first 2 numbers are the x and y coordinates.
     */
  if (strncmp(s2+1, "SECTOR", strlen("SECTOR")) != 0) {
    strcpy(s, line);
    return;
  }
    /* for now make the gross assumption that
       the only arguments are x, y and a third "arg"
     */
  s2 = line;
  s2 = strchr(s2, ':')+1;
  sscanf(s2, "%d", &xabs);
  s2 = strchr(s2, ':')+1;
  sscanf(s2, "%d", &yabs);
  s2 = strchr(s2, ':')+1;
  sscanf(s2, "%d", &arg);
  if (user.id == 0) {
    x_rel = xabs;
    y_rel = yabs;
  } else {
    x_rel = xrel(xabs, yabs, user.np->capital);
    y_rel = yrel(xabs, yabs, user.np->capital);
  }
  /*    sprintf(s3, "%d:%d:%d\n", xabs, yabs, arg); (debug) */
  sprintf(s3, ":%d:%d:%d\n", x_rel, y_rel, arg);
  strcpy(strchr(s, ':'), s3);
  /*    statline(s, "final s, hit space"); */
}

int zoom_del_h_spell(h_list, sw)
     Sh_spell *h_list;
     WINDOW *sw;
{
  WINDOW *zoomw;
  int n, i;
  char s[EXECLEN];

    /* first ask the user which spell s/he wants to see */
  mvwaddstr(sw, 18, 1, " Remove which spell? ");
  wclrtoeol(sw);
  box(sw, '|', '-');
  wrefresh(sw);
  if (wget_number(sw, &n) < 0 || n < 0) {
    return -1;
  }
  for (i = 0; h_list && i < n; ++i) {
    h_list = h_list->next;
  }
  if (h_list == NULL) {
    return -1;
  }

  delete_hanging_spell(h_list);
  return 1;
}

