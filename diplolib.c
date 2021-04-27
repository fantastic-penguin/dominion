/* diplolib.c -- basic, non-visual diplomacy routines */

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

/* what follows here is a list of ALL routines in this file and
   a one-line blurb stating what it does......
*/

/* allocate_diplo(n): allocates space for a new diplomacy matrix */
/* init_diplo(n): initializes diplo matrix of size n */
/* increase_diplo(dm_old, dm_new, n, np): used when new nations are added */
/* read_in_diplo(dm, n): reads in current diplo matrix from file */
/* read_initial_diplo(dm, n): reads beginning-of-turn diplomacy in */
/* dump_diplo(np, dm, n): dumps new diplo information to a file */
/* update_diplo(): called by dominionupdate to update newly met nations */
/* have_met(dm, nation1, nation2): returns true if nation 1 and 2 have met */
/* get_indices(dm, nation1, nation2, n1, n2): takes ids, gives indices in dm */
/* handle_meet(dm, nation1, nation2): actually changes from UNMET to NEUTRAL */
/* get_diplo_status(dm, nation1, nation2): will return the status of n1 to n2*/
/* set_diplo_status(dm, nation1, nation2, New_Status): changes status */
/* are_allied(nation1, nation2): returns 1 if they are allied */
/* lock_diplo(np): lock the diplomacy file */
/* unlock_diplo(): unlock the diplomacy file */
/* diplo_is_locked(): see if the diplo file is locked */

#include "dominion.h"
#include "misc.h"
#include "functions.h"
#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

extern Sworld world;
extern int (*wrapx)(), (*wrapy)();

/* */
/* allocate_diplo(): allocates space for a new diplomacy matrix */
/* */

Sdiplo **allocate_diplo(n)
     int n;			/* number of nations */
{
  int i;
  Sdiplo **dm;

  dm = (Sdiplo **) malloc(n*sizeof(Sdiplo *));
  for (i = 0; i < n; ++i) {
    dm[i] = (Sdiplo *) malloc(n*sizeof(Sdiplo));
  }
  return dm;
}

/* */
/* initializes a diplomacy data structure at the beginning
   of the game, when there is no file, and all nations are unmet! */
/* */

void init_diplo(n)
     int n;			/* number of nations */
{
  Sdiplo **dm;
  int i, j;
  FILE *fp, *fopen();

  dm = allocate_diplo(n);

  for (i = 0; i < n; ++i) {
    for (j = 0; j < n; ++j) {
      dm[i][j].status = UNMET;
      dm[i][j].self_id = world.nations[i].id;
      dm[i][j].neighbor_id = world.nations[j].id;
    }
    dm[i][i].status = SELF;
  }

  if ((fp = fopen(DIPLO_FILE, "w")) == NULL) {
    printf("sorry, cannot open file %s for writing\n", DIPLO_FILE);
    return;
  }
  critical();
  for (i = 0; i < n; ++i) {
    fwrite(dm[i], n*sizeof(Sdiplo), 1, fp);
  }
  fclose(fp);
  noncritical();
}

/* increase_diplo(): Called when a nation has been added to world.
     take a diplo matrix, and add a new blank entry to it.
     assume that dm_old and dm_new both have storage */

void increase_diplo(dm_old, dm_new, n, np)
     Sdiplo **dm_old, **dm_new;
     int n;			/* number of nations *before* increase */
     Snation *np;
{
  int i, j;

    /* first copy the old stuff */
  for (i = 0; i < n; ++i) {
    for (j = 0; j < n; ++j) {
      dm_new[i][j] = dm_old[i][j];
    }
  }
    /* then put defaults for the new stuff */
  for (i = 0; i <= n; ++i) {
    dm_new[n][i].status = UNMET;
    dm_new[n][i].self_id = np->id;  /* was world.nations[i].id */
    dm_new[n][i].neighbor_id = world.nations[i].id;
      /* now do the transpose */
    dm_new[i][n].status = UNMET;
    dm_new[i][n].self_id = world.nations[i].id;
    dm_new[i][n].neighbor_id = np->id; /* was world.nations[n].id */
  }
  dm_new[n][n].status = SELF;
}


  /* reads information into the diplomacy file */
int read_in_diplo(dm, n)
     Sdiplo **dm;
     int n;			/* number of nations */
{
  int i;
  FILE *dfp, *fopen();

  if ((dfp = fopen(DIPLO_FILE, "r")) == NULL) {
    return -1;
  }

  for (i = 0; i < n; ++i) {
    fread(dm[i], n*sizeof(Sdiplo), 1, dfp);
  }
  fclose(dfp);
  return 1;
}

/* This reads in the diplomacy matrix as it was at the beginning of
   the turn.  It is important to do this, because the software must
   know if the user is changing diplomacy by more than 2 levels.
 */
void read_initial_diplo(dm, n)
     Sdiplo **dm;
     int n;			/* number of nations */
{
  int i;
  FILE *dfp, *fopen();

  if ((dfp = fopen(INIT_DIPLO_FILE, "r")) == NULL) {
    return;
  }

  for (i = 0; i < n; ++i) {
    fread(dm[i], n*sizeof(Sdiplo), 1, dfp);
  }
  fclose(dfp);
}

  /* dumps the data structure to the diplomacy file. */
void dump_diplo(np, dm, n)
     Snation *np;		/* the nation that is writing (if any) */
     Sdiplo **dm;
     int n;
{
  FILE *dfp, *fopen();
  int i;

  if ((dfp = fopen(DIPLO_FILE, "w")) == NULL) {
    return;
  }
  critical();
  lock_diplo(np);
  for (i = 0; i < n; ++i) {
    fwrite(dm[i], n*sizeof(Sdiplo), 1, dfp);
  }
  fclose(dfp);
  noncritical();
  unlock_diplo();
}


/* update_diplo(): called by the update routine so that when two
     nations have met, they will have their status changed from UNMET
     to NEUTRAL.  */

void update_diplo()
{
  Sdiplo **dm;
  struct pt_list *plist;
  Sarmy *army_ptr;
  struct armyid *sect_alist;
  Ssector *sect_ptr;
  Snation *cn;
  int x,y, i,j, k, cn_id;
  FILE *fp, *fopen();
  char cmd_str[PATHLEN];

/*   printf("updating diplomacy\n"); */
  dm = allocate_diplo(world.n_nations);
  read_in_diplo(dm, world.n_nations);

  for (k = 1; k < world.n_nations; ++k) {   /* to run through all nations */
    cn = &world.nations[k];        /* current nation */
    cn_id = world.nations[k].id;  /* current nation id */

    for (plist = cn->ptlist; plist != NULL; plist = plist->next){
      x = plist->pt.x;
      y = plist->pt.y;
      for(i= x-LAND_SIGHT; i <= x+LAND_SIGHT; ++i) {
	for(j= y-LAND_SIGHT; j <= y+LAND_SIGHT; ++j) {
	    /* unowned */
	  if ((world.map[(*wrapx)(i,j)][(*wrapy)(i,j)].owner != 0) &&
              (world.map[(*wrapx)(i,j)][(*wrapy)(i,j)].owner != cn_id)) {
	    handle_meet(dm, cn_id,
			world.map[(*wrapx)(i,j)][(*wrapy)(i,j)].owner);
	  }
	}
      }
    }
    for (army_ptr = cn->armies; army_ptr != NULL; army_ptr = army_ptr->next){
      x = army_ptr->pos.x;
      y = army_ptr->pos.y;
      for(i= x-ARMY_SIGHT; i <= x+ARMY_SIGHT; ++i) {
	for(j= y-ARMY_SIGHT; j <= y+ARMY_SIGHT; ++j) {
	  sect_ptr = &world.map[(*wrapx)(i,j)][(*wrapy)(i,j)];
	    /* unowned */
	  if ((world.map[(*wrapx)(i,j)][(*wrapy)(i,j)].owner != 0) &&
	      (world.map[(*wrapx)(i,j)][(*wrapy)(i,j)].owner != cn_id)) {
	    handle_meet(dm, cn_id,
			world.map[(*wrapx)(i,j)][(*wrapy)(i,j)].owner);
	  }
	  for (sect_alist = sect_ptr->alist; sect_alist != NULL;
	       sect_alist = sect_alist->next) {
	    if ((sect_alist->owner != 0) && (sect_alist->owner != cn_id)) {
	      handle_meet(dm, cn_id, sect_alist->owner);
	    }
	  }
	}
      }
    }
  }
  dump_diplo(NULL, dm, world.n_nations);
  free_diplo(dm, world.n_nations);
  sprintf(cmd_str, "cp %s %s\n", DIPLO_FILE, INIT_DIPLO_FILE);
  system(cmd_str);
/*    printf("done with diplomacy... too slow??\n"); */
}

int have_met(dm, nation1, nation2)
     Sdiplo **dm;
     int nation1, nation2;
{
  int ind1, ind2;

  ind1=0; ind2=0;
  if ((nation1 >= world.n_nations) || (nation2 >= world.n_nations)) {
    return(0);    /* bad nation id.... return false */
  }
  get_indices(dm, nation1, nation2, &ind1, &ind2);

  if ((dm[ind1][ind2].status==UNMET)||(dm[ind1][ind2].status == SELF)) { 
				/* if unmet, or self    */
    return(0);			/* false */
  } else {
    return(1);			/* true */
  }

}

void get_indices(dm, nation1, nation2, n1, n2)
     Sdiplo **dm;
     int nation1, nation2, *n1, *n2;
{

// /*  while(nation1 != dm[(*n1)++][0].self_id)      /* find nation 1 */
// /*    ;
//   (*n1)--;
//   while(nation2 != dm[*n1][(*n2)++].neighbor_id)  /* find nation 2 */
// /*    ;
//   (*n2)--;
// */
    *n1 = nation1;
    *n2 = nation2;
}


void handle_meet(dm, nation1, nation2)
     Sdiplo **dm;
     int nation1, nation2;
{

  int n1, n2; 

  n1=0; n2=0;
  if (!have_met(dm,nation1,nation2)) {
    get_indices(dm, nation1, nation2, &n1, &n2);
    dm[n1][n2].status = NEUTRAL;      /* set their status to neutral */
    n1=0; n2=0;                /* now do transpose */
    get_indices(dm, nation2, nation1, &n2, &n1);
    dm[n2][n1].status = NEUTRAL;  

  }
}


int get_diplo_status(dm, nation1, nation2)
     Sdiplo **dm;
     int nation1, nation2;
{
  int n1, n2;

  n1=0; n2=0;
  if ((nation1 >= world.n_nations) || (nation2 >= world.n_nations)) {
    return(0);    /* bad nation id.... return false */
  };
  get_indices(dm, nation1, nation2, &n1, &n2);
  return(dm[n1][n2].status);
}

void set_diplo_status(dm, nation1, nation2, New_Status)
     Sdiplo **dm;
     int nation1, nation2, New_Status;
{
  int n1,n2;

  n1 = 0; n2 = 0;
  if ((nation1 >= world.n_nations) || (nation2 >= world.n_nations)) {
    printf("Bad Nation ID in set_diplo_status.\n");    /* bad nation id... */
  };
  get_indices(dm, nation1, nation2, &n1, &n2);
  dm[n1][n2].status = New_Status;
}

/* */
/* free_diplo(): frees space for of diplomacy matrix */
/* */

void free_diplo(dm, n)
     Sdiplo **dm;
     int n;			/* number of nations */
{
  int i;

  for (i = 0; i < n; ++i) {
    free(dm[i]);
  }
  free(dm);

}

#ifdef OLD_NPC_DIPLO
void update_npc_diplo()
{
  Sdiplo **dm, **allocate_diplo();
  Snation *np;
  int Loop;
  dm = allocate_diplo(world.n_nations);
  read_in_diplo(dm, world.n_nations);
  for (Loop = 1; Loop < world.n_nations; Loop++) {
    np = &world.nations[Loop];
    if (np->npc_flag !=0) {
      do_npc_diplo(dm, np);
    }
  }
  dump_diplo(NULL, dm, world.n_nations);
  free_diplo(dm, world.n_nations);
}

void do_npc_diplo(dm,np)
     Sdiplo **dm;
     Snation *np;
{
  int Loop;
  
  /* Take care of NPC diplomacy here */
  
  for (Loop = 1; Loop < world.n_nations; Loop++) {
    set_diplo_status(dm,np->id,Loop,get_diplo_status(dm,Loop,np->id));
  }
}
#endif /* OLD_NPC_DIPLO */

  /* this returns 1 if these two nationes are allied to each other.
     It uses the user.diplo_matrix, instead of reading it in, for speed.
   */
int are_allied(id1, id2)
     int id1, id2;
{
/*  Sdiplo **dm, **allocate_diplo(); */
  extern Suser user;
  Sdiplo **dm = user.diplo_matrix;

/*  dm = allocate_diplo(world.n_nations);
  read_in_diplo(dm, world.n_nations);
*/

  if (dm[id1][id2].status >= ALLIED && dm[id2][id1].status >= ALLIED) {
    return 1;
  }
  /* free_diplo(dm, world.n_nations); */
  return 0;
}

  /* creates a diplomacy lock file, and puts some information into it */
int lock_diplo(np)
     Snation *np;
{
  FILE *lockfp, *fopen();
  long now_secs;

  if ((lockfp = fopen("diplo.lock", "w")) == NULL) {
    return -1;
  }
  now_secs = time(0L);
  fprintf(lockfp, "%ld; Nation: %s; time: %s", now_secs,
	  np ? np->name : "none", ctime(&now_secs));
  fclose(lockfp);
  return 1;
}

void unlock_diplo()
{
  unlink("diplo.lock");
}

  /* returns 1 if the diplo file is locked */
diplo_is_locked()
{
  FILE *lock_fp, *fopen();
  char line[EXECLEN];
  long now_secs, lock_secs;

  if ((lock_fp = fopen("diplo.lock", "r")) == NULL) {
    return 0;			/* there is no lock file */
  }
    /* now see if the lock file is outdated.  if so, remove it */
  fscanf(lock_fp, "%s", line);
  sscanf(line, "%d", &lock_secs);
  now_secs = time(0L);
  if (abs(now_secs - lock_secs) > 2*30) { /* older than 2 minutes? */
    unlink("diplo.lock");
    return 0;
  }
  return 1;			/* if not, it is locked!! */
}
