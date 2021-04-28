/* battle.c -- functions relating to resolving battles between nations    */

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

/* against(dm,list,nation) - checks a list of armies (in a sector) for    */
/*         any army which the nation specified is against (at WAR/JIHAD)  */
/* count_force(np,sp,ap) - calculates the EFFECTIVE force of an army      */
/*         which a unit represents including nation, sector, army bonuses */
/* free_list(list) - frees up memory which was used in the list           */
/* status_check(list,status) - checks a 'sector list' of armies for a     */
/*         particular status and returns one if it occurs, else zero      */
/* supports(dm,list,nation) - checks a list of armies for any army which  */
/*         the nation specified supports (TREATY)                         */
/**************************************************************************/
/* These remaining functions are probably useless for other purposes      */
/**************************************************************************/
/* dobattles() - resolves all battles in the world during an update       */
/* move_intercepts(dm) - moves each army with INTERCEPT status to         */
/*         bordering hostile armies and sets to ATTACK status.  If there  */
/*         is no hostile army locally, keep army in INTERCEPT mode.       */
/* add_to_list(list,army,mail_list) - adds the army to the list specified */
/*         and also to the mail list IF it does not already occur there.  */
/* extract_losses(list,pct_loss,mail_list,type) - Extracts losses from    */
/*         armies in a sector.  If type is 3, the army list is of neutrals*/
/* total_bonus(np,sp,ap) - gets TOTAL bonus for that army                 */
/* battle_mail(mail_list,s) - adds string s to mail for nations in list   */
/* intro_battle_mail(mail_list,x,y) - sends intro message with rel coord. */
/* intro_battle_news(...) - posts news about the battle                   */
/* single_mail(nation,s) - adds string s to mail for nation #nation       */
/* is_war(dm,x,y) - sees if any war should happen in this sector          */
/* battle(dm,x,y,fp) - main routine which deals with all battles in a sector */

#include "dominion.h"
#include "misc.h"
#include "army.h"
#include "functions.h"

#include <stdio.h>

/* used as type parameter values passed to extract_losses */

#define ALLIES 1
#define ENEMIES 2
#define NEUTRALS 3


extern Sworld world;
int (*wrapx)(), (*wrapy)();
FILE *mailfile;

void dobattles()
{
  int x,y;
  Sdiplo **dm, **allocate_diplo();
  FILE *tmp_fp, *fopen();	/* for the news posting */
  char tmp_name[100];
  char subj[100];
  dm = allocate_diplo(world.n_nations);
  read_in_diplo(dm,world.n_nations);

  printf("Doing battles...\n");
  fflush(stdout);

    /* a temporary file name for this news posting */
/*  tmp_name = tmpnam(NULL, "dominion"); */
  strcpy(tmp_name, "dominionXXXXXX");
  mktemp(tmp_name);

  if (strlen(tmp_name) == 0) {
    fprintf(stderr, "Error getting temp file name\n");
    return;
  }
  if ((tmp_fp = fopen(tmp_name, "w")) == NULL) {
    fprintf(stderr, "Error opening file %s for writing\n", tmp_name);
    return;
  }
/*  fprintf(tmp_fp, "Date: Thon %d\n", world.turn);
  fprintf(tmp_fp, "From: Update\n");
  fprintf(tmp_fp, "Subj: battles from thon %d to thon %d\n", world.turn-1,
	  world.turn);*/
  sprintf(subj, "battles from thon %d to thon %d",world.turn-1,world.turn);

  move_intercepts(dm);

  for (x = 0; x < world.xmax; x++) {
    for (y = 0; y < world.ymax; y++) {
      if (world.map[x][y].alist && world.map[x][y].alist->next
	  && is_war(dm, x, y, tmp_fp)) {
/*	battle(dm, x, y, tmp_fp); */
      }
    }
  }
  fclose(tmp_fp);
  post_news_file(tmp_name, NEWS_GROUP,subj,0);
  printf("just posted file to newsgroup <%s>\n", NEWS_GROUP);
}

  /* takes all armies in intercept mode and sees if
     it can move them to attack an enemy army.
   */
void move_intercepts(dm)
     Sdiplo **dm;
{
  int nation,x,y;
  Snation *np;
  Ssector *sp;
  Sarmy *ap;

  for (nation = 1; nation < world.n_nations; nation++) {
    np = &world.nations[nation];
    ap = np->armies;
    while (ap != NULL) {
      if (ap->status == A_INTERCEPT) {
	for (x = ap->pos.x - 1; x <= ap->pos.x + 1; x++) {
	  for (y = ap->pos.y - 1; y <= ap->pos.y + 1; y++) {
	    sp = &world.map[(*wrapx)(x,y)][(*wrapy)(x,y)];
	      /* army might not be in intercept mode any more,
		 in which case we don't make it move.  that way
		 an intercept army will only move once.
	       */
	    if (ap->status == A_INTERCEPT && against(dm, sp->alist, nation)
		  /* be careful about moving into bad altitude */
		&& (good_army_altitude(np, sp, ap) || is_kamikaze(ap))) {
	      printf("%s army %d is intercepting from (%d,%d) to (%d,%d)\n",
		     np->name, ap->id, ap->pos.x, ap->pos.y, x, y);
	      ap->status = A_ATTACK;
/*	      if (sect_n_armies(&world.map[ap->pos.x][ap->pos.y]) > 1) { */
	      delete_army_sector(&world.map[ap->pos.x][ap->pos.y],ap);
/*	      } else {
		free(sp->alist);
		sp->alist = NULL;
	      }
*/
	      ap->pos.x = (*wrapx)(x,y);
	      ap->pos.y = (*wrapy)(x,y);
	      insert_army_sector(sp, ap);
	    }
	  }
	}
      }
      ap = ap->next;
    }
  }
}

/*
     against(): searches throught the list of armies
                and checks for their diplomatic status
		with 'nation' and vice versa.  When
		the nation status is either WAR or JIHAD
		then it returns 1 else 0
                This routine is called from battle()
*/

int against(dm,list,nation)
     Sdiplo **dm;
     struct armyid *list;
     int nation;
{
  struct armyid *tmp;
  int ds1,ds2;
  
  tmp = list;
  while (tmp != NULL) {
    ds1 = get_diplo_status(dm,nation,tmp->owner);
    ds2 = get_diplo_status(dm,tmp->owner,nation);
    if (ds1 == WAR || ds1 == JIHAD || ds2 == WAR || ds2 == JIHAD) {
      return 1;
    }
    tmp = tmp->next;
  }
  return 0;
}

/*  this routine is being called from battle() */

int supports(dm,list,nation)
     Sdiplo **dm;
     struct armyid *list;
     int nation;
{
  struct armyid *tmp;
  int ds1,ds2;
  
  tmp = list;
  while (tmp != NULL) {
    if (tmp->owner == nation) {	/* a nation is allied with itself */
      return 1;
    }
    ds1 = get_diplo_status(dm,nation,tmp->owner);
    ds2 = get_diplo_status(dm,tmp->owner,nation);
    if (ds1 == TREATY && ds2 == TREATY) {
      return 1;
    }
    tmp = tmp->next;
  }
  return 0;
}

/* this routine is being called from battle() as well */
/* also being called from battle() */

void add_to_list(list,army,mail_list)
     struct armyid **list, *army, **mail_list;
{
  struct armyid *tmp;

  tmp = (struct armyid *) malloc(sizeof(struct armyid));
  *tmp = *army;
  tmp->next = (*list);
  (*list) = tmp;

  tmp = (*mail_list);
  while (tmp != NULL) {
    if (tmp->owner == army->owner) {
      return;
    }
    tmp = tmp->next;
  }
  tmp = (struct armyid *) malloc(sizeof(struct armyid));
  *tmp = *army;
  tmp->next = (*mail_list);
  (*mail_list) = tmp;
}

/***************************************************
     total_bonus() returns all bonuses of an army
     if an army is in TREATY with the owner of the 
     sector then they can enjoy sector defense bonuses
     and have a choice of being in DEFEND, GARRISON,
     AMBUSH etc.
     if an army is in garrison mode they get extra 10
     this function has the flexibility in case we want
     other modes.  For starters I will add some experimental
     stuff with ALLIED status with sector's owner
     ALLIED will give 1/2 of sector bonuses and +5 for
     garrison mode.   A nation with very strong 
     sector bonuses will invite more nations to be in
     treaty with him because any time they fight in a sector
     owned by this strong nation will enjoy the sector
     bonuses as well.
     This routine is called from battle() and from extract_losses
*************************************************************/

int total_bonus(np,sp,ap,dm)
Snation *np;
Ssector *sp;
Sarmy *ap;
Sdiplo **dm;
{
  int bonus = 0;
  int ds1,ds2;  /* test to see if army is in trearty with sector owner */
     /* if they are in treaty with each other then the army can share the*/
     /* sector's defense bonuses. */

  if (sp->owner == np->id) {
     bonus += np->defense;
     bonus += sp->defense;
     if (ap->status == A_GARRISON)
       bonus += 10;
   } else {
  ds1 = get_diplo_status(dm,sp->owner,np->id);
  ds2 = get_diplo_status(dm,np->id,sp->owner);  

	/* count the nation's bonus */
   if ((ds1 == TREATY || ds1 == ALLIED) && (ds2 == TREATY || ds2 == ALLIED)) {
      switch(ap->status) {
        case A_DEFEND: bonus += np->defense;
                     if (ds1 == TREATY)
                       bonus += sp->defense;
                     else /* ALLIED */
                       bonus += (sp->defense)/2;
                    break;
        case A_ATTACK:
                    { bonus += np->attack;
                       if (ds1 == TREATY)
                         bonus += sp->defense;
                       else /* ALLIED */
                         bonus += (sp->defense)/2;
		    }
                     break;
        case A_GARRISON:
                   { bonus += np->defense;
                     if (ds1 == TREATY) {
                       bonus += sp->defense;
                       bonus += 10; }
                     else {/* ALLIED */
                       bonus += (sp->defense)/2;
                       bonus += 5; }
		    }
                     break;
        default: break;
      } /* end switch */
    } else 
        bonus += np->attack;  /* not allied with owner */
                                /* just attack bonuses */
  }

  bonus += ap->sp_bonus;	/* army's special bonus */

  return bonus;
}

  /* returns the force of that army (minimum 1) */
  /* this is being called by battle() and extract_losses()
     and makes a call to total bonus.  It is important to
     know that **dm is being passed along and not used
     until it reaches total_bonus() */

int count_force(np,sp,ap,dm)
     Snation *np;
     Ssector *sp;
     Sarmy *ap;
     Sdiplo **dm;  /* passed along to total bonus */
{
  int      force;  

  force = (ap->n_soldiers*(100+total_bonus(np,sp,ap,dm)))/100;
  if (force < 1) {
    return 1;			/* return at least 1 */
  }
  return force;
}

int count_men(force_list,flags)
/*
   This counts the nubmer of units in a given force counting only those
   units without the flags in flags set.
*/
struct armyid *force_list;
int flags;
{
  struct armyid *tmp_alist;
  Sarmy *ap = NULL, *get_army();
  Snation *np;
  int total = 0;

  for (tmp_alist= force_list; tmp_alist != NULL; tmp_alist = tmp_alist->next) {
    np = &world.nations[tmp_alist->owner];
    ap = get_army(np, tmp_alist->id);
    if ((!(ap->flags & flags)) && !(is_spirit(ap))) {
      total += ap->n_soldiers;
    }
  }
  return total;
}

int count_machine(force_list)
/*
   This counts the nubmer of units in a given force counting only those
   units without the flags in flags set.
*/
struct armyid *force_list;
{
  struct armyid *tmp_alist;
  Sarmy *ap = NULL, *get_army();
  Snation *np;
  int total = 0;

  for (tmp_alist= force_list; tmp_alist != NULL; tmp_alist = tmp_alist->next) {
    np = &world.nations[tmp_alist->owner];
    ap = get_army(np, tmp_alist->id);
    if (is_machine(ap)) {
      total += ap->n_soldiers;
    }
  }
  return total;
}

int mach_bonus_avg(force_list)
/*
   This computes the average bonus gained from machines.
*/
struct armyid *force_list;
{
  struct armyid *tmp_alist;
  Sarmy *ap = NULL, *get_army();
  Snation *np;
  int tot_mach_bonus = 0, total_units = 0;

  for (tmp_alist= force_list; tmp_alist != NULL; tmp_alist = tmp_alist->next) {
    np = &world.nations[tmp_alist->owner];
    ap = get_army(np, tmp_alist->id);
    if (is_machine(ap))
    {
      tot_mach_bonus += ap->sp_bonus * ap->n_soldiers;
      total_units += ap->n_soldiers;
    }
  }
  return (tot_mach_bonus / total_units);
}

/* This function is being called from battle() */

int extract_losses(list, pct_loss, mail_list, type, dm)
     struct armyid *list;
     float pct_loss;
     struct armyid *mail_list;
     int type;
     Sdiplo **dm;
{
  struct armyid *tmp_aid;
  Snation *np;
  Ssector *sp;
  Sarmy *ap, *get_army();
  int killed, dead = 0;
  char s[EXECLEN], name[NAMELEN];

  tmp_aid = list;
  if (tmp_aid != NULL) {
    if (type == NEUTRALS) {
      sprintf(s, "---<<< NEUTRAL FORCES IN THE REGION >>>---\n");
    } else {
        if (type == ALLIES)
          sprintf(s, "---<<<  First force >>>---\n");
        else
          sprintf(s, "---<<< Second force >>>---\n");
    }
    battle_mail(mail_list,s);
  }
  while (tmp_aid != NULL) {
    np = &world.nations[tmp_aid->owner];
    ap = get_army(np,tmp_aid->id);
    sp = &world.map[ap->pos.x][ap->pos.y];
      /* special case for mages and ships:  they
	 only die if more than 80% of supporting armies
	 are killed.
       */
    if ((strcmp(ap->type, "Mage") == 0) || is_cargo(ap)) {
      if (pct_loss > 0.8) {
	killed = ap->n_soldiers;
      } else {
	killed = 0;
      }
    } else {
      killed = (int) (ap->n_soldiers * pct_loss + .5);
        /* if less than 5 soldiers, or if kamikaze unit, kill them */
      if ((ap->n_soldiers - killed) < 5 || is_kamikaze(ap)) {
	killed = ap->n_soldiers;
      }
    }

    if (strlen(ap->name) == 0) {
      strcpy(ap->name, "(no name)");
    } else {
      sprintf(name, " \"%s\" ", ap->name);
    }
    sprintf(s,
	"\t%s: Army %s (%d):\n\t\t%d %s\n",
	    np->name, name, ap->id, ap->n_soldiers, ap->type);
    battle_mail(mail_list,s);

/* AH YOU LITTLE BUGGER, the cause of the segmentation fault mystery
   I didn't look at this sprintf() funtion and it does in fact call
   total_bonus() and count_force.  So I just added an extra parameter
   **dm
 */
    sprintf(s, "\t\tbonus %d, force %d;\n\t\tloses %d men.\n",
	    total_bonus(np, sp, ap, dm), count_force(np, sp, ap, dm), killed);
    battle_mail(mail_list,s);

    ap->n_soldiers -= killed;
    dead += killed;
    if (ap->n_soldiers == 0) {	/* Army destroyed */
      delete_army_sector(sp, ap);
      delete_army_nation(np, ap);
    }
    tmp_aid = tmp_aid->next;
  }
    /* now that we have the dead count, we can raise some of them
       to join with the vampire units.  this code is primitive:
       just for example, it does not take into account that more
       people may rise than actually died (if there are lots of
       vampire units).
     */
  for (tmp_aid = list; tmp_aid != NULL; tmp_aid = tmp_aid->next) {
    np = &world.nations[tmp_aid->owner];
     /* only if we *do* get the army can we go on. it might have been killed */
    if ((ap = get_army(np, tmp_aid->id))) {
/*      sp = &world.map[ap->pos.x][ap->pos.y]; why??? */
      if (is_vampire(ap)) {
          /* notice that an army canot grow by more than a certain amount */
	ap->n_soldiers += min((int) (dead*VAMPIRE_FRACT), 10*ap->n_soldiers);
      }
    }
  }
  return dead;
}

void free_list(list)
     struct armyid *list;
{
  struct armyid *tmp;

  while (list != NULL) {
    tmp = list;
    list = list->next;
    free(tmp);
  }
}

int status_check(list,status)
     struct armyid *list;
     int status;
{
  struct armyid *tmp;
  Sarmy *ap, *get_army();
  
  tmp = list;
  while (tmp != NULL) {
    ap = get_army(&world.nations[tmp->owner],tmp->id);
    if (ap->status == status) {
      return 1;
    }
    tmp = tmp->next;
  }
  return 0;
}

void battle_mail(mail_list,s)
     struct armyid *mail_list;
     char s[];
{
  struct armyid *alist;

  alist = mail_list;
  while (alist != NULL) {
    single_mail(alist->owner,s);
    alist = alist->next;
  }
}

  /* prints the introductory message, with relative
     coordinates for each nation.
   */
void intro_battle_mail(mail_list, x, y)
     struct armyid *mail_list;
     int x, y;
{
  Snation *np;
  struct armyid *alist = mail_list;
  char s[EXECLEN];

  while (alist) {
    Snation *sect_owner = &world.nations[world.map[x][y].owner];

    np = &world.nations[alist->owner];
    sprintf(s, "\nBattle Reported in Sector (%d, %d) [owner: %s]:\n",
	    xrel(x, y, np->capital), yrel(x, y, np->capital),
	    sect_owner->id == 0 ? "unowned" : sect_owner->name);
    single_mail(alist->owner, s);
    alist = alist->next;
  }
}

  /* prints some stuff to the News about the battle */
void intro_battle_news(news_fp, x, y, ally_list, enemy_list, neutral_list)
     FILE *news_fp;
     int x, y;
     struct armyid *ally_list, *enemy_list, *neutral_list;
{
  Snation *np;
  struct armyid *alist;		/* to run through the lists */
  Sarmy *ap, *get_army();
  Ssector *sp = &world.map[x][y];
  char s[EXECLEN];

  np = &world.nations[world.map[x][y].owner]; /* nation in which it happens */
  fprintf(news_fp, "\nBattle Reported in %s", np->id ? np->name
	  : "unowned land");
  if (strlen(sp->name) > 0) {
    fprintf(news_fp, " (\"%s\")", sp->name);
  }

  fprintf(news_fp, ".\nInvolved armies:\n");
/*  fprintf(news_fp,   "**************\n"); */
  alist = ally_list;
  while (alist) {		/* run through the participants */
    np = &world.nations[alist->owner];
    ap = get_army(np, alist->id);
    if (is_spirit(ap)) {
      fprintf(news_fp, "\t%s (spirit %s)\n", np->name, ap->name);
    } else {
      fprintf(news_fp, "\t%s (army %s)\n", np->name, ap->name);
    }
    alist = alist->next;
  }
  fprintf(news_fp, "versus\n");
  alist = enemy_list;
  while (alist) {		/* run through the participants */
    np = &world.nations[alist->owner];
    ap = get_army(np, alist->id);
    if (is_spirit(ap)) {
      fprintf(news_fp, "\t%s (spirit %s)\n", np->name, ap->name);
    } else {
      fprintf(news_fp, "\t%s (army %s)\n", np->name, ap->name);
    }
    alist = alist->next;
  }
  if (neutral_list) {
    fprintf(news_fp, "Neutral Armies:\n");
    fprintf(news_fp, "--------------:\n");
  }
  alist = neutral_list;
  while (alist) {		/* run through the participants */
    np = &world.nations[alist->owner];
    fprintf(news_fp, "\t%s\n", np->name);
    alist = alist->next;
  }
}

void single_mail(nation,s)
     int nation;
     char s[];
{
  char mailname[NAMELEN];

  if (world.nations[nation].npc_flag != 0) return;

  sprintf(mailname, "mail%d", nation);
  if ( (mailfile = fopen(mailname, "a") ) == NULL)
  {
    fprintf(stderr,"Error: Cannot reopen mail file %s\n",mailname);
    clean_exit();
    exit(1);
  }
  fprintf(mailfile, "%s", s);
  fclose(mailfile);
}

void battle(dm, x, y, news_fp, ally_list, enemy_list, neutral_list, mail_list)
     Sdiplo **dm;
     int x,y;
     FILE *news_fp;			/* for reporting to the news */
     struct armyid *ally_list, *enemy_list, *neutral_list, *mail_list;
{
  Snation *np = NULL;
  Ssector *sp = &world.map[x][y];
  Sarmy *ap = NULL, *get_army();
/*  struct armyid *ally_list, *enemy_list, *neutral_list, *mail_list, *tmp; */
  struct armyid *tmp_alist;
  int ally_force, enemy_force, neutral_force;
  int ally_dead, enemy_dead, neutral_dead;
  int enemy_act_mach, ally_act_mach, neutral_act_mach, mach_force_fact;
  double defense_loss = 0.0;

  float ally_losses, enemy_losses, neutral_losses;
  int nation;

  char mailname[NAMELEN], s[EXECLEN];

  /* Initialize the three lists for armies in the sector's battle */

/*  ally_list = enemy_list = neutral_list = mail_list = NULL; */
  ally_force = enemy_force = neutral_force = 0;
  ally_dead = enemy_dead = neutral_dead = 0;
  ally_losses = enemy_losses = neutral_losses = 0.0;

  /* Now begin breaking the armies up by affiliations and sum forces */
  for (tmp_alist = ally_list; tmp_alist != NULL; tmp_alist = tmp_alist->next) {
    np = &world.nations[tmp_alist->owner];
    ap = get_army(np, tmp_alist->id);
    ally_force += count_force(np,sp,ap,dm);
  }
  for(tmp_alist = enemy_list; tmp_alist != NULL; tmp_alist = tmp_alist->next) {
    np = &world.nations[tmp_alist->owner];
    ap = get_army(np, tmp_alist->id);
    enemy_force += count_force(np,sp,ap,dm);
  }
  for(tmp_alist=neutral_list; tmp_alist != NULL; tmp_alist = tmp_alist->next){
    np = &world.nations[tmp_alist->owner];
    ap = get_army(np, tmp_alist->id);
    neutral_force += count_force(np,sp,ap,dm);
  }

/*  tmp = sp->alist;
  while (tmp != NULL) {
    np = &world.nations[tmp->owner];
    ap = get_army(np,tmp->id);
    nation = tmp->owner;
    if (ally_list == NULL
	|| supports(dm,ally_list,nation)
        || against(dm,enemy_list,nation)) {
      add_to_list(&ally_list,tmp,&mail_list);
      ally_force += count_force(np,sp,ap,dm);
    } else {
      if (supports(dm,enemy_list,nation) 
         || against(dm,ally_list,nation)) {
	add_to_list(&enemy_list,tmp,&mail_list);
	enemy_force += count_force(np,sp,ap,dm);
      } else {
	add_to_list(&neutral_list,tmp,&mail_list);
	neutral_force += count_force(np,sp,ap,dm);
      }
    }
    tmp = tmp->next;
  }
*/
/* Now adjust the forces for the presence of macines */
  if ((ally_act_mach = min(count_machine(ally_list) * MEN_PER_MACHINE,
                        count_men(ally_list,(AF_MACHINE)))) != 0)
  {
    ally_force += ally_act_mach * mach_bonus_avg(ally_list) / 100;
  }
  if ((enemy_act_mach = min(count_machine(enemy_list) * MEN_PER_MACHINE,
                     count_men(enemy_list,(AF_MACHINE)))) != 0)
  {
    enemy_force += enemy_act_mach * mach_bonus_avg(enemy_list) / 100;
  }
  if ((neutral_act_mach = min(count_machine(neutral_list) * MEN_PER_MACHINE,
                            count_men(neutral_list,(AF_MACHINE)))) != 0)
  {
    neutral_force += neutral_act_mach * mach_bonus_avg(neutral_list) / 100;
  }
  /* Now calculate loss percentages for each side */
  if (ally_force != 0 && enemy_force != 0 && 
     (status_check(ally_list,A_ATTACK) || status_check(enemy_list,A_ATTACK) ||
      status_check(ally_list,A_OCCUPY) || status_check(enemy_list,A_OCCUPY))) {

      /* send an introductory message to all users,
	 giving the relative coords of sector in which
	 battle happens.  also send out some news.
       */
    intro_battle_mail(mail_list, x, y);
    intro_battle_news(news_fp, x, y, ally_list, enemy_list, neutral_list);

    ally_losses = (1.0*enemy_force*enemy_force)
      /(1.0*(ally_force*ally_force + enemy_force*enemy_force));
    enemy_losses = (1.0*ally_force*ally_force)
      /(1.0*(ally_force*ally_force + enemy_force*enemy_force));
    if (ally_losses < enemy_losses) {
      neutral_losses = ally_losses/10.0;
    } else {
      neutral_losses = enemy_losses/10.0;
    }
/* Now we mess with the fortifications. */
  np = &world.nations[enemy_list->owner];
  defense_loss = 0.0;
  if (sp->owner == np->id)
  {
    if (ally_act_mach != 0)
    { 
      defense_loss = count_men(ally_list,AF_MACHINE);
      defense_loss = defense_loss/ally_act_mach;
      defense_loss *= enemy_losses;
      defense_loss = (int)(sp->defense * defense_loss);
      sp->defense -= (int)defense_loss;
    }
  }
  np = &world.nations[ally_list->owner];
  if (sp->owner == np->id)
  {
    if (enemy_act_mach != 0)
    { 
      defense_loss = count_men(enemy_list,AF_MACHINE);
      defense_loss = defense_loss/enemy_act_mach;
      defense_loss *= ally_losses;
      defense_loss = (int)(sp->defense * defense_loss);
      sp->defense -= (int)defense_loss;
    }
  }

    /* Now extract losses from each group */
    /* calls to extract_losses() */

    ally_dead = extract_losses(ally_list,ally_losses,mail_list,ALLIES,dm);
    enemy_dead = extract_losses(enemy_list,enemy_losses,mail_list,ENEMIES,dm);
    neutral_dead =
      extract_losses(neutral_list,neutral_losses,mail_list,NEUTRALS,dm);
  } 
  if (defense_loss != 0.0)
  {
    sprintf(s, "\nSector Defenses reduced by %d to %d\n",
                               (int)defense_loss,sp->defense);
    battle_mail(mail_list,s);
  }

  /* Now free up all the memory we ate with our army lists! */
  free_list(ally_list);
  free_list(enemy_list);
  free_list(neutral_list);
  free_list(mail_list);
}

  /* this is an ugly routine which goes into great detail
     to see if a war is actually taking place in a given
     sector.  it is totally inefficient, but until we
     redo the battle code, it will have to do.  the previous
     code used to not yield battles in some cases.
   */
int is_war(dm, x, y, news_fp)
     Sdiplo **dm;
     int x, y;
     FILE *news_fp;
{
  Ssector *sp = &world.map[x][y];
  Snation *np, *tmp_np;
  struct armyid *alist, *tmp_alist;
  int id1, id2, stat1, stat2;

/*  printf("**********sector (%d,%d)***************\n", x, y); */
  alist = sp->alist;
  while (alist != NULL) {
/*    printf("alist = (%d,%d)\n", alist->owner, alist->id); */
    np = &world.nations[alist->owner];
    id1 = np->id;
    tmp_alist = sp->alist;
    while (tmp_alist != NULL) {
/*      printf("\ttmp_alist = (%d,%d)\n", tmp_alist->owner, tmp_alist->id); */
      tmp_np = &world.nations[tmp_alist->owner];
      id2 = tmp_np->id;
      stat1 = get_diplo_status(dm, id1, id2);
/*      printf("stat1 = %d\n", stat1); */
      stat2 = get_diplo_status(dm, id2, id1);
/*      printf("stat2 = %d\n", stat2); */
      if (stat1 == WAR || stat1 == JIHAD || stat2 == WAR || stat2 == JIHAD) {
	struct armyid *other_tmp_alist;
	struct armyid *ally_list, *enemy_list, *neutral_list, *mail_list;
	int nation;

	ally_list = enemy_list = neutral_list = mail_list = NULL;

	printf("Found battle between %s and %s\n", np->name, tmp_np->name);
	add_to_list(&ally_list, tmp_alist, &mail_list);
	other_tmp_alist = sp->alist;
	  /* now we add everything else in this sector to some list */
	while (other_tmp_alist != NULL) {
	    /* bad kludge */
	  nation = other_tmp_alist->owner;
	  /* remember:  we have already added an ally.  don't
	     duplicate him.
	   */
	  if (other_tmp_alist->owner != tmp_alist->owner
	      || other_tmp_alist->id != tmp_alist->id) {
	    if (ally_list == NULL
		|| supports(dm,ally_list,nation)
		|| against(dm,enemy_list,nation)) {
	      add_to_list(&ally_list,other_tmp_alist,&mail_list);
/*	      ally_force += count_force(np,sp,ap,dm); */
	    } else {
	      if (supports(dm,enemy_list,nation) 
		  || against(dm,ally_list,nation)) {
		add_to_list(&enemy_list,other_tmp_alist,&mail_list);
/*		enemy_force += count_force(np,sp,ap,dm); */
	      } else {
		add_to_list(&neutral_list,other_tmp_alist,&mail_list);
/*		neutral_force += count_force(np,sp,ap,dm); */
	      }
	    }
	  }
	  other_tmp_alist = other_tmp_alist->next;
	}
	battle(dm, x, y, news_fp,
	       ally_list, enemy_list, neutral_list, mail_list);
	return 1;
      }
      tmp_alist = tmp_alist->next;
    }
    alist = alist->next;
  }
  return 0;
}
