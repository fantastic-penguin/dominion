  /* spelllib.c -- handls loading spells for a nation, and keeping
                   them "hanging" for a certain number of turns.
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

#include "dominion.h"
#include "misc.h"
#include "army.h"
#include "functions.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

  /* army and spirit types */
extern struct army_type *army_types;
extern struct spirit_type *spirit_types;

extern Sworld world;
extern int debug;
extern Sh_spell *hanging_spells;

extern Sh_spell *dead_spells;
extern Suser user;


  /* this writes out the hanging spells after the update, decreasing
     the thons_left parameter by one, and removing the spell
     once the thons_left reaches zero.
   */
void write_h_spells()
{
  FILE *hang_fp, *fopen();
  char line[EXECLEN];
  int i;
  Sh_spell *h_spells;

  if ((hang_fp = fopen(HANGING_SPELLS_FILE, "w")) == NULL) {
    printf("cannot write file %s\n", HANGING_SPELLS_FILE);
    return;
  }
  for (h_spells=hanging_spells; h_spells != NULL; h_spells = h_spells->next) {
      /* only write them out if they have not expired */
    if (h_spells->thons_left != -1) {
      fprintf(hang_fp, "%s\n", h_spells->name);
      fprintf(hang_fp, "%d\n", h_spells->nat_id);
      fprintf(hang_fp, "%d\n", h_spells->thons_left);
      fprintf(hang_fp, "%d\n", h_spells->n_lines);
      for (i = 0; i < h_spells->n_lines; ++i) { /* write out the exec lines */
	fprintf(hang_fp, "%s", h_spells->lines[i]);
	  /* we might need to tack on a newline */
	if (h_spells->lines[i][strlen(h_spells->lines[i])-1] != '\n') {
	  fprintf(hang_fp, "\n");
	}
      }
    }
  }
  fclose(hang_fp);
}

  /* we need list as a pointer to pointer, so as to add at
     the start of the list (which is easy and fast)
   */
void add_h_spell(listp, h_spellp)
     Sh_spell **listp, *h_spellp;
{
  Sh_spell *tmp;

  if (listp) {			/* list should not be null in this func. */
    tmp = (Sh_spell *)malloc(sizeof(Sh_spell));
    *tmp = *h_spellp;
    tmp->next = *listp;
    *listp = tmp;
  }
}

  /* removes a hanging spell from the list */
void delete_h_spell(listp, h_spellp)
     Sh_spell **listp, *h_spellp;
{
}

  /* prepares a new hanging spell structure, to be
     filled in with its exec lines
   */
void prepare_h_spell(h_spellp, name, nat_id, thons_left, n_lines)
     Sh_spell *h_spellp;
     char name[];
     int nat_id, thons_left, n_lines;
{
  int i;

  strcpy(h_spellp->name, name);
  h_spellp->next = NULL;
  h_spellp->nat_id = nat_id;
  h_spellp->thons_left = thons_left;
  h_spellp->n_lines = n_lines;
  h_spellp->lines = (char **)malloc(h_spellp->n_lines*sizeof(char*));
  for (i = 0; i < h_spellp->n_lines; ++i) {
    h_spellp->lines[i] = (char *)malloc(EXECLEN*sizeof(char));
    strcpy(h_spellp->lines[i], "");
  }
}

/* compares 2 hanging spells and returns 0 if same 1 if not */
int h_spell_compare(sp1, sp2)
Sh_spell *sp1, *sp2;
{
  int i;

  if (sp1->nat_id != sp2->nat_id) return 1;
  if (sp1->thons_left != sp2->thons_left) return 1;
  if (sp1->n_lines != sp2->n_lines) return 1;
  if (strncmp(sp1->name, sp2->name,NAMELEN) != 0) return 1;
  for (i = 0 ; i < sp1->n_lines; i++)
  {  
    if (strcmp(sp1->lines[i], sp2->lines[i]) != 0) return 1;
  }
  return 0;
};

int is_dead_spell(sp,flag)
int flag;
Sh_spell *sp;
{
  Sh_spell *tmp;

  tmp = dead_spells;
  while (tmp != NULL)
  {
    if (h_spell_compare(tmp,sp) == 0)
    { 
      got_dead_h_spell(tmp);
      return 1;
    }
    tmp = tmp->next;
  }
  return 0;
}

void load_dead_hspells(up,flag)
int flag;
     Suser *up;			/* up can also be NULL */
{
  FILE *dead_fp, *fopen();
  int done = 0, i;
  char line[EXECLEN], filename[PATHLEN];
  Sh_spell h_spell;

  sprintf(filename,"exec/%s.%d","dead_spells",up->id);
  if ((dead_fp = fopen(filename, "r")) == NULL) {
    if (debug) printf("no file %s\n", filename);
    return;
  }
  while (!done) {
    h_spell.next = NULL;
    if (fscanf(dead_fp, "%s", h_spell.name) < 0) {
      done = 1;			/* useless? */
      break;
    }
    fscanf(dead_fp, "%d", &h_spell.nat_id);
    fscanf(dead_fp, "%d", &h_spell.thons_left);
    fscanf(dead_fp, "%d", &h_spell.n_lines);
    if (debug) {
      printf("nation id %d, spell <%s>, time_left = %d, n_lines = %d\n",
	   h_spell.nat_id, h_spell.name, h_spell.thons_left, h_spell.n_lines);
    }
    h_spell.lines = (char **)malloc(h_spell.n_lines * sizeof(char *));

    for (i = 0; i < h_spell.n_lines; ++i) { /* read in the exec lines */
      h_spell.lines[i] = (char *)malloc(EXECLEN*sizeof(char));
      fscanf(dead_fp, "%s", h_spell.lines[i]);
    }
    add_h_spell(&dead_spells, &h_spell);
  }
  fclose(dead_fp);
  if (flag == 1) { unlink(filename); }
}

  /* this is fundamental:  it loads all spells from the global
     "hanging_spells" file and runs the exec lines it finds.
     Note:  it must load all the spells, not just those for the
     current user, because spells by other users affect the current
     user too.  One effect is that spells, like diplomacy, take
     immediate effect, rather than waiting for the update.  Hmm,
     is that good or bad?

     At the same time, the spells of the current user (if we are in
     the game rather than the update) are stored into a list for that
     user to examine, apart from the global list.
   */

void load_h_spells(up)
     Suser *up;			/* up can also be NULL */
{
  FILE *hang_fp, *fopen();
  int done = 0, i;
  char line[EXECLEN];
  Sh_spell h_spell;
  struct argument exec_args[N_EXEC_ARGS];
  Snation *np;

    /* start with the global list being NULL */
  hanging_spells = NULL;
  if (up) {
    up->h_spells = NULL;
  }
  if ((hang_fp = fopen(HANGING_SPELLS_FILE, "r")) == NULL) {
    if (debug) printf("no file %s\n", HANGING_SPELLS_FILE);
    return;
  }
  while (!done) {
    h_spell.next = NULL;
    if (fscanf(hang_fp, "%s", h_spell.name) < 0) {
      done = 1;			/* useless? */
      break;
    }
    fscanf(hang_fp, "%d", &h_spell.nat_id);
    fscanf(hang_fp, "%d", &h_spell.thons_left);
    fscanf(hang_fp, "%d", &h_spell.n_lines);
    if (debug) {
      printf("nation id %d, spell <%s>, time_left = %d, n_lines = %d\n",
	   h_spell.nat_id, h_spell.name, h_spell.thons_left, h_spell.n_lines);
    }
    h_spell.lines = (char **)malloc(h_spell.n_lines * sizeof(char *));

    for (i = 0; i < h_spell.n_lines; ++i) { /* read in the exec lines */
      h_spell.lines[i] = (char *)malloc(EXECLEN*sizeof(char));
      fscanf(hang_fp, "%s", h_spell.lines[i]);
    }
    if (!is_dead_spell(&h_spell,1))
    {
      for (i = 0 ; i < h_spell.n_lines; ++i)
      {
        if (i % 2 == 0) {
	  parse_exec_line(h_spell.lines[i], exec_args);
          np = &world.nations[h_spell.nat_id];
          run_exec_line(np, exec_args);
        }
      }
      add_h_spell(&hanging_spells, &h_spell);
      if (up && (up->id == h_spell.nat_id || up->id == 0)) {
	add_h_spell(&up->h_spells, &h_spell);
      }
    }
  }
  fclose(hang_fp);
}

void delete_hanging_spell(sp1)
Sh_spell *sp1;
{
  Sh_spell *prev, *next, *tmp, *tmp2;
  struct argument exec_args[N_EXEC_ARGS];
  int i;

  tmp = user.h_spells; tmp2 = NULL; prev = NULL;
  while ((tmp != NULL) && (tmp2 == NULL))
  {
    if (h_spell_compare(sp1, tmp) == 0)
    {
      tmp2 = tmp;
    } else
    {
      prev = tmp;
      tmp = tmp->next;
    }
  }
  if (prev == NULL)
  { 
    user.h_spells = tmp2->next;
  } else
  {
    prev->next = tmp2->next;
  }
  for (i = 0 ; i < tmp2->n_lines; i++)
  {
    if (i % 2 == 1)
    {
      parse_exec_line(tmp2->lines[i], exec_args);
      run_exec_line(user.np, exec_args);
    }
  }
  write_dead_spell(tmp2);
  if (is_army_spell(tmp2)) reset_spelled_flags();
  free_h_spell(tmp2);
}

void got_dead_h_spell(sp1)
Sh_spell *sp1;
{
  Sh_spell *prev, *tmp, *tmp2;

  tmp = dead_spells; tmp2 = NULL; prev = NULL;
  while ((tmp != NULL) && (tmp2 == NULL))
  {
    if (sp1 == tmp)
    {
      tmp2 = tmp;
    } else
    {
      prev = tmp;
      tmp = tmp->next;
    }
  }
  if (tmp2 != NULL)
  {
    if (prev == NULL)
    { 
      dead_spells = tmp2->next;
    } else
    {
      prev->next = tmp2->next;
    }
    free_h_spell(tmp2);
  }
}

void free_h_spell(sp1)
Sh_spell *sp1;
{
  int i;  

  for (i = 0 ; i < sp1->n_lines ; i++)
  {
    free(sp1->lines[i]);
  }
  free(sp1);
}

void reset_spelled_flags()
{
  Sh_spell *tmp;
  
  tmp = user.h_spells;

  while (tmp != NULL)
  {
    reset_one_spell(tmp);
    tmp = tmp -> next;
  }
}

void reset_one_spell(sp1)
Sh_spell *sp1;
{
  struct argument exec_args[N_EXEC_ARGS];
 
  if (strcmp(sp1->name, "hide_army") == 0)
  {
    if (sp1->n_lines < 4) return;
    parse_exec_line(sp1->lines[2], exec_args);
    run_exec_line(user.np, exec_args);
  }
  else if (strcmp(sp1->name, "fly_army") == 0)
  {
    if (sp1->n_lines < 4) return;
    parse_exec_line(sp1->lines[2], exec_args);
    run_exec_line(user.np, exec_args);
  }
  else if (strcmp(sp1->name, "vampire_army")== 0 )
  {
    if (sp1->n_lines < 4) return;
    parse_exec_line(sp1->lines[2], exec_args);
    run_exec_line(user.np, exec_args);
  }
  else if (strcmp(sp1->name, "burrow_army")== 0 )
  {
    if (sp1->n_lines < 4) return;
    parse_exec_line(sp1->lines[2], exec_args);
    run_exec_line(user.np, exec_args);
  }
  else if (strcmp(sp1->name, "water_walk") == 0)
  {
    if (sp1->n_lines < 4) return;
    parse_exec_line(sp1->lines[2], exec_args);
    run_exec_line(user.np, exec_args);
  }
  else if (strcmp(sp1->name, "mag_bonus") == 0)
  {
    if (sp1->n_lines < 4) return;
    parse_exec_line(sp1->lines[2], exec_args);
    run_exec_line(user.np, exec_args);
  }
}

int is_army_spell(sp1)
Sh_spell *sp1;
{
  struct argument exec_args[N_EXEC_ARGS];
 
  if (strcmp(sp1->name, "hide_army") == 0)
  {
    return 1;
  }
  else if (strcmp(sp1->name, "fly_army") == 0)
  {
    return 1;
  }
  else if (strcmp(sp1->name, "vampire_army")== 0 )
  {
    return 1;
  }
  else if (strcmp(sp1->name, "burrow_army")== 0 )
  {
    return 1;
  } 
  else if (strcmp(sp1->name, "water_walk") == 0)
  {
    return 1;
  }
  else if (strcmp(sp1->name, "mag_bonus") == 0)
  {
    return 1;
  }
  return 0;
}

void write_dead_spell(sp1)
Sh_spell *sp1;
{
  FILE *dead_fp, *fopen();
  char line[EXECLEN], filename[PATHLEN];
  int i;

  sprintf(filename,"exec/%s.%d","dead_spells",user.id);
  if ((dead_fp = fopen(filename, "a")) == NULL) {
    if (debug) printf("no file %s\n", filename);
    return;
  }
  fprintf(dead_fp, "%s\n", sp1->name);
  fprintf(dead_fp, "%d\n", sp1->nat_id);
  fprintf(dead_fp, "%d\n", sp1->thons_left);
  fprintf(dead_fp, "%d\n", sp1->n_lines);
  for (i = 0; i < sp1->n_lines; ++i) { /* write out the exec lines */
    fprintf(dead_fp, "%s", sp1->lines[i]);
     /* we might need to tack on a newline */
    if (sp1->lines[i][strlen(sp1->lines[i])-1] != '\n') {
    fprintf(dead_fp, "\n");
    }
  }
  fclose(dead_fp);
}

void clear_dead_hspells()
{
  Sh_spell *h_spells;
  int i,end;
  struct argument exec_args[N_EXEC_ARGS];

  for (h_spells=hanging_spells; h_spells != NULL; h_spells = h_spells->next)
  {
    for (i = 0; i < h_spells->n_lines; ++i) { /* check the exec lines */
/* Remove that goddman newline */
      end = strlen(h_spells->lines[i]);
      if (h_spells->lines[i][end-1] == '\n') {
        h_spells->lines[i][end - 1] = '\0';
      }
    }
    if (is_dead_spell(h_spells,1))
    {
      h_spells->thons_left = 0;
    }
  }
}

/* This routine checks to see if this army has a spell cast on it by  */
/* checking to see if the flags are different from the default.       */
/* If new spells are developed that change things besides flags, then */
/* other mechanism's will need to be used.                            */
int is_spelled(ap)
Sarmy *ap;
{
  int i;
  int def_flags;
  Snation *np = user.np;

  i = army_type_index(ap->type);
  if (i != -1 ) {
    def_flags = army_types[i].flags;
    if (def_flags & AF_INVERSE_ALT) {
      if (np->race.pref_alt >= SEA_LEVEL) {
        def_flags |= AF_WATER;
      } else {
        def_flags |= AF_LAND;
      }
    }
    if (def_flags != ap->flags) {
      return 1;
    } else {
      return 0;
    }
  } else {
    if ((i = spirit_type_index(ap->type)) < 0) {
      return 0;  /* We can't figure out what it is, so it's not spelled */
    }
    if (spirit_types[i].flags != ap->flags) {
      return 1;
    } else {
      return 0;
    }
  }
}
