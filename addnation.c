  /* addnation.c -- primitive program to add a nation to the world */

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
#include <ctype.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>

#include "dominion.h"
#include "misc.h"
#include "functions.h"
#include "army.h"

#define INIT_CIV 10000
#define RADIUS 8		/* minimum distance from other nations */
#define MAX_TRIES 10000		/* max tries to find a good capital */

extern Sworld world;
extern Suser user;
extern struct race_list *races;	/* list of races */
extern struct army_type *army_types; /* array of available armies */
extern char libdir[];
extern int debug;
extern int (*wrapx)(), (*wrapy)();

char get_nation_mark ();
int sigquit();

Sdiplo **allocate_diplo();

int main(argc, argv)
     int argc;
     char *argv[];
{
  int npcs_get_mail = 0;
  extern int interrupt();
  int n;
  int c;
  extern char *optarg;
  extern int optind;
  char add_fn[255];		/* get NPCs from this file name */

  stdscr = NULL;		/* to know if curses is initialized */
  add_fn[0] = '\0';
  strcpy(libdir, DEF_LIBDIR);
  while ((c = getopt(argc, argv, "md:f:x")) != EOF) {
    switch (c) {
    case 'm':			/* make these NPCs get mail */
      npcs_get_mail = 1;
      break;
    case 'x':			/* set debugging mode */
      debug = 1;
      break;
    case 'd':			/* set the LIBDIR path */
      strcpy(libdir, optarg);
      break;
    case 'f':			/* the name of the file with NPCs */
      strcpy(add_fn, optarg);
      break;
    default:
      break;
    }
  }
  if (chdir(libdir) == -1) {
    fprintf(stderr,"Error: cannot cd to directory %s\n",libdir);
    clean_exit();
    exit(1);
  }

  SRND(time((long *) 0L));	/* initialize random number generator */

  read_world(&world, WORLD_FILE);
  read_races();

  if (!verify_gamemaster()) {
    printf("Hey, you can't add a nation!\n");
    clean_exit();
    exit(1);
  }

  if (is_master_lock()) {
    printf("\nThere is a master lock\n");
    clean_exit();
    exit(1);
  }
  set_master_lock();
  if (is_any_lock()) {
    printf("There is a lock;you should make sure the nation is not playing\n");
    del_master_lock();
    clean_exit();
    exit(1);
  }
  /* initialize curses if we are adding a
     nation interactively (i.e. not NPCs)
   */
  if (strlen(add_fn) == 0) {
    initscr();			/* Initialize curses */
    cbreak();			/* set cbreak mode */
    noecho();			/* unset echo mode */
  }
  noncritical();

  if ((n = find_free_nation(&world)) == -1) {
    printf("No free nations, sorry.\n");
    clean_exit();
    exit(1);
  }

  if (debug) {
    printf("First free nation id is %d.\n", n);
  }

  fflush(stdin);

  if (stdscr) {
    refresh();
  }

  /* check if we are adding NPCs from file add_fn[] */
  if (strlen(add_fn) != 0) {
      /* MCG Kludge Time */
    char full_path_fn[255];
    strcpy(full_path_fn, "misc/");
    strcat(full_path_fn, add_fn);
    printf("\nAdding NPC nations\n\n");
    add_npcs(full_path_fn, npcs_get_mail);
  } else {			/* else we add a single player */
    add_player(n);
  }

  clean_exit();
  return 0;
}

  /* inserts a player into the world with id = n */
void add_player(n)
     int n;
{
  Snation tmp_nation;			/* as we form the nation */
  char racemark;
  char mag_ord[NAMELEN], pass_try1[NAMELEN], pass_try2[NAMELEN];

  clear();

  tmp_nation.id = n;

  echo();
  mvaddstr(0, 0, "Nation's name: ");
  refresh();
/*  getstr(tmp_nation.name); */
  wget_string(stdscr,tmp_nation.name,NAMELEN);

  while (!unique_name(tmp_nation.name)) {  
    clear();
    mvprintw(0, 0, "Sorry, \"%s\" is already taken.", tmp_nation.name);
    mvaddstr(2, 0, "Enter nation's name: ");
    refresh();
/*    getstr(tmp_nation.name); */
    wget_string(stdscr,tmp_nation.name,NAMELEN);
  }
  noecho();

  get_crypt_pass("\nEnter the password for your nation: ", pass_try1, stdscr,
		 NULL);
  get_crypt_pass("\nType it once more: ", pass_try2, stdscr, NULL);


  /* make sure user knows their pass. */
  while  (strcmp(pass_try1, pass_try2) != 0){
    clear();
    printw("They don't match, try again\n\r");
    get_crypt_pass("Give nation's password: ", pass_try1, stdscr, NULL);
    get_crypt_pass("\nPlease type it once more: ", pass_try2, stdscr, NULL);
  }
  strcpy(tmp_nation.passwd, pass_try1);
  echo();

  printw("\n\rGive your leader's name: ");
  refresh();
/*   getstr(tmp_nation.leader); */
  wget_string(stdscr,tmp_nation.leader,NAMELEN);
  racemark = get_race_mark();

  tmp_nation.mark = get_nation_mark();

  choose_mag_order(mag_ord);
    /* each magic order brings some advantages.  insert them now */
  add_special_mag(&tmp_nation, mag_ord);

  clear();
  refresh();

    /* Set up a new, non-NPC nation */
  if (setup_new_nation(tmp_nation.name, tmp_nation.passwd, tmp_nation.leader,
	racemark, tmp_nation.mark, mag_ord, NOT_NPC, 50, 5, 5) < 0) {
    printw("\n\r  Could not add nation %s\n\r", tmp_nation.name);
    refresh();
    noecho();
  } else {
    move(0, LINES-1);
    refresh();
    endwin();
    stdscr = NULL;
    write_world(&world, WORLD_FILE);
  }
}

  /* make sure that the guy knows the game master password */
int verify_gamemaster()
{
  char s[NAMELEN], *getpass(), *crypt();
  
  get_crypt_pass("Enter game master's password: ", s, NULL, NULL);
  return !strcmp(s, world.nations[0].passwd);
}


int find_free_nation(wp)		/* return first free slot, -1 if none */
     Sworld *wp;
{
  int i;
  for (i = 0; i < NATIONS; ++i) {
    if (strlen(wp->nations[i].name) == 0) {
      return i;			/* found a free one! */
    }
  }
  return -1;
}

void cleanup()
{
/*  clear(); */
  refresh();
}

  /* cleanup and prepare to exit */
void clean_exit()
{
  del_master_lock();
  if (stdscr) {
    refresh();
    endwin(); /* end curses */
  }
}

  /* initial military setup */
#define INITIAL_SOLDIERS 10*OCCUPYING_SOLDIERS
void setup_armies(np)
     Snation *np;
{
  Sarmy army, make_army();
  int i;
  char name [NAMELEN];

  load_army_types();
    /* we make 10 armies of equal size. the first one has special handling. */
  army = make_army(army_types[0].type, "Cavemen 0", INITIAL_SOLDIERS/10,
		   A_DEFEND, np->id, np->capital);
  army.mvpts = basic_move_rate(np);
  army.mvratio = 100;		/* ration of initial to final move points */
  army.id = 0;			/* make sure, this is FIRST army! */
  if (debug) {
    printf("made a new army: owner=%d,x=%d,y=%d,num=%d", army.owner,
	   army.pos.x,army.pos.y,army.n_soldiers);
  }
  army.next = NULL;
  np->armies = (Sarmy *) malloc(sizeof(Sarmy));	/* create army list */
  *(np->armies) = army;
  /* the sector must also have a list of armies */
  world.map[army.pos.x][army.pos.y].alist
    = (struct armyid *) malloc(sizeof(struct armyid));
  world.map[army.pos.x][army.pos.y].alist->owner = army.owner;
  world.map[army.pos.x][army.pos.y].alist->id = army.id;
  world.map[army.pos.x][army.pos.y].alist->next = NULL;
  for (i = 1; i < 10; ++i) {
    sprintf(name, "%s", army_types[0].type);
    army = make_army(army_types[0].type, name, INITIAL_SOLDIERS/10,
		     A_DEFEND, np->id, np->capital);
      /* first time around, they move at full rate */
    army.mvpts = basic_move_rate(np);
    army.mvratio = 100;
    army.id = i;
    army.next = NULL;
    insert_army_nation(np, &army, i);
    insert_army_sector(&world.map[army.pos.x][army.pos.y], &army);
  }
  np->n_armies = 10;
    /* figure out bonuses; crude formula for now */
  np->attack = np->race.strength/4 + np->race.intel/12;
  np->defense = np->race.strength/4 + np->race.intel/12;
  np->spy = 0;
  np->secrecy = 0;
}

  /* initialize fields in this nation's data structure
     that are relevant to the economy.
   */
void setup_economy(np)
     Snation *np;
{
  np->taxes = 10;		/* tax rate, % of income */
  np->charity = 0;		/* money given to the poor */
  np->money = 100000;		/* initial wealth */
  np->jewels = 20000;
  np->metal = 20000;
  np->food = 100000;
    /* default values, until user learns how to use her/his budget screen */
  np->tech_r_d = 30;
  np->tech_r_d_metal = 40;
  np->mag_r_d = 30;
  np->mag_r_d_jewels = 40;
  np->spy_r_d = 0;
  np->cur_mag_r_d = 0;
  np->cur_mag_r_d_jewels = 0;
  np->cur_tech_r_d = 0;
  np->cur_tech_r_d_metal = 0;
  np->cur_spy_r_d = 0;
}

int get_race_mark()
{
  char racemark;
  int done = 0;
  int race_counter=0;
  struct race_list *rlp;	/* to manipulate the list */
  char tmp_sprintf [70];

  clear ();

  mvaddstr (2, 0, "Races: ");
  rlp = races;
  while (rlp != NULL) {
    sprintf (tmp_sprintf, "(%c) : %s\n", rlp->race.mark, rlp->race.name);
    mvaddstr (4+race_counter++, 4, tmp_sprintf);
    rlp = rlp->next;
  }
  do {
    mvaddstr (0, 0, "Pick a race and enter its mark: ");
    refresh ();
    racemark = getch();
    done = 0;
    rlp = races;
    while (rlp != NULL) {
      if (rlp->race.mark == racemark) {
	done = 1;
	break;
      }
      rlp = rlp->next;
    }
  } while (!done);
  return racemark;
}

int setup_race(np,racemark)
     Snation *np;
     char racemark;
{
  struct race_list *rlp;	/* So we can work with the races list */
  
  rlp = races;
  while(rlp != NULL) {
    if (rlp->race.mark == racemark) { /* Did we find that race? */
      np->race = rlp->race;
      return 1;			/* If we found it, return a 1 */
    }
    rlp = rlp->next;
  }
  return -1;			/* We didn't find it */
}

void setup_skills(np)
     Snation *np;
{
  np->tech_skill = 0;
  np->mag_skill = 0;
  
  /* Nations begin with a certain production and it increases from there */
  
  np->farm_skill = np->race.farming;
  np->mine_skill = np->race.mining;

  np->spell_pts = 0;
}

void choose_mag_order(s)
     char s[];
{
  strcpy(s, "");
  clear ();
  list_mag_orders();

  do {
    mvaddstr (0, 0, "Choose one of the listed magical orders: ");
    clrtoeol ();
    refresh ();
    wget_string (stdscr,s,NAMELEN);
  } while (!is_good_order(s));
}

  /* sets up the capital and a few outlying sectors; if after
     MAX_TRIES it does not find a good place, it exits.
   */
int setup_capital(np)
     Snation *np;
{
  int pref, i, j, x, y, distance, tries,
    remaining_pop = INIT_CIV + RND() % 100, beginning_pop;
  Ssector *sp;
  int radius, n_sects;		/* dist. from capital for other sects */

  beginning_pop = remaining_pop;
  tries = 0;
  pref = 0;
  /* a negative capital x-coordinate means we have not found
     a good location for the capital yet.
   */
  x = -1;
  distance = 7;

  while (x < 0) {
    distance = max(1, distance - 1);
    for (i = 0; i < 10; i++) {
      do {
	  /* keep trying random sectors as long
	     as things aren't good.
	   */
	x = RND() % world.xmax;
	y = RND() % world.ymax;
	++tries;
	if (tries >= MAX_TRIES) {
	  printf("  Cannot find space for capital of nation %s\n", np->name);
	  return -1;
	}
      } while ((world.map[x][y].owner != 0) || !isolated(x, y) ||
	       (!good_altitude(&world.map[x][y], np)) ||
	       (world.map[x][y].metal > 0) ||
	       (world.map[x][y].jewels > 0));  /*see if it's taken!*/
      if (sect_desire(np, x, y) > pref) {
	pref = sect_desire(np, x, y);
	np->capital.x = x;
	np->capital.y = y;
      }
    }
    for (i = np->capital.x - distance; i <= np->capital.x + distance; i++) {
      for (j = np->capital.y - distance; j <= np->capital.y + distance; j++) {
	if (np->race.pref_alt < SEA_LEVEL) { /* under water race */
	  if (world.map[(*wrapx)(i,j)][(*wrapy)(i,j)].owner != 0 &&
	      world.map[(*wrapx)(i,j)][(*wrapy)(i,j)].altitude >= SEA_LEVEL) {
	    x = -1;		/* water race cannot be on land */
	  }
	} else {		/* above race */
	  if (world.map[(*wrapx)(i,j)][(*wrapy)(i,j)].owner != 0 &&
	      world.map[(*wrapx)(i,j)][(*wrapy)(i,j)].altitude < SEA_LEVEL) {
	    x = -1;		/* land race canot be underwater */
	  }
	}
      }
    }
  }
  x = np->capital.x;
  y = np->capital.y;
  putchar('\n');
  np->ptlist = NULL;
  addsector(np, x, y);
  world.map[x][y].n_people = beginning_pop/2; /* half people in capital */
  remaining_pop -= beginning_pop/2;
  world.map[x][y].designation = D_CAPITAL;
  np->n_sects = 1;

  radius = 1;
  n_sects = 1;
  while (n_sects < 9) {		/* must give user 9 sectors */
    for (i=x-radius; i<=x+radius; i++) {
      for (j=y-radius; j<=y+radius; j++) {
	if (i != 0 || j != 0) {	/* skip the capital */
	  sp = &world.map[wrapx(i,j)][wrapy(i,j)];
	  if (sp->owner != 0
	      || (np->race.pref_alt >= SEA_LEVEL && sp->altitude < SEA_LEVEL)
	      || (np->race.pref_alt < SEA_LEVEL && sp->altitude >= SEA_LEVEL)
	      || n_sects >= 9) {
	  } else {
	    addsector(np, (*wrapx)(i,j), (*wrapy)(i,j));
	    sp->n_people = beginning_pop/16;
	    remaining_pop -= beginning_pop/16;
	    sp->designation = default_desig(sp);
/*	    printf("rad %d; %d,%d: %d people, remaining_pop=%d (of %d)\n",
		   radius, i, j, sp->n_people, remaining_pop, beginning_pop);
*/
	    ++n_sects;
	  }
	}
      }
    }
    ++radius;			/* try bigger and bigger circles */
  }
    /* now if any people are left, put them in the capital */
  world.map[x][y].n_people += remaining_pop;
/*   printf("Done with nation %s\n", sp->name);*/
/*  printf("capital has %d people\n", world.map[x][y].n_people); */
  return 1;
}

  /* this gives a sector a default designation of
     D_FARM unless some other resource is present
   */
int default_desig(sp)
     Ssector *sp;
{
  if (sp->jewels > 0) {
    if (sp->metal > sp->jewels) {
      return D_METAL_MINE;
    } else {
      return D_JEWEL_MINE;
    }
  }
  if (sp->metal > 0) {
    return D_METAL_MINE;
  } else {
    return D_FARM;
  }
}

  /* the workhorse behind addnation */
// int setup_new_nation(nation_name, nation_pass, leader_name, nation_race,
// 		 nation_mark, mag_ord, npc_flag, npcagg, npcexp, npciso)
//      char nation_name[NAMELEN],nation_pass[PASSLEN],leader_name[NAMELEN],
//        mag_ord[NAMELEN], nation_race;
//      Symbol nation_mark;
//      int npc_flag, npcagg, npcexp, npciso;
int setup_new_nation(char nation_name[NAMELEN], char nation_pass[PASSLEN], char leader_name[NAMELEN],
    char nation_race, Symbol nation_mark, char mag_ord[NAMELEN], int npc_flag, int npcagg, int npcexp, int npciso)
{
  Sdiplo **dm_old, **dm_new;
  Snation tmp;
  int n;			/* Free nation ID number */
  char cmd_str[PATHLEN];

  if ((n = find_free_nation(&world)) == -1) {
    printf("No free nations, sorry.\n");
    return -1;
  }

  tmp.id = n;
  if (!unique_name(nation_name)) { /* Should reject identical names */
    printf("Already have a nation called %s.\n", nation_name);
    return -1;
  }
  strcpy(tmp.name, nation_name);
  strcpy(tmp.passwd, nation_pass);
  strcpy(tmp.leader, leader_name);
  strcpy(tmp.mag_order, mag_ord);

  setup_race(&tmp, nation_race);

  printf("Doing nation %s\n", nation_name);

  if (!free_nation_mark(&world, nation_mark)) {
    printf("  Already have a nation using %c as their nation mark.\n",
	   nation_mark);	/* Should reject identical nation marks */
    return(-1);
  }
  tmp.mark = nation_mark;

  setup_economy(&tmp);
  setup_skills(&tmp);

  tmp.npc_flag = npc_flag;
  tmp.npc_agg = npcagg;
  tmp.npc_iso = npciso;
  tmp.npc_exp = npcexp;

  if (setup_capital(&tmp) < 0) { /* Adds a capital and land */
    /* if there is no good place for the nation */
    return -1;
  }
  setup_armies(&tmp);		/* must be done after capital */

  world.nations[n] = tmp;	/* OK, now add this nation */
  ++world.n_nations;

    /* now update the diplo file, so it is bigger */
  dm_old = allocate_diplo(world.n_nations-1);
  dm_new = allocate_diplo(world.n_nations);
  read_in_diplo(dm_old, world.n_nations-1);
  increase_diplo(dm_old, dm_new, world.n_nations-1, &tmp);
  dump_diplo(NULL, dm_new, world.n_nations);
    /* also make a copy of the initial diplomacy file */
  sprintf(cmd_str, "cp %s %s\n", DIPLO_FILE, INIT_DIPLO_FILE);
  system(cmd_str);
  free_diplo(dm_old, world.n_nations-1);
  free_diplo(dm_new, world.n_nations);

  return 1;
}

void add_npcs(npc_fn, npcs_get_mail)
     char * npc_fn;
     int npcs_get_mail;
{
  FILE *fp;
  char s[110], racemark, nationmark;
  char nation_name[NAMELEN], leader_name[NAMELEN], mag_order[NAMELEN];
  int n_npcs, npc_agg, npc_exp, npc_iso;
  int i;

  if ((fp = fopen(npc_fn, "r")) == NULL) {
    printf("Cannot open npc file.\n");
    return;
  }

  fgets(s, 100, fp);
  while (s[0] == '#') {		/* Ignoring the comments */
    fgets(s, 100, fp);
  }
  sscanf(s, "%d", &n_npcs);

  for (i = 0; i < n_npcs; i++) {
    fgets(s, 100, fp);
    s[strlen(s)-1] = '\0';
    if (s[0] != '#') {
      sscanf(s, "%s : %s : %c : %c : %s : %d : %d : %d", nation_name,
	     leader_name, &racemark, &nationmark, mag_order,
	     &npc_agg, &npc_exp, &npc_iso);
      if (debug) {
	printf("  Nation: %s\n  Leader: %s\nRacemark: %c\n Ntnmark: %c\n",
	       nation_name, leader_name, racemark, nationmark);
	printf("Mag_Ordr: %s\nAgressiv: %d\n  Expand: %d\n Isolate: %d\n\n",
	       mag_order, npc_agg, npc_exp, npc_iso);
      }
      if (setup_new_nation(nation_name, world.nations[0].passwd, leader_name,
		       racemark, nationmark, mag_order,
		       npcs_get_mail ? NPC_MAIL : NPC_NOMAIL,
		       npc_agg, npc_exp, npc_iso) < 0) {
	/* if we cannot set up this NPC for whatever reason */
	printf("    Did NOT set up nation %s\n\n", nation_name);
	continue;		/* go on to the next NPC */
      }
    }
  }
  fclose(fp);
  printf("\n");
  write_world(&world, WORLD_FILE);
}

  /* critical() for the update/make/add is different from the game */
void critical()
{
  signal(SIGINT, SIG_IGN);
  signal(SIGQUIT, SIG_IGN);
}
void noncritical()
{
  signal(SIGINT, sigquit);
  signal(SIGQUIT, sigquit);
}


int sigquit ()
{
  if (stdscr) {
    standout();
    mvprintw(0, 65, "Quitting...");
    standend();
    refresh();
  } else {
    printf("\nQuitting...\n");
  }
  clean_exit();
  exit(1);
}

/* Lifted from misc.c and maglib.c */
void show_race_w(rp)
     Srace *rp;
{
}

void list_mag_orders()
{
  int n_orders, i;
  FILE *fp;
  char line[200];
  char **order_list;
  char mag_counter=0;

  if ((fp = fopen(MAG_ORDERS, "r")) == NULL) {
    printw("\ncannot find file %s.  this is bad.\n\r", MAG_ORDERS);
    clean_exit();
    exit(1);
  }
  while (fgets(line, 200, fp) != NULL) {
    if (line[0] != '#') {
      sscanf(line, "%d", &n_orders);
      break;			/* we got the number of orders */
    }
  }
  order_list = (char **) malloc(n_orders*sizeof(char *));

  mvaddstr (2, 0, "Magical orders: ");

  for (i = 0; i < n_orders; ) {
    fgets(line, NAMELEN, fp);
    if (line[0] != '#') {
      mvprintw(3 + mag_counter++, 4, "%s", line);
      order_list[i] = (char *) malloc(NAMELEN*sizeof(char));
      strcpy(order_list[i], line);
      ++i;
    }
  }

  fclose(fp);
}

  /* prompt a user for a nation mark */
#define valid_nation_mark(c) (strchr(valid_mark_str, c))
char get_nation_mark()
{
  int attempt_counter=0;
  char mark;
  char valid_mark_str[128];

  form_valid_mark_str(valid_mark_str);

  clear();

  mvaddstr(0, 0, "Which nation mark do you want? ");
  refresh();

  mark = getch();
  while (!free_nation_mark(&world, mark) || !valid_nation_mark(mark)) {
    if (attempt_counter > 15) {
      attempt_counter = 15;
    }
    if (!free_nation_mark(&world, mark)) {
      mvprintw(2 + attempt_counter++, 0,
		"Sorry, the nation mark %c is taken already.", mark);
    } else if (!valid_nation_mark(mark)) {
      mvprintw(2 + attempt_counter++, 0,
		"Sorry, the nation mark %c is not valid.", mark);
      clrtoeol();
      mvprintw(2 + attempt_counter++, 0, "Valid nation marks are: ");
      mvprintw(2 + attempt_counter++, 0, "%s", valid_mark_str);
      ++attempt_counter;
    }
    mvaddstr(0, 0, "Which nation mark do you want? ");
    refresh();
    mark = getch();
  }
  return mark;

#ifdef OLD
  if (!free_nation_mark(&world, mark))  { /* see if it's already used */
    do {
      if (attempt_counter > 20) attempt_counter=20;
      mvprintw (2 + attempt_counter++, 0,
                "Sorry, the nation mark %c is taken already.", mark);
      mvprintw (0, 0, "Which nation mark do you want: ");
      refresh ();
      mark = getch ();
    } while (!free_nation_mark(&world, mark));
  }

  /* now check if it is a nation mark that is not valid
     (like a space, or a dash or a dot or twiddle)
   */
  if (!valid_nation_mark(mark))  {
    do {
      if (attempt_counter > 20) {
	attempt_counter=20;
      }
      mvprintw(2 + attempt_counter++, 0,
	       "Sorry, the nation mark %c is not valid.", mark);
      mvprintw(2 + attempt_counter++, 0, "Valid nation marks are: ");
      addstr(valid_mark_str);
      mvprintw(0, 0, "Which nation mark do you want: ");
      refresh();
      mark = getch();
    } while (!free_nation_mark(&world, mark));
  }
  return mark;
#endif /* OLD */
}

  /* returns true if there is no owned sector too near this one */
int isolated(x, y)
{
  int i, j;

  for (i = x-RADIUS; i <= x+RADIUS; ++i) {
    for (j = y-RADIUS; j <= y+RADIUS; ++j) {
        /* see if one of these nearby sectors is owned */
      if (world.map[(*wrapx)(i, j)][(*wrapy)(i, j)].owner != 0) {
	return 0;		/* oops, we are *not* isolated */
      }
    }
  }
  return 1;
}

  /* put some stuff into a nation's exec file to get them started with
     the privilege of their magic order.
   */
void add_special_mag(np, mag_ord)
     Snation *np;
     char mag_ord[];
{
  char fname[255], line[2*EXECLEN];
  FILE *magfp, *execfp;
  int done = 0;

  strcpy(fname, MAG_PREFIX);
  strcat(fname, mag_ord);
  if ((magfp = fopen(fname, "r")) == NULL) {
    clean_exit();
    exit(1);
  }
  sprintf(fname, "exec/exec%d", np->id);
  if ((execfp = fopen(fname, "a")) == NULL) {
    clean_exit();
    exit(1);
  }
  while (!done) {
    if (fgets(line, EXECLEN, magfp) == NULL) {
      fclose(magfp);
      fclose(execfp);
      return;
    }
    if (line[0] != '#') {	/* avoid comments */
      if (strncmp(line, "EXEC:", strlen("EXEC:")) == 0) {
/*	printf("found an exec line %s\n", line);
	refresh();
	getch();
	getch();
*/
	fprintf(execfp, "%s", line+strlen("EXEC:"));
      }
    }
  }
}

/* Please note that this is the same routine as in cur_stuff.c but linking
   that into this code would require a plethora of other object files to 
   successfully compile.
*/
  /* gets a string str of max length len */
int wget_string (w, rets, len)
     WINDOW * w;
     char * rets;
     int len;
{
  char s [80];
  int pos, done;
  int x, y, i;
  int oldpos;		/* Used for ^W */
  noecho ();

  if (w == NULL) {
    w = stdscr;
  }

  pos = 0;
  done = 0;

  getyx (w, y, x);
  wrefresh (w);

  while (!done) {
    s [pos] = wgetch (stdscr);
    switch (s[pos]) {
    case '\n':
    case '\r':
      s [pos] = '\0';
      done = 1;
      break;
    case '\b':
    case DEL:
      if (pos > 0) {
	pos--;
	s[pos] = '\0';
	wmove (w, y, x + pos);
	waddch (w, ' ');
	wmove (w, y, x + pos);
      }
      break;
    case CTL('U'):
      wmove (w, y, x);
      for (i=0; i < pos; i++) {
	waddch (w, ' ');
      }
      wmove (w, y, x);
      pos = 0;
      s [pos] = '\0';
      break;
    case CTL('W'):
      oldpos = pos;
      while (pos != 0 && s[pos] == ' ') {
	pos --;
      }
      if (pos) {
	pos --;
      }
      while (pos != 0 && s[pos] != ' ') {
	pos --;
      }
      wmove (w, y, x + pos);
      while (oldpos != pos) {
	oldpos--;
	waddch (w, ' ');
      }      
      wmove (w, y, x + pos);
      break;
    default:
      waddch (w, s [pos]);
      pos++;
      break;
    }
    wrefresh (w);
  }

  if (pos == 0) {
    return 0;
  }
  s [len-1] = '\0';
  strcpy (rets, s); 
  return 1;
}

  /* return true if sector (x, y) has at least n connected sectors
     on land (or in water if (x, y) is in water) surrounding it.
   */
int has_connected_mass(x, y, n)
     int x, y, n;
{
  Ssector *sp = &world.map[x][y];
  int n_found = 1, prev_n_found = 1; /* count (x, y) */
  int i, j;

  /* go about (x, y) one circle at a time.  after each circle,
     see if any new connected sectors were found.  if none were,
     and n_found < n, then NAH... doesn't work!!  put it off for now.
   */
}

  /* makes a list of valid nation marks in the given string;
     a mark is valid if it doesn't make a [d] [n] (nation mark
     display) confusing.  For example, <space>, '.' and so on
     would confuse.
   */
void form_valid_mark_str(mark_str)
     char mark_str[];
{
  char tmp_str[128];

  strcpy(tmp_str, "");
  /* run through the ascii table (only those chars that are printable) */
  strcat(tmp_str, "!\"#$%&'()*+,");
  /* skip '-' and '.' */
  strcat(tmp_str, "/0123456789:;<=>"); /* now skip '?' */
  strcat(tmp_str, "@ABCDEFGHIJKLMNOPQRSTUVWXYZ");
  strcat(tmp_str, "[\]^_`abcdefghijklmnopqrstuvwxyz{|}");
  strcpy(mark_str, tmp_str);
}
