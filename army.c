  /* army.c -- visual army stuff;  see also armylib.c */

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
#include "cur_stuff.h"
#include "functions.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern Sworld world;
extern Suser user;
extern char help_tag[];

int legal_move();
Sarmy *get_army();
extern struct army_type *army_types;
extern struct army_flags army_flags [];

  /* this shows the armies inside the current sector.
     the info is put in the the main window.
   */
void show_armies(sp)
     Ssector *sp;
{
  Sarmy *ap;
  Snation *np;
  struct armyid *alist;
    /* we need to know how many are hidden, for display purposes */
  int i, n_hidden = 0, n_armies;
  char s[NAMELEN];
  char apparent_type[NAMELEN];	/* if an army is disguised */
  int visibility = user.visible_sectors[sp->loc.x][sp->loc.y];
  int n_shown, first_shown = 0, current_index; /* to handle scrolling */
  Sarmy **army_array;

  if (user.current_army == -1) {
    user.current_army = first_sect_army(sp);
  }

  if (!(visibility & SEE_ARMIES)) {
    user.last_n_armies = 0;
    return;
  }

    /* we use this array to store all pointers, because it is useful
       to access the armies as an array for the scrolling display
     */
  alist = sp->alist;
  current_index = 0;
  n_armies = sect_n_armies(sp);
  army_array = (Sarmy **) malloc(n_armies * sizeof(Sarmy *));
  for (i = 0; i < n_armies; ++i) {
    np = &world.nations[alist->owner];
    ap = get_army(np, alist->id);
    army_array[i] = ap;
      /* we want to know which the current army is,
	 so that we can highlight it.
       */
    if (ap->id == user.current_army && ap->owner == user.id) {
      current_index = i;
    }
    alist = alist->next;
  }

    /* cycle through armies and show them */
  alist = sp->alist;
  n_shown = min(7, n_armies);
  if ((n_armies - current_index) <= n_shown/2) { /* are we near end of list */
    first_shown = n_armies - n_shown;
  } else {
    first_shown = max(current_index-n_shown/2, 0);
  }
  for (i = 0; i < n_shown; ++i) {
    ap = army_array[i+first_shown];
    if ((ap->owner == user.id) && (ap->id == user.current_army)) {
      standout();
    }
    if (ap->owner == user.id || user.id == 0) {	/* your armies */
      if (ap->id == user.current_army) {
	mvprintw(2*(i-n_hidden), ARMYW_X,
		 ">%.10s:%d %.8s", world.nations[ap->owner].name,
		 ap->n_soldiers, ap->type);
      } else {
	mvprintw(2*(i-n_hidden), ARMYW_X,
		 " %.10s:%d %.8s", world.nations[ap->owner].name,
		 ap->n_soldiers, ap->type);
      }
      clrtoeol();
    } else if (!is_hidden(ap)) { /* other people's armies */
      get_apparent_type(ap, apparent_type);
      mvprintw(2*(i-n_hidden), ARMYW_X,
	       " %.10s:%d %.8s", world.nations[ap->owner].name,
	       ap->n_soldiers, apparent_type);
    } else {			/* is hidden */
      ++n_hidden;
    }
    clrtoeol();
      /* we have printed the first army description
	 line; now print the other one.
       */
    if (ap->owner == user.id || user.id == 0) {
      get_army_status(ap, s);
      mvprintw(2*(i-n_hidden)+1, ARMYW_X,
	       " #%d; mv=%d;%s", ap->id, ap->mvpts, s);
    } else {
      move(2*(i-n_hidden)+1, ARMYW_X);
    }
      /* tricky line;  if it is there it causes this bug
	 with hidden armies; I try only clearing if the
	 army is not hidden.  that might solve the problem,
	 but still cause trouble if the army is hidden and
	 belongs to me.  I will test it.
       */
    if (!(is_hidden(ap) && ap->owner != user.id)) {
      clrtoeol();		/* do it for non-enemy hidden armies */
    }
    if ((ap->owner == user.id) && (ap->id == user.current_army)) {
      standend();
    }
    alist = alist->next;
  }
  if (n_shown < user.last_n_armies) {
    for (i = n_armies-n_hidden; i < user.last_n_armies; i++) {
      move(2*i, ARMYW_X);
      clrtoeol();
      move(2*i+1, ARMYW_X);
      clrtoeol();
    }
  }
  user.last_n_armies = n_shown;	/* n_armies - n_hidden; */
  free(army_array);
}

  /* this is the general army menu, which allows
     you to choose an army action.
   */
void army_menu()
{
  WINDOW *aw;
  int x, y;
  char c;
  int done = 0;
  Ssector *sp;			/* current sector */
  Sarmy *ap;

  x = user.cursor.x;
  y = user.cursor.y;
  sp = &world.map[x][y];

  if (!user.xmode) {
    aw = newwin(8, COLS-2, LINES-10, 1);
  }
  while (!done) {
    if (user.xmode) {
      statline("(d,D,E,n,p,N,P,m,s,t,l,z,Z,-,+)", "army_menu");
    } else {
      statline("Choose the army action you want (space to exit)", "army_menu");
      mvwaddstr(aw, 1, 1,
"  Choose Army: [n]ext, [p]revious, [N]/[P] (absolute), [j]ump to #");
      mvwaddstr(aw, 2, 1,
" Command Army: [m]ove, change [s]tatus, [t]ransport, [D]isband army");
      mvwaddstr(aw, 3, 1,
"    Army Info: [l]ist available army types,[z]oom,[Z] global zoom,[E]xamine");
      mvwaddstr(aw, 4, 1,
"Army Creation: [d]raft troops, [-]split army, [+]merge armies");
      box(aw, '|', '-');
      wrefresh(aw);
    }
    set_cursor();
    strcpy(help_tag, "Diplomacy and war");
    switch (c = mygetch()) {
    case ' ':
      done = 1;
      break;
    case 'j':
      done = jarmy ();
      break;
    case 'm':
      if (!user.xmode) {
	werase(aw);
	touch_all_wins();
      }
      critical();		/* make sure people don't exit while moving */
      move_army(user.current_army, user.xmode ? NULL : aw);
      noncritical();		/* now they can exit:  move is saved */
      break;
    case 't':			/* allow access to the transp. menu */
      transport();
      break;
    case 'p':
      previous_army();		/* pick the previous army */
      if (user.display == ARMY_MOVECOST) {
	draw_map ();
      }
      break;
    case 'n':
      next_army();		/* pick the next army */
      if (user.display == ARMY_MOVECOST) {
	draw_map ();
      }
      break;
    case 'N':			/* pick the next army, no matter where */
      if ((ap=get_army(user.np,next_nation_army(user.np, user.current_army)))) {
	user.current_army = ap->id;
	user.cursor = ap->pos;
	just_moved();
	re_center(ap->pos.x, ap->pos.y);
	draw_map();
      } else {
	user.current_army = -1;
      }
      break;
    case 'P':
      if ((ap=get_army(user.np,prev_nation_army(user.np, user.current_army)))) {
	user.current_army = ap->id;
	user.cursor = ap->pos;
	just_moved();
	re_center(ap->pos.x, ap->pos.y);
	draw_map();
      } else {
	user.current_army = -1;
      }
      break;
    case 's':
      change_army_status(user.xmode ? NULL : aw, user.current_army);
      break;
    case 'd':
      list_available_armies(&user, user.xmode ? NULL : aw);
      draft_army(user.np);
      if (user.current_army == -1) {
        user.current_army = first_sect_army(sp);
      }
      break;
    case 'D':
      if ((ap = get_army(user.np, user.current_army)) == NULL) {
	return;
      }
      army_disband (sp, ap);
      break;
    case 'E':
      if ((ap = get_army(user.np, user.current_army)) == NULL) {
	return;
      }
      army_examine(ap);
      break;
    case 'l':
        /* if we pass NULL, it means we are in xmode */
      list_available_armies(&user, user.xmode ? NULL : aw);
      get_space();
      break;
    case 'z':
      zoom_armies(&user, sp);
      break;
    case 'Z':
      zoom_armies(&user, NULL);
                     /* with a null pointer for sector... */
      break;
    case '+':
      ap = get_army (user.np, user.current_army);
      if (ap == NULL) {
	break;
      }
      army_merge (ap);
      break;
    case '-':
      ap = get_army (user.np, user.current_army);
      if (ap == NULL) {
	break;
      }
      army_split (ap);
      break;
    default:
      break;
    }
    ap = get_army(user.np,user.current_army);
    if (ap != NULL) {
      sp = &world.map[ap->pos.x][ap->pos.y];
    }
    if (!user.xmode) {
      wmove(aw, 3, 1);
      wclrtobot(aw);
      touchwin(aw);
    }
    statline2("", "");
    show_armies(sp);
    refresh();
  }
  if (!user.xmode) {
    delwin(aw);
    touch_all_wins();
  }
}

/* Merge selected army with another army */

int army_merge (ap)
Sarmy * ap;
{
  struct argument args[N_EXEC_ARGS];
  Sarmy * ap2;
  Ssector * sp;
  char stmp [EXECLEN];
  int army_num;

  sp = &world.map[user.cursor.x][user.cursor.y];

  sprintf (stmp, "Merge army %d into army #? ", ap->id);
  statline2_prompt (stmp, "army_merge");
  if (wget_number (stdscr, &army_num) <= 0) {
    statline2 ("", "");
    return 0;
  }
  ap2 = get_army (user.np, army_num);
  if (ap2 == NULL) {
    statline2 ("", "");
    return 0;
  }
  if (is_in_transport(ap) || is_in_transport (ap2)) {
    statline2_err("Hit space to get back", "Army is in transport");
    statline2 ("", "");
    return 0;
  }
  if (cargo_not_empty(&ap->cargo) || cargo_not_empty (&ap2->cargo)) {
    statline2_err("Hit space to get back", "Army has a cargo");
    statline2 ("", "");
    return 0;
  }
  if (is_spelled(ap) || is_spelled(ap2))
  {
    statline2_err("Hit space to get back", "Army is under a Spell");
    statline2 ("", "");
    return 0;
  }     
  if (ap2->pos.x != sp->loc.x || ap2->pos.y != sp->loc.y ||
      strcmp(ap->type,ap2->type) != 0 || ap == ap2) {
    statline2 ("", "");
    return 0;
  }
  sprintf(stmp, "AMERGE:%d:%d\n", ap->id, ap2->id);
  gen_exec(stmp);
    /* have the exec routine do the work!! */
  parse_exec_line(stmp,args);
  run_exec_line(user.np,args);
    /* make sure there is a current army selected */
  user.current_army = ap2->id;
  return 1;
}

int army_split (ap)
Sarmy * ap;
{
  struct argument args[N_EXEC_ARGS];
  Ssector * sp;
  char stmp [EXECLEN];
  int num_troops;

  sp = &world.map[user.cursor.x][user.cursor.y];

  sprintf (stmp, "Split how many units from army %d? ", ap->id);

  statline2_prompt (stmp, "army_split");
  if (wget_number (stdscr, &num_troops) <= 0) {
    statline2 ("", "");
    return 0;
  }
  if (num_troops < 1 || num_troops > ap->n_soldiers - 1) {
    statline2 ("", "");
    return 0;
  }    
  if (is_in_transport(ap)) {
    statline2_err("Hit space to get back", "Army is in transport");
    statline2 ("", "");
    return 0;
  }
  if (cargo_not_empty(&ap->cargo)) {
    statline2_err("Hit space to get back", "Army has a cargo");
    statline2 ("", "");
    return 0;
  }
  if (is_spirit (ap)) {
    statline2_err ("Hit space to get back", "Cannot split spirits");
    statline2 ("", "");
    return 0;
  }
  if (is_spelled(ap))
  {
    statline2_err("Hit space to get back", "Army is under a Spell");
    statline2 ("", "");
    return 0;
  }     
  sprintf(stmp, "ASPLIT:%d:%d\n", ap->id, num_troops);
  gen_exec(stmp);
    /* have the exec routine do the work!! */
  parse_exec_line(stmp,args);
  run_exec_line(user.np,args);
    /* make sure there is a current army selected */
  return 1;
}

  /* allow the user to move an army */
void move_army(id,aw)
     int id;
     WINDOW *aw;
{
  int army_move_comment();
    /* army being moved (and cargo if there) */
  Sarmy *ap, *cargo_ap, *get_army();
  Ssector *sp_initial, *sp_final;
  Pt pos;
  char s[EXECLEN];
  int /* mv_initial, */ mv_final;

  strcpy(help_tag, "Moving your armies");

  ap = get_army(user.np, id);
  if ((ap == NULL) || (ap->id != id)) {
    beep();
    statline("Type space to continue", "No Army Selected");
    get_space();
    return;
  }
  if ((ap->owner != user.id)) {
    beep();
    statline2_err("Type space to continue", "Army not yours");
    return;
  }
  if ((ap->mvpts <= 0)) {
    beep();
    statline2_err("Type space to continue", "No movement left");
    return;
  }
  if (is_in_transport(ap)) {
    beep();
    statline2_err("Type space to continue", "This army is loaded");
    return;
  }

    /* remove it from old sector */
  pos = user.cursor;		/* old position */
  sp_initial = &world.map[pos.x][pos.y];
    /* see if there is still something in the list */
    delete_army_sector(sp_initial, ap); /* remove from sector list of armies */

    /* now for the guts of it: drag the army along,
       and get new position at the end.
     */
  pos = drag_cursor(ap->pos, DRAG_REL, army_move_comment, legal_move);
  ap->pos = pos;		/* army is in a new place!! */
  mv_final = ap->mvpts;
    /* adding army to new sector */
  sp_final = &world.map[pos.x][pos.y];
  insert_army_sector(sp_final, ap);
    /* put the user's cursor at new position */
  user.cursor = pos;
    /* make the exec line for this army movement */
  sprintf(s, "AMOVE:%d:%d:%d:%d\n",
	  ap->id, pos.x, pos.y, mv_final);
  gen_exec(s);
    /* not sure if I should call this here or in insert_army_sector() */
  find_visible_sectors(user.visible_sectors);
  user.current_army = ap->id;

    /* now see if we should also move the cargo (recursively) */
  if (is_cargo(ap) && ap->cargo.army >= 0) {
    cargo_ap = ap;
    while ((cargo_ap = get_army(user.np, cargo_ap->cargo.army))) {
	delete_army_sector(sp_initial, cargo_ap);
      cargo_ap->pos = sp_final->loc;
      insert_army_sector(sp_final, cargo_ap);
      /* make the exec line for this army movement */
      sprintf(s, "AMOVE:%d:%d:%d:%d\n",
	      cargo_ap->id, pos.x, pos.y, cargo_ap->mvpts);
      gen_exec(s);
    }
  }
    /* now some code to allow an army in TRADED mode to
       give itself to the recipient.  this only happens
       on other peoples' trade posts.
     */
  if (ap->status == A_TRADED) donate_army(ap, sp_final);
}

  /* run through the armies in a sector and pick the next */
void next_army()
{
  int old_id;
  Sarmy *ap = get_army(user.np, user.current_army);
  Ssector *sp = &world.map[user.cursor.x][user.cursor.y];

  if ((ap == NULL) || (sp->alist == NULL)) {
    return;
  }
  old_id = ap->id;

  if ((user.current_army = next_sect_army(sp, ap)) == -1) {
    user.current_army = old_id;
  }
}
  /* run through the armies in a sector and pick the previous */
void previous_army()
{
  Sarmy *ap = get_army(user.np, user.current_army);
  Ssector *sp = &world.map[user.cursor.x][user.cursor.y];

  if ((ap == NULL) || (sp->alist == NULL)) {
    return;
  }

  if ((user.current_army = prev_sect_army(sp, ap)) == -1) {
    user.current_army = first_sect_army(sp);
  }
}
  /* run through the armies in a nation and pick the next */
int next_nation_army(np, old_id)
     Snation *np;
     int old_id;
{
  Sarmy *ap = get_army(np, old_id);

  if (np->armies == NULL) {
    return -1;
  }
  if (ap == NULL) {
    return np->armies->id;	/* go to the start */
  }
  if (ap->next == NULL) {	/* we are at the end of the line */
    return old_id;
  }
  return ap->next->id;		/* the next army id!! */
}
  /* run through the armies in a nation and pick the previous */
int prev_nation_army(np, old_id)
     Snation *np;
     int old_id;
{
  Sarmy *ap = np->armies;

  while (ap) {
    if (ap->next && ap->next->id == old_id) {
      return ap->id;
    }
    ap = ap->next;
  }
  return old_id;
}

  /* change the status of the army as it appears in both
     the nation's army list and the sector army list.
   */
void change_army_status(aw,id)
     WINDOW *aw;
     int id;			/* army id */
{
  char c;
  Sarmy *ap;
  Ssector *sp = &world.map[user.cursor.x][user.cursor.y];
  char execstr[EXECLEN];

  if ((ap = get_army(user.np, id)) == NULL) {
    beep();
    statline("type space to continue", "army not yours");
    get_space();
    return;
  }

  if (aw) {
    statline2("Enter your new status.", "change_army_status");
    wmove(aw, 5, 2);
    wclrtoeol(aw);
    mvwprintw(aw, 5, 2, " [a]ttack, [d]efend, [o]ccupy,");
    mvwprintw(aw, 6, 2, " [p]atrol, [i]ntercept, [g]arrison or [t]raded");
    box(aw,'|','-');
    wrefresh(aw);
  } else {
    statline2("[a,d,o,n,p,i,g,t]", "change_army_status");
  }
  switch(c = getch()) {
  case 'a':
    ap->status = A_ATTACK;
    sprintf(execstr, "ASTAT:%d:%d\n", ap->id, ap->status);
    gen_exec(execstr);
    break;
  case 'd':
    ap->status = A_DEFEND;
    sprintf(execstr, "ASTAT:%d:%d\n", ap->id, ap->status);
    gen_exec(execstr);
    break;
  case 'o':
    if (can_occupy(ap)) {
      if (sp->owner != ap->owner) {
	ap->status = A_OCCUPY;
	ap->mvpts = 0;
	sprintf(execstr, "ASTAT:%d:%d\n", ap->id, ap->status);
	gen_exec(execstr);
      } else {
	statline2_err("type space to continue", "cannot occupy your sector!");
      }
    } else {
      statline2("type space to continue", "this army cannot occupy");
      get_space();
    }
    break;
  case 'p':
    if (can_patrol(ap)) {
      ap->status = A_PATROL;
      sprintf(execstr, "ASTAT:%d:%d\n", ap->id, ap->status);
      gen_exec(execstr);
    } else {
      statline2_err("type space to continue", "this army cannot patrol");
    }
    break;
  case 'i':
    if (can_intercept(ap)) {
      ap->status = A_INTERCEPT;
      sprintf(execstr, "ASTAT:%d:%d\n", ap->id, ap->status);
      gen_exec(execstr);
    } else {
      statline2_err("type space to continue", "this army cannot intercept");
    }
    break;
  case 'g':
    if (can_garrison(ap)) {
      ap->status = A_GARRISON;
      sprintf(execstr, "ASTAT:%d:%d\n", ap->id, ap->status);
      gen_exec(execstr);
    } else {
      statline2_err("type space to continue", "this army cannot garrison");
    }
    break;
  case 't':
    ap->status = A_TRADED;
      /* see if the army is ready to be traded */
    if (sp->owner != user.id && sp->designation == D_TRADE_POST) {
      sprintf(execstr, "ASTAT:%d:%d\n", ap->id, ap->status);
      gen_exec(execstr);
      donate_army(ap, sp);
    }
    break;
  default:
    break;
  }
  show_armies(&world.map[user.cursor.x][user.cursor.y]);
}

  /* this function is used by move_army() with drag_cursor(),
     and returns 0 if the given point cannot be moved to
     by the given army.
   */
int legal_move(pt, np, id)
     Pt pt;
     Snation *np;
     int id;			/* army id */
{
  Sarmy *ap;
  Ssector *sp = &world.map[pt.x][pt.y], *old_sp;
  int cost;

  wrap(&pt);
    /* find the army we are moving around */
  ap = get_army(np, id);

  /* this is if we want to make it that armies cannot go
     more than 1 step from land that they own.  for now, we
     jack up the move cost instead, so this is commented out.
   */

  cost = get_army_move_cost(np, sp, ap);
  if (ap->mvpts < cost) {
    return 0;
  }

  ap->pos = pt;
  user.cursor = pt;		/* so the map is drawn right */
  army_visibility(user.visible_sectors, ap); /* update the visibility */
  ap->mvpts -= cost;
  return 1;			/* if we are here, must be a legal move */
}

  /* this function returns the status of the army, so it
     can be put in the statline while the army is moved.  it
     will also return a 0 if move points are over, or 1 if
     there are still move points.
   */
int army_move_comment(s)
     char *s;
{
  Sarmy *ap = get_army(user.np, user.current_army);
  int cost;

  cost = get_army_move_cost(user.np, &world.map[ap->pos.x][ap->pos.y], ap);

  sprintf(s, "army %d; move points left: %d; cost %d", ap->id, ap->mvpts,cost);
  return (ap->mvpts > 0) ? 0 : 1;
}

  /* draft an army for that nation */
int draft_army(np)
     Snation *np;
{
  WINDOW *w;
  int size, ret, i;
  Sarmy army, make_army();	/* the new army!! */
  Ssector *sp = &world.map[user.cursor.x][user.cursor.y];
    /* exec string, army's type and army's name */
  char s[EXECLEN], type[NAMELEN], name[NAMELEN], c;
  extern struct s_desig_map desig_map[];

  strcpy(help_tag, "Army types");

  if (sp->owner != user.np->id) {
    statline2_err("hit space to continue", "Must be your sector");
    return -1;
  }
/* Make sure the sector is not hostile */
  if (has_hostile(sp)) {
    statline2_err("hit space to continue", "Cannot draft from hostile sector");
    return -1;
  }
  
  statline("choose your army/navy/caravan type", "draft_army");
  if (!user.xmode) {
    w = newwin(6, 60, LINES-14, COLS/5);
    mvwprintw(w, 1, 1, "You have %d armies;  first free one is #%d",
	      np->n_armies, free_army_id(np));
    mvwprintw(w, 2, 1, "What type of army/navy/caravan do you want? ");
    box(w, '|', '-');
    wrefresh(w);
  }
  noecho();
  cbreak();
  c = getch();
  if (c == '?') {
    online_info();
    return -1;
  } else if (c == ' ') {
    if (!user.xmode) {
      wrefresh(w);
      delwin(w);
      touch_all_wins();
    }
    return -1;
  }
  for(i = 0; i < user.n_army_types; i++) {
    if (army_types[i].type_char == c) {
      strcpy(type, army_types[i].type);
      break;
    }
  }
				/* While it is still the same army_types... */
  if (!strchr (army_types [i].draft_places, desig_map[sp->designation].mark)) {
    statline2_err ("hit space", "bad place to draft");
    if (!user.xmode) {
      wrefresh (w);
      delwin (w);
    }
    return -1;
  }
  if (!is_avail_army_type(&user, type)) {
    statline2_err("Hit space to get back", "Bad army type");
    if (!user.xmode) {
      wrefresh(w);
      delwin(w);
      touch_all_wins();
    }
    return -1;
  }
  if (!user.xmode) {
    wprintw(w, "%s", type);
  }

  strcpy (name, type);

  if (user.xmode) {
    sprintf(s, "(default: %s): ", name);
    statline2_prompt(s, "Name your army");
    ret = wget_name(stdscr, name);
  } else {
    mvwprintw(w, 3, 1, "Name your army (default: %s):  ", name);
    ret = wget_name(w, name);
  }
    /* if they just type return, use the default name */
  if (ret <= 0) {
    sprintf(name, "%s %d", type, free_army_id(np));
  }

  if (user.xmode) {
    statline("", "How many units?");
    move(LINES-1, 0);
    ret = wget_number(stdscr, &size);
  } else {
    mvwprintw(w, 4, 1, "How many units do you want? ");
    box(w, '|', '-');
    ret = wget_number(w, &size);
  }
  if ((ret <= 0) || (size <= 0)) {
    if (!user.xmode) {
      wrefresh(w);
      delwin(w);
      touch_all_wins();
    }
    return -1;
  }
  army = make_army(type, name, size, A_DEFEND, np->id, sp->loc);
  army.id = free_army_id(np);
  army.next = NULL;
    /* now see if we can afford the army */
  if (army.n_soldiers
      > (sp->n_people - desig_map[sp->designation].min_employed)) {
    beep();
    statline2("space to go on", "not enough people in sector");
    get_space();
    if (!user.xmode) {
      wrefresh(w);
      delwin(w);
      touch_all_wins();
    }
    return -1;
  }
  if (army_cost(&army) > user.np->money) {
    beep();
    statline2("space to go on", "not enough money to draft");
    get_space();
    if (!user.xmode) {
      wrefresh(w);
      delwin(w);
      touch_all_wins();
    }
    return -1;
  }
  if (army_cost_metal(&army) > user.np->metal) {
    beep();
    statline2("space to go on", "not enough metal to draft");
    get_space();
    if (!user.xmode) {
      wrefresh(w);
      delwin(w);
      touch_all_wins();
    }
    return -1;
  }

    /* well, if we have reached this point, it means that we
       can afford this army, so let us insert it into the lists
     */
  np->n_armies++;
  if (np->armies == NULL) {	/* special case:  empty list */
    np->armies = (Sarmy *) malloc(sizeof(Sarmy));
    *(np->armies) = army;
    np->armies->next = NULL;
  } else {
    insert_army_nation(np, &army, -1);
  }
  insert_army_sector(sp, &army);
  sp->n_people -= army.n_soldiers;
  np->money -= army_cost(&army);
  np->metal -= army_cost_metal(&army);
    /* now prepare the exec string for making the army and costs */
  sprintf(s, "AMAKE:%d:%d:%d:%d:%s:%s\n", army.id, army.n_soldiers,
	  army.pos.x, army.pos.y, army.type, army.name);
  gen_exec(s);
  cpeople_sector(sp, -army.n_soldiers);
  cmoney(np, -army_cost(&army));
  cmetal(np, -army_cost_metal(&army));

  show_armies(sp);

  if (!user.xmode) {
    wrefresh(w);
    delwin(w);
    touch_all_wins();
  }
  just_moved();
  return 1;
}

  /* get the first army in that sector that belongs to you */
int first_sect_army(sp)
     Ssector *sp;
{
  int id = -1;			/* default:  no army selected */
  struct armyid *alist = sp->alist;
  char s[100];

  while (alist != NULL) {
    if (alist->owner == user.id) {
/* this routine sometimes messes up */
      id = alist->id;
      break;
    }
    alist = alist->next;
  }

  return id;
}

  /* takes a sector, an army (possibly in that sector), and
     returns the id of the next army you own in that sector,
     or -1 if you don't have any more
   */
int next_sect_army(sp, ap)
     Ssector *sp;
     Sarmy *ap;
{
  struct armyid *alist = sp->alist;
  char s[100];

  if (ap == NULL) {		/* in case the army given was bogus */
    return first_sect_army(sp);
  }

    /* first get up to the given army */
  while ((alist != NULL) &&
	 !((alist->id == ap->id) && (alist->owner == ap->owner))) {
    alist = alist->next;
  }
/*  if (alist->next == NULL) {	// we are at the end of the list */
/*    return -1;
  }
*/
  
  if ((alist == NULL) || (alist = alist->next) == NULL) {
    return -1;
  }
    /* now look for the next army owned by the current user */
  while ((alist != NULL) && (alist->owner != ap->owner)) {
    alist = alist->next;
  }
  if (alist == NULL) {
    return -1;
  }
  return alist->id;		/* in this case, we have a valid next army */
}

  /* takes a sector, an army (should be in that sector), and
     returns the id of the previous army you own in that sector,
     or -1 if you don't have any more.
   */
int prev_sect_army(sp, ap)
     Ssector *sp;
     Sarmy *ap;
{
  struct armyid *alist = sp->alist, *previous = sp->alist;
  previous = NULL;

  if ((ap == NULL) || (alist == NULL)) {
    return first_sect_army(sp);
  }
    /* now get up to the given army */
  while ((alist != NULL) &&
	 !((alist->owner == ap->owner) && (alist->id == ap->id))) {
    if (alist->owner == ap->owner) {
      previous = alist;
    }
    alist = alist->next;
  }

  return (previous == NULL) ? -1 : previous->id;
}

  /* gives a list of army types available to this user */
void list_available_armies(up, aw)
     Suser *up;
     WINDOW *aw;		/* army window */
{
  int i, count = 0;
  Savail_army *avail_armies = up->avail_armies;
  char s[200];

  strcpy(s, " ");
  if (aw) {
    wmove(aw, 1, 1);
    wclrtobot(aw);
    mvwprintw(aw, 1, 20, "AVAILABLE ARMIES");
    wmove(aw, 2, 1);
  }
  while (avail_armies != NULL) {
    if ((i = army_type_index(avail_armies->type)) >= 0) {
      if (aw) {
	wprintw(aw, " %s(%c) ", army_types[i].type, army_types[i].type_char);
      } else {
	sprintf(s, "%s%c ", s, army_types[i].type_char);
      }
      if (aw && avail_armies->next != NULL) {
	waddstr(aw, ",");
      }
      if (aw) { wclrtoeol(aw); }
      ++count;
      if (aw && (count % 5) == 0) { /* new line */
	wmove(aw, 2+count/5, 1);
      }
    }
    avail_armies = avail_armies->next;
  }
  if (aw) {
    box(aw, '|', '-');
    wrefresh(aw);
    statline2("Type space to get back", "list_available_armies");
  } else {
    statline2(s, "list_available_armies");
  }
}


int army_disband(sp, ap)
Ssector *sp;
Sarmy * ap;
{
  int army_num, done, index, metal_return;
  Snation *np = user.np;
  char s[EXECLEN], c;
  struct argument args[N_EXEC_ARGS];

  if (sp->owner != np->id) {
    statline2_err("Hit space to get back", "You don't own sector");
    return 0;
  }

  if (is_in_transport(ap)) {
    statline2_err("Hit space to get back", "Army is in transport");
    return 0;
  }
  if (is_cargo(ap) && cargo_not_empty(&ap->cargo)) {
    statline2_err("Hit space to get back", "Caravan has stuff loaded");
    return 0;
  }
  if (is_spelled(ap)) {
    statline2_err("Hit space to get back", "Army under a spell");
    return 0;
  }
  sprintf (s, "Disband army %d.  Are you sure? ", ap->id);
  statline2_prompt (s, "army_disband");

  switch (c = getch()) {
  case 'Y':
  case 'y':
    sprintf(s, "ADISBAND:%d\n", ap->id);
    index = army_type_index (ap->type);
    if (index != -1) {  /* If it's not a spirit */
      metal_return = (army_types [index].metal_draft * ap->n_soldiers) *
	DISBAND_RETURN;
      np->metal +=metal_return;
      cmetal (np, metal_return);
    }

    gen_exec(s);
    parse_exec_line(s,args);
    run_exec_line(np,args);
    break;
  case 'N': case 'n':
  default: 
    break;
  }

  if (get_army(user.np, user.current_army) == NULL) {
    user.current_army = first_sect_army(sp);
  }
  just_moved ();
}

  /* give a name to an army */
void army_name (ap)
     Sarmy * ap;
{
  char name[NAMELEN], s[EXECLEN];
  int ret;

  statline2_prompt ("Give army name: ", "");
  ret = wget_name(stdscr, name);
  if (ret > 0) {
    strcpy(ap->name, name);
      /* now generate the exec instruction */
    sprintf(s, "ANAME:%d:%s\n", ap->id, ap->name);
    gen_exec(s);
  }
  noecho();
  cbreak();
}

  /* give a detailed description of all armies in a nation,
     or just those in the "current_sp".
   */
int zoom_armies(up, sp)
Suser *up;
Ssector *sp;
{
  WINDOW *azlw, * azmw;
  Sarmy * armies;
  int army_count, maxlen_win;
  int num_shown;
  int show_cargo = 0;
  int done = 0;
  int page = 1;
  int maxpage;

  armies = up->np->armies;
  army_count = 0;

  while (armies != NULL) {	/* Count the number of armies to be shown  */
    if (sp != NULL) {
      if (armies->pos.x == sp->loc.x
	      && armies->pos.y == sp->loc.y) {
	army_count ++;
      }
    } else {
      army_count++;
    }
    armies = armies->next;
  }

  if (army_count == 0) {
    return 0;
  }

  maxlen_win = LINES - 3;
  if (!user.xmode) {		/* Allow for the non-expert menu */
    maxlen_win -= 4;
  }

  if (army_count <= maxlen_win - 4) { /* Don't use the entire window len? */
    maxlen_win = army_count + 4;
    num_shown = army_count;
    if (user.xmode) {
      azlw = newwin (maxlen_win, COLS, LINES - (2 + maxlen_win), 0);
    }
    else {
      azlw = newwin (maxlen_win, COLS, LINES - (2 + 4 + maxlen_win), 0);
    }
  }
  else {
    num_shown = maxlen_win - 4;
    azlw = newwin (maxlen_win, COLS, 1, 0);
  }
  maxpage = army_count / (maxlen_win - 4); 
  maxpage ++;
  
  box (azlw, '|', '-');

  mvwprintw (azlw, 1, 1, 
" ID Name       Size Type        Status  Bonus       ");
  mvwprintw (azlw, 1, 47, "%s", show_cargo ? "Cargo" : "Maint");
  mvwprintw (azlw, 2, 1, 
"--- ---------  ---- ----------- ------- ----  ------------------------------");


  while (!done) {
    char c;
    int i;
    char s [EXECLEN];
    Sarmy * ap;
    Ssector * sp2;
    
    sp2 = &world.map[user.cursor.x][user.cursor.y];

    statline2 ("", "");
    for (i = 3; i < maxlen_win - 1; i++) {
      wmove (azlw, i, 0);
      wclrtoeol (azlw);
    }
    if (user.xmode) {
      azmw = NULL;
      statline ("[>,<,c,D,E,N,+,-]", "zoom_armies");
    } else {
      azmw = newwin (4, COLS, LINES - 6, 0);
      box (azmw, '|', '-');
      mvwprintw(azmw, 1, 1,
		" [>]/[.]next pg, [<]/[,]prev pg,[N]ame army, [c] examine %s ",
		show_cargo ? "costs" : "cargo");
      mvwprintw(azmw, 2, 1,
	" [D]isband army, [-]split army, [+]merge army [E]xamine army");
      statline ("Choose zoom_army command (space to exit)", "zoom_armies");
      wrefresh (azmw);
    }

    box (azlw, '|', '-');
    zoom_army_page (azlw, up->np->armies, sp, page,maxlen_win-4, show_cargo);

    do { c = getch (); } while (strchr (" <>.,NcD-+E", c) == NULL);

    switch (c) {
    case ' ':
      done = 1;
      break;
    case 'c':
      show_cargo = !show_cargo;
      mvwprintw (azlw, 1, 47, "%s", show_cargo ? "Cargo" : "Maint");
      wrefresh (azlw);
      if (azmw) {
	mvwprintw(azmw, 1, 1,
	  " [>]/[.]next pg, [<]/[,]prev pg,[N]ame army, [c] examine %s ",
		  show_cargo ? "costs" : "cargo");
	wrefresh (azmw);
      }
      break;
    case '>':
    case '.':
      if (page != maxpage) {
	page ++;
      }
      break;
    case '<':
    case ',':
      if (page != 1) {
	page --;
      }
      break;
    case 'N':
      statline2_prompt ("Name army #", "army_name"); 
      if (((wget_number (stdscr, &i)) <= 0) || 
	((ap  = get_army (user.np, i)) == NULL)) {
	break;
      }
      army_name (ap);
      break;
    case 'E':
      statline2_prompt ("Examine army #", "army_examine"); 
      if (((wget_number (stdscr, &i)) <= 0) || 
	((ap  = get_army (user.np, i)) == NULL)) {
	break;
      }
      army_examine(ap);
      break;
    case 'D':
      statline2_prompt ("Disband army #", "army_disband"); 
      if (((wget_number (stdscr, &i)) <= 0) || 
	((ap  = get_army (user.np, i)) == NULL)) {
	break;
      }
      army_disband (sp2, ap);
      break;
    case '-':
      statline2_prompt ("Split army #", "army_split"); 
      if (((wget_number (stdscr, &i)) <= 0) || 
	((ap  = get_army (user.np, i)) == NULL)) {
	break;
      }
      army_split (ap);
      break;
    case '+':
      statline2_prompt ("Merge army #", "army_merge"); 
      if (((wget_number (stdscr, &i)) <= 0) || 
	((ap  = get_army (user.np, i)) == NULL)) {
	break;
      }
      army_merge (ap);
      break;
    }
  }

  if (azmw) { delwin(azmw); }
  delwin(azlw);
  touch_all_wins();
  return 0;
}

/* This function is used by zoom_armies to select the page of armies you want*/

int zoom_army_page (azlw, armies, sp, page, len_page, show_cargo)
WINDOW * azlw;
Sarmy * armies;
Ssector * sp;
int page, len_page, show_cargo;
{
  int army_count, i;
  Sarmy * ap = armies;

  while (ap != NULL) {
    if (sp != NULL) {
      if (sp->loc.x == ap->pos.x && sp->loc.y == ap->pos.y) {
	army_count ++;
      }
    }
    else {
      army_count ++;
    }
    ap = ap->next;
  }

  if (army_count <= len_page) {
    zoom_list_armies (azlw, armies, sp, 0, len_page, show_cargo);
    return 0;
  }
  if ((page - 2) * len_page > army_count) {	/* Too high a page number */
    return 1;
  }
  ap = armies;

  for (i=0; i < (page-1) * len_page; i++) {
    ap = ap->next;
  }
  if (ap != NULL) {
    zoom_list_armies (azlw, armies, sp, ap->id, len_page, show_cargo);
  }
  return 0;
}

/* This function is used by zoom_armies to list armies in the azlw win */

void zoom_list_armies (azlw, armies, sp, start_army, len_win, cargo)
WINDOW * azlw;
Sarmy * armies;
Ssector * sp;
int start_army;			/* Starting army number */
int len_win;			/* Number of army lines allowed in window */
int cargo;			/* Cargo mode? */
{
  int army_count;
  int row;
  char * slash_pos;
  Scargo * snails;		/* Sorry for the pun :-) */
  char * contents ();		/* Returns string of stuff, nicely formatted */
  char s [EXECLEN];
  char zooms [EXECLEN];

  while (armies != NULL) {	/* Get to the starting army number */
    if (armies->id >= start_army) { /* In case it doesn't actually exist... */
      break;
    }
    armies = armies->next;
  }

  if (sp != NULL) {		/* If not zoom ALL armies, */
    do {			/* Check that the first army is on sector */
      if (armies->pos.x == sp->loc.x && armies->pos.y == sp->loc.y) {
	break;
      }
      armies = armies->next;
    } while (armies != NULL);
  }

  /* Now display the armies */

  row = 3;
  while (armies != NULL && row-3 < len_win) {
    mvwprintw(azlw, row,  1, "%3d    ", armies->id);
    mvwprintw(azlw, row,  5, "%s                   ", armies->name);
    mvwprintw(azlw, row, 16, "%4d  ", armies->n_soldiers);
    mvwprintw(azlw, row, 21, "%s           ", armies->type);
    get_army_status(armies, s);
    slash_pos = (char *) strchr (s, (int) '/');
    if (slash_pos != NULL) {
      *slash_pos = '\0';
    }
    mvwprintw(azlw, row, 33, "%s     ", s);
    mvwprintw(azlw, row, 42, "%3d    ", armies->sp_bonus);

    strcpy (zooms, "");
    if (cargo) {
      if (!is_cargo (armies)) {
	sprintf (zooms, "   not a transport");
      }
      else {
	snails = &armies->cargo;
	sprintf (zooms, "%s", 
		 contents (snails->money, snails->metal, snails->jewels,
			   snails->food, snails->people, snails->army,
			   &snails->title, 0));
      }
    }
    else {
      sprintf (zooms, "%s",
	       contents (armies->money_maint * armies->n_soldiers,
			 armies->metal_maint * armies->n_soldiers,
			 armies->jewel_maint * armies->n_soldiers, 0,
			 0, -1, NULL, get_spell_pts_maint (armies))); 
    }

    mvwprintw(azlw, row, 47, zooms);

    armies = armies->next;
    if (sp != NULL && armies) {	/* Again, check if we want to discriminate...*/
      do {
	if (armies->pos.x == sp->loc.x && armies->pos.y == sp->loc.y) {
	  break;
	}
	armies = armies->next;
      } while (armies != NULL);
    }
    row ++;
  }
  wrefresh (azlw);
}

    /* now some code to allow an army in TRADED mode to
       give itself to the recipient.  this only happens
       on other peoples' trade posts.
     */
void donate_army(ap, sp)
     Sarmy *ap;
     Ssector *sp;
{
  if (is_cargo (ap) && cargo_not_empty (&ap->cargo)) {
    statline2_err ("Error: transport still has a cargo", "donate_army");
  } 
  else if (ap->flags & AF_IN_TRANSPORT) {
    statline2_err ("Error: army is still in transport", "donate_army");
  }
  else if ((ap->status == A_TRADED) && (sp->designation == D_TRADE_POST)
      && (sp->owner !=  user.np->id)) {
    statline2("Do you want to trade this army right here [y/n]? ", "");
    switch (getch()) {
      Scargo cargo;
    case 'y':
    case 'Y':
      cargo.money = cargo.metal = cargo.jewels = cargo.food = 0;
      cargo.people = 0;
      cargo.army = ap->id;
      cargo.title.x = -1;
      cargo.title.y = -1;
      donate_cargo(sp->loc.x, sp->loc.y,
		   user.np->id, sp->owner, &cargo);
      break;
    default:
      break;
    }
  }
}

extern int (*wrapx)(), (*wrapy)();

int jarmy ()
{
  char s[100];
  int army_num;
  Sarmy * ap;
  int x, y;

  ap = user.np->armies;

  statline2 ("Army number to jump to? ", "jump_army");
  move (LINES-2, 25);
  refresh ();
  if (wget_number(stdscr, &army_num) < 1) {
    statline2("", "");
    return;
  }

  while (ap != NULL) {
    if (ap->id == army_num) {
      break;
    }
    ap = ap->next;
  }

  if (ap == NULL) {
    statline2_err ("Hit space to continue", "army does not exist");
    return 0;
  }
  if (ap->id == army_num) {
    x = ap->pos.x;
    y = ap->pos.y;
    user.cursor.x = (*wrapx)(x,y);
    user.cursor.y = (*wrapy)(x,y);
    user.center = user.cursor;
    user.current_army = ap->id;
    just_moved();
  }
  statline2("", "");
  return 1;
}

void army_examine(ap)
     Sarmy * ap;
{
  WINDOW * flagw, * infow;
  char s [NAMELEN], * slash_pos;
  int n_flags = 0;
  int i, rowpos;
  char * flags [20];
  Snation * np = &world.nations [ap->owner];

  infow = newwin (9, 32, 0, 0);
  box (infow, '|', '-');

  get_army_status (ap, s);

  slash_pos = (char *)strchr (s, (int)'/');
  n_flags = 0;
  if (slash_pos != NULL) {
    n_flags = strlen(slash_pos) - 1;
    *slash_pos = '\0';
  }

  mvwprintw (infow, 1, 2, "Name: %8s (#%d)", ap->name, ap->id);
  mvwprintw (infow, 2, 2, "%d Movepoints, %d left",
	     army_move_rate (np, ap), ap->mvpts);
  mvwprintw (infow, 3, 2, "Type: %d %s", ap->n_soldiers, ap->type);
  mvwprintw (infow, 4, 2, "Status: %s  Bonus: %d",
	     s, ap->sp_bonus);
  if (is_cargo (ap)) {
    mvwprintw (infow, 5,2, "Capacity: %d/Weight: %d",
	       ap->n_soldiers * CARAVAN_CAPACITY, army_weight (ap));
  }
  else {
    mvwprintw (infow, 5,2, "Weight: %d", army_weight (ap));
  }
  mvwprintw (infow, 6,2, "Maintenance per thon:");
  mvwprintw (infow, 7,2, "  %s",
	     contents (ap->money_maint * ap->n_soldiers,
		       ap->metal_maint * ap->n_soldiers,
		       ap->jewel_maint * ap->n_soldiers, 0,
		       0, -1, NULL, get_spell_pts_maint (ap))); 
  wrefresh (infow);

  if (n_flags > 0) {
    flagw = newwin (n_flags + 2, COLS - 40, 0, 40);

    rowpos = 1;
    for (i = 0; i < 32; i++) {
      if (strlen (army_flags [i].description) > 0 &&
		  ap->flags & (0x1 << i)) {
	mvwprintw (flagw, rowpos, 1, "%s", army_flags [i].description);
	rowpos++;
      }
    }
    box (flagw, '|', '-');
    wrefresh (flagw);
  }
  else {
    flagw = NULL;
  }

  statline2_err ("hit space to continue", "army_examine");
  werase (infow);
  if (flagw) {
    werase (flagw);
  }
  touch_all_wins ();
}
