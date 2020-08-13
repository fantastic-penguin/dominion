   /* nplib.c -- modules involved in NPC movemaking */

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
#ifdef SYSV
# include <string.h>
#else
# include <strings.h>
#endif /* SYSV */

#include "dominion.h"
#include "misc.h"
#include "army.h"
#include <math.h>
#include <curses.h>
#include <stdio.h>
#include <ctype.h>
#include <signal.h>

#define CUTOFF1 -30		/* army bonus for draft cutoff 1 (npc_draft) */
#define CUTOFF2 0		/* army bonus for draft cutoff 2 (npc_draft) */

extern Sworld world;
extern Suser user;
extern struct s_desig_map desig_map[];
extern int (*wrapx)(), (*wrapy)();
extern int debug;
extern struct army_type *army_types;
extern struct spirit_type *spirit_types;

extern float npc_food_need,npc_metal_need,npc_jewel_need,npc_money_need;
extern int opt_army_size,atwar,npc_specific;

/*--------------------find_visible_sectors()-------------------------------
   NOTE: the following routine copied from user.c to avoid fiddling with
   the Makefile and increasing the size of dom_update.  One difference,
   viewall is not used. 

   this routine goes through the entire map and figures out
   which sectors are visible by the user.
---------------------------------------------------------------------------*/
/*
find_visible_sectors(visible_sectors)
     int **visible_sectors;
{
  int x, y, i, j;
  struct pt_list *plist;
  Sarmy *ap;
  Ssector *sp;

  for (i = 0; i < world.xmax; ++i) {
    for (j = 0; j < world.ymax; ++j) {
      visible_sectors[i][j] = SEE_NOTHING;
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
*/
/*------------------------------get_good_types()-----------------------------
	This function fills an array with the indexed to army_types that will
be good for this npc.
	Basically, the npc will only draft armies like spearmen and cavemen
when it is in desperate straits.  For now, npc's will not draft transports,
water-walk armies, kamikaze armies, front-line armies, or machines, because
handling these well is beyond me at this point.
	The npc decides whether or not to draft armies with low bonuses based
on the proportion of current#of troops to desired #of troops.  The desired
is  civilians * npc_agg/200 when at war, and civil. * npc_agg/300 otherwise.
The cutoffs are 25+npc/2 % and npc/2 %, for armies with -1 to -30 bonuses and
armies with <-30 bonuses, respectively.
	For example, a nation with an aggressiveness of 50 and a civilian
population of 10,000 will not draft cavemen when it has more than 625 soldiers,
and will not draft spearmen when it has more than 1250 soldiers, and will not
draft at all when it has more than 2500 soldiers. (when at war)
---------------------------------------------------------------------------*/
get_good_types(good_armies,current,cut1,cut2)
int good_armies[MAX_TYPES];
int current,cut1,cut2;	/* current # of troops and cutoffs */
{
  Savail_army *tmp_avail;
  struct army_type this_atype;
  int type_index,ngood = 0,badflags;

  badflags = AF_WATER |  AF_LAND | AF_INVERSE_ALT | AF_KAMIKAZE | AF_FRONT_LINE
	   | AF_MACHINE | AF_CARGO | AF_WIZARD;

  for(tmp_avail = user.avail_armies;tmp_avail ; tmp_avail = tmp_avail->next){
    type_index = army_type_index(tmp_avail->type);
    this_atype = army_types[type_index];
    if (this_atype.flags & badflags)
	 continue;
    if(this_atype.bonus < CUTOFF1 && current > cut1)
      continue;
    else if(this_atype.bonus < CUTOFF2 && current > cut2)
      continue;
		/* replace army type with race-specific type if appropriate */
    if(npc_specific &&
	army_types[npc_specific-1].bonus >= this_atype.bonus &&
	this_atype.bonus >= 0)
      good_armies[ngood++] = npc_specific - 1;
    else
      good_armies[ngood++] = type_index;
    }
  if (debug)
    printf("ngood = %d\n",ngood);
  return(ngood);
}

/*---------------------------tmp_army_better()-------------------------------
	This function checks tmpap and oldap to see if tmpap would be better
for ap to merge with.  Better is defined as big enough after the merge
to take sectors, but smaller after the merge than the other army.
This is a crude way of preventing one army from growing too large.
Edit to your heart's content, if you think it will make the npc's better.
	(This function is called by npc_merge)
------------------------------------------------------------------------*/
tmp_army_better(ap,tmpap,oldap)
Sarmy *ap,*tmpap,*oldap;
{
  if(strcmp(tmpap->type,ap->type)){	/* different types */
    return(0);
  }
  
  if(!oldap)				/* anything is better than nothing */
    return(1);

  if(tmpap->n_soldiers + ap->n_soldiers < opt_army_size)
    return(0);				/* already have oldap */

  if(tmpap->n_soldiers < oldap->n_soldiers)
    return(1);				/* enough soldiers but not too big */
  else
    return(0);				/* too big */
}

/*---------------------------find_desire()------------------------------------
	Fill the desire array with the desireability for each sector on the
map.  The base desire goes from zero to half the length of the shortest side.
To find the final desire, the base desire is squared.  The desire for the
surrounding sectors is modified.  If the base desire is N, then all the
adjacent sectors have their desireability increased by N-1, the next sectors
out are increased by N-2, etc.  This will hopefully keep npc's moving in the
right general direction. (the base desire of owned sectors is 0, unless at war)
---------------------------------------------------------------------------*/
find_desire(np,des_array)
Snation *np;
struct desire{
	int base,final;
	} **des_array;
{
  int x,y,i,d;

  for(x = 0; x < world.xmax;x++)
    for(y = 0; y < world.ymax; y++){
      des_array[x][y].base = npc_des(np,x,y);
      des_array[x][y].final = 0;
    }

  for(x = 0; x < world.xmax;x++)
    for(y = 0; y < world.ymax; y++){
      d = des_array[x][y].base;
      des_array[x][y].final += d*d;
      if(atwar && d > 0){		/* sector owned by us or enemy */
        des_array[x][y].final += world.map[x][y].n_people/25;
        if (world.map[x][y].designation == D_CAPITAL) 
          des_array[x][y].final *= 2;
        }
      if(user.visible_sectors[x][y])
        for(i = 1;i < d && add_square(x,y,i,d-i,des_array,np);i++);
      }
/* This makes a big mess--------
    if(debug){
    for(y = 0; y < world.ymax; y++){
      for(x = 0; x < world.xmax;x++)
        printf("%2d,",des_array[x][y].base);
      printf("\n");
    }
    for(y = 0; y < world.ymax; y++){
      for(x = 0; x < world.xmax;x++)
        printf("%3d,",des_array[x][y].final);
      printf("\n");
    }
  } */
}

/*-------------------------------npc_des()-------------------------------------
	This function calculates the desireability of a sector to an npc.
This desireability will be calculated differently when the npc is at war
than when it is at peace.  For now, just try to expand.
-----------------------------------------------------------------------------*/
npc_des(np,x,y)
Snation *np;
int x,y;
{
  int num,i,j,dstat;
  Sdiplo **dm;

  if(!good_altitude(&world.map[x][y],np))
    return(0);

  if(atwar && world.map[x][y].owner){
    dm = user.diplo_matrix;
    dstat = get_diplo_status(dm, np->id, world.map[x][y].owner);
    if(world.map[x][y].owner != np->id && (dstat != WAR && dstat != JIHAD))
  	return(0);
  } else if (world.map[x][y].owner == np->id)
    return (0);

  if(atwar && world.map[x][y].owner){
    if(world.map[x][y].owner == np->id){
      switch(world.map[x][y].designation){
        case D_FARM:
          num = 3 + world.map[x][y].soil * npc_food_need;
	  break;
        case D_METAL_MINE:
          num = 3 + world.map[x][y].metal * npc_metal_need;
	  break;
        case D_JEWEL_MINE:
          num = 3 + world.map[x][y].jewels * npc_jewel_need;
	  break;
        case D_CITY: case D_UNIVERSITY:
          num = 11;
	  break;
	case D_CAPITAL:
	  num = 8 + np->npc_agg / 10;
	  break;
        default:
	  break;
        }
      }
    else{
      num = 12;
  /*    switch(world.map[x][y].designation){
        case D_FARM:
          num = 8 * npc_food_need;
	  break;
        case D_METAL_MINE:
          num = 8 * npc_metal_need;
	  break;
        case D_JEWEL_MINE:
          num = 8 * npc_jewel_need;
	  break;
        case D_CITY: case D_UNIVERSITY: case D_CAPITAL:
	  num = 12;
	  break;
        default:
	  num = 7;
	  break;
        } */
  
      for(i = -1;i < 2;i++)
        for(j = -1;j < 2;j++)
          if(world.map[(*wrapx)(x+j,y+i)][(*wrapy)(x+j,y+i)].owner == np->id)
            num++;
      /* if(world.map[x][y].n_people >= 0)
        num += sqrt((double)world.map[x][y].n_people)/5.0; */
      }
    }
  
  else{
    num = world.map[x][y].soil * npc_food_need;
    num += world.map[x][y].metal * npc_metal_need;
    num += world.map[x][y].jewels * npc_jewel_need;
    for(i = -1;i < 2;i++)
      for(j = -1;j < 2;j++)
        if(world.map[(*wrapx)(x+j,y+i)][(*wrapy)(x+j,y+i)].owner == np->id)
          num++;
  }

/*  if(atwar && !world.map[x][y].owner)
    return(num/4);
  else */
  num = min ( num, min (world.xmax, world.ymax) );
    return(num/2);
}

/*----------------------------add_square()---------------------------------
	This function adds a number, add to the desireability of the sectors
radius away from x,y.  Radius is defined rather loosely here, since the sectors
form a square.
---------------------------------------------------------------------------*/
add_square(x,y,radius,add,des_array,np)
int x,y,radius,add;
struct desire{
	int base,final;
	} **des_array;
Snation *np;
{
int i,tx,ty,id,flag = 0;
Ssector *sp;

  id = np->id;
  tx = x-radius;			/* bottom side from lower left corner */
  ty = y-radius;
  for(i = 0; i < radius*2;i++){
    sp = &world.map[(*wrapx)(tx+i,ty)][(*wrapy)(tx+i,y)];
    if(!good_altitude(sp,np))
      flag++;
    des_array[(*wrapx)(tx+i,ty)][(*wrapy)(tx+i,ty)].final += add;
  }

  ty = y+radius;			/* left side from upper left corner */
  for(i = 0; i < radius*2;i++){
    sp = &world.map[(*wrapx)(tx,ty-i)][(*wrapy)(tx-i,y-i)];
    if(!good_altitude(sp,np))
      flag++;
    des_array[(*wrapx)(tx,ty-i)][(*wrapy)(tx,ty-i)].final += add;
  }

  tx = x+radius;			/* top side from upper right corner */
  for(i = 0; i < radius*2;i++){
    sp = &world.map[(*wrapx)(tx-i,ty)][(*wrapy)(tx-i,y)];
    if(!good_altitude(sp,np))
      flag++;
    des_array[(*wrapx)(tx-i,ty)][(*wrapy)(tx-i,ty)].final += add;
  }

  ty = y-radius;			/* right side from lower right corner */
  for(i = 0; i < radius*2;i++){
    sp = &world.map[(*wrapx)(tx,ty+i)][(*wrapy)(tx,y+i)];
    if(!good_altitude(sp,np))
      flag++;
    des_array[(*wrapx)(tx,ty+i)][(*wrapy)(tx,ty+i)].final += add;
  }

  return((flag > 4*radius) ? 0:1);	/* equal to zero if lots of water */
}

/*----------------------------check_moves()-----------------------------------
	Ary represents the area of the map centered at the army ap.  This
function fills ary with the maximum movepoints the army can have left when
it arrives at the sector.  Of course, routes that are outside of the section
of the map in ary are not considered, so a larger value for NPC_SIDE will
sometimes result in better movement.  However, since the algorithm includes
an insertion, it's best to keep the value small in the interest of time.
--------------------------------------------------------------------------*/
check_moves(np,ap,ary)
Snation *np;
Sarmy *ap;
struct tmp_map{
  int mvcost,mvleft;
  } ary[NPC_SIDE][NPC_SIDE];
{
  int i,j,k;
  int x,y,tx,ty,mapx,mapy;
  int mv,mvleft;
  int head=0,tail=0;
  struct mvlist{
    int mv;
    Pt pos;
  } list[NPC_SIDE * NPC_SIDE];	/* doesn't really need to be this big */
  Ssector *sect;

  for(x = 1; x < NPC_SIDE-1; x++)
  for(y = 1; y < NPC_SIDE-1; y++){
    tx = x - (NPC_VIEW+1) + ap->pos.x;	/* translate ary */
    ty = y - (NPC_VIEW+1) + ap->pos.y;	/* coords to map */
    sect = &world.map[(*wrapx)(tx,ty)][(*wrapy)(tx,ty)];
    ary[x][y].mvleft = -1;
    ary[x][y].mvcost = get_army_move_cost(np,sect,ap);
  }
 
  for(i = 0;i < NPC_SIDE * NPC_SIDE;i++)	/* initialize list of points */
  list[i].mv = 0;
 
  /* start army in center of ary with whatever movement it has */
  ary[NPC_VIEW+1][NPC_VIEW+1].mvleft = ap->mvpts;
  list[0].mv = ap->mvpts;
  list[0].pos.x = NPC_VIEW+1;
  list[0].pos.y = NPC_VIEW+1;

  for(head = 0; list[head].mv; head++){		/* until list is empty */
    mv = list[head].mv;
    for(x = list[head].pos.x-1;x <= list[head].pos.x + 1;x++)
      for(y = list[head].pos.y-1;y <= list[head].pos.y + 1;y++)
        if(ary[x][y].mvleft == -1 && ary[x][y].mvcost <= mv){
          mvleft = mv - ary[x][y].mvcost;
          ary[x][y].mvleft = mvleft;
          if(mvleft){
            for(j = tail;list[j].mv < mvleft;j--);
            for(k = tail;k > j;k--)
              list[k+1] = list[k];
            list[j+1].mv = mvleft;
            list[j+1].pos.x = x;
            list[j+1].pos.y = y;
            tail++;
          }
        }
    }

if(debug){
  printf("ary for army %d= :\n",ap->id);
  for(y = 0; y < NPC_SIDE; y++){
    for(x = 0;x < NPC_SIDE;x++)
      printf("[%d,%d],",ary[x][y].mvleft,ary[x][y].mvcost);
    printf("\n");
    }
  }
}

/*--------------------------do_npc_diplo()------------------------------------
	Update the diplomacy for npcs.  Change the diplomacy according to
a random number, and the npc aggressiveness.  If the number is less than
the npc aggressiveness, then the diplomacy status goes down.  Otherwise
it goes up.
---------------------------------------------------------------------------*/
do_npc_diplo(np)
     Snation *np;
{
  int i,dip_from,dip_to,num,up,down;
  Snation *tmpnp;
  
  /* Take care of NPC diplomacy here */
  
  atwar = 0;
  for(i = 1;i < world.n_nations;i++){
    tmpnp = &world.nations[i];
    if (is_active_ntn(tmpnp) && (!tmpnp->npc_flag || NPC_FIGHT)) { 
      if (!have_met(user.diplo_matrix, np->id, i))
        continue;
      dip_to = get_diplo_status(user.diplo_matrix, np->id, i);
      dip_from = get_diplo_status(user.diplo_matrix, i, np->id);
      if (debug)
	printf("from = %d, to = %d\n",dip_from,dip_to);
      down = np->npc_agg * DIP_CHANGE / 100;
      up = (100 - np->npc_agg) * DIP_CHANGE / 100;
      num = RND()%100;
      if (debug)
	printf("num = %d ",num);
      if(num < np->npc_agg){
        if(num < down/2 || dip_from < dip_to)
          dip_to--;
        if(num < down)
	  dip_to--;
	}
      else{
	num = 100 - num;
        if(num < up/2)
	  dip_to++;
        if(num < up)
	  dip_to++;
	}
      if(dip_to > dip_from)
        dip_to = dip_from;
      dip_to = max(dip_to,JIHAD);
      if (debug)
	printf("up = %d, down = %d, newto = %d\n",up,down,dip_to);
      if(dip_to == WAR || dip_to == JIHAD)
        atwar = 1;
      set_diplo_status(user.diplo_matrix,np->id,i,dip_to);
      }
    }
  dump_diplo(np,user.diplo_matrix, world.n_nations);
}

/*---------------------------init_npc_mage()-----------------------------------
	Initiate a mage for the npc.  Copied from menus.c
-----------------------------------------------------------------------------*/
init_npc_mage(np,sp)
Snation *np;
Ssector *sp;
{
  Sarmy army, make_army();

  army = make_army("Mage", "Mage", 1, A_DEFEND, np->id, sp->loc);
  army.jewel_maint = 1000;
  army.id = free_army_id(np);
  army.flags |= AF_WIZARD;
  army.next = NULL;
    /* now insert it into the list */
  np->n_armies++;
  if (np->armies == NULL) { /* special case:  empty list */
    np->armies = (Sarmy *) malloc(sizeof(Sarmy));
    *(np->armies) = army;
    np->armies->next = NULL;
  } else {
    insert_army_nation(np, &army, -1);
  }
  insert_army_sector(sp, &army);
  np->jewels -= INITIATION_JEWELS;
}
