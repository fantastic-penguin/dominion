  /* dominion.c -- main loop of dominion */

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
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>   // getuid
#include <sys/stat.h> // umask

  /* most of these data structures are declared in ext.c */
extern Suser user;
extern Sworld world;
extern struct race_list *races;
extern char libdir[];
extern int euid, ruid;
extern char help_tag[];
extern int interrupt();
extern int (*keymap[128])();
extern char *get_update_time(), *update_time;

// forward declarations
void init_screen();
void online_info();
void read_races();
void read_world(Sworld* wp, char fname[]);
void print_nations();
void usageerr(int argc, char *argv[]);
void clean_exit();
void init();
void init_user(int innation, char nation[]);
void init_keymap();
void init_screen();
void intro(Sworld *wp, Snation *np);
void noncritical();
void main_loop();
void cleanup();
void clean_exit();

int old_umask;			/* to reset when we leave */
#ifdef ANDREW
int beroot = 0;
#endif

int main(argc, argv)
int argc;
char *argv[];
{
  int c;
  int innation = 0;
  char nation[NAMELEN];
  extern char *optarg;
  extern int optind;
#ifdef BSD
    /* BSD uses getwd(), which does not allocate static space */
  extern char current_dir[], *getwd();
#else /* BSD */
  extern char *current_dir, *getcwd();
#endif /* BSD */
    /* starting help tag is Top */
  strcpy(help_tag, "Top");

    /* now get some unix system information */
  ruid = getuid();
  euid = geteuid();
  old_umask = umask(077);
/*  printf("ruid = %d, euid = %d\n", ruid, euid); */
#ifdef BSD
  getwd(current_dir);
#else /* BSD */
  current_dir = getcwd(NULL, PATHLEN);
#endif /* BSD */
/*  printf("current dir is %s\n", current_dir); */
  strcpy(libdir, DEF_LIBDIR);

  user.xmode = 0;
#ifndef ANDREW
  while ((c = getopt(argc, argv, "n:d:vphc--")) != EOF)
#else
  while ((c = getopt(argc, argv, "n:d:vuphc--")) != EOF)
#endif
    switch (c) {
#ifdef ANDREW
     case 'u':
       beroot = 0;
       break;
#endif
    case 'n':
      innation = 1;
      strcpy(nation, optarg);
      break;
    case 'v':
      /*      viewall = 1; */
      break;
    case 'd':
      strcpy(libdir, optarg);
      break;
    case 'h':
      chdir(libdir);
      init_screen();
      online_info();
      resetty();
      endwin();
      exit(1);
    case 'p':
      chdir(libdir);
      read_races();
      read_world(&world, WORLD_FILE);
      print_nations();
      exit(1);
   case 'c':
      chdir(libdir);
      printf("Last update at %s\n",get_update_time());
      exit(0);
    default:
      usageerr(argc,argv);
      exit(1);
   }
#ifdef ANDREW
  if (beroot == 1) {
    Authenticate();
    beGames();
  }
#endif
  if (chdir(libdir) != 0) {
    fprintf(stderr,"Error: cannot cd to directory %s\n",libdir);
    clean_exit();
    exit(1);
  }
  update_time = get_update_time();
/*  printf("libdir=%s,def_libdir=%s\n", libdir, DEF_LIBDIR); */

  init();			/* set up data structures */
  init_user(innation, nation);
    /* init keymap after user is loaded, because of master commands */
  init_keymap();
  init_screen();
  intro(&world, user.np);
  noncritical();		/* normal signal operation */
  main_loop();
  cleanup();
  clean_exit();
  exit(0);
}

init()
{
  printf("Initializing...\r\n");
  SRND(time(0L));		/* initialize random number generator */
  read_races();			/* get races from races file */
    /* read in the world */
  read_world(&world, WORLD_FILE);
}

	/* this is where all the work is done  */
main_loop()
{
  char c;
  char s[80];

  for ( ; ; ) {
    sprintf(s, "Nation %s; money %d; Thon %d;   type %c for help",
	    user.np->name, user.np->money, world.turn, user.help_char);
    statline(s, "draw_map_regular");
    if (user.just_moved) {
      draw_map();
      user.just_moved = 0;
    }
    set_cursor();
    switch(c = getch()) {
    default:
      (*keymap[c % 128])();	/* make sure no high bits get there */
      (void) re_center(user.cursor.x, user.cursor.y); /* ignore return val */
      if (user.just_moved) {
	wrap(&user.cursor);
	if (!army_is_in_sector(&world.map[user.cursor.x][user.cursor.y],
			       user.id, user.current_army)) {
	  user.current_army
	    = first_sect_army(&world.map[user.cursor.x][user.cursor.y]);
	}
      }
      break;
    }
  }
}

  /* clean up graphics, and flush out the exec file */
cleanup()
{
  int i;

  statline("", "cleanup");
/*  delay_output(200); */
  resetty();
  endwin();
  putchar('\n');
  putchar('\n');
  gen_exec(NULL);
}

clean_exit()
{
  extern int is_in_diplo;

  if (is_in_diplo) {
    unlink("DIPLOCK");
  }
  del_lock(user.id);
  umask(old_umask);
#ifdef ANDREW
  if (beroot == 1) {
    bePlayer();
    unAuth();
    beroot = 0;
  }
#endif
}

critical()			/* while writing data files, don't bug me!! */
{
  signal(SIGINT, SIG_IGN);
  signal(SIGQUIT, SIG_IGN);
#ifdef SIGTSTP
  signal(SIGTSTP, SIG_IGN);
#endif /* SIGTSTP */
  signal(SIGHUP, SIG_IGN);
}

noncritical()			/* normal operation */
{
  signal(SIGINT, interrupt);
  signal(SIGQUIT, interrupt);
#ifdef SIGTSTP
  signal(SIGTSTP, interrupt);
#endif /* SIGTSTP */
  signal(SIGHUP, interrupt);
}

  /* just prints a list of all nations to the terminal */
print_nations()
{
  int i;
  Snation *np;

  printf("%s %-14s[%s]    %-15s %-10s\n", "Id", "Name",
	 "mark", "Leader", "Race");
  printf("----------------------------------------------------------------------\n");
  for (i = 1; i < world.n_nations; ++i) {
    np = &world.nations[i];
    printf("%2d %-15s [%c]     %-15s %-10s %-10s %-5s\n", i, np->name,np->mark,
	   np->leader, np->race.name, is_active_ntn(np) ? "" : "DESTROYED",
	   np->npc_flag ? "npc" : "");
  }
}

