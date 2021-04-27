/* economy.c -- routines which deal with the economy, revenue and
               expenses of all types.
 */

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

/* calc_revenue(np) - find the amount of tax revenue for a nation  */
/* calc_metal(np) - find the amount of metal produced by nation    */
/* calc_jewels(np) - find the amount of jewels produced by nation  */
/* calc_food(np) - find the amount of food produced by nation      */
/* calc_expend(np) - calculate a nation's expenditures             */
/* calc_expend_metal(np) - calculate metal expenditures for nation */
/* calc_expend_jewels(np) - calculate jewel expenditures for nation*/
/* calc_expend_food(np) - calculate food expenditures for a nation */
/* sector_metal(sp) - calculate metal produced in a single sector  */
/* sector_jewels(sp) - calculate jewels produced in single sector  */
/* sector_food(sp) - calculate food produced in a single sector    */
/* military_maint(np) - returns total amount of money spent for armies */
/* military_maint_metal(np) - total amount of metal spent for armies   */
/* military_maint_jewels(np) - total amount of jewels spent for armies */
/* army_maint_money(ap) - amount of money needed to maintain given army*/
/* construct_cost(type) - cost to construct object                 */
/* construct_cost_metal(type) - metal cost to construct object     */
/* get_employed(np) - how many employed in that nation             */
/* get_emp_met(np) - how many employed metal miners                */
/* get_emp_jws(np) - how many employed jewel miners                */
/* get_emp_farm(np) - how many employed farmers                    */
/* (same for unemployed)                                           */
/* get_avg_money(wp) - average money in the world                  */
/* emp_desire(np,x,y) - gives the employment desireability in x,y  */
/* n_workers(sp) - number of people employed in that sect.         */
/* prod_level(np) - Relative level of production of a nation       */

#include "dominion.h"
#include "misc.h"
#include "army.h"
#include "costs.h"
#include "cur_stuff.h"
#include "functions.h"
#include <stdio.h>
#include <ctype.h>
#include <signal.h>
#include <math.h>

extern Sworld world;
extern struct s_desig_map desig_map[];
extern struct s_altitude_map altitude_map[];
extern struct item_map terrains[];
extern struct item_map climates[];
extern Suser user;
extern int debug;
extern int (*wrapx)(), (*wrapy)();

#define PROD_POWER (7.0/6.0)

// forward decl
int army_maint_money(Sarmy *ap);

double prod_level(np)
     Snation *np;
{
  return (1.0 - pow((double)(np->taxes)/100.0,PROD_POWER));
}

  /* calculate the amount of tax revenue for a given nation */
int calc_revenue(np)
     Snation *np;
{
  Ssector *sp;
  struct pt_list *points;
  int income = 0, taxes = np->taxes, pop;
    /* first thing: run through the list of that nation's sectors
       and see what income might come from the various sectors
     */
  if ((points = np->ptlist) == NULL) {
    return 0;			/* nation has no sectors!! */
  }
  do {
    sp = &(world.map[points->pt.x][points->pt.y]);
    pop = n_workers(sp);
    income+=(taxes*prod_level(np)*pop*desig_map[sp->designation].revenue)/100;
  } while ((points = points->next) != NULL);
  return income;
}


/* int calc_metal (Snation *) -- calculates metal owned by all sectors owned.
   - Cycles through all sectors owned, and computes metal production for
     that sector.
   - Sums them up, and returns the total.
*/ 

int calc_metal (np)
     Snation *np;
{
  Ssector *sp;
  struct pt_list *points;
  int metal_total = 0;
  
  if ((points = np->ptlist) == NULL) {
    return 0;			/* nation has no sectors!! */
  }
  do {
    sp = &(world.map [points->pt.x][points->pt.y]);
    switch (sp->designation) {
    case D_METAL_MINE:
      metal_total += sector_metal(sp);
      break;
    default:
      break;
    }
  } while ((points = points->next) != NULL);
  
  return (metal_total);
}


/* int calc_jewels (Snation *) -- calculates jewels owned by all sectors owned.
 *  - Cycles through all sectors owned.
 *  - If sector is a jewel mine, computes the jewels by:
 *        jewels_per_civilian * number_of_people
 *  - returns summation.
 */

int calc_jewels (np)
Snation *np;
{
  Ssector *sp;
  struct pt_list *points;
  int jewels_total = 0;

  if ((points = np->ptlist) == NULL) {
    return 0;			/* nation has no sectors!! */
  }
  do {
    sp = &(world.map[points->pt.x][points->pt.y]);
    
    switch (sp->designation) {
    case D_JEWEL_MINE:
      jewels_total += sector_jewels(sp);
      break;
    default:
      break;
    }
  } while ((points = points->next) != NULL);

  return (jewels_total);
}

  /* how much food is produced in the nation */
int calc_food(np)
     Snation *np;
{
  Ssector *sp;
  struct pt_list *points;
  int food_total = 0;

  if ((points = np->ptlist) == NULL) {
    return 0;			/* nation has no sectors!! */
  }
  do {
    sp = &(world.map[points->pt.x][points->pt.y]);
    switch (sp->designation) {
    case D_FARM:
      food_total += sector_food(sp);
      break;
    default:
      break;
    }
  } while ((points = points->next) != NULL);
  
  return food_total;
}

  /* calculate a nation's expenditures */
int calc_expend(np)
     Snation *np;
{
  int percent;

  percent = np->charity + np->tech_r_d + np->mag_r_d + np->spy_r_d;

  return (percent*calc_revenue(np)) / 100 + military_maint(np)
    + non_profit_maint(np);;
}

int calc_expend_metal(np)
     Snation *np;
{
  int percent;

  percent = np->tech_r_d_metal;

  return (percent*calc_metal(np)) / 100 + military_maint_metal(np);
}

int calc_expend_jewels(np)
     Snation *np;
{
  int percent;

  percent = np->mag_r_d_jewels;

  return (percent*calc_jewels(np)) / 100 + military_maint_jewels(np);
}

int calc_expend_food(np)
     Snation *np;
{
  int food_eaten;

  food_eaten = (int) (get_n_civil(np) * EAT
		      + SOLD_EAT_FACTOR * get_n_soldiers(np) * EAT);

  return food_eaten;
}

  /* now come a couple of routines that calculate
     the amount of a resource (metal, jewels, food)
     for a SINGLE sector
   */
int sector_metal(sp)
     Ssector *sp;
{
  int i, j, n_refineries = 0;
  float metal;
  Snation *np = &world.nations[sp->owner];

    /* see how many active refineries are close by.
       a refinery is active if there are the minimum number
       of people emplyed in it.
     */
  for (i = sp->loc.x-1; i <= sp->loc.x+1; ++i) {
    for (j = sp->loc.y-1; j <= sp->loc.y+1; ++j) {
      if ((world.map[(*wrapx)(i,j)][(*wrapy)(i,j)].designation == D_REFINERY)
	  &&
	  (world.map[(*wrapx)(i,j)][(*wrapy)(i,j)].n_people
	   >= desig_map[D_REFINERY].min_employed)) {
	++n_refineries;
      }
    }
  }
    /* add to metal production depending on the
       number of refineries and the mining skill
     */
  metal = n_workers(sp) * (1.0 + np->mine_skill/100.0) * sp->metal;
  metal *= (1.0 + REFINERY_FACT*n_refineries);
  
  return (int)(metal * prod_level(np));
}

int sector_jewels(sp)
     Ssector *sp;
{
  Snation *np = &world.nations[sp->owner];
  float jewels;
  
  jewels = n_workers(sp) * (1.0 + np->mine_skill/100.0) * sp->jewels;

  return (int) (jewels * prod_level(np));
}

int sector_food(sp)
     Ssector *sp;
{
  Snation *np = &world.nations[sp->owner];
  float food;

  food = (1.0 + np->farm_skill/100.0) * ((sp->soil+1)/6.0)
    * n_workers(sp) * FOOD_PROD;

  return (int) (food * prod_level(np));
}

  /* this returns the TOTAL amount of money spent
     to maintain the nation's military forces
   */
int military_maint(np)
     Snation *np;
{
  Sarmy *ap;
  int total = 0;

  for (ap = np->armies; ap != NULL; ap = ap->next) {
    total += army_maint_money(ap);
  }

  return total;
}
  /* this returns the TOTAL amount of metal spent
     to maintain the nation's military forces
   */
int military_maint_metal(np)
     Snation *np;
{
  Sarmy *ap;
  int total = 0;

  for (ap = np->armies; ap != NULL; ap = ap->next) {
    total += ap->n_soldiers * ap->metal_maint;
  }

  return total;
}
  /* this returns the TOTAL amount of jewels spent
     to maintain the nation's military forces
   */
int military_maint_jewels(np)
     Snation *np;
{
  Sarmy *ap;
  int total = 0;

  for (ap = np->armies; ap != NULL; ap = ap->next) {
    total += ap->n_soldiers * ap->jewel_maint;
  }
  return total;
}
  /* this returns the TOTAL amount of jewels spent
     to maintain the nation's military forces
   */
int military_maint_spell_pts(np)
     Snation *np;
{
  Sarmy *ap;
  int total = 0;
  struct spirit_type *stype, *get_spirit_type();

  for (ap = np->armies; ap != NULL; ap = ap->next) {
/*    if (is_spirit(ap)) {
      if ((stype = get_spirit_type(&user, ap->type)) == NULL) {
	printf(
    "\r\nBAD BUG: is_spirit(), but can't get which spirit from type %s\r\n",
	       ap->type);
      } else {
	total += (ap->n_soldiers * ap->spell_pts_maint) / stype->size;
      }
    }
*/
    if (ap->spell_pts_maint != 0)  {
      total += get_spell_pts_maint(ap);
    }
  }
  return total;
}

  /* this gives the money needed to maintain a given army */
army_maint_money(ap)
     Sarmy *ap;
{
  int money_maint = 0, index;
  extern struct army_type *army_types;

  if (is_army(ap)) {
    index = army_type_index(ap);
    money_maint = army_types[index].money_maint * ap->n_soldiers;
  }
    /* in other cases, money maint is zero */

  return  money_maint + ARMY_OVERHEAD;
}

  /* this returns the TOTAL amount of money spent to
     maintain the nation's non-profit centers (hospitals  ...)
   */
int non_profit_maint(np)
     Snation *np;
{
  struct pt_list *ptlist = np->ptlist; /* nation's list of owned sectors */
  Ssector *sp;
  int total = 0;

  for ( ; ptlist != NULL; ptlist = ptlist->next) {
    sp = &world.map[ptlist->pt.x][ptlist->pt.y];
    switch (sp->designation) {
    case D_UNIVERSITY:
      total += UNIV_MAINT_COST;
      break;
    case D_HOSPITAL:
      total += HOSPITAL_MAINT_COST;
      break;
    default:
      break;
    }
  }
  return total;
}

int get_n_students(np)
     Snation *np;
{
  Ssector *sp;
  struct pt_list *ptlist = np->ptlist;
  int n = 0;

  while (ptlist != NULL) {
    sp = &world.map[ptlist->pt.x][ptlist->pt.y];
    if (sp->designation == D_UNIVERSITY) {
      n += n_workers(sp);
    }
    ptlist = ptlist->next;
  }
  return n;
}

int get_n_priests (np)
     Snation *np;
{
  Ssector *sp;
  struct pt_list *ptlist = np->ptlist;
  int n = 0;

  while (ptlist != NULL) {
    sp = &world.map[ptlist->pt.x][ptlist->pt.y];
    if (sp->designation == D_TEMPLE) {
      n += n_workers(sp);
    }
    ptlist = ptlist->next;
  }
  return n;
}

int get_employed(np)
     Snation *np;
{
  Ssector *sp;
  struct pt_list *ptlist = np->ptlist;
  int n = 0;

  while (ptlist != NULL) {
    sp = &world.map[ptlist->pt.x][ptlist->pt.y];
        /* only people who are employed */
    n += n_workers(sp);
    ptlist = ptlist->next;
  }
  return n;
}

int get_unemployed(np)
     Snation *np;
{
  Ssector *sp;
  struct pt_list *ptlist = np->ptlist;
  int n = 0;

  while (ptlist != NULL) {
    sp = &world.map[ptlist->pt.x][ptlist->pt.y];
        /* only people who are un-employed */
    n += max(0, sp->n_people - n_workers(sp));
    ptlist = ptlist->next;
  }
  return n;
}

int get_emp_met(np)
     Snation *np;
{
  Ssector *sp;
  struct pt_list *ptlist = np->ptlist;
  int n = 0;

  while (ptlist != NULL) {
    sp = &world.map[ptlist->pt.x][ptlist->pt.y];
        /* only people who are employed */
    if (sp->designation == D_METAL_MINE) {
      n += n_workers(sp);
    }
    ptlist = ptlist->next;
  }
  return n;
}

int get_unemp_met(np)
     Snation *np;
{
  Ssector *sp;
  struct pt_list *ptlist = np->ptlist;
  int n = 0;

  while (ptlist != NULL) {
    sp = &world.map[ptlist->pt.x][ptlist->pt.y];
    if (sp->designation == D_METAL_MINE) {
        /* only people who are un-employed */
      n += max(0, sp->n_people - n_workers(sp));
    }
    ptlist = ptlist->next;
  }
  return n;
}

int get_emp_jwl(np)
     Snation *np;
{
  Ssector *sp;
  struct pt_list *ptlist = np->ptlist;
  int n = 0;

  while (ptlist != NULL) {
    sp = &world.map[ptlist->pt.x][ptlist->pt.y];
        /* only people who are employed */
    if (sp->designation == D_JEWEL_MINE) {
      n += n_workers(sp);
    }
    ptlist = ptlist->next;
  }
  return n;
}

int get_unemp_jwl(np)
     Snation *np;
{
  Ssector *sp;
  struct pt_list *ptlist = np->ptlist;
  int n = 0;

  while (ptlist != NULL) {
    sp = &world.map[ptlist->pt.x][ptlist->pt.y];
    if (sp->designation == D_JEWEL_MINE) {
        /* only people who are un-employed */
      n += max(0, sp->n_people - n_workers(sp));
    }
    ptlist = ptlist->next;
  }
  return n;
}

int get_emp_farm(np)
     Snation *np;
{
  Ssector *sp;
  struct pt_list *ptlist = np->ptlist;
  int n = 0;

  while (ptlist != NULL) {
    sp = &world.map[ptlist->pt.x][ptlist->pt.y];
        /* only people who are employed */
    if (sp->designation == D_FARM) {
      n += n_workers(sp);
    }
    ptlist = ptlist->next;
  }
  return n;
}

int get_unemp_farm(np)
     Snation *np;
{
  Ssector *sp;
  struct pt_list *ptlist = np->ptlist;
  int n = 0;

  while (ptlist != NULL) {
    sp = &world.map[ptlist->pt.x][ptlist->pt.y];
    if (sp->designation == D_FARM) {
        /* only people who are un-employed */
      n += max(0, sp->n_people - n_workers(sp));
    }
    ptlist = ptlist->next;
  }
  return n;
}

  /* the service sector!! */
int get_emp_serv(np)
     Snation *np;
{
  return get_employed(np)-get_emp_met(np)-get_emp_jwl(np)-get_emp_farm(np);
}

int get_unemp_serv(np)
     Snation *np;
{
  return get_unemployed(np)
    - get_unemp_met(np) - get_unemp_jwl(np) - get_unemp_farm(np);
}

  /* tax revenue from the service sector */
int calc_serv_revenue(np)
     Snation *np;
{
  Ssector *sp;
  struct pt_list *points;
  int income = 0;
  int taxes = np->taxes;
    /* first thing: run through the list of that nation's sectors
       and see what income might come from the various sectors
     */
  if ((points = np->ptlist) == NULL) {
    return 0;			/* nation has no sectors!! */
  }
  do {
    sp = &(world.map[points->pt.x][points->pt.y]);
    switch (sp->designation) {
    case D_METAL_MINE:
    case D_JEWEL_MINE:
    case D_FARM:
      break;
    default:			/* other things (service sector) */
      income += (taxes * prod_level(np)*n_workers(sp)*
                            desig_map[sp->designation].revenue)/100;
      break;
    }
  } while ((points = points->next) != NULL);
  return income;
}

  /* average quantities of all nations in the world */
int get_avg_money(wp)
      Sworld *wp;
{
  Snation *np;
  int i, n_nations = 0, total = 0;

  for (i = 1; i < wp->n_nations; ++i) {
    np = &wp->nations[i];
    if (is_active_ntn(np)) {
      ++n_nations;
      total += np->money;
    }
  }
  return n_nations ? total/n_nations : 0;
}

int get_avg_metal(wp)
      Sworld *wp;
{
  Snation *np;
  int i, n_nations = 0, total = 0;

  for (i = 1; i < wp->n_nations; ++i) {
    np = &wp->nations[i];
    if (is_active_ntn(np)) {
      ++n_nations;
      total += np->metal;
    }
  }
  return n_nations ? total/n_nations : 0;
}

int get_avg_jewels(wp)
      Sworld *wp;
{
  Snation *np;
  int i, n_nations = 0, total = 0;

  for (i = 1; i < wp->n_nations; ++i) {
    np = &wp->nations[i];
    if (is_active_ntn(np)) {
      ++n_nations;
      total += np->jewels;
    }
  }
  return n_nations ? total/n_nations : 0;
}

int get_avg_food(wp)
      Sworld *wp;
{
  Snation *np;
  int i, n_nations = 0, total = 0;

  for (i = 1; i < wp->n_nations; ++i) {
    np = &wp->nations[i];
    if (is_active_ntn(np)) {
      ++n_nations;
      total += np->food;
    }
  }
  return n_nations ? total/n_nations : 0;
}

int get_avg_civil(wp)
      Sworld *wp;
{
  Snation *np;
  int i, n_nations = 0, total = 0;

  for (i = 1; i < wp->n_nations; ++i) {
    np = &wp->nations[i];
    if (is_active_ntn(np)) {
      ++n_nations;
      total += get_n_civil(np);
    }
  }
  return n_nations ? total/n_nations : 0;
}

int get_avg_soldiers(wp)
      Sworld *wp;
{
  Snation *np;
  int i, n_nations = 0, total = 0;

  for (i = 1; i < wp->n_nations; ++i) {
    np = &wp->nations[i];
    if (is_active_ntn(np)) {
      ++n_nations;
      total += get_n_soldiers(np);
    }
  }
  return n_nations ? total/n_nations : 0;
}


int get_avg_sectors(wp)
     Sworld *wp;
{
  Snation *np;
  int i, n_nations = 0, total = 0;

  for (i=1; i < wp->n_nations; i++) {
    np = &wp->nations[i];
    if (is_active_ntn(np)) {
      ++n_nations;
      total += np->n_sects;
    }
  }
  return n_nations ? total/n_nations : 0;
}


int get_per_occu_land(wp)
     Sworld *wp;
{
  int x, y, land = 0, n_occu = 0;

  for (y = 0; y < wp->ymax; y++)
    for (x = 0; x < wp->xmax; x++)
      if (wp->map[x][y].altitude >= SEA_LEVEL) {
	land++;
	if (wp->map[x][y].owner != 0)
	  n_occu++;
      }
  return (n_occu*100)/land;
}

int get_per_occu_water(wp)
     Sworld *wp;
{
  int x, y, water = 0, n_occu = 0;

  for (y = 0; y < wp->ymax; y++)
    for (x = 0; x < wp->xmax; x++)
      if (wp->map[x][y].altitude < SEA_LEVEL) {
	water++;
	if (wp->map[x][y].owner != 0)
	  n_occu++;
      }
  if (water != 0) {
    return (n_occu*100)/water;
  } else {
    return 0;
  }
}


  /* return the number of active nations (including game master) */
int get_n_act_ntn(wp)
     Sworld *wp;
{
  int i, total = 0;

  for (i = 0; i < wp->n_nations; ++i) {
    if (is_active_ntn(&wp->nations[i])) {
      ++total;
    }
  }
  return total;
}

int n_workers(sp)
     Ssector *sp;
{
  double race_factor;

  race_factor = sqrt(world.nations[sp->owner].race.repro/10.0);
 return min(sp->n_people,
	    (int) (desig_map[sp->designation].max_employed*race_factor));
}

int emp_desire(np, x, y)	/* desireability for employment */
     Snation *np;
     int x, y;
{
  int unemployed;
  Ssector *sp = &world.map[x][y];

/*  unemployed = min(0, n_workers(sp) - sp->n_people); */
    /* what we return is the percentage of people employed */
  return sp->n_people ? (100*n_workers(sp))/sp->n_people : 100;
}
