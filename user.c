/* user.c -- stuff relating to the "current user" */

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

#include <stdio.h>
#ifdef SYSV
# include <string.h>
#else
# include <strings.h>
#endif /* SYSV */
#include <time.h>

#include "dominion.h"
#include "misc.h"
#include "functions.h"

extern Suser user;
extern Sworld world;
extern int debug;
extern int (*wrapx)(), (*wrapy)();
extern int viewall;
extern double get_version(),atof();
extern char *update_time, *get_update_time(), *mail_forwarding(),*civ_move[];

void usageerr(argc, argv)
int argc;
char *argv[]; 
{
  fprintf(stderr, "usage: %s -[n nation] [-d dir] [-x] [-h] [-p] [-c]\n", argv[0]);
}

  /* initializes the user's data structure, and does
     other tasks concerned with loading up the game
   */
void init_user(innation, nation)
     int innation;		/* do we have a nation name already? */
     char nation[];		/* pre-entered nation name */
{
  char passwd[NAMELEN];
  char *s, *getpass(), *crypt();
  int c;
  char syscmd[NAMELEN];
  int i;
  Sdiplo **allocate_diplo();

  printf("initializing user...\r\n");
  load_army_types();
  load_spirit_types();
    /* in case the master changed your password */
  load_master_execs();
  if (!innation)
    {
      printf("which nation would you like to play? ");
      dom_getline(nation, NAMELEN);
    }
  if ((user.id = get_nation_id(nation)) == -1) {
    printf ("\r\nnation does not exist, sorry\r\n");
    clean_exit ();
    exit (1);
  }
  get_crypt_pass("Your nation's password: ", passwd, NULL, NULL);
  if (strcmp(world.nations[user.id].passwd, passwd)) {
    printf("\r\nTry again\r\n");
    get_crypt_pass("Your nation's password: ", passwd, NULL, NULL);
    user.id = get_nation_id(nation);
    if (strcmp(world.nations[user.id].passwd, passwd)) {
      printf("\r\nwrong password, sorry\r\n");
      clean_exit();
      exit(1);
    }
  }

  handle_locks(user.id);

  user.np = &world.nations[user.id];

    /* now check to see if this nation has been destroyed */
  if (user.np->capital.x == -1 && user.np->capital.y == -1) {
    if (user.np->id == 0) {	/* nation 0 cannot be destroyed!!! */
      user.np->capital.x = 0;
      user.np->capital.y = 0;
    } else {
      printf("\r\nYour nation has been destroyed.\n");
      printf("Ask your Gamemaster for your last mail.\n");
      clean_exit();
      exit(0);
    }
  }
    /* find out which army types are available to the user */
  user.avail_armies = NULL;
  get_avail_armies(&user, user.np->tech_skill);
    /* start the user off with all spells s/he deserves.
       note that, because of spirits, this has to be done
       before load_nation(), since the spirit list is used
       in the exec parsing.
     */
  user.spell_list = NULL;
  user.spirit_list = NULL;
  get_spells(&user, user.np->mag_skill);
  get_spirits(&user, user.np->mag_skill);
    /* fundamental step:  load exec file */
  if (user.id != 0) {		/* gamemaster is already loaded */
    load_nation(user.id, user.np);
  } else {
    load_options(user.np);
  }
    /* now set fields for the ustruct */
  user.cursor = user.center = user.np->capital;
  user.help_char = '?';
  user.map_style = NORMAL_MAP;
  user.display = DESIGNATION;
  if (user.id != 0) {
    user.highlight = H_OWNED;
  } else {			/* for gamemaster, don't highlight */
    user.highlight = H_NONE;
  }
  user.underwater = 0;		/* user is not underwater at start */
  if (user.np->race.pref_alt < 0) {
    user.underwater = 1;	/* merfolk or whatever */
  }
  user.n_execs = 0;
  user.current_army =
    first_sect_army(&world.map[user.cursor.x][user.cursor.y]);
  user.just_moved = 1;
  user.last_n_armies = 0;
    /* super user visibility */
  if (user.id == 0) {
    viewall = 1;
  }
  user.show_sect_win = 1;
    /* load user's diplomacy statuses, for fast access later;
       also remember the initial values, so users cannot change
       their status by more than one step at a time.
     */
  user.diplo_matrix = allocate_diplo(world.n_nations);
  read_in_diplo(user.diplo_matrix, world.n_nations);
/* Load removed spells first */
  load_dead_hspells(&user,0);

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
}

  /* for a fixed army, update its visibility range */
void army_visibility(visible_sectors, ap)
     int **visible_sectors;
     Sarmy *ap;
{
  int x = ap->pos.x, y = ap->pos.y, i, j;
  Ssector *sp;

  sp = &world.map[x][y];
  if (has_hidden(sp) && sp->owner != user.id) {
    visible_sectors[x][y] = SEE_ARMIES;
  } else if (sp->owner != user.id) {
    visible_sectors[x][y] = SEE_ARMIES;
  } else {
    visible_sectors[x][y] = SEE_ALL;
  }
  for (i = x-ARMY_SIGHT; i <= x+ARMY_SIGHT; ++i) {
    for (j = y-ARMY_SIGHT; j <= y+ARMY_SIGHT; ++j) {
      sp = &world.map[(*wrapx)(i,j)][(*wrapy)(i,j)];
      if (has_hidden(sp) && sp->owner != user.id) {
	/* if sect. is hidden, we don't see it */
      } else {
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

  /* this allows a user to set her/his options */
void options()
{
  WINDOW *optw;
  char c;
  int done = 0;
  char *forwarding = mail_forwarding(user.id);

  optw = newwin(8, 50, 5, 10);
  while (!done) {
    mvwprintw(optw, 1, 2, "[x]: toggle expert mode (%s)",
	      user.np->opts->expert_mode ? "on" : "off");
    wclrtoeol(optw);

    if (user.np->opts->mail_forward) {
      mvwprintw(optw, 2, 2, "[f]: Change mail forwarding (\"%s\")",
                   user.np->opts->mail_forward);
    } else {
      mvwprintw(optw, 2, 2, "[f]: Change mail forwarding (none)");
    }
    wclrtoeol(optw);
    mvwprintw(optw, 3, 2, "[c]: Toggle civilian movement (%s)",
                     civ_move[user.np->opts->civ_movemode]);
    wclrtoeol(optw);
    if (user.np->opts->mail_reader) {
      mvwprintw(optw, 4, 2, "[m]: Mail Program (%s)",
                                   user.np->opts->mail_reader);
    } else {
      mvwprintw(optw, 4, 2, "[m]: Mail Program (internal)");
    }
    wclrtobot(optw);
    box(optw, '|', '-');
    wrefresh(optw);
    statline("Choose an option, hit space to get back.", "options");
    switch (c = getch()) {
    case 'x':
      user.np->opts->expert_mode = !user.np->opts->expert_mode;
      user.xmode = user.np->opts->expert_mode;
      break;
    case 'f':
      ask_for_forwarding(optw);
      break;
    case 'c':
        /* cycle through the various migration modes */
      user.np->opts->civ_movemode = (user.np->opts->civ_movemode + 1) % 3;
      break;
    case 'm':
      ask_for_mail_reader(optw);
      break;
    case ' ':
      done = 1;
      break;
    default:
      break;
    }
  }
  delwin(optw);
  user.just_moved = 1;
  save_options(user.np);

  touch_all_wins();
}

static void ask_for_forwarding(win)
     WINDOW *win;
{
  char buf[200];

  mvwprintw(win, 5, 2, "New mail address? (<return> for no forwarding)");
  mvwprintw(win, 6, 4, "-->");
  wrefresh(win);

  if (user.np->opts->mail_forward != NULL) {
    free(user.np->opts->mail_forward);
    user.np->opts->mail_forward = NULL;
  }
  if (wget_string(win, buf,200) != 0) {
    if ((user.np->opts->mail_forward = (char *)malloc ((strlen(buf) + 1) * 
          sizeof (char))) == NULL) { mem_error(); }
    strcpy(user.np->opts->mail_forward, buf);
  } 
}

void ask_for_mail_reader(win)
     WINDOW *win;
{
  char buf[200];

  if (user.np->opts->mail_reader) { free(user.np->opts->mail_reader); }

  mvwprintw(win, 5, 2, "New mail reader? (<return> for internal mail)");
  mvwprintw(win, 6, 4, "-->");
  wrefresh(win);

  if (wget_string(win, buf, 200))  {
    if ((user.np->opts->mail_reader = (char *)malloc ((strlen(buf) + 1) * 
          sizeof (char))) == NULL) { mem_error(); }
    strcpy(user.np->opts->mail_reader, buf);
  } else {
    user.np->opts->mail_reader = NULL;
  }
}

  /* check if there is a lock file, and handle the situation if there is */
void handle_locks(id)
     int id;
{
  FILE *lock_fp, *is_locked();
  char *timestr;
  long secs;			/* seconds marked in lock file */

    /* check if there is a master lock file */
  if (is_master_lock()) {
    printf("There is a master lock file.  You cannot play right now.\n");
    clean_exit();
    exit(1);
  }
  if (strcmp(update_time,get_update_time()) != 0)
  {
    fprintf(stderr,"Error: Update has occured.  Please restart program \n");
    clean_exit();
    exit(0);
  }

    /* see if there is a lock file for this nation */
  if (lock_fp = is_locked(id)) {
    fscanf(lock_fp, "%ld", &secs);
    fclose(lock_fp);
    timestr = ctime(&secs);
    printf("\r\nYour nation is already being played.\n\r");
    printf("That session seems to have been started at %s", timestr);
    printf("If that session is no more and you want to play now, type [y] ");
    if (getchar() != 'y') {
      printf("OK, then you will have to wait until that session is over\n");
      clean_exit();
      exit(1);
    }
  }
    /* set a lock file */
  set_lock(id);
}

