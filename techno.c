  /* techno.c -- updates technology for a nation in dominion */

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

#include <stdio.h>
#include <math.h>

extern int debug;
extern Suser user;

#define METAL_TECH_POWER (3.0/4.0)

  /* this routine calculates the new tech_skill for a nation */
new_tech_skill(np)
     Snation *np;
{
  int increase;

  increase =
    (int) (((np->race.intel + univ_intel(np)) / 100.0) *
	   (sqrt(1.0 * (calc_revenue(np)*np->tech_r_d/100.0 +
			np->money * np->cur_tech_r_d / 100))
	    * TECH_MONEY_FACTOR +  pow((double)(calc_metal (np) *
             (np->tech_r_d_metal / 100.0) + np->metal * 
              np->cur_tech_r_d_metal / 100),METAL_TECH_POWER) 
               * TECH_METAL_FACTOR));
  return np->tech_skill + increase;
}

  /* this routine sees if the user should get a new
     technology level, by comparing its old and new tech skills
   */
get_new_techno(np, old_skill, new_skill, mailfile)
     Snation *np;
     int old_skill, new_skill;
     FILE *mailfile;
{
  FILE *fp, *fopen();
  int level;
  char name[NAMELEN], line[EXECLEN], execstr[EXECLEN];
  int done = 0;

  if ((fp = fopen(TECHNO_FILE, "r")) == NULL) {
    printf("could not open technology file %s\n", TECHNO_FILE);
    return -1;
  }

  while (!done) {
    if (fgets(line, EXECLEN, fp) == NULL) {
      done = 1;
      break;			/* we are done */
    }
    if (line[0] != '#') {
      sscanf(line, "%s%d", name, &level);
      if (debug) {
	printf("name = <%s>, level = %d\n", name, level);
      }
      if ((level > old_skill) && (level <= new_skill)) {
	if (debug) {
	  printf("deserves a new tech: old_skill=%d, new_skill=%d\n",
		 old_skill, new_skill);
	}
	if (mailfile) fprintf(mailfile,
		"You get technology power <%s>, level %d, which gives you:\n",
		name, level);
	get_tech_entry(fp, np, mailfile);
      } else {
	skip_tech_entry(fp);
      }
    }
  }
  fclose(fp);
}

  /* read in a specific tech entry, delimited by begin
     and end, and add it to the nation's ability.  this routine
     should only be called if the nation *deserves* the new tech
     power.
   */
get_tech_entry(fp, np, mailfile)
     FILE *fp;
     Snation *np;
     FILE *mailfile;
{
  char line[EXECLEN], *line2;
  struct argument exec_args[N_EXEC_ARGS];

  do {
    fgets(line, EXECLEN, fp);
  } while ((strlen(line) == 0) && (line[0] == '#')); /* skip blank lines */
  if (strncmp(line, "begin", strlen("begin")) != 0) {
    printf("syntax error:  did not find a begin\n");
  } else {
    if (debug) {
      printf("got a begin\n");
    }
  }
  for (;;) {
    fgets(line, EXECLEN, fp);
    if (line[strlen(line)-1] == '\n') {
      line[strlen(line)-1] = '\0';
    }
    line2 = line;
    while (*line2 == ' ') {	/* skip spaces at start */
      ++line2;
    }				/* now line2 is ready for exec parsing */
    if (strncmp(line2, "end", strlen("end")) == 0) {
      if (debug) {
	printf("got an end\n");
      }
      break;
    }
      /* if that was not the end, we can parse the exec command */
    if (debug) {
      printf("line = <%s>, line2 = <%s>, ABOUT TO PARSE\n", line, line2);
    }
    if (mailfile) fprintf(mailfile, "%s\n", line);
    if (line2[0] != '#') {
      parse_exec_line(line2, exec_args);
      run_exec_line(np, exec_args);
    }
  }
}

  /* wait for an "end" in the techno file */
skip_tech_entry(fp)
     FILE *fp;
{
  char line[EXECLEN];
  do {
    fgets(line, EXECLEN, fp);
  } while (strncmp(line, "end", strlen("end")) != 0);
}

  /* this is the routine actually called by the update program */
dotechno(np, mailfile)
     Snation *np;
     FILE *mailfile;
{
  int old_skill, new_skill;

  old_skill = np->tech_skill;
  new_skill = new_tech_skill(np);

  if (debug) {
    printf("nation %s has old_skill = %d, new_skill = %d\n",
	   np->name, old_skill, new_skill);
  }
  if (mailfile) fprintf(mailfile,
	"Your skill in technology has increased from %d to %d\n",
	old_skill, new_skill);
  get_new_techno(np, old_skill, new_skill, mailfile);
  np->tech_skill = new_skill;	/* save the change */

/*  np->tech_skill = 0; */
}
