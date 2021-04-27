/* transport.c -- routines dealing with transportation */

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
#include <stdlib.h>
#include <unistd.h>

extern Suser user;
extern Sworld world;
extern char help_tag[];
extern WINDOW * sectw;
extern struct army_type * army_types;


// /* Main transport functions */
// /* int transport ();  /* Coordinates everything (top level) */
// /* int transport_load (Sarmy *, WINDOW *); /* Actually loads stuff */
// /* int transport_unload (Sarmy *, WINDOW *); /* Unloads stuff */
// /* int transport_transfer (Sarmy *, WINDOW *); /* Transfers stuff */

// /* Cargo weight functions */
// /* int army_weight (Sarmy *); /* Figures out an army's manweight */ 
// /* int caravan_weight (Sarmy *); /* Figures out a caravan's manweight */
// /* int cargo_weight (Scargo *); /* Figures out a cargo's manweight */
// /* char * make_cargo_statline (Scargo *); /* Makes a cargo statline */

  /* allows you to load/unload cargo from a caravan */
int transport()
{
  WINDOW *tw;
  char type[NAMELEN];
  int ret, size;
  Snation *np = user.np;
  Ssector *sp =  &world.map[user.cursor.x][user.cursor.y];
  Sarmy *ap, *get_army();
  char s[EXECLEN];
  int done = 0;

    /* before I make the new window, I should check if the user
       is transporting on a caravan or ship, or if they are transporting
       on something else (like an army or a spirit)
     */
  ap = get_army(np, user.current_army);
  if ( (ap == NULL) || !is_cargo(ap) ) {
    statline2_err ("hit space", "must select a caravan or navy");
    return -1;
  }
  if (user.xmode) {
    tw = NULL;
  } else {
    statline("hit space to exit", "transportation");
    tw = newwin(7, COLS/2+28, LINES-10, COLS/4-14);
  }

  while (!done) {
    if (tw != NULL) {
      mvwprintw(tw, 1, 1, "Do you wish to [l]oad, [u]nload or [t]ransfer? ");
      wclrtobot(tw);
      box(tw, '|', '-');
      wrefresh(tw);
    }
    cargo_statline(user.xmode ? NULL : tw, &user);

    strcpy(help_tag, "Transportation");
    switch (mygetch()) {
    case 'l':
      if (ap->flags & AF_IN_TRANSPORT) {
	statline2_err ("hit space", "cannot load a caravan in transit");
      }
      else transport_load(ap, user.xmode ? NULL : tw);
      statline2("", "");
      break;
    case 'u':
      if (!cargo_not_empty (&ap->cargo)) {
	statline2_err ("hit space", "cannot unload an empty caravan"); 
      }
      else transport_unload(ap, user.xmode ? NULL : tw);
      statline2("", "");
      break;
    case 't':
      if (!cargo_not_empty (&ap->cargo)) {
	statline2_err ("hit space", "nothing to transfer");
      }
      else transport_transfer(ap, user.xmode ? NULL : tw);
      statline2("", "");
      break;
    case ' ':
      done = 1;
      break;
    default:
      break;
    }
  }

  if (tw != NULL) {
    delwin(tw);
    wrefresh (sectw);
    touchwin(stdscr);
  }
  just_moved ();
  return 1;
}

  /* load onto a caravan or ship */
void transport_load (ap, tw)
     Sarmy *ap;
     WINDOW *tw;
{
  Snation *np = user.np;
  Ssector *sp = &world.map[ap->pos.x][ap->pos.y];
  Sarmy *loaded_army;
  int quantity, done = 0, ret; 
  int doit;
  char c;
  char s[100];

  strcpy(help_tag, "Transportation");

  while (!done) {
    if (tw != NULL) {
      wmove(tw, 4, 1);
      wclrtobot(tw);
    }
    cargo_statline(tw, &user);
    if (tw != NULL) {
      box(tw, '|', '-');
    }
    if (user.xmode) {
      statline2("  [s,m,j,f,a,p,t]", "load");
    } else {
      wmove (tw, 1, 0);
      wclrtoeol (tw);
      box (tw, '|', '-');
      mvwprintw(tw, 2, 2, "What do you want to load? ");
      mvwprintw(tw, 3, 2,
		" [s]hekels, [m]etal, [j]ewels, [f]ood, [a]rmy, [p]eople, [t]itle ");
      wrefresh(tw);
    }
    do { c = mygetch(); } while (strchr (" smjfapt", c) == NULL);

    doit = 1;
    
    /* If anything besides a title or an exit, prompt */
    if (c != ' ' && c != 't') {
      sprintf (s, (c == 'a') ? "(%c) Which army? " : "(%c) Quantity? ", c);
      if (user.xmode) {
	statline2(s, "");
	move(LINES-2, strlen(s));
      } else {
	mvwprintw(tw, 4, 5, s);
      }
      ret = wget_number(user.xmode ? stdscr : tw, &quantity);
      if (ret <= 0) {
	doit = 0;
      }
    }

    strcpy (s, "");

    if (!good_loading_place (np, sp, c, quantity)) {
      statline2_err ("Can't load that here", "load");
    } else if (load_too_big (np, ap, c, quantity)) {
      statline2_err ("Too much (space)", "load");
    } else if (quantity >= 0 && doit) {
      switch (c) {
      case ' ':
	done = 1;
	break;
      case 's':
	if (quantity <= np->money) {
	  ap->cargo.money += quantity;
	  np->money -= quantity;
	  cmoney (np, -quantity);
	}
	else sprintf (s, "Sorry, not enough in storage (space)");
	break;
      case 'j':
	if (quantity <= np->jewels) {
	  ap->cargo.jewels += quantity;
	  np->jewels -= quantity;
	  cjewels (np, -quantity);
	}
	else sprintf (s, "Sorry, not enough in storage (space)");
	break;
      case 'm':
	if (quantity <= np->metal) {
	  ap->cargo.metal += quantity;
	  np->metal -= quantity;
	  cmetal (np, -quantity);
	}
	else sprintf (s, "Sorry, not enough in storage (space)");
	break;
      case 'f':
	if (quantity <= np->food) {
	  ap->cargo.food += quantity;
	  np->food -= quantity;
	  cfood (np, -quantity);
	}
	else sprintf (s, "Sorry, not enough in storage (space)");
	break;
      case 'a':
	if (ap->cargo.army == -1) {
	  if (army_is_in_sector(sp, ap->owner, quantity)) {
	    if (ap->id != quantity) {
	      ap->cargo.army = quantity;
	      loaded_army = get_army (np, quantity);
	      loaded_army->flags |= AF_IN_TRANSPORT;
	      aflag_set (loaded_army, AF_IN_TRANSPORT);
	      show_armies (sp);
	    }
	    else sprintf(s,"That would be an interesting topological exercise!");	  }
	  else sprintf (s, "Sorry, can't load across sectors!");
	}
	else sprintf (s, "Sorry, can only transport one army at a time.");
	break;
      case 'p':
	if (quantity <= sp->n_people) {
	  ap->cargo.people += quantity;
	  sp->n_people -= quantity;
	  cpeople_sector (sp, -quantity);
	  if (user.xmode) {
	    just_moved ();
	    show_sector (user.cursor.x, user.cursor.y);
	  }
	}
	else sprintf (s, "Sorry, too large a number (space)");
	break;
      case 't':
	if (!has_traded (sp)) {
	  if (sp->designation != D_CAPITAL) {
	    if (ap->cargo.title.x == -1) {
	      sp->flags |= SF_TRADED;
	      sprintf (s, "FLAG_SET_SECTOR:%d:%d:%d\n", sp->loc.x, sp->loc.y,
		       SF_TRADED);
	      gen_exec (s);
	      sprintf (s, "");
	      ap->cargo.title = sp->loc;
	    }
	    else sprintf (s, "Sorry, can only load one title per caravan");
	  }
	  else sprintf (s, "Sorry, but you can't give away your nation...");
	}
	else sprintf (s, "Sector is already traded");
	break;
      }
      if (strlen (s) > 0) {
	statline2_err (s, "load");
      }
      else { adjust_cargo (ap); }
    }
  }
}

/* unload from a caravan */
void transport_unload(ap, tw)
     Sarmy *ap;
     WINDOW *tw;
{
  Snation *np = user.np;
  Ssector *sp = &world.map[ap->pos.x][ap->pos.y];
  int recip_id;			/* the guy you to whom give the cargo */
  Scargo dropped_cargo;		/* how much you drop */
  char c;
  char s[100];
  int done = 0, ret, quantity, bad_load = 0;
  Sarmy *trans_ap;

  /* initialize that nothing is traded */
  dropped_cargo.money = 0;
  dropped_cargo.metal = 0;
  dropped_cargo.jewels = 0;
  dropped_cargo.food = 0;
  dropped_cargo.people = 0;
  dropped_cargo.army = -1;
  dropped_cargo.title.x = -1;
  dropped_cargo.title.y = -1;

  trans_ap = get_army(np, ap->cargo.army);
  recip_id = sp->owner;

  while (!done) {
    cargo_statline(tw, &user);
    if (tw != NULL) {
      box(tw, '|', '-');
      wrefresh(tw);
    }
    if (tw == NULL) {
      statline2("  [s,m,j,f,p,a,t,*]", "unload");
    } else {
      wmove (tw, 1, 0);
      wclrtoeol (tw);
      box (tw, '|', '-');
      mvwprintw(tw, 2, 2, "What do you want to unload? ");
      mvwprintw(tw, 3, 2,
		"[s]hekels/[m]etal/[j]ewels/[f]ood/[p]eople/[a]rmy/[t]itle/[*]");
      wrefresh (tw);
    }
    do { c = mygetch (); } while (strchr ("*smjaftp ", c) == NULL);
    
    if (!good_unloading_place(np, sp, c, ap->cargo.army)) {
      statline2_err("Cannot unload that here (space)", "");
    }
    else {
      if (c != ' ' && c != 'a' && c != 't' && c != '*') {
	sprintf (s, "(%c) Quantity? ", c);
	if (tw == NULL) {
	  statline2 (s, "unload");
	  move (LINES-2, strlen(s));
	} else {
	  mvwprintw (tw, 4, 7, s);
	}
	ret = wget_number(user.xmode ? stdscr : tw, &quantity);
      }

      strcpy (s, "");

      /* Actually unload the stuff */
      if (strchr (" at*", c) != NULL || (ret >= 0) ) {
	switch (c) {
	case 's':
	  if (quantity <= ap->cargo.money && quantity > 0) {
	    ap->cargo.money -= quantity;
	    dropped_cargo.money += quantity;
	  }
	  else sprintf (s, "Invalid number (space)");
	  break;
	case 'm':
	  if (quantity <= ap->cargo.metal && quantity > 0) {
	    ap->cargo.metal -= quantity;
	    dropped_cargo.metal += quantity;
	  }
	  else sprintf (s, "Invalid number (space)");
	  break;
	case 'j':
	  if (quantity <= ap->cargo.jewels && quantity > 0) {
	    ap->cargo.jewels -= quantity;
	    dropped_cargo.jewels += quantity;
	  }
	  else sprintf (s, "Invalid number (space)");
	  break;
	case 'f':
	  if (quantity <= ap->cargo.food && quantity > 0) {
	    ap->cargo.food -= quantity;
	    dropped_cargo.food += quantity;
	  }
	  else sprintf (s, "Invalid number (space)");
	  break;
	case 'p':
	  if (quantity <= ap->cargo.people && quantity > 0) {
	    ap->cargo.people -= quantity;
	    dropped_cargo.people += quantity;
	  }
	  else sprintf (s, "Invalid number (space)");
	  break;
	case 't':		/* drop a title to land */
	  if (ap->cargo.title.x != -1) {
	    dropped_cargo.title = ap->cargo.title;
	    ap->cargo.title.x = -1;
	    ap->cargo.title.y = -1;
	  }
	  else sprintf (s, "No sector title loaded (space)");
	  break;
	case 'a':
	  if (ap->cargo.army >= 0) {
	    dropped_cargo.army = ap->cargo.army;
	    ap->cargo.army = -1;
	  }
	  else sprintf (s, "No army loaded (space)");
	  break;
	case '*':
	  if (ap->cargo.army >= 0) {
	    dropped_cargo.army = ap->cargo.army;
	    ap->cargo.army = -1;
	  }
	  dropped_cargo.people = ap->cargo.people;
	  ap->cargo.people = 0;
	  dropped_cargo.jewels = ap->cargo.jewels;
	  ap->cargo.jewels = 0;
	  dropped_cargo.metal = ap->cargo.metal;
	  ap->cargo.metal = 0;
	  dropped_cargo.food = ap->cargo.food;
	  ap->cargo.food = 0;
	  dropped_cargo.money = ap->cargo.money;
	  ap->cargo.money = 0;
	  if (ap->cargo.title.x != -1) {
	    dropped_cargo.title = ap->cargo.title;
	    ap->cargo.title.x = -1;
	    ap->cargo.title.y = -1;
	  }
	  done = 1;
	  break;
	case ' ':
	  done = 1;
	  break;
	default:
	  break;
	}
	if (strlen (s) > 0) {
	  statline2_err (s, "");
	  bad_load = 1;
	}
      }
    }
  }
  if (bad_load == 1) { ; }
  else if (recip_id == user.id) { /* on your land, just put the stuff down */
    np->money += dropped_cargo.money;
    np->metal += dropped_cargo.metal;
    np->jewels += dropped_cargo.jewels;
    np->food += dropped_cargo.food;
    sp->n_people += dropped_cargo.people;
    if (dropped_cargo.title.x != -1) {
      Ssector *traded_sp;
      traded_sp = &world.map[dropped_cargo.title.x][dropped_cargo.title.y];
      traded_sp->flags &= ~SF_TRADED;
        /* now prepare the exec string */
      sprintf(s, "FLAG_CLEAR_SECTOR:%d:%d:%d\n", traded_sp->loc.x,
	      traded_sp->loc.y, SF_TRADED);
      gen_exec(s);
    }
    cmoney(np, dropped_cargo.money);
    cmetal(np, dropped_cargo.metal);
    cjewels(np, dropped_cargo.jewels);
    cfood(np, dropped_cargo.food);
    cpeople_sector(sp, dropped_cargo.people);
      /* just drop your army on your own land */
    if (dropped_cargo.army != -1) {
      trans_ap->flags &= ~AF_IN_TRANSPORT;
      aflag_clear(trans_ap, AF_IN_TRANSPORT);
      dropped_cargo.army = -1;
    }
  } 
  else { /* Not in your land */
    if (dropped_cargo.army != -1) { /* Clear army transport flags */
      if (is_front_line (trans_ap) || !(trans_ap->status == A_TRADED)) {
	dropped_cargo.army = -1;	/* don't donate it!! */
      }
      trans_ap->flags &= ~AF_IN_TRANSPORT;
      aflag_clear(trans_ap, AF_IN_TRANSPORT);
    }
    if (cargo_not_empty(&dropped_cargo)) {
      donate_cargo(sp->loc.x, sp->loc.y, user.id, recip_id, &dropped_cargo);
    }
  } 
  adjust_cargo(ap);		/* tell the exec file that cargo is changed */
  if (tw == NULL) { /* Update the sector window */
    just_moved ();
    show_sector (user.cursor.x, user.cursor.y);
  }
  show_armies(sp);
}

void transport_transfer (ap, tw)
Sarmy * ap;
WINDOW * tw;
{
  Snation *np = user.np;
  Ssector *sp = &world.map [ap->pos.x][ap->pos.y];
  Sarmy * recip;
  int recip_num;
  char c;
  char s [EXECLEN];
  int ret, quantity;

  if (tw == NULL) {
    statline2("  [s,m,j,f,p,a,t,*]", "transfer");
  } else {
    wmove (tw, 1, 0);
    wclrtoeol (tw);
    box (tw, '|', '-');
    mvwprintw(tw, 2, 2, "What do you want to transfer? ");
    mvwprintw(tw, 3, 2,
	      "[s]hekels/[m]etal/[j]ewels/[f]ood/[p]eople/[a]rmy/[t]itle/[*]");
    wrefresh (tw);
  }

  do { c = mygetch (); } while (strchr ("*smjaftp ", c) == NULL);

  if (c == ' ') {
    statline2 ("", "");
    return; 
  }

  if (c != 'a' && c != 't' && c != '*') {
    sprintf (s, "(%c) Quantity? ", c);
    if (tw == NULL) {
      statline2 (s, "transfer");
      move (LINES-2, strlen(s));
    } else {
      mvwprintw (tw, 4, 7, s);
      wrefresh (tw);
    }
    ret = wget_number(user.xmode ? stdscr : tw, &quantity);
    if (ret <= 0 || c == ' ') {
      statline2 ("", "");
      return;
    }
  }

  sprintf (s, "Transfer to army #? ");
  if (tw == NULL) {
    statline2 (s, "transfer");
    move (LINES-2, strlen (s));
  } else {
    mvwprintw (tw, 4, 7, "Transfer to army #? ");
    wrefresh (tw);
   }
  ret = wget_number(user.xmode ? stdscr : tw, &recip_num);

  if (ap->id == recip_num || (recip = get_army (user.np, recip_num)) == NULL) {
    statline2 ("", "");
    return;
  }

  /* Actually transfer */
  if (strchr ("at*", c) != NULL) {
    if (c == '*') {
      int capacity;

      if (ap->cargo.army != -1 && recip->cargo.army != -1) {
	statline2_err ("hit space", "only one army allowed per caravan");
	return;
      }
      if (recip->cargo.title.x != -1 && ap->cargo.title.x != -1) {
	statline2_err ("hit space", "only one title allowed per caravan");
	return;
      }
      if (is_cargo (recip)) {
	capacity = recip->n_soldiers * CARAVAN_CAPACITY;
      } else capacity = 0;   /* Someone made a booboo */

      if ((capacity - cargo_weight (&recip->cargo))
	  >= cargo_weight (&ap->cargo)) {
	recip->cargo.money += ap->cargo.money;
	ap->cargo.money = 0;
	recip->cargo.jewels += ap->cargo.jewels;
	ap->cargo.jewels = 0;
	recip->cargo.metal += ap->cargo.metal;
	ap->cargo.metal = 0;
	recip->cargo.food += ap->cargo.food;
	ap->cargo.food = 0;
	recip->cargo.people += ap->cargo.people;
	ap->cargo.people = 0;
	recip->cargo.army = ap->cargo.army;
	ap->cargo.army = -1;
	recip->cargo.title.x = ap->cargo.title.x;
	recip->cargo.title.y = ap->cargo.title.y;
	ap->cargo.title.x = -1;
	ap->cargo.title.y = -1;
      }
      else {
	statline2_err ("hit space", "cargo too big to transfer");
	return;
      }
    }
    else {
      if (load_too_big (np, recip, c, quantity)) {
	statline2_err ("hit space", "load too big");
	return;
      }
      switch (c) {
      case 'a':
	if (recip->cargo.army != -1) {
	  statline2_err ("hit space", "army already loaded");
	  return;
	}
	recip->cargo.army = ap->cargo.army;
	ap->cargo.army = -1;
	break;
      case 't':
	if (recip->cargo.title.x != -1) {
	  statline2_err ("hit space", "title already loaded");
	  return;
	}
	recip->cargo.title.x = ap->cargo.title.x;
	recip->cargo.title.y = ap->cargo.title.y;
	ap->cargo.title.x = -1;
	ap->cargo.title.y = -1;
	break;
      default:
	break;
      }
    }
  }
  else {
    if (load_too_big (np, recip, c, quantity)) {
      statline2_err("hit space", "load too big");
      return;
    }
    switch (c) {
    case 's':
      if (quantity <= ap->cargo.money && quantity > 0) {
	ap->cargo.money -=quantity;
	recip->cargo.money += quantity;
      }
      else statline2_err ("hit space", "Invalid quantity");
      break;
    case 'm':
      if (quantity <= ap->cargo.metal && quantity > 0) {
	ap->cargo.metal -=quantity;
	recip->cargo.metal += quantity;
      }
      else statline2_err ("hit space", "Invalid quantity");
      break;
    case 'j':
      if (quantity <= ap->cargo.jewels && quantity > 0) {
	ap->cargo.jewels -=quantity;
	recip->cargo.jewels += quantity;
      }
      else statline2_err ("hit space", "Invalid quantity");
      break;
    case 'f':
      if (quantity <= ap->cargo.food && quantity > 0) {
	ap->cargo.food -=quantity;
	recip->cargo.food += quantity;
      }
      else statline2_err ("hit space", "Invalid quantity");
      break;
    case 'p':
      if (quantity <= ap->cargo.people && quantity > 0) {
	ap->cargo.people -=quantity;
	recip->cargo.people += quantity;
      }
      else statline2_err ("hit space", "Invalid quantity");
      break;
    default:
      break;
    }
  }
  adjust_cargo (ap);
  adjust_cargo (recip);
  statline2 ("", "");
}

#define is_city(sp) (sp->designation == D_CITY || sp->designation == D_CAPITAL)

/* you can only load a caravan in certain places */
// int good_loading_place(np, sp, type, quantity)
//      Snation *np;
//      Ssector *sp;
//      char type;			/* type of cargo */
//      int quantity;		/* amount, or army id */
int good_loading_place(Snation *np, Ssector *sp, char type, int quantity)
{
  Sarmy *ap, *get_army();

  switch (type) {
  case 's':
    if (!(sp->owner == np->id) || !is_city(sp)) {
      return 0;
    }
    break;
  case 'm':
    if (!(sp->owner == np->id) || !(is_city(sp) ||
				 sp->designation == D_METAL_MINE)) {
      return 0;
    }
    break;
  case 'j':
    if (!(sp->owner == np->id) || !(is_city(sp) ||
				    sp->designation == D_JEWEL_MINE)) {
      return 0;
    }
    break;
  case 'f':
    if (!(sp->owner == np->id) || (!(is_city(sp) ||
				    sp->designation == D_FARM))) {
      return 0;
    }
    break;
  case 'p':
     /* you can only load people in your own non-hostile sectors */
    if (!(sp->owner == np->id)|| !(good_altitude(sp, np))||(has_hostile(sp))) {
      return 0;
    }
    break;
  case 't':
      /* you can only load a title in your own land, 
         and if the sector is not traded. */
    if ((sp->owner != np->id) || !(good_altitude(sp, np))) {
      return 0;
    }
    break;
  case 'a':
    if (!(ap = get_army(np, quantity))
	|| !(army_is_in_sector(sp, np->id, quantity))
	|| ((sp->owner != np->id) && !is_front_line (ap) &&
	sp->designation != D_TRADE_POST)) { 
      return 0;
    }
    break;
  case ' ':			/* this means we don't really load */
    break;
  default:			/* bad type (someone screwed up) */
    return 0;
  }
  return 1;
}

/* you can only unload a caravan in certain places */
// int good_unloading_place(np, sp, type, quantity)
//      Snation *np;
//      Ssector *sp;
//      char type;			/* type of thing being unloaded */
//      int quantity;		/* how much, or army id */
int good_unloading_place(Snation *np, Ssector *sp, char type, int quantity)
{
  Sarmy *ap, *get_army();

  switch (type) {
  case 's':
  case 'm':
  case 'j':
  case 'f':
  case 't':
    if ((np->id == sp->owner) || (is_trade_place(sp))) {
      return 1;
    }
    return 0;
  case '*':
    return 1;
  case 'p':
    if ((sp->designation == D_TRADE_POST || sp->owner == np->id)
	&& good_altitude(sp, np)) {
      return 1;
    }
    return 0;
  case 'a':
    if (!(ap = get_army(np, quantity))) {
      return 0;
    }
    if (!good_army_altitude(np, sp, ap)) {
      return 0;
    }
    if (sp->owner == np->id) {
      return 1;
    }
    if (is_front_line(ap)) {
      return 1;
    }
    if (sp->designation == D_TRADE_POST) {
      return 1;
    }
    return 0;
  case ' ':
    return 1;
  default:
    return 0;
  }
/*
  if (np->id == 0) {
    return 0;
  }
  if (np->id == sp->owner) {
    return 1;
  }
  if ( (np->id != sp->owner) && (sp->designation == D_TRADE_POST) ) {
    return 1;
  }
  return 0;
*/
}


/* keeps a status line on the transportation window which
   which shows you the cargo on the currently selected army */
void cargo_statline(w, up)
     WINDOW *w;
     Suser *up;
{
  char * s;
  Sarmy *ap, *get_army();
  char * make_cargo_statline ();

  if ((ap = get_army(up->np, up->current_army)) == NULL) {
    return;
  }
  
  s = make_cargo_statline (&ap->cargo);

  if (w) {
    mvwaddstr(w, 5, 2, s);
    wclrtoeol(w);
    box (w, '|', '-');
    wrefresh(w);
  } else {			/* if w is NULL, we are in xmode */
    statline(s, "transp[l,u,t]");
  }
}

/* gives cargo from one nation to another */
void donate_cargo(x, y, from_id, to_id, cargo)
     int from_id, to_id;
     Scargo *cargo;
{
  Sarmy *ap, *get_army();
  Ssector *sp;
  FILE *fp, *fopen();
  struct argument args[N_EXEC_ARGS];
  char s[NAMELEN];
  int n_people;

  while (cargo_is_locked()) {
    sleep(1);
  }
  critical();
  lock_cargo();
  if ((fp = fopen(CARGO_FILE, "a")) == NULL) {
    statline("hit space", "serious error:  cannot append to trade file");
    get_space();
    return;
  }
    /* coordinates of where the trade happened */
  fwrite(&x, sizeof(int), 1, fp);
  fwrite(&y, sizeof(int), 1, fp);
  fwrite(&from_id, sizeof(int), 1, fp);
  fwrite(&to_id, sizeof(int), 1, fp);
  fwrite(cargo, sizeof(Scargo), 1, fp);
    /* handle the donation of an army */
  if ((ap = get_army(user.np, cargo->army)) != NULL) {
    fwrite(ap, sizeof(Sarmy), 1, fp);
    if (!is_spirit (ap)) {
      n_people = ap->n_soldiers;
    }
    else n_people = 0;
    sprintf(s, "ADISBAND:%d\n", ap->id);
    gen_exec(s);
    parse_exec_line(s,args);
    run_exec_line(user.np,args);
    /* Unfortunately, ADISBAND puts the people from the disbanding
       onto the trade sector, thus furnishing some free people ... */
    sp = &world.map[ap->pos.x][ap->pos.y];
    cpeople_sector (sp, -n_people);
    sp->n_people -= n_people;
    delete_army_sector(sp, ap);
    delete_army_nation(user.np, ap);
  }
    /* now handle the donation of a land title */
  sp = &world.map[cargo->title.x][cargo->title.y];

  fclose(fp);
  unlock_cargo();
  noncritical();
}

/* lock and unlock the cargo data file */
void lock_cargo()
{
  FILE *fp, *fopen();
  char fname[NAMELEN];

  strcpy(fname, CARGO_FILE);
  strcat(fname, ".LOCK");

  if ((fp = fopen(fname, "w")) == NULL)
  {
    fprintf(stderr,"Error: Cannot open lockfile %s\n",CARGO_FILE);
    clean_exit();
    exit(1);
  }
}

void unlock_cargo()
{
  char fname[NAMELEN];

  strcpy(fname, CARGO_FILE);
  strcat(fname, ".LOCK");

  unlink(fname);
}

/* check if the cargo data file is locked */
int cargo_is_locked()
{
  FILE *fp, *fopen();
  char fname[NAMELEN];

  strcpy(fname, CARGO_FILE);
  strcat(fname, ".LOCK");

  if ((fp = fopen(fname, "r")) == NULL) {
    return 0;
  }
  fclose(fp);
  return 1;
}

/* valid trading places are cities, capitals and trade posts */
int is_trade_place(sp)
     Ssector *sp;
{
  switch (sp->designation) {
  case D_CAPITAL:
  case D_CITY:
  case D_TRADE_POST:
    return 1;
  default:
    return 0;
  }
}

/* returns true if there is a cargo */

int cargo_not_empty(cargop)
     Scargo *cargop;
{
  if (cargop->money > 0 || cargop->metal > 0 || cargop->jewels > 0
      || cargop->food > 0 || cargop->people > 0 || cargop->army != -1
      || cargop->title.x != -1 || cargop->title.y != -1) {
    return 1;
  }
  return 0;
}

/* returns true if we cannot fit this additional load
   onto the ship or caravan. */
// int load_too_big(np, ap, c, quantity)
//      Snation *np;
//      Sarmy *ap;			/* the carrier */
//      char c;			/* the type of load */
//      int quantity;		/* how much */
int load_too_big(Snation *np, Sarmy *ap, char c, int quantity)
{
  Sarmy *cargo_ap;
  float capacity, quantity_weight;

  if (is_cargo (ap)) {
    capacity = ap->n_soldiers * CARAVAN_CAPACITY;
  } else capacity = 0;   /* Someone made a booboo */

  switch (c) {
  case 's':			/* money and other things */
    quantity_weight = quantity * MONEY_WEIGHT;
    break;
  case 'm':
    quantity_weight = quantity * METAL_WEIGHT;
    break;
  case 'j':
    quantity_weight = quantity * JEWEL_WEIGHT;
    break;
  case 'f':
    quantity_weight = quantity * FOOD_WEIGHT;
    break;
  case 'p':
    quantity_weight = quantity;
    break;
  case 'a':
    if ((cargo_ap = get_army(np, quantity))) {
      quantity_weight = army_weight (cargo_ap);
    }
    else { quantity_weight = 0; }  /* no army ?!? */
    break;
  case 't':
    quantity_weight = 0;
    break;
  default: /* Someone made a booboo */
    return 0;
  }
  quantity_weight += cargo_weight (&ap->cargo);

  if (quantity_weight > capacity) { 
    return 1;
  }
  
  return 0;
}

/* Returns the weight of an army, whether normal or a caravan/navy */
int army_weight (ap)
Sarmy * ap;
{
  int weight, index;
  float weight_per;

  index = army_type_index (ap->type);

  if (is_cargo(ap)) {
    weight = caravan_weight (ap);
  }
  else if (is_spirit (ap)) {
    weight = ap->n_soldiers;
  }
  else {
    weight_per = ((float) army_types [index].metal_draft / 100.0) + 
      ((float) army_types [index].metal_maint / 100) + 1.0;
    weight = weight_per * ap->n_soldiers;
  }
  return weight;
}

/* Returns the weight of a caravan, plus any subsidiary cargo */
int caravan_weight (ap)
Sarmy * ap;
{
  int weight = 0, caravan_index;

  caravan_index = army_type_index (ap->type);

  /* Get the weight of the cargo */
  weight += cargo_weight (&ap->cargo);
  /* Caravans have 1000 weight (when empty :-) */
  weight += army_types [caravan_index].metal_draft * ap->n_soldiers;

  return weight;
}

/* Returns the weight of a cargo */
int cargo_weight (cargo)
Scargo * cargo;
{
  int weight = 0;

  if (cargo->army >= 0) {
    weight += army_weight (get_army (user.np, cargo->army));
  }
  weight += cargo->people;
  weight += cargo->jewels * JEWEL_WEIGHT;
  weight += cargo->metal * METAL_WEIGHT;
  weight += cargo->food * FOOD_WEIGHT;
  weight += cargo->money * MONEY_WEIGHT;

  return weight;
}

/* Builds cargo statline out of the cargo struct */
char * make_cargo_statline (cargo)
Scargo * cargo;
{
  char * rstatline;
  char tmps [60];

  if ((rstatline = (char *)malloc (sizeof (char) * 100)) == NULL) {
    clean_exit();
    exit (-1);
  }

  sprintf (rstatline, "(%d mw)", cargo_weight (cargo));

  strcat (rstatline, contents (cargo->money, cargo->metal, cargo->jewels,
			       cargo->food, cargo->people, cargo->army,
			       &cargo->title, 0));

  return rstatline;
}

/* puts out an exec line describing the new cargo of a caravan */
void adjust_cargo(ap)
     Sarmy *ap;
{
  char s[EXECLEN];

  sprintf(s, "ACARGO:%d:%d:%d:%d:%d:%d:%d:%d:%d\n", ap->id, ap->cargo.money,
       ap->cargo.metal, ap->cargo.jewels, ap->cargo.food,
       ap->cargo.people, ap->cargo.army, ap->cargo.title.x, ap->cargo.title.y);
  gen_exec(s);
}
