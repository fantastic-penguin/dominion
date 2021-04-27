  /* spells.c -- the spells are coded here */

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
#include "functions.h"

#include <stdio.h>
#include <string.h>

  /* how many new move points the haste spall gives you */
#define HASTE_POINTS 8

extern Sworld world;
extern Sh_spell *hanging_spells;
extern Suser user;

  /* the spell_structures[] array stores the spell names
     together with the function pointers that execute them.
   */
int spell_caltitude(), spell_fertility(), spell_hide_army(),
  spell_fly_army(), spell_vampire_army(), spell_burrow_army(),
  spell_haste_army(), spell_water_walk(),
  spell_mag_bonus(), spell_merge(), spell_cmetal(), spell_cjewels(),
  spell_fireburst(), spell_inferno(), spell_hide_sector(), spell_castle(),
  spell_sacrifice();

struct spell_struct {  char name[NAMELEN];  int (*func)(); };

static struct spell_struct spell_structures[] = {
  {"caltitude", spell_caltitude}, {"fertility", spell_fertility},
  {"hide_army", spell_hide_army}, {"fly_army", spell_fly_army},
  {"vampire_army", spell_vampire_army}, {"burrow_army", spell_burrow_army},
  {"water_walk", spell_water_walk}, {"haste_army", spell_haste_army},
  {"mag_bonus", spell_mag_bonus}, {"merge", spell_merge},
  {"cmetal", spell_cmetal},
  {"cjewels", spell_cjewels}, {"fireburst", spell_fireburst},
  {"inferno", spell_inferno}, {"hide_sector", spell_hide_sector},
  {"castle", spell_castle,}, {"sacrifice", spell_sacrifice}
};

  /* this actually executes a spell */
int exec_spell(spellp, w)
     Sspell *spellp;
     WINDOW *w;
{
  int cost = 0, i;
  Sh_spell h_spell;
  struct argument exec_args[N_EXEC_ARGS];

  for (i = 0; i < sizeof(spell_structures)/sizeof(struct spell_struct); ++i) {
    if (strcmp(spellp->name, spell_structures[i].name) == 0) {
        /* each spell function returns the cost of casting */
      cost = spell_structures[i].func(&user, w, spellp, &h_spell);
      if (cost > 0) {		/* only if there is a real cost */
	user.np->spell_pts -= cost;
	cspell_pts(user.np, -cost);
	  /* add the spell to the list of hanging spells */
	add_h_spell(&user.h_spells, &h_spell);
	add_h_spell(&hanging_spells, &h_spell);
	  /* execute the spell */
	for (i = 0; i < h_spell.n_lines; ++i) {
	  if (i % 2 == 0) {
	    parse_exec_line(h_spell.lines[i], exec_args);
	    run_exec_line(user.np, exec_args);
	  }
	}
	  /* should put this in a more efficient place */
	write_h_spells();
      }
      return cost;		/* we found our spell, now return */
    }
  }
  return cost;
}

  /* terraforming spells */
int spell_caltitude(up, w, spellp, h_spellp)
     Suser *up;
     WINDOW *w;
     Sspell *spellp;
     Sh_spell *h_spellp;
{
  Ssector *sp = &world.map[up->cursor.x][up->cursor.y];
  int cost, change = 0;
  char s[EXECLEN];

  if (w) {
    mvwprintw(w, 4, 1,
	 "Cost %d spell points.  Do you wish to [+]raise or [-]lower sector? ",
	 spellp->cost);
    wrefresh(w);
  } else {
    sprintf(s, "Costs %d; [+] or [-]? ", spellp->cost);
    statline(s, "cast_spell");
    move(LINES-1, strlen(s));
  }
  cost =  spellp->cost;
  switch (getch()) {
  case '+':
    change = 1;
    break;
  case '-':
    change = -1;
    break;
  default:
    break;
  }
    /* only apply the spell if the new altitude is good */
  if (sp->altitude+change > MOUNTAIN_PEAK ||  sp->altitude+change < TRENCH) {
    return 0;
  }
    /* prepare the h_spell struct, and then insert the exec lines */
  prepare_h_spell(h_spellp, spellp->name, up->id, spellp->duration, 2);
  sprintf(h_spellp->lines[0], "CALTITUDE_SECTOR:%d:%d:%d\n",
	  sp->loc.x, sp->loc.y, change);
  sprintf(h_spellp->lines[1], "CALTITUDE_SECTOR:%d:%d:%d\n",
	  sp->loc.x, sp->loc.y, -change);
  return cost;
}

int spell_fertility(up, w, spellp, h_spellp)
     Suser *up;
     WINDOW *w;
     Sspell *spellp;
     Sh_spell *h_spellp;
{
  Ssector *sp = &world.map[up->cursor.x][up->cursor.y];
  int cost, change = 0;
  char s[EXECLEN];

  if (w) {
    mvwprintw(w, 4, 1,
     "Cost %d spell points.  Do you wish to [+]increase or [-]decrease soil? ",
     spellp->cost);
    wrefresh(w);
  } else {
    sprintf(s, "Costs %d; [+] or [-]? ", spellp->cost);
    statline(s, "cast_spell");
    move(LINES-1, strlen(s));
  }
  cost =  spellp->cost;
  switch (getch()) {
  case '+':
    change = 1;
    break;
  case '-':
    change = -1;
    break;
  default:
    break;
  }

    /* prepare the h_spell struct, and then insert the exec lines */
  prepare_h_spell(h_spellp, spellp->name, up->id, spellp->duration, 2);
  sprintf(h_spellp->lines[0], "CSOIL_SECTOR:%d:%d:%d\n",
	  sp->loc.x, sp->loc.y, change);
  sprintf(h_spellp->lines[1], "CSOIL_SECTOR:%d:%d:%d\n",
	  sp->loc.x, sp->loc.y, -change);
  return cost;
}

int spell_cmetal(up, w, spellp, h_spellp)
     Suser *up;
     WINDOW *w;
     Sspell *spellp;
     Sh_spell *h_spellp;
{
  Ssector *sp = &world.map[up->cursor.x][up->cursor.y];
  int cost, change = 0;
  char s[EXECLEN];

  if (w) {
    mvwprintw(w, 4, 1,
    "Cost %d spell points.  Do you wish to [+]increase or [-]decrease metal? ",
    spellp->cost);
    wrefresh(w);
  } else {
    sprintf(s, "Costs %d; [+] or [-]? ", spellp->cost);
    statline(s, "cast_spell");
    move(LINES-1, strlen(s));
  }
  cost =  spellp->cost;
  switch (getch()) {
  case '+':
    change = 1;
    break;
  case '-':
    change = -1;
    break;
  default:
    break;
  }

    /* prepare the h_spell struct, and then insert the exec lines */
  prepare_h_spell(h_spellp, spellp->name, up->id, spellp->duration, 2);
  sprintf(h_spellp->lines[0], "CMETAL_SECTOR:%d:%d:%d\n",
	  sp->loc.x, sp->loc.y, change);
  sprintf(h_spellp->lines[1], "CMETAL_SECTOR:%d:%d:%d\n",
	  sp->loc.x, sp->loc.y, -change);
  return cost;
}

int spell_cjewels(up, w, spellp, h_spellp)
     Suser *up;
     WINDOW *w;
     Sspell *spellp;
     Sh_spell *h_spellp;
{
  Ssector *sp = &world.map[up->cursor.x][up->cursor.y];
  int cost, change = 0;
  char s[EXECLEN];


  if (w) {
    mvwprintw(w, 4, 1,
   "Cost %d spell points.  Do you wish to [+]increase or [-]decrease jewels? ",
   spellp->cost);
    wrefresh(w);
  } else {
    sprintf(s, "Costs %d; [+] or [-]? ", spellp->cost);
    statline(s, "cast_spell");
    move(LINES-1, strlen(s));
  }
  cost =  spellp->cost;
  switch (getch()) {
  case '+':
    change = 1;
    break;
  case '-':
    change = -1;
    break;
  default:
    break;
  }

    /* prepare the h_spell struct, and then insert the exec lines */
  prepare_h_spell(h_spellp, spellp->name, up->id, spellp->duration, 2);
  sprintf(h_spellp->lines[0], "CJEWELS_SECTOR:%d:%d:%d\n",
	  sp->loc.x, sp->loc.y, change);
  sprintf(h_spellp->lines[1], "CJEWELS_SECTOR:%d:%d:%d\n",
	  sp->loc.x, sp->loc.y, -change);
  return cost;
}

  /* this really messes up a sector */
int spell_fireburst(up, w, spellp, h_spellp)
     Suser *up;
     WINDOW *w;
     Sspell *spellp;
     Sh_spell *h_spellp;
{
  Ssector *sp = &world.map[up->cursor.x][up->cursor.y];
  int cost = 0, change = 0;
  char c;
  char s[EXECLEN];

  if (w) {
    mvwprintw(w, 4, 1, "Cost %d spell points.  Go ahead? ", spellp->cost);
    wclrtoeol(w);
    wrefresh(w);
  } else {
    sprintf(s, "Costs %d; go ahead? ", spellp->cost);
    statline_prompt(s, spellp->name);
  }
  if ((c = getch()) == 'y' || c == 'Y') {
      /* prepare the h_spell struct, and then insert the exec lines */
    cost =  spellp->cost;
    prepare_h_spell(h_spellp, spellp->name, up->id, spellp->duration, 6);
    sprintf(h_spellp->lines[0], "CSOIL_SECTOR:%d:%d:%d\n",
	    sp->loc.x, sp->loc.y, -sp->soil);
    sprintf(h_spellp->lines[1], "CSOIL_SECTOR:%d:%d:%d\n",
	    sp->loc.x, sp->loc.y, sp->soil);
    sprintf(h_spellp->lines[2], "DESIG_SECTOR:%d:%d:%d\n", sp->loc.x,
	    sp->loc.y, sp->designation == D_CAPITAL ? D_CAPITAL : D_NODESIG);
    sprintf(h_spellp->lines[3], "DESIG_SECTOR:%d:%d:%d\n",
	    sp->loc.x, sp->loc.y, sp->designation);
    sprintf(h_spellp->lines[4], "CPEOPLE_SECTOR:%d:%d:%d\n",
	    sp->loc.x, sp->loc.y, -sp->n_people/3);
    sprintf(h_spellp->lines[5], "CPEOPLE_SECTOR:%d:%d:%d\n",
	    sp->loc.x, sp->loc.y, sp->n_people/3);
    user.just_moved = 1;
  }
  return cost;
}
  /* makes a sector completely inaccessible */
int spell_inferno(up, w, spellp, h_spellp)
     Suser *up;
     WINDOW *w;
     Sspell *spellp;
     Sh_spell *h_spellp;
{
  Ssector *sp = &world.map[up->cursor.x][up->cursor.y];
  int cost = 0, change = 0;
  char c;
  char s[EXECLEN];

    /* you can only cast inferno on your own sectors */
  if (sp->owner != up->id) {
    return -1;
  }
  if (w) {
    mvwprintw(w, 4, 1, "Cost %d spell points.  Go ahead? ", spellp->cost);
    wclrtoeol(w);
    wrefresh(w);
  } else {
    sprintf(s, "Costs %d; go ahead? ", spellp->cost);
    statline_prompt(s, spellp->name);
  }
  if ((c = getch()) == 'y' || c == 'Y') {
      /* prepare the h_spell struct, and then insert the exec lines */
    cost =  spellp->cost;
    prepare_h_spell(h_spellp, spellp->name, up->id, spellp->duration, 4);
    sprintf(h_spellp->lines[2], "DESIG_SECTOR:%d:%d:%d\n", sp->loc.x,
	    sp->loc.y, sp->designation == D_CAPITAL ? D_CAPITAL : D_NODESIG);
    sprintf(h_spellp->lines[3], "DESIG_SECTOR:%d:%d:%d\n", sp->loc.x,
	    sp->loc.y, sp->designation == D_CAPITAL ? D_CAPITAL : D_NODESIG);
    sprintf(h_spellp->lines[0], "FLAG_SET_SECTOR:%d:%d:%d\n",
	    sp->loc.x, sp->loc.y, SF_IMPENETRABLE);
    sprintf(h_spellp->lines[1], "FLAG_CLEAR_SECTOR:%d:%d:%d\n",
	  sp->loc.x, sp->loc.y, SF_IMPENETRABLE);
      /* also:  kill all the people, once and permanently */
    cpeople_sector(sp, -sp->n_people);
    sp->n_people = 0;
    user.just_moved = 1;
  }
  return cost;
}

  /* these spells set various army flags */
int spell_hide_army(up, w, spellp, h_spellp)
     Suser *up;
     WINDOW *w;
     Sspell *spellp;
     Sh_spell *h_spellp;
{
  int id, cost;			/* army id and cost of casting */
  Sarmy *ap, *get_army();
  char s[EXECLEN];

  if (w) {
    mvwprintw(w, 4, 1,
      "Cost: %d spell pts./100 sold.  Which army do you want to hide? ",
      spellp->cost);
    wrefresh(w);
  } else {
    sprintf(s, "Cost %d pts/100 men; which army? ", spellp->cost);
    statline_prompt(s, "fly_army");
  }
  if ( (wget_number(w, &id) <= 0) ||
       ((ap = get_army(up->np, id)) == NULL) ||
       (ap->pos.x != up->cursor.x) || (ap->pos.y != up->cursor.y) ) {
    return -1;
  }
  cost = (spellp->cost * ap->n_soldiers + 99)/100;
  if (cost > up->np->spell_pts) {
    return -1;
  }
  if (w) {
    wmove(w, 4, 1);
    wclrtoeol(w);
  }
    /* prepare the h_spell struct, and then insert the exec lines */
  prepare_h_spell(h_spellp, spellp->name, up->id, spellp->duration, 2);
  sprintf(h_spellp->lines[0], "AFLAG_SET:%d:%d\n",
	  ap->id, AF_HIDDEN);
  sprintf(h_spellp->lines[1], "AFLAG_CLEAR:%d:%d\n",
	  ap->id, AF_HIDDEN);
  return cost;
}

int spell_fly_army(up, w, spellp, h_spellp)
     Suser *up;
     WINDOW *w;
     Sspell *spellp;
     Sh_spell *h_spellp;
{
  int id, cost;			/* army id and cost of casting */
  Sarmy *ap, *get_army();
  char s[EXECLEN];

  if (w) {
    mvwprintw(w, 4, 1,
	    "Cost: %d spell pts./100 sold.  Which army do you want to fly? ",
	    spellp->cost);
    wrefresh(w);
  } else {
    sprintf(s, "Cost %d pts/100 men; which army? ", spellp->cost);
    statline_prompt(s, "hide_army");
  }
  if ( (wget_number(w, &id) <= 0) ||
       ((ap = get_army(up->np, id)) == NULL) ||
       (ap->pos.x != up->cursor.x) || (ap->pos.y != up->cursor.y) ) {
    return -1;
  }
  cost = (spellp->cost * ap->n_soldiers + 99)/100;
  if (cost > up->np->spell_pts) {
    return -1;
  }
  if (w) {
    wmove(w, 4, 1);
    wclrtoeol(w);
  }
    /* prepare the h_spell struct, and then insert the exec lines */
  prepare_h_spell(h_spellp, spellp->name, up->id, spellp->duration, 2);
  sprintf(h_spellp->lines[0], "AFLAG_SET:%d:%d\n",
	  ap->id, AF_FLIGHT);
  sprintf(h_spellp->lines[1], "AFLAG_CLEAR:%d:%d\n",
	  ap->id, AF_FLIGHT);
  return cost;
}

int spell_vampire_army(up, w, spellp, h_spellp)
     Suser *up;
     WINDOW *w;
     Sspell *spellp;
     Sh_spell *h_spellp;
{
  int id, cost;			/* army id and cost of casting */
  Sarmy *ap, *get_army();
  char s[EXECLEN];

  if (w) {
    mvwprintw(w, 4, 1,
      "Cost: %d spell pts./100 sold.  Which army do you want to vampirize? ",
       spellp->cost);
    wrefresh(w);
  } else {
    sprintf(s, "Cost %d pts/100 men; which army? ", spellp->cost);
    statline_prompt(s, "vampire_army");
  }
  if ( (wget_number(w, &id) <= 0) ||
       ((ap = get_army(up->np, id)) == NULL) ) {
    return -1;
  }
  cost = (spellp->cost * ap->n_soldiers + 99)/100;
  if (w) {
    wmove(w, 4, 1);
    wclrtoeol(w);
  }
    /* prepare the h_spell struct, and then insert the exec lines */
  prepare_h_spell(h_spellp, spellp->name, up->id, spellp->duration, 2);
  sprintf(h_spellp->lines[0], "AFLAG_SET:%d:%d\n",
	  ap->id, AF_VAMPIRE);
  sprintf(h_spellp->lines[1], "AFLAG_CLEAR:%d:%d\n",
	  ap->id, AF_VAMPIRE);
  return cost;
}
int spell_burrow_army(up, w, spellp, h_spellp)
     Suser *up;
     WINDOW *w;
     Sspell *spellp;
     Sh_spell *h_spellp;
{
  int id, cost;			/* army id and cost of casting */
  Sarmy *ap, *get_army();
  char s[EXECLEN];

  if (w) {
    mvwprintw(w, 4, 1,
      "Cost: %d spell pts./100 sold.  Which army do you want to send underground? ",
       spellp->cost);
    wrefresh(w);
  } else {
    sprintf(s, "Cost %d pts/100 men; which army? ", spellp->cost);
    statline_prompt(s, "burrow_army");
  }
  if ( (wget_number(w, &id) <= 0) ||
       ((ap = get_army(up->np, id)) == NULL) ) {
    return -1;
  }
  cost = (spellp->cost * ap->n_soldiers + 99)/100;
  if (w) {
    wmove(w, 4, 1);
    wclrtoeol(w);
  }
    /* prepare the h_spell struct, and then insert the exec lines */
  prepare_h_spell(h_spellp, spellp->name, up->id, spellp->duration, 2);
  sprintf(h_spellp->lines[0], "AFLAG_SET:%d:%d\n",
	  ap->id, AF_UNDERGROUND);
  sprintf(h_spellp->lines[1], "AFLAG_CLEAR:%d:%d\n",
	  ap->id, AF_UNDERGROUND);
  return cost;
}

int spell_water_walk(up, w, spellp, h_spellp)
     Suser *up;
     WINDOW *w;
     Sspell *spellp;
     Sh_spell *h_spellp;
{
  int id, cost;			/* army id and cost of casting */
  Sarmy *ap, *get_army();
  char s[EXECLEN];

  if (w) {
    mvwprintw(w, 4, 1,
    "Cost: %d spell pts./100 sold.  Which army do you want to walk on water? ",
       spellp->cost);
    wrefresh(w);
  } else {
    sprintf(s, "Cost %d pts/100 men; which army? ", spellp->cost);
    statline_prompt(s, "walk_water");
  }
  if ( (wget_number(w, &id) <= 0) ||
       ((ap = get_army(up->np, id)) == NULL) ||
       (ap->pos.x != up->cursor.x) || (ap->pos.y != up->cursor.y) ) {
    return -1;
  }
  cost = (spellp->cost * ap->n_soldiers + 99)/100;
  if (cost > up->np->spell_pts) {
    return -1;
  }
  if (w) {
    wmove(w, 4, 1);
    wclrtoeol(w);
  }
    /* prepare the h_spell struct, and then insert the exec lines */
  prepare_h_spell(h_spellp, spellp->name, up->id, spellp->duration, 2);
  sprintf(h_spellp->lines[0], "AFLAG_SET:%d:%d\n",
	  ap->id, AF_WATER);
  sprintf(h_spellp->lines[1], "AFLAG_CLEAR:%d:%d\n",
	  ap->id, AF_WATER);
  return cost;
}

int spell_haste_army(up, w, spellp, h_spellp)
     Suser *up;
     WINDOW *w;
     Sspell *spellp;
     Sh_spell *h_spellp;
{
  int id, cost;			/* army id and cost of casting */
  Sarmy *ap, *get_army();
  char s[EXECLEN];

  if (w) {
    mvwprintw(w, 4, 1,
      "Cost: %d spell pts./100 sold.  Which army do you want to hasten? ",
       spellp->cost);
    wrefresh(w);
  } else {
    sprintf(s, "Cost %d pts/100 men; which army? ", spellp->cost);
    statline_prompt(s, "haste_army");
  }
  if ( (wget_number(w, &id) <= 0) ||
       ((ap = get_army(up->np, id)) == NULL) ) {
    return -1;
  }
  cost = (spellp->cost * ap->n_soldiers + 99)/100;
  if (w) {
    wmove(w, 4, 1);
    wclrtoeol(w);
  }
    /* prepare the h_spell struct, and then insert the exec lines */
  prepare_h_spell(h_spellp, spellp->name, up->id, spellp->duration, 0);
/*  sprintf(h_spellp->lines[0], "AFLAG_SET:%d:%d\n",
	  ap->id, AF_VAMPIRE);
  sprintf(h_spellp->lines[1], "AFLAG_CLEAR:%d:%d\n",
	  ap->id, AF_VAMPIRE);
*/
  ap->mvpts += HASTE_POINTS;	/* increase movement */
    /* make the exec line for this move point change; we
       implement it as a trivial movement with change in move points.
     */
  sprintf(s, "AMOVE:%d:%d:%d:%d\n",
	  ap->id, ap->pos.x, ap->pos.y, ap->mvpts);
  gen_exec(s);
  return cost;
}

int spell_mag_bonus(up, w, spellp, h_spellp)
     Suser *up;
     WINDOW *w;
     Sspell *spellp;
     Sh_spell *h_spellp;
{
  int id, cost;			/* army id and cost of casting */
  Sarmy *ap, *get_army();
  char s[EXECLEN];

  if (w) {
    mvwprintw(w, 4, 1,
       "Cost: %d spell pts./100 sold.  Which army do you want to enhance? ",
       spellp->cost);
    wrefresh(w);
  } else {
    sprintf(s, "Cost %d pts/100 men; which army? ", spellp->cost);
    statline_prompt(s, "mag_bonus");
  }
  if ( (wget_number(w, &id) <= 0) ||
       ((ap = get_army(up->np, id)) == NULL) ||
       (ap->pos.x != up->cursor.x) || (ap->pos.y != up->cursor.y) ) {
    return -1;
  }
  cost = (spellp->cost * ap->n_soldiers + 99)/100;
  if (cost > up->np->spell_pts) {
    return -1;
  }
  if (w) {
    wmove(w, 4, 1);
    wclrtoeol(w);
  }
    /* prepare the h_spell struct, and then insert the exec lines */
  prepare_h_spell(h_spellp, spellp->name, up->id, spellp->duration, 2);
  sprintf(h_spellp->lines[0], "CABONUS:%d:%d\n", ap->id, 30);
  sprintf(h_spellp->lines[1], "CABONUS:%d:%d\n", ap->id, -30);
  return cost;
}

  /* this is a Unity spell which merges people into
     an existing spirit.  you can only make the spirit
     as big as twice its basic size.  the people
     are taken from the current sector.  this is permanent,
     it is not a hanging spell.  In fact, it is more of
     an ability than a spell.
   */
int spell_merge(up, w, spellp, h_spellp)
     Suser *up;
     WINDOW *w;
     Sspell *spellp;
     Sh_spell *h_spellp;
{
  int id, cost, ind;		/* army id, cost of casting and spirit index */
  extern struct spirit_type *spirit_types;
  Sarmy *ap, *get_army();
  char s[EXECLEN];
  int n_merged = 0;		/* number of people merged */
  Ssector *sp = &world.map[up->cursor.x][up->cursor.y];

  if (w) {
    mvwprintw(w, 4, 1,
       "Cost: %d spell pts./100 sold.  Which spirit do you want to enhance? ",
       spellp->cost);
    wrefresh(w);
  } else {
    sprintf(s, "Cost %d pts/100 men; which spirit? ", spellp->cost);
    statline_prompt(s, "merge");
  }
  if ( (wget_number(w, &id) <= 0)
      || ((ap = get_army(up->np, id)) == NULL)
      || (ap->pos.x != up->cursor.x) || (ap->pos.y != up->cursor.y)
      || !is_spirit(ap) ) {
    return -1;
  }
  if (has_hostile(sp) || sp->owner != user.id) {
    statline2_err ("hit space", "cannot merge on a hostile sector");
    return -1;
  }
  if (is_spelled(ap)) {
    statline2_err ("hit space", "cannot merge on a spelled army");
    return -1;
  }
  ind = spirit_type_index(ap->type);
  if (w) {
    mvwprintw(w, 4, 1,
	      "How many men do you want to merge (or subtract) (max %d)? ",
	      2*spirit_types[ind].size - ap->n_soldiers);
    wclrtoeol(w);
    wrefresh(w);
  } else {
    sprintf(s, "Merge how many (max %d)? ",
	    2*spirit_types[ind].size - ap->n_soldiers);
    statline_prompt(s, "merge");
  }
    /* make sure that the spirit will not become too big or too small */
  if ( (wget_number(w, &n_merged) <= 0)
      || (n_merged > 0 && n_merged > 2*spirit_types[ind].size - ap->n_soldiers)
      || n_merged < -ap->n_soldiers+1 || n_merged > sp->n_people) {
    return -1;
  }
  cost = (spellp->cost * ap->n_soldiers + 99)/100; /* rounding up */
  if (cost > 0 && cost > up->np->spell_pts) {
    return -1;
  }
  if (w) {
    wmove(w, 4, 1);
    wclrtoeol(w);
  }
    /* this is not a hanging spell.  we just do the work once. */
  ap->n_soldiers += n_merged;
  ap->mvpts = 0;
  ap->mvratio = 0;
  sprintf(s, "AINCREASE:%d:%d\n", ap->id, n_merged);
  gen_exec(s);
  sp->n_people -= n_merged;
  cpeople_sector(sp, -n_merged);
  user.just_moved = 1;
  return -1;
}

  /* hide a sector from view */
int spell_hide_sector(up, w, spellp, h_spellp)
     Suser *up;
     WINDOW *w;
     Sspell *spellp;
     Sh_spell *h_spellp;
{
  Ssector *sp = &world.map[up->cursor.x][up->cursor.y];
  int cost;
  char s[EXECLEN];

  if (w) {
    mvwprintw(w, 4, 1,
    "Cost %d spell points.  Go ahead (y/n)? ",
    spellp->cost);
    wrefresh(w);
  } else {
    sprintf(s, "Costs %d; go ahead (y/n)? ", spellp->cost);
    statline_prompt(s, "hide_sector");
  }
  cost =  spellp->cost;
  switch (getch()) {
  case 'y':
  case 'Y':
    break;
  default:
    return -1;
  }

  /* prepare the h_spell struct, and then insert the exec lines */
  prepare_h_spell(h_spellp, spellp->name, up->id, spellp->duration, 2);
  sprintf(h_spellp->lines[0], "FLAG_SET_SECTOR:%d:%d:%d\n",
	  sp->loc.x, sp->loc.y, SF_HIDDEN);
  sprintf(h_spellp->lines[1], "FLAG_CLEAR_SECTOR:%d:%d:%d\n",
	  sp->loc.x, sp->loc.y, SF_HIDDEN);
  user.just_moved = 1;
  return cost;
}

  /* these are used to set and clear army flags (in the exec file) */
void aflag_set(ap, flag)
     Sarmy *ap;
     int flag;
{
  char s[EXECLEN];

  sprintf(s, "AFLAG_SET:%d:%d\n", ap->id, flag);
  gen_exec(s);
}

void aflag_clear(ap, flag)
     Sarmy *ap;
     int flag;
{
  char s[EXECLEN];

  sprintf(s, "AFLAG_CLEAR:%d:%d\n", ap->id, flag);
  gen_exec(s);
}

/* For chess; transfers the spirit to the nations capital  */
int spell_castle (up, w, spellp, h_spellp)
     Suser * up;
     WINDOW * w;
     Sspell * spellp;
     Sh_spell * h_spellp;
{
  Ssector * sp; 
  Ssector * capital = &world.map [up->np->capital.x][up->np->capital.y];
  Ssector * cur = &world.map [up->cursor.x][up->cursor.y];
  Sarmy * ap;
  char s [EXECLEN];
  int army_num;

  if (w) {
    mvwprintw (w, 4, 1,
	       "Costs %d spell points.  Go ahead (y/n)? ", spellp->cost);
    wrefresh (w);
  } else {
    sprintf (s, "Costs %d; go ahead (y/n)? ", spellp->cost);
    statline (s, "castle");
  }
    
  switch (getch ()) {
  case 'y':
  case 'Y':
    break;
  default:
    return -1;
  }

  sprintf (s, "Castle army #");
  if (w) {
    wmove (w, 4,1);
    wclrtoeol (w);
    box (w, '|', '-');
    mvwprintw (w, 4, 1, s);
  }
  else {
    statline (s, "castle");
    move (LINES-1, strlen (s));
    refresh ();
  }
  if (wget_number (w, &army_num) <= 0) {
    return -1;
  }
  if ((ap = get_army (up->np, army_num)) == NULL) {
    return -1;
  }
      
  if (ap->pos.x != up->cursor.x || ap->pos.y != up->cursor.y) {
    statline2_err ("Hit space", "Must be on the same sector as army");
    return 0;
  }

  sp = &world.map[ap->pos.x][ap->pos.y];

  delete_army_sector (sp, ap);

  ap->pos.x = capital->loc.x;
  ap->pos.y = capital->loc.y;
  
  insert_army_sector (capital, ap);

  sprintf (s, "ACASTLE:%d\n", ap->id);
  gen_exec (s);

  cspell_pts (up->np, spellp->cost);
  up->np->spell_pts -= spellp->cost;
  return -1;
}

int spell_sacrifice(up, w, spellp, h_spellp)
     Suser * up;
     WINDOW * w;
     Sspell * spellp;
     Sh_spell * h_spellp;
{
  char s [EXECLEN];
  Ssector * sp = &world.map [up->cursor.x][up->cursor.y];
  int num;

  sprintf (s, "Sacrifice how many people (%d people per sp. point)? ",
	   SACRIFICED_FRACT);

  statline_prompt(s, "sacrifice");
  if (wget_number(stdscr, &num) <= 0) {
    return 0;
  }
  if (sp->owner != up->np->id) {
    statline2_err ("hit space", "sorry, must sacrifice your own people");
    return 0;
  } else if (has_hostile (sp)) {
    statline2_err ("hit space", "sorry, must sacrifice friendly people");
    return 0;
  } else if (sp->n_people < num) {
    statline2_err ("hit space", "too many!");
    return 0;
  } else if (num < 0) {
    statline2_err ("hit space", "nice try");
    return 0;
  }

  cpeople_sector (sp, -num);
  cspell_pts (up->np, (int)(num / SACRIFICED_FRACT));
  sp->n_people -= num;
  up->np->spell_pts += (int) (num / SACRIFICED_FRACT);
  up->just_moved = 1;

  return 0;
}
