  /* movement.c - routines that affect movement and move costs */

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

/* get_move_cost(np,sp) - calculates a sector's move cost for a nation    */
/* get_army_move_cost(np,ap,sp) - calculates a sector's move cost
   for a given army                                                       */
/* good_altitude(sp,np) - TRUE if nation is compatible with sect. alt.    */
/* good_army_altitude(np,sp,ap) - TRUE if army is compatible with sect. alt. */

#include "dominion.h"
#include "misc.h"
#include "army.h"
#include <stdio.h>

#define TOO_MUCH_MOVE_COST 1000	/* basically impenetrable */

  /* returns the move cost for a given army over a given sector
     if ap is NULL we should consider giving a "generic" move
     cost for that sector.
   */
get_army_move_cost(np, sp, ap)
     Snation *np;
     Ssector *sp;
     Sarmy *ap;
{
  int cost = 1;

  if (has_impenetrable(sp)) {
    return TOO_MUCH_MOVE_COST;
  }
  cost += alt_mc(np, sp, ap);
  cost += climate_mc(np, sp, ap);
  cost += terrain_mc(np, sp, ap);
  cost += patrol_mc(np, sp, ap);
  cost += roads_mc(np, sp, ap);
  cost += diplo_mc(np, sp, ap);
  return max(1, cost);		/* can't be negative */
}

  /* generic move cost for a given nation on a given
     sector.  this is displayed in the sector window,
     but can be quite different from the move
     cost for an army with special flags. this value
     is the move cost for a basic army type, such
     as "Infantry".
   */
get_generic_move_cost(np, sp)
     Snation *np;
     Ssector *sp;
{
  int cost = 1;

  if (has_impenetrable(sp)) {
    return 100;
  }
  cost += alt_mc(np, sp, NULL);
  cost += climate_mc(np, sp, NULL);
  cost += terrain_mc(np, sp, NULL);
  cost += patrol_mc(np, sp, NULL);
  cost += roads_mc(np, sp, NULL);
  cost += diplo_mc(np, sp, NULL);
  return max(1, cost);		/* can't be negative */
}

  /* here follow a bunch of routines that give the
     partial move cost for a given army on a given
     sector, due to altitude, climate, diplomacy,
     patrols, and so on...  if the army pointer
     is NULL, then a "generic" value is returned
     which does not account for special army flags
   */
  /* this macro does the real work */
#define alt_cost(pref_alt, sp) (abs(pref_alt - sp->altitude)/2)

alt_mc(np, sp, ap)
     Snation *np;
     Ssector *sp;
     Sarmy *ap;
{
  int virt_pref_alt = np->race.pref_alt; /* virtual preferred altitude */

  if (ap == NULL) {		/* generic value */
    return alt_cost(virt_pref_alt, sp);
  }
    /* if ap is not null, we examine special army properties */
  if (is_flight(ap)) {		/* flying armies ignore altitude */
    return 0;
  }

    /* see if army can make it into that sector */
  if (!good_army_altitude(np, sp, ap)) {
    return TOO_MUCH_MOVE_COST;
  }
  if (np->race.pref_alt >= SEA_LEVEL && sp->altitude < SEA_LEVEL
      && is_water(ap)) {	/* water armies are adapted to shallows */
    virt_pref_alt = SHALLOWS;
  }
  if (np->race.pref_alt < SEA_LEVEL && sp->altitude >= SEA_LEVEL
      && is_land(ap)) {		/* land armies are adapted to lowlands */
    virt_pref_alt = LOWLANDS;
  }

  return alt_cost(virt_pref_alt, sp);
}

climate_mc(np, sp, ap)
     Snation *np;
     Ssector *sp;
     Sarmy *ap;
{
  int cost = 0;

  cost += abs(np->race.pref_climate - sp->climate)/3;
  return cost;
}

terrain_mc(np, sp, ap)
     Snation *np;
     Ssector *sp;
     Sarmy *ap;
{
  int cost = 0;

  cost += abs(np->race.pref_terrain - sp->terrain)/2;
  return cost;
}

patrol_mc(np, sp, ap)
     Snation *np;
     Ssector *sp;
     Sarmy *ap;
{
  int cost = 0;

  if (are_patrols(np, ap, sp)) {
    cost += 5;
  }
  return cost;
}

roads_mc(np, sp, ap)
     Snation *np;
     Ssector *sp;
     Sarmy *ap;
{
  int cost = 0;

  cost -= sp->roads;
  return cost;
}

diplo_mc(np, sp, ap)
     Snation *np;
     Ssector *sp;
     Sarmy *ap;
{
  int cost = 0;

  if (ap && is_flight(ap)) {	/* flying armies ignore diplomacy */
    return cost;
  }
    /* here the move cost goes up if we are not buddy-buddy with the owner */
  if (sp->owner != np->id) {
    if (sp->owner == 0) {
      cost += 2;
    } else if (are_allied(np->id, sp->owner)) {
      cost += 1;
    } else {
      cost += 5;		/* non-allied land: mucho move cost */
    }
  }
  return cost;
}


  /* this takes care of whether a given race can
     move to a given sector from the altitude point of view
   */
good_altitude(sp, np)
     Ssector *sp;
     Snation *np;
{
  if (sp->altitude >= SEA_LEVEL) { /* if above water */
    return (np->race.pref_alt >= SEA_LEVEL || has_bubble(sp));
  }
  return (np->race.pref_alt < SEA_LEVEL || has_bubble(sp)); /* underwater */
}
  /* this takes care of whether a given army of a given race can
     move to a given sector from the altitude point of view
   */
good_army_altitude(np, sp, ap)
     Snation *np;
     Ssector *sp;
     Sarmy *ap;
{
  if (has_bubble(sp)) {
    return 1;
  }
  if (is_water(ap) && is_land(ap)) {
    return 1;
  }
  if (is_in_transport(ap)) {
    return 1;			/* carried by someone else */
  }
    /* first look at the case where we are above water */
  if (sp->altitude >= SEA_LEVEL) {
    if (np->race.pref_alt >= SEA_LEVEL) {
        /* water armies can only be in water (for land race) */
      if (is_water(ap) && !is_coastal_sect(np, sp, ap)) {
	return 0;
      }
      return 1;			/* it's OK: land army on land. */
    }
      /* otherwise we are a water race on land */
    if (is_land(ap)) {
      return 1;
    }
    if (is_water(ap) && is_coastal_sect(np, sp, ap)) {
      return 1;			/* water army can stay on coast */
    }
    return 0;
  }
    /* now the case where we are an underwater sector */
  if (np->race.pref_alt >= SEA_LEVEL) { /* land race under water */
    if (is_water(ap)) {
      return 1;
    }
    return 0;
  }
    /* now the case of a water race under water */
  if (is_land(ap) && !is_coastal_sect(np, sp, ap)) {
    return 0;
  }
  return 1;


/*  return (np->race.pref_alt < SEA_LEVEL || has_bubble(sp)); /* underwater */
}
