/* world.c -- function dealing with the world; its use and implementation */

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

/* wrap(pp) - wraps a point to its proper location                        */
/* latitude(x,y) - returns the latitude of a particular point             */
/* map_alt(altitude) - maps the altitude value to the correct data entry  */
/* xrel(x,y) - gives the relative x coordinate from nation's capital      */
/* yrel(x,y) - gives the relative y coordinate from nation's capital      */
/* xdist(x,y,x1,y1) - gives the x distance between (x,y) and (x1,y1)      */
/* ydist(x,y,x1,y1) - gives the y distance between (x,y) and (x1,y1)      */
/* sect_desire(np,x,y) - gives the desireability value of x,y for nation  */
/* is_coastal_sect(np,sp,ap) - is this coast (for given nation and army)? */
/* is_active_ntn(np) - returns 1 if the nation has not been destroyed     */
/* are_patrols(np, moving_ap, sp) - returns true if moving_ap gets slowed */

#include "dominion.h"
#include "misc.h"
#include "army.h"

#include <stdio.h>
#include <math.h>

extern Sworld world;
extern Suser user;
extern struct s_altitude_map altitude_map[];
extern struct s_desig_map desig_map[];
extern int (*wrapx)(), (*wrapy)();

extern int abs(int);
#include "functions.h"

/* Wrapping functions are the heart and soul of the world's shape */
/* these main functions, wrapx and wrapy, choose the proper function for */
/* the proper shape of the world in question. */

/* This function wraps the entire point */

void wrap(pp)
Pt *pp;
{
pp->x = (*wrapx)(pp->x,pp->y);
pp->y = (*wrapy)(pp->x,pp->y);
}

/* Wrap functions for a TORUS world */

int torus_wrapx(x,y)  /* produce new value of x, if x is too big or negative */
int x,y;
{
if (x >= world.xmax) {
return x % world.xmax;
}
while (x < 0) {
x += world.xmax;
}
return x;
}

int torus_wrapy(x,y)  /* produce new value of y, if y is too big or negative*/
int x,y;
{
if (y >= world.ymax) {
return y % world.ymax;
}
while (y < 0) {
y += world.ymax;
}
return y;
}

/* Wrap function for a pinched cylinder world */
int pinch_wrapxy(int x, int y, int which);  // forward decl

int pinch_wrapx(x,y)
int x,y;
{
return pinch_wrapxy(x,y,1);
}

int pinch_wrapy(x,y)
int x,y;
{
return pinch_wrapxy(x,y,2);
}

int pinch_wrapxy(x,y,which) /* if which=1 then return x */
int x,y,which;
{
while (y < 0) {
y = -(y + 1);
x = x + (world.xmax / 2);
}
while (y >= world.ymax) {
y = 2 * world.ymax - (y + 1);
x = x + (world.xmax / 2);
}
while (x < 0) {
x = x + world.xmax;
}
if (x >= world.xmax) {
x = x % world.xmax;
}
if (which == 1) {
return x;
} else {
return y;
}
}

/* A latitude function for the world which returns a float angle of the */
/* latitude of the y-coordinate given.  Equator is 0 degrees and the poles */
/* are 90 degrees (north pole) and negative 90 degrees (south pole) */

int torus_latitude(int y);  // forward decl

int latitude(x,y)
int x,y;
{
float angle;
switch(world.geo.topology) {
case TORUS:
angle = torus_latitude(y); break;
}
return angle;
}

/* Returns the latitude of a y-coordinate for a pinched cylinder. */

int pinch_latitude(y)
int y;
{
float eq_dist, angle;
eq_dist = y - (world.ymax / 2.0);
if (eq_dist < 0) {
eq_dist = eq_dist + 1.0;
}
angle = 90.0 * (eq_dist / (world.ymax / 2.0));
return -(angle);
}

/* Returns the latitude of a y-coordinate for a torus */

int torus_latitude(y)
int y;
{
float eq_dist, angle;
eq_dist = y - (world.ymax / 2.0);
angle = 90.0 * (eq_dist / (world.ymax / 2.0));
return angle;
}

int map_alt(altitude)
int altitude;
{
altitude -= altitude_map[0].value;
return altitude;
}

int xdist(int x, int y, int x1, int y1);  // forward decl
int ydist(int x, int y, int x1, int y1);  // forward decl

/* These functions give the coordinates of a sector relative to the user's */
/* capital.  They differ depending on which type of world you use! */
int xrel(x, y, cap)
int x, y;
Pt cap;
{
x=xdist(x, y, cap.x, cap.y);
/*  if (x < 0) {
return(x + world.xmax);
} */
return(x);
}

int yrel(x, y, cap)
int x,y;
Pt cap;
{
y = ydist(x, y, cap.x, cap.y);
/*  if (y < 0) {
return(y + world.ymax);
} */
return(y);
}

/* Functions to return the how far (x,y) is from (x1,y1) */
int xdist(x,y,x1,y1)
int x,y,x1,y1;
{
switch (world.geo.topology) {
case TORUS:
x = x - x1;
if (x <= (0 - world.xmax / 2)) {
x += world.xmax;
}
if (x > world.xmax / 2) {
x = x - world.xmax;
}
return x;
break;    
}
}

int ydist(x,y,x1,y1)
int x,y,x1,y1;
{
switch (world.geo.topology) {
case TORUS:
y = y - y1;
if (y <= (0 - world.ymax / 2)) {
y += world.ymax;
}
if (y > world.ymax / 2) {
y = y - world.ymax;
}
return y;
break;
}
}

int pinched_dist(x,y,x1,y1,choice)
int x,y,x1,y1,choice;
{
int dx,dy,dx2,dy2;
float r1,r2;
dx = x - x1;
dy = y - y1;
if (dx > world.xmax / 2) {
dx = dx - world.xmax;
}
if (dx <= (0 - world.xmax / 2)) {
dx = dx + world.xmax;
}

dy2 = y + y1 + 1;
if (dy2 > world.ymax) {
dy2 = world.ymax * 2 - dy2;
} else {
dy2 = -(dy2);
}
dx2 = x - x1 + world.xmax / 2;
if (dx2 >= world.xmax / 2) {
dx2 = dx2 - world.xmax;
}

r1 = sqrt(dx*dx + dy*dy);
r2 = sqrt(dx2*dx2 + dy2*dy2);
if (r1 <= r2) {
if (choice == 1) {
return dx;
} else {
return dy;
}
} else {
if (choice == 1) {
return dx2;
} else {
return dy2;
}
}
}

/* Old Sector Desire
int sect_desire(np, x, y)
Snation *np;
int x, y;
{
int terrain_d,climate_d,altitude_d,total_d;

terrain_d = abs(world.map[x][y].terrain - np->race.pref_terrain);
climate_d = abs(world.map[x][y].climate - np->race.pref_climate);
altitude_d = abs(world.map[x][y].altitude - np->race.pref_alt);
total_d  = max(0,300-(terrain_d*terrain_d)*10);
total_d += max(0,50-climate_d*3);
total_d += max(0,300-(altitude_d*altitude_d)*8);
total_d += world.map[x][y].metal*5;
total_d += world.map[x][y].soil*4;
total_d += world.map[x][y].jewels*6;
if (world.map[x][y].designation == D_CITY || world.map[x][y].designation == D_CAPITAL) {
total_d *= 3;
}
return total_d;
}
*/

int sect_desire(np, x, y)
Snation *np;
int x, y;
{
  int terrain_d,climate_d,altitude_d,total_d;

  terrain_d = abs(world.map[x][y].terrain - np->race.pref_terrain);
  climate_d = abs(world.map[x][y].climate - np->race.pref_climate);
  altitude_d = abs(world.map[x][y].altitude - np->race.pref_alt);

  total_d  = max(0,230-(terrain_d*terrain_d)*10);
  total_d += max(0,250-(climate_d*climate_d)*10);
  total_d += max(0,250-(altitude_d*altitude_d)*20);
  if (user.id == world.map[x][y].owner)
  {
    switch (world.map[x][y].designation)
    {
      case D_FARM: total_d += world.map[x][y].soil * 9; break;
      case D_METAL_MINE: total_d += world.map[x][y].metal * 11; break;
      case D_JEWEL_MINE: total_d += world.map[x][y].jewels * 13; break;
      default:
      { 
        total_d += 37;
        total_d += world.map[x][y].soil;
        total_d += world.map[x][y].metal;
        total_d += world.map[x][y].jewels;
      }
    }
  } else
  {
    total_d += world.map[x][y].soil * 7;
    total_d += world.map[x][y].metal * 9;
    total_d += world.map[x][y].jewels * 11;
  }
  return total_d;
}

/* this function returns true if there is a non-treaty army
   in patrol/intercept mode in the neighbourhood of the given sector.
   if none is found, it returns 0.  If the "moving_ap" is NULL,
   then you ignore the issue of whether moving_ap is in flight.
 */
int are_patrols(np, moving_ap, sp)
     Snation *np;
     Sarmy *moving_ap;
     Ssector *sp;
{
  int found = 0, done = 0, x, y;
  Sarmy *ap, *get_army();
  struct armyid *alist;

  if (moving_ap && is_underground(moving_ap)) {
    return 0;			/* underground armies can't be intercepted */
  }
    /* here we check if there are patrol or intercept units around */
  for (x = sp->loc.x - 1; ((x <= sp->loc.x + 1) && (done == 0)); x++) {
    for (y = sp->loc.y - 1; ((y <= sp->loc.y + 1) && (done == 0)); y++) {
      done = 0;
      if (np->npc_flag
          || user.visible_sectors[(*wrapx)(x,y)][(*wrapy)(x,y)] & SEE_ARMIES) {
	alist = world.map[(*wrapx)(x,y)][(*wrapy)(x,y)].alist;
	while (alist != NULL) {
	  if (alist->owner == sp->owner) {
	    ap = get_army(&world.nations[alist->owner],alist->id);
	    if ( (ap->status == A_PATROL || ap->status == A_INTERCEPT) &&
	        /* OK, we got an army on intercept or patrol.
		   now check the issue of flight.
		 */
		(!(moving_ap && is_flight(moving_ap)) || is_missiles(ap))
		/* make sure that we only slow down armies that are
		   not our own.  In the future, this should
		   also let TREATY armies pass...
		 */
		&& (moving_ap && (ap->owner != moving_ap->owner))
		&& (get_diplo_status(user.diplo_matrix,
				    ap->owner, moving_ap->owner != TREATY))) {
	      alist = NULL;
	      found = 1;
	      done = 1;
	    } else {
	      alist = alist->next;
	    }
	  } else {
	    alist = alist->next;
	  }
	}
      }
    }
  }
  return found;
}

void init_wrap() 
{
  int torus_wrapx(), torus_wrapy();
  switch(world.geo.topology) {
  case TORUS:
    wrapx = torus_wrapx;
    wrapy = torus_wrapy;
    break;
  default:
    printf("What world is this???\n");
    wrapx = torus_wrapx;
    wrapy = torus_wrapy;
    break;
  }
}

  /* returns 1 if this is a coastal sector (for that race and army),
     0 otherwise.  Note that this only applies if the army has the
     L or W flag, since other armies don't get any benefit from
     coastal sectors:  they should drown anyway.
   */
int is_coastal_sect(np, sp, ap)
     Snation *np;		/* nation that wants to go there */
     Ssector *sp;
     Sarmy *ap;
{
  int land, i, j, x, y;

  if (is_land(ap) && is_water(ap)) {
    return 0;			/* amphibious don't have a coastline */
  }
  if (is_land(ap)) {
    land = 1;
  } else if (is_water(ap)) {
    land = 0;
  } else {			/* coast only applies to L and W flags */
    return 0;
  }
    /* case: we are land army in water; look for land nearby! */
  if (land && sp->altitude < SEA_LEVEL) {
    for (i = sp->loc.x-1; i <= sp->loc.x+1; ++i) {
      for (j = sp->loc.y-1; j <= sp->loc.y+1; ++j) {
	x = wrapx(i, j);
	y = wrapy(i, j);
	if (world.map[x][y].altitude >= SEA_LEVEL) {
	  return 1;
	}
      }
    }
    return 0;			/* no shore nearby */
  }
    /* case: we are water army on land; look for water nearby! */
  if (!land && sp->altitude >= SEA_LEVEL) {
    for (i = sp->loc.x-1; i <= sp->loc.x+1; ++i) {
      for (j = sp->loc.y-1; j <= sp->loc.y+1; ++j) {
	x = wrapx(i, j);
	y = wrapy(i, j);
	if (world.map[x][y].altitude < SEA_LEVEL) {
	  return 1;
	}
      }
    }
    return 0;			/* no shor nearby */
  }
}

  /* tells you if a nation has not been destroyed */
int is_active_ntn(np)
     Snation *np;
{
  if (np->id == 0) {
    return 1;
  }
  if (!(np->capital.x == -1 && np->capital.y == -1)) {
    return 1;
  }
  return 0;
}
