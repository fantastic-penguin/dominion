/* maglib.c -- routines having to do with magic powers */

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
#include "functions.h"
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>

  /* dynamic array, to be read from file */
extern struct spirit_type *spirit_types;
extern Suser user;

extern int debug;
extern FILE *mailfile;

  /* this is the routine actually called by the update program */
domagic(np, mailfile)
     Snation *np;
     FILE *mailfile;
{
  int old_skill, new_skill, old_pts, new_pts;

  old_skill = np->mag_skill;
  old_pts = np->spell_pts;
  new_skill = new_mag_skill(np);
    /* must subtract the spell points used by spirits and such */
  new_pts = new_spell_pts(np) - military_maint_spell_pts(np);
  if (debug) {
    printf("nation %s has old_mag_skill = %d, new_skill = %d\n",
	   np->name, old_skill, new_skill);
    printf("nation %s has old_pts = %d, new_pts = %d\n",
	   np->name, old_pts, new_pts);
  }
  if (mailfile) {
    fprintf(mailfile,
	    "Your skill in magic has increased from %d to %d\n",
	    old_skill, new_skill);
    fprintf(mailfile,
	    "You have gained %d spell points, for a new total of %d\n",
	    new_pts-old_pts, new_pts);
  }
/*  get_new_spell(&user, old_skill, new_skill); should be in load_nation() or init_user() */
  np->mag_skill = new_skill;
  np->spell_pts = new_pts;
}

#define JEWEL_MAG_POWER (3.0/4.0)

  /* this routine calculates the new mag_skill for a nation */
new_mag_skill(np)
     Snation *np;
{
  int increase;

  increase = (int) 
    ((np->race.mag_apt + priestliness(np)) / 100.0 *
     (sqrt(1.0 * (calc_revenue (np) * np->mag_r_d/100.0
	   + np->cur_mag_r_d * np->money/100.0)) * MAG_MONEY_FACTOR
      + pow((double)(calc_jewels (np) * np->mag_r_d_jewels / 100.0 +
	 np->jewels * np->cur_mag_r_d_jewels / 100.0),JEWEL_MAG_POWER)
         * (MAG_JEWEL_FACTOR * 10))); /* 10 * the number of spell pts */
  return np->mag_skill + increase;
}

  /* this calculates how many new spell points a nation gets this turn.
     note that spell points accumulate but little.
   */
new_spell_pts(np)
     Snation *np;
{
  int new_pts;

  new_pts = (int)( pow((calc_jewels (np) * (np->mag_r_d_jewels / 100.0) +
             np->jewels * np->cur_mag_r_d_jewels / 100.0),JEWEL_MAG_POWER)
		    * MAG_JEWEL_FACTOR *
		  ((np->race.mag_apt + priestliness (np)) / 100.0));
  if (np->spell_pts < 0) {
    return new_pts + np->spell_pts;
  } else {
    return (3*new_pts + np->spell_pts)/3; /* a mild accumulation */
  }
}

  /* these spells are loaded in init_user() */
get_spells(up, skill)
     Suser *up;
     int skill;
{
  FILE *fp, *fopen();
  Sspell spell, *tmp_spell;
  char fname[2*NAMELEN], line[EXECLEN];
  int done = 0, level; /* cost */

  if (debug)
    printf("nation %s, skill=%d\n", up->np->name, skill);
  strcpy(fname, MAG_PREFIX);
  strcat(fname, up->np->mag_order);

  if ((fp = fopen(fname, "r")) == NULL) {
    printf("could not open file for magic order %s.  returning.\n",
	   up->np->mag_order);
    return -1;
  }
  while (!done) {
    if (fgets(line, EXECLEN, fp) == NULL) {
      done = 1;
      fclose(fp);
      return;
    }
    if (line[0] != '#') {	/* lines with '#' are comments */
        /* make sure this is a spell and NOT a
	   summoned spirit or an exec line
	 */
      if (strncmp(line, "summon_", strlen("summon_")) != 0
	  && strncmp(line, "EXEC:", strlen("EXEC:")) != 0) {
	sscanf(line, "%s%d%d%d", spell.name, &level, &spell.cost,
	       &spell.duration);
          /* add it to the user's spell list (if they deserve) */
	if (level <= skill) {
	  if (debug) {
	    printf("found %s, level=%d, cost=%d, duration=%d\n", spell.name,
		   level, spell.cost, spell.duration);
	    printf("nation %s gets new spell <%s>\n",
		   up->np->name, spell.name);
	  }
	  if (up->spell_list == NULL) {
	    up->spell_list = (Sspell *) malloc(sizeof(Sspell));
	    *(up->spell_list) = spell;
	    up->spell_list->next = NULL;
	  } else {
	    tmp_spell = up->spell_list;
	    while (tmp_spell != NULL) { /* find end of list */
	      if (tmp_spell->next == NULL) {
		tmp_spell->next = (Sspell *) malloc(sizeof(Sspell));
		spell.next = NULL; /* making sure */
		*(tmp_spell->next) = spell;
		break;
	      }
	      tmp_spell = tmp_spell->next;
	    }
	  }
	}
      }
    }
  }
  fclose(fp);
}

  /* these spirits are loaded in init_user() */
get_spirits(up, skill)
     Suser *up;
     int skill;
{
  FILE *fp, *fopen();
  Sspirit spirit, *tmp_spirit;
  char fname[2*NAMELEN], line[EXECLEN];
  int done = 0, level;		/* , cost */

  if (debug)
    printf("nation %s, new_skill=%d\n",
	   up->np->name, skill);
  strcpy(fname, MAG_PREFIX);
  strcat(fname, up->np->mag_order);

  if ((fp = fopen(fname, "r")) == NULL) {
    printf("could not open file for magic order %s.  returning.\n",
	   up->np->mag_order);
    return -1;
  }
  while (!done) {
    if (fgets(line, EXECLEN, fp) == NULL) {
      done = 1;
      fclose(fp);
      return;
    }
    if (line[0] != '#') {	/* lines with '#' are comments */
      sscanf(line, "summon_%s%d%d", spirit.type, &level, &spirit.cost);
        /* now we either get the spirit, or skip until the "end" of it */
        /* now see if the line starts with a "summon_" */
      if (strncmp(line, "summon_", strlen("summon_")) == 0) {
	  /* add it to the user's spirit list (if they deserve) */
	if (level <= skill) {
	  if (up->spirit_list == NULL) {
	    up->spirit_list = (Sspirit *) malloc(sizeof(Sspirit));
	    *(up->spirit_list) = spirit;
	    up->spirit_list->next = NULL;
	  } else {
	    tmp_spirit = up->spirit_list;
	    while (tmp_spirit != NULL) { /* find end of list */
	      if (tmp_spirit->next == NULL) {
		tmp_spirit->next = (Sspirit *) malloc(sizeof(Sspirit));
		spirit.next = NULL; /* making sure */
		*(tmp_spirit->next) = spirit;
		break;
	      }
	      tmp_spirit = tmp_spirit->next;
	    }
	  }
	}
      }
    }
  }
  fclose(fp);
}


  /* show a user's collection of spells and spirits */
list_user_spells(up)
     Suser *up;
{
  Sspell *tmp_spell;
  Sspirit *tmp_spirit;

  printf("Spells for Nation %s, of magic order %s:\n",
	 up->np->name, up->np->mag_order);
  for (tmp_spell = up->spell_list; tmp_spell != NULL;
       tmp_spell = tmp_spell->next) {
    show_spell(tmp_spell);
  }
  printf("Spirits available to %s, of magic order %s:\n",
	 up->np->name, up->np->mag_order);
  for (tmp_spirit = up->spirit_list; tmp_spirit != NULL;
       tmp_spirit = tmp_spirit->next) {
    show_spirit(tmp_spirit);
  }
}

show_spell(spellp)
     Sspell *spellp;
{
  int i;

  printf("Spell <%s>, cost %d\n",
	 spellp->name, spellp->cost);
}
show_spirit(spiritp)
     Sspirit *spiritp;
{
  int i;

/*  printf("Spirit <%s>, level %d, cost %d\n",
	 spiritp->type, spiritp->level, spiritp->cost);
  for (i = 0; i < spiritp->n_lines; ++i) {
    printf("%s", spiritp->lines[i]);
  }
*/
  printf("removed, since spirit/spell system has changed");
}

  /* at startup, this reads the spirit_types[] from a file */
load_spirit_types()
{
  FILE *fp, *fopen();
  char line[210];
  int i;

  if ((fp = fopen(SPIRIT_TYPES_FILE, "r")) == NULL) {
    printf("cannot open spirit types file.  quitting.\n");
    perror("load_spirit_types");
    clean_exit();
    exit(1);
  }

  do {
    fgets(line, 200, fp);
  } while (line[0] == '#');
    /* we should have the line with the number of spirit types in file */
  sscanf(line, "%d", &user.n_spirit_types);

  spirit_types =
 (struct spirit_type *) malloc(user.n_spirit_types*sizeof(struct spirit_type));

  for (i = 0; i < user.n_spirit_types; ) {
    fgets(line, 100, fp);
    line[strlen(line)-1] = '\0';
    if (line[0] != '#') {		/* ignore comments */
      sscanf(line,"%s : %1s : %d : %f : %d : %d : %d : %d : %d",
	     spirit_types[i].type, &spirit_types[i].type_char,
	     &spirit_types[i].size, &spirit_types[i].move_factor,
	     &spirit_types[i].bonus,
	     &spirit_types[i].spell_pts_draft, &spirit_types[i].jewel_draft,
	     &spirit_types[i].jewel_maint, &spirit_types[i].flags);
      ++i;
    }
  }

  fclose(fp);
}

  /* returns true if this nation can draft that type of spirit */
is_valid_spirit_type(np, type)
     Snation *np;
     char type[];
{
  int i;

    /* for now, the all spirit types are fair game */
  for (i = 0; i < user.n_spirit_types; ++i) {
    if (strncmp(spirit_types[i].type, type, NAMELEN) == 0) {
      return 1;			/* got it!!! */
    }
  }
  return 0;			/* we did not find that spirit type */
}
