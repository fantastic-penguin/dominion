   /* npc.c -- modules involved in NPC movemaking */

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

extern Sworld world;
extern Suser user;
extern struct s_desig_map desig_map[];
extern int (*wrapx)(), (*wrapy)();
extern Sdiplo **allocate_diplo();
extern int debug;
extern struct army_type *army_types;
extern struct spirit_type *spirit_types;

struct desire {
  int base,final;
};

float npc_food_need,npc_metal_need,npc_jewel_need,npc_money_need;
int opt_army_size,atwar,npc_specific;

/*-----------------------------npc_moves()------------------------------------
	This function makes the moves for an npc
----------------------------------------------------------------------------*/
npc_moves(np)
     Snation *np;
{
  int i;
  struct desire **des_array;

  des_array = (struct desire **) malloc(world.xmax*sizeof(struct desire*));
  for (i = 0; i < world.xmax; ++i)
    des_array[i] = (struct desire *) malloc(world.ymax*sizeof(struct desire));
  if (debug)
    printf("doing init\n");
  init_npc(np);
/*  npc_needs(np); */
  do_npc_draft(np);
  if (debug)
    printf("doing merge\n");
  do_npc_merge(np);
  if (debug)
    printf("doing split\n");
  do_npc_split(np);
  if (debug)
    printf("doing summon\n");
  do_npc_summon(np);
  if (debug)
    printf("doing armies\n");
  do_npc_armies(np,des_array);
  if (debug)
    printf("doing redesig\n");
  do_npc_redesig(np);

  for (i = 0; i < world.xmax; i++)	/* free desire array */
    free(des_array[i]);
  free(des_array);

  for (i = 0; i < world.xmax; ++i) {	/* free visible sectors array */
    free(user.visible_sectors[i]);
  }
  free(user.visible_sectors);

  free_diplo(user.diplo_matrix, world.n_nations);
}

/*-------------------------------init_npc()----------------------------------
	This is basically the function init_user, copied from user.c.  There
are some things that init_user does that this does not do.
	Also, this function sets global varibles like needs and race-specific
army types.
---------------------------------------------------------------------------*/
init_npc(np)
     Snation *np;
{
  int c,i,d;
  FILE *fp, *fopen();
  char line[EXECLEN];

    /* find out which army types are available to the user */
  user.avail_armies = NULL;
  get_avail_armies(&user, np->tech_skill);

	/*------ set npc_specific to be 1 + the index of the race 
		 specific army type, if any.  ---------------*/
  npc_specific = 0;
  if ((fp = fopen(RACES_FILE, "r")) != NULL) {
    while (1) {
      if (fgets(line, EXECLEN, fp) == NULL)
        break;
      if (strncmp(line, np->race.name, strlen(np->race.name)) == 0
	  && strncmp(line+strlen(np->race.name), "_armies:",
		     strlen("_armies:")) == 0) {
        if (line[strlen(line)-1] == '\n') {
          line[strlen(line)-1] = '\0';
	}
	npc_specific = army_type_index(strchr(line, ':')+1) + 1;
      }
    }
    fclose(fp);
  }

  user.spirit_list = NULL;
  get_spirits(&user, np->mag_skill);
    /* now set fields for the ustruct */
  user.underwater = 0;		/* user is not underwater at start */
  if (np->race.pref_alt < 0) {
    user.underwater = 1;	/* merfolk or whatever */
  }
    /* load user's diplomacy statuses, for fast access later;
       also remember the initial values, so users cannot change
       their status by more than one step at a time.
     */
  user.diplo_matrix = allocate_diplo(world.n_nations);
/*  user.init_diplo = allocate_diplo(world.n_nations); */
  read_in_diplo(user.diplo_matrix, world.n_nations);
/*  read_initial_diplo(user.init_diplo, world.n_nations); */

/* now do diplomacy and set the atwar variable to 1 if npc is at war */
  do_npc_diplo(np);

    /* load hanging spells, and put them in this user's list */
  load_h_spells(&user);
    /* calculate visibility matrix for this user.
       this might depend on spells, so do it after
       loading spells.
     */
  user.visible_sectors = (int **) malloc(world.xmax*sizeof(int *));
  for (i = 0; i < world.xmax; ++i) {
    user.visible_sectors[i] = (int *) malloc(world.ymax*sizeof(int));
  }
  find_visible_sectors(user.visible_sectors);


/*  npc_money_need =(float)calc_expend(np)/((float)calc_revenue(np)+1.0);
  npc_metal_need =(float)calc_expend_metal(np)/((float)calc_metal(np)+1.0)+.3;
npc_jewel_need =(float)calc_expend_jewels(np)/((float)calc_jewels(np)+1.0)+.3;*/

  npc_metal_need = atwar ? 1.3:1.0; /* for now */
  npc_jewel_need = atwar ? 1.3:1.0; /* for now */
  npc_food_need = (float)calc_expend_food(np)/((float)calc_food(np)+1.0);
  if(atwar){
    opt_army_size = get_n_soldiers(np)*2/np->n_sects;
    opt_army_size=max(OCCUPYING_SOLDIERS*3/2,opt_army_size);
  }
  else{
    opt_army_size = OCCUPYING_SOLDIERS;
  }

  if(debug)
    printf("opt size = %d.\n",opt_army_size);

  if(atwar){
    np->taxes = 20;
    np->tech_r_d_metal = 0;
    if (next_thon_jewels(np) < MAGE_JEWELS_MAINT) {
      np->mag_r_d_jewels = 0;
    } else {
      if (calc_jewels(np) * 2 > np->jewels) {
	np->mag_r_d_jewels = np->jewels * 50 / calc_jewels(np);
      } else {
	np->mag_r_d_jewels = 100;
      }
    }
    np->mag_r_d = 15;
    np->tech_r_d = 15;
  } else {
    np->taxes = 10;
    np->tech_r_d_metal = 30;
    np->mag_r_d_jewels = 0;
    np->mag_r_d = 30;
    np->tech_r_d = 30;
  }
}

/*---------------------------do_npc_draft()-----------------------------------
	Draft armies.  This routine has been altered to take technology
level into account.
----------------------------------------------------------------------------*/
do_npc_draft(np)
     Snation *np;
{
  Sarmy ap, make_army();
  Ssector *sp;
  Savail_army *can_draft;
  struct army_type this_atype;
  int max_sold,cur_sold,ngood,good_armies[MAX_TYPES],i,drafted;
  int cut1,cut2,start,stop,step;
  char type[NAMELEN];

	/*-------- Initialize variables for npc drafting -------*/
  if(atwar) max_sold = get_n_civil(np)*np->npc_agg/200;
  else max_sold = get_n_civil(np)*np->npc_agg/300;

  cut1 = max_sold*(np->npc_agg/2)/100;		/* -30 bonus cutoff */
  cut2 = max_sold*(np->npc_agg/2 + 25)/100;	/* -1 to -29 bonus cutoff */
  cur_sold = get_n_soldiers(np);
  ngood = get_good_types(good_armies,cur_sold,cut1,cut2);

	/*--- now draft from the cheap armies if we are desperate ------*/
  if(cur_sold < cut2 || (cur_sold - cut2 < max_sold - cur_sold)){
    start = 0;
    stop = ngood;
    step = 1;
  } else{
    start = ngood-1;
    stop = -1;
    step = -1;
  }
	/*--- get cut 2 out of the way if we have already passed it ---*/
  if(cur_sold > cut2) cut2 = 2*max_sold;

  if(ngood == 0)	/* no armies we want to draft */
    return;

		/*------ Now draft armies ---------*/
  while(cur_sold < max_sold){
    drafted = 0;
    for(i = start;(i != stop) && (cur_sold < max_sold);i += step){
      this_atype = army_types[good_armies[i]];
      strcpy(type,this_atype.type);
      sp = &world.map[np->capital.x][np->capital.y];
      ap = make_army(type,type, opt_army_size, A_DEFEND, np->id, sp->loc);
      ap.id = free_army_id(np);
      ap.next = NULL;
      if (ap.n_soldiers > sp->n_people || army_cost(&ap) > np->money ||
            army_cost_metal(&ap) > np->metal) {
        break;
      }
      ++np->n_armies;
      cur_sold += opt_army_size;
      drafted = 1;
      if (debug)
        printf("npc drafted %s\n",type);

      if (np->armies == NULL) {
        np->armies = (Sarmy *) malloc(sizeof(Sarmy));
        *(np->armies) = ap;
        np->armies->next = NULL;
      } else {
        insert_army_nation(np, &ap, -1);
      }
      insert_army_sector(sp, &ap);
      sp->n_people -= ap.n_soldiers;
      np->money -= army_cost(&ap);
      np->metal -= army_cost_metal(&ap);
    }
  if(cur_sold > cut2){			/* need to recalc good types */
    cut1 = max_sold*(np->npc_agg/2)/100;	/* -30 bonus cutoff */
    cut2 = max_sold*(np->npc_agg/2 + 25)/100;	/* -1 to -29 bonus cutoff */
    ngood = get_good_types(good_armies,cur_sold,cut1,cut2);
    if(cur_sold > cut1) cut1 = 2*max_sold;
    if(cur_sold > cut2) cut2 = 2*max_sold;
  }
  if(!drafted || !ngood)	/* No armies can be drafted anymore */
    return;
  }
}

/*----------------------------do_npc_summon()--------------------------------
	Have the npc draft a mage, if necessary, and summon spirits.
Basically, find the net spell points and draft the biggest spirit you can.
Don't draft caravan spirits.
---------------------------------------------------------------------------*/
do_npc_summon(np)
Snation *np;
{
  int have_mage = 0,sindex,net_points,badflags;
  Sarmy army,make_army();
  struct spirit_type this_stype;
  Ssector *sp;
  Sspirit *sptr,*tmp_sptr;

  if(!atwar || np->spell_pts < 2)
    return;

  badflags = AF_INVERSE_ALT & AF_CARGO;
  if (user.underwater)
    badflags |= AF_LAND;
  else
    badflags |= AF_WATER;

  sp = &world.map[np->capital.x][np->capital.y];

  have_mage = get_first_mage(np);
  if (!have_mage) {
    if (np->jewels >= INITIATION_JEWELS && next_thon_jewels(np) > 6000)
      init_npc_mage(np,sp);
    else
      return;			/* no mage, can't summon */
  }
  
  net_points = new_spell_pts(np) - military_maint_spell_pts(np);
  while(net_points){		/* while we can still summon */
    tmp_sptr = 0;
    for(sptr = user.spirit_list ; sptr ; sptr = sptr->next){
      if (np->spell_pts < sptr->cost) 
	break;
      tmp_sptr = sptr;
    }
    if (!tmp_sptr)
      break;

    sindex = spirit_type_index(tmp_sptr->type);
    this_stype = spirit_types[sindex];
    while ((this_stype.flags & badflags) && sptr != user.spirit_list){
      for (sptr = user.spirit_list ;sptr->next != tmp_sptr ;sptr = sptr->next);
      sindex = spirit_type_index(sptr->type);
      this_stype = spirit_types[sindex];
      tmp_sptr = sptr;
      }

    if (this_stype.flags & badflags)
      break;

    if (debug)
      printf("npc summons %s\n",tmp_sptr->type);

    army = make_army(this_stype.type, this_stype.type, this_stype.size,
		     A_DEFEND, np->id, sp->loc);
    army.id = free_army_id(np);
    army.next = NULL;
    army.flags = this_stype.flags;

  /*============ now insert it into the list ============*/

    ++np->n_armies;
    if (np->armies == NULL) { 		/* special case:  empty list */
      np->armies = (Sarmy *) malloc(sizeof(Sarmy));
      *(np->armies) = army;
      np->armies->next = NULL;
    } else {
      insert_army_nation(np, &army, -1);
    }
    insert_army_sector(sp, &army);
    np->spell_pts -= spirit_types[sindex].spell_pts_draft;
    net_points = new_spell_pts(np) - military_maint_spell_pts(np);
  }
}

/*-----------------------------do_npc_merge()--------------------------------
	merge npc armies together.  If an army has less than an optimal
number of units, check for other armies of the npc that are reachable and
within NPC_VIEW sectors.  Move to the one that will give the greatest
movement left and merge.  Prefer to merge with other armies that have
less than the optimal number.
---------------------------------------------------------------------------*/
do_npc_merge(np)
  Snation *np;
{
  int i,tx,ty,x,y,finalmv;
  Pt finalpos;
  Sarmy *bestarmy,*ap,*tmpap,*get_army();
  struct armyid *alist;
  struct argument args[N_EXEC_ARGS];
  struct tmp_map {
    int mvcost,mvleft;
  } legal_moves[NPC_SIDE][NPC_SIDE];

  for(i = 0;i < NPC_SIDE; i++){			/* fill in border of legal mv */
    legal_moves[0][i].mvleft = -2;
    legal_moves[NPC_SIDE-1][i].mvleft = -2;
    legal_moves[i][0].mvleft = -2;
    legal_moves[i][NPC_SIDE-1].mvleft = -2;
  }

  for(ap = np->armies ;ap ;ap = ap->next){	/* for all npcs armies */
    if(ap->n_soldiers >= max(opt_army_size*2/3,OCCUPYING_SOLDIERS))
      continue;					/* skip loop if army ok. */
    if(is_mage(ap))
      continue;
    bestarmy = 0;
    check_moves(np,ap,legal_moves);		/* new function */

    /* now check for armies belonging to this player that can be moved to */

 /*   printf("%d needs to merge.\n",ap->id); */
    for (x = ap->pos.x-NPC_VIEW; x <= ap->pos.x+NPC_VIEW; x++) {
      for (y = ap->pos.y-NPC_VIEW; y <= ap->pos.y+NPC_VIEW; y++) {
	alist = world.map[(*wrapx)(x,y)][(*wrapy)(x,y)].alist;
	while(alist){
	  if(alist->owner == np->id){		/* if same owner */
	    tx = x - ap->pos.x + NPC_VIEW + 1;
	    ty = y - ap->pos.y + NPC_VIEW + 1;	/* coords in legal_move */
	    if(legal_moves[tx][ty].mvleft >= 0){
	      tmpap = get_army(np, alist->id);
	    /* printf("looking at army %d\n",tmpap->id); */
	      if(tmp_army_better(ap,tmpap,bestarmy) && tmpap != ap){
	    /* printf("best is %d\n",tmpap->id); */
	        bestarmy = tmpap;
		finalpos.x = x;
		finalpos.y = y;
		finalmv = legal_moves[tx][ty].mvleft;
              }
	    }
	  }
	  alist = alist->next;
	}	/* while alist */
      }		/* for y */
    }		/* for x */

  /* now move the army to the correct location and merge.  */
    if(bestarmy){
      if (debug)
        printf("npc army %d merges with %d.\n",ap->id,bestarmy->id);
      args[1].data.num = ap->id;
      alist = world.map[(*wrapx)(x,y)][(*wrapy)(x,y)].alist;
      args[2].data.num = (*wrapx)(finalpos.x,finalpos.y);
      args[3].data.num = (*wrapy)(finalpos.x,finalpos.y);
      args[4].data.num = finalmv;
      cmd_amove(np,args);		/* move army to new location */
      args[1].data.num = ap->id;
      args[2].data.num = bestarmy->id;
      cmd_amerge(np,args);
    }
  }		/* for ap */
}

/*--------------------------------do_npc_split()------------------------------
	Split armies that have grown too large into manageable armies
----------------------------------------------------------------------------*/
do_npc_split(np)
Snation *np;
{
  Sarmy *ap;
  struct argument args[N_EXEC_ARGS];

  for(ap = np->armies ;ap ;ap = ap->next){	/* for all npcs armies */
    if(ap->n_soldiers < opt_army_size*2 || is_spirit(ap))
      continue;					/* skip loop if army ok. */
    while(ap->n_soldiers >= opt_army_size*2){
      if (debug)
	printf ("npc splits %d troops from army %d\n",opt_army_size,ap->id);
      args[1].data.num = ap->id;
      args[2].data.num = opt_army_size;
      cmd_asplit(np,args);
    }
  }
}

/*-------------------------------do_npc_armies()------------------------------
	This routine moves npc armies to desirable sectors.  Des_array is
an array containing the desirability of each sector on the map.  legal_moves
is an array containing the sectors that the army can see, with the sectors
that the army can move to marked with the number of movepoints the army will
have left at that point.  The function looks for the most desirable sectors
that the army can move to and puts them in the highlist array.  It then
randomly picks a point from this array and moves the army to it.
----------------------------------------------------------------------------*/
do_npc_armies(np,des_array)
  Snation *np;
  struct desire **des_array;
{
  int i,x,y,tx,ty,des_here,high_des,nhigh;
  struct pt highlist[NPC_SIDE*NPC_SIDE];
  Sarmy *ap;
  struct argument args[N_EXEC_ARGS];
  struct tmp_map {
    int mvcost,mvleft;
  } legal_moves[NPC_SIDE][NPC_SIDE],final;

  find_desire(np,des_array);			/* fill desireability array */
  for(i = 0;i < NPC_SIDE; i++){			/* fill in border of legal mv */
    legal_moves[0][i].mvleft = -2;
    legal_moves[NPC_SIDE-1][i].mvleft = -2;
    legal_moves[i][0].mvleft = -2;
    legal_moves[i][NPC_SIDE-1].mvleft = -2;
  }

  /* Have NPC nation take sectors */
  
  if (np->n_armies == 0) {	/* If there are no armies, forget it */
    return;
  }
  for (ap = np->armies; ap; ap = ap->next) {
    if (debug)
      printf("%d\n", ap->id);
    if(is_mage(ap) && ap->n_soldiers == 1)
      continue;
    check_moves(np,ap,legal_moves);		/* new function */
    high_des = -1;
    for (x = ap->pos.x-NPC_VIEW; x <= ap->pos.x+NPC_VIEW; x++) {
      for (y = ap->pos.y-NPC_VIEW; y <= ap->pos.y+NPC_VIEW; y++) {

/* Now check if it's the most desireable sector so far */
/* and if it can be moved to. */

        if (good_altitude(&world.map[(*wrapx)(x,y)][(*wrapy)(x,y)],np)) {
	  des_here = des_array[(*wrapx)(x,y)][(*wrapy)(x,y)].final;
	  tx = x - ap->pos.x + NPC_VIEW + 1;
	  ty = y - ap->pos.y + NPC_VIEW + 1;
	  if (legal_moves[tx][ty].mvleft >= 0){
	    if(des_here > high_des) {
	      nhigh = 1;
	      highlist[0].x = x;
	      highlist[0].y = y;
	      high_des = des_here;
	    }
	    if(des_here == high_des) {
	      highlist[nhigh].x = x;
	      highlist[nhigh++].y = y;
	    }
          }
	}
      }		/* end y loop */
    }		/* enc x loop */

    /* Move army to one of the desireable sectors and change status to OCCUPY */
    /* Change the desireability of the sector so not all the armies go there */

    i = RND() % nhigh;
    tx = highlist[i].x - ap->pos.x + NPC_VIEW + 1;
    ty = highlist[i].y - ap->pos.y + NPC_VIEW + 1;
    x = (*wrapx)(highlist[i].x,highlist[i].y);
    y = (*wrapy)(highlist[i].x,highlist[i].y);
    args[1].data.num = ap->id;
    args[2].data.num = x;
    args[3].data.num = y;
    args[4].data.num = legal_moves[tx][ty].mvleft;
    cmd_amove(np,args);		/* move army to new location */
    if(good_altitude(&world.map[x][y],np) || is_water(ap)){
      args[2].data.num = A_OCCUPY;
      cmd_astat(np,args);
    }
    des_array[x][y].final = des_array[x][y].final * opt_army_size /
			(opt_army_size+ap->n_soldiers);
  }
}

/*------------------------------do_npc_redesig()-------------------------------
	Redesignate sectors that the npc owns.
-----------------------------------------------------------------------------*/
do_npc_redesig(np)
     Snation *np;
{
  struct pt_list *Pointer;
  int Loop,x,y;
 
  /* Have NPC nation redesignate sectors */
  
  Pointer = np->ptlist;
  for (Loop = 0; Loop < np->n_sects; Loop++) {
    x = Pointer->pt.x;
    y = Pointer->pt.y;
    if (world.map[x][y].n_people > 0 &&
	world.map[x][y].designation == D_NODESIG) {
      if ((world.map[x][y].metal + world.map[x][y].jewels > 0)
	  && ((world.map[x][y].metal > world.map[x][y].soil - 6)
	  || (world.map[x][y].jewels > world.map[x][y].soil - 6))) {
	if (world.map[x][y].metal > world.map[x][y].jewels) {
	  world.map[x][y].designation = D_METAL_MINE;
	  np->money -= desig_map[D_METAL_MINE].price;
	} else {
	  world.map[x][y].designation = D_JEWEL_MINE;
	  np->money -= desig_map[D_JEWEL_MINE].price;
	}
      } else {
	if (world.map[x][y].soil >= 0) {
	  world.map[x][y].designation = D_FARM;
	  np->money -= desig_map[D_FARM].price;
	}
      }
    }
  Pointer = Pointer->next;
  }
}
