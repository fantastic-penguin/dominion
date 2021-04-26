  /* file.c -- file operations */

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

/* this module contains routines that read and write the
   world data file, and also routines that handle lock files.
 */

#include <stdio.h>
#ifdef SYSV
# include <string.h>
#else
# include <strings.h>
#endif /* SYSV */
#include <stdlib.h>
#include <time.h>
#include <fcntl.h>
#include <unistd.h>

#include "dominion.h"
#include "misc.h"
#include "functions.h"

#define UPDATE_FILE "last_update"

extern int debug, compressed_world;
extern struct race_list *races;
extern Suser user;

void read_world(wp, fname)
     Sworld *wp;
     char fname[];
{
  int i, j;
  FILE *fp, *fopen();
  char cmd[200];

  compressed_world = check_compressed_world(fname);
  if (compressed_world) {
    sprintf(cmd, "zcat %s", fname);
    if ((fp = popen(cmd, "r")) == NULL) {
      printf("\r\ncannot open pipe <%s> for reading\r\n", cmd);
      clean_exit();
      exit(1);
    }
  } else {
    if ((fp = fopen(fname, "r")) == NULL) {
      printf("\r\ncannot open file <%s> for reading\r\n", fname);
      clean_exit();
      exit(1);
    }
  }
  fprintf(stderr, "reading world data file %s", fname);

  fread(&wp->turn, sizeof(int), 1, fp); /* get current turn */
  fread(&wp->xmax, sizeof(int), 1, fp);	/* get world size */
  fread(&wp->ymax, sizeof(int), 1, fp);
/*  fprintf(stderr, "world size is %dx%d\n", wp->xmax, wp->ymax); */

    /* make space for world sectors */
  wp->map = (Ssector **) malloc(wp->xmax*sizeof(Ssector *));
  for (i =  0; i < wp->xmax; ++i) {
    wp->map[i] = (Ssector *) malloc(wp->ymax * sizeof(Ssector));
  }
	/* now read in the world map */
  for (i = 0; i < wp->xmax; ++i) {
    for (j = 0; j < wp->ymax; ++j) {
      fread(&wp->map[i][j], sizeof(Ssector), 1, fp);
      wp->map[i][j].alist = NULL;
    }
    if (i % (wp->xmax/8) == 0) {
      fprintf(stderr, ".");
      fflush(stdout);
    }
  }
    /* load geography */
  fread(&wp->geo, sizeof(wp->geo), 1, fp);
  fprintf(stderr, ".");
  fflush(stdout);
        /* now read in nations */
  fread(&wp->n_nations, sizeof(int), 1, fp);
/*  printf("reading in %d nation%c\n",
	 wp->n_nations, wp->n_nations == 1 ? ' ' : 's'); */
  for (i = 0; i < wp->n_nations; ++i) {
      /* must find a better way of loading linked lists
         (when I make the nations be a linked list!!)
       */
    read_nation(&wp->nations[i], fp, wp);
    load_options(&wp->nations[i]);

    fprintf(stderr, ".");
    fflush(stdout);
/*    if (debug) {
      printf("just read nation %d\n", i);
      show_nation(&wp->nations[i]);
      putchar('.');
    }
*/
  }
  if (compressed_world) {
    pclose(fp);
  } else {
    fclose(fp);
  }
  init_wrap();
  fprintf(stderr, "\n");
}

  /* save the world data file */
void write_world(wp, fname)
     Sworld *wp;		/* pointer to the world */
     char fname[];
{
  int i, j;
  FILE *fp, *fopen();
  char cmd[200];		/* for compression command */

  if (compressed_world) {
    sprintf(cmd, "compress > %s.Z", fname);
    if ((fp = popen(cmd, "w")) == NULL) {
      printf("\r\ncannot open pipe <%s> for writing\r\n", cmd);
      clean_exit();
      exit(1);
    }
  } else {
    if ((fp = fopen(fname, "w")) == NULL) {
      printf("cannot open file <%s> for writing\n", fname);
      clean_exit();
      exit(1);
    }
  }
  critical();
  fwrite(&wp->turn, sizeof(int), 1, fp); /* write current turn number */
  fwrite(&wp->xmax, sizeof(int), 1, fp);
  fwrite(&wp->ymax, sizeof(int), 1, fp);
  for (i = 0; i < wp->xmax; ++i) {
    for (j = 0; j < wp->ymax; ++j) {
      fwrite(&wp->map[i][j], sizeof(Ssector), 1, fp);
    }
  }
    /* write out geography */
  fwrite(&wp->geo, sizeof(wp->geo), 1, fp);

  printf("writing out %d nation%c\n",
	 wp->n_nations, wp->n_nations == 1 ? ' ' : 's');
  fwrite(&wp->n_nations, sizeof(int), 1, fp);
	/* only save as many nations as there are */
  for (i = 0; i < wp->n_nations; ++i) {
    write_nation(&wp->nations[i], fp);
    if (debug) {
      putchar('.');
    }
  }

  if (compressed_world) {
    pclose(fp);
  } else {
    fclose(fp);
  }
  noncritical();		/* delicate phase over */
}

  /* short hand */
#define WRITE_DATUM(x) fwrite(&x, sizeof(x), 1, fp)

void write_nation(np, fp)
     Snation *np;
     FILE *fp;
{
  int i;
  Sarmy *ap;			/* for armies */
  struct pt_list *pp;		/* for owned sectors */

    /* here is how we save a nation:  write out the actual nation data
       structure first, and after it write out the various linked lists
       (which are the army list and the list of owned points).
     */

  fwrite(np, sizeof(Snation), 1, fp);

    /* write out list of country's armies */
  ap = np->armies;
  for (i = 0; i < np->n_armies; ++i) {
    fwrite(ap, sizeof(Sarmy), 1, fp);
    if (debug) {
      putchar('a');
    }
    ap = ap->next;
  }

    /* write list of owned locations */
  pp = np->ptlist;
  for (i = 0; i < np->n_sects; ++i) {
    fwrite(pp, sizeof(struct pt_list), 1, fp);
    if (debug) {
      putchar('s');
    }
    pp = pp->next;
  }
}

void read_nation(np, fp, wp)
     Snation *np;
     FILE *fp;
     Sworld *wp;
{
  int i, x, y;
  Ssector *sp;			/* for tmp use */
  Sarmy army;			/* for armies */
  struct pt_list *pp;		/* for owned sectors */

    /* here is how we load a nation:  read out the actual nation data
       structure first, and after it read out the various linked lists
       (which are the army list and the list of owned
       points).  This is harder than writing because we must insert
       all into linked lists.
     */

  fread(np, sizeof(Snation), 1, fp); /* this is the easy part */

    /* now reset to zero the nation's "current" R&D values:
       that way the special R&D investments of the previous turn
       are zeroed, and only the percent-of-revenue values are saved
     */
  np->cur_mag_r_d = np->cur_mag_r_d_jewels = 0;
  np->cur_tech_r_d = np->cur_tech_r_d_metal = 0;
  np->cur_spy_r_d = 0;

  /* read list of country's armies add each army to the list for
     its nation and for its sector. Special case for first army.
   */
  np->armies = NULL;
  for (i = 0; i < np->n_armies; ++i) {
    fread(&army, sizeof(Sarmy), 1, fp);
    army.next = NULL;
    x = army.pos.x;
    y = army.pos.y;
    sp = &(wp->map[x][y]);
    if (i == 0) {
/*      army.id = 0; */
      np->armies = (Sarmy *) malloc(sizeof(Sarmy));
      *(np->armies) = army;
      np->armies->next = NULL;
    } else {
/*      if (debug) {
	printf("Calling insert_army_nation, id=%d\n", army.id);
      }
*/
      insert_army_nation(np, &army, army.id);
/*      if (debug) {
	printf("Called insert_army_nation.\n");
      }
*/
    }
/*    if (debug) {
      printf("Calling insert_army_sector (%d,%d)\n", sp->loc.x, sp->loc.y);
    }
*/
    insert_army_sector(sp, &army);
  /*  if (debug) {
      printf("Called insert_army_sector.\n");
    }
*/
  }

  /* read list of owned locations (special case for first one) */
  if (np->n_sects > 0) {
    np->ptlist = (struct pt_list *) malloc(sizeof(struct pt_list));
    fread(np->ptlist, sizeof(struct pt_list), 1, fp);
    /*    np->ptlist->pt = np->capital; */
    np->ptlist->next = NULL;
    pp = np->ptlist;
  }
  for (i = 1; i < np->n_sects; ++i) {
    pp->next = (struct pt_list *) malloc(sizeof(struct pt_list));
    fread(pp->next, sizeof(struct pt_list), 1, fp);
    pp = pp->next;
    pp->next = NULL;
    if (debug) {
      putchar('s');
    }
  }
}

  /* reads in the list of all races */
void read_races()
{
  FILE *fp, *fopen();
  char s[200];
  int i, n_races;
  Srace tmp_race;		/* for temporary use */
  struct race_list *rlp;	/* to make the linked list */

  if ((fp = fopen(RACES_FILE, "r")) == NULL) {
    printf("cannot open races file.  you might have the wrong directory..\n");
    clean_exit();
    exit(1);
  }

    /* the initial race is the master, and is hardwired */
  races = (struct race_list *) malloc(sizeof(struct race_list));
  strcpy(races->race.name, "Master");
  races->race.mark = 'C';
  races->race.strength = 0;
  races->race.repro = 0;
  races->race.mortality = 0;
  races->race.intel = 0;
  races->race.speed = 0;
  races->race.stealth = 0;
  races->race.pref_alt = 0;
  races->race.pref_terrain = 0;
  races->race.pref_climate = 0;
  races->race.mag_apt = 0;
  races->race.farming = 0;
  races->race.mining = 0;
  races->next = NULL;

  rlp = races;

    /* now get the number of races from the file
       (we trust that the file is consistent)
     */
  fgets(s, 180, fp);
  while (s[0] == '#') {		/* ignore comments */
/*    if (debug) {
      printf("<%s>", s);
    }
*/
    fgets(s, 180, fp);
  }
/*  if (debug) {
    printf("<%s>", s);
  }
*/
  sscanf(s, "%d", &n_races);	/* first line has number of races */

  for (i = 0; i < n_races; ) { /* now read them in!! */
    char c;
    char name[180];
    fgets(s, 180, fp);
    s[strlen(s)-1] = '\0';
/*    if (debug) {
      printf("<%s>", s);
    }
*/
    if (s[0] != '#') {		/* skip comments */
      ++i;
      sscanf(s,
        "%s : %1s : %d : %d : %d : %d : %d : %d : %d : %d : %d : %d : %d : %d",
	     tmp_race.name, &tmp_race.mark, &tmp_race.strength,
	     &tmp_race.repro, &tmp_race.mortality, &tmp_race.intel,
	     &tmp_race.speed, &tmp_race.stealth, &tmp_race.pref_alt,
	     &tmp_race.pref_terrain, &tmp_race.pref_climate,
	     &tmp_race.mag_apt, &tmp_race.farming, &tmp_race.mining);

/*      if (debug) {
	show_race(&tmp_race);
      }
*/
        /* now that we have loaded it, add it to the list */
      rlp->next = (struct race_list *) malloc(sizeof(struct race_list));
      rlp = rlp->next;
      rlp->race = tmp_race;
      rlp->next = NULL;
    }
  }
  fclose(fp);
}

  /* this routine checks to see if the world file is
     in a compressed format.  It it is, it returns 1.
     Otherwise, 0.
   */
int check_compressed_world(fname)
     char fname[];
{
  char Zname[200];
  FILE *fopen(),*fp;

  strcpy(Zname, fname);
  strcat(Zname, ".Z");		/* if .Z file exists, it must be compressed! */
  if ((fp = fopen(Zname, "r")) != NULL) {
    fclose(fp);
    return 1;
  }
  return 0;
}

  /* sets a master lock file */
void set_master_lock()
{
  close(creat("lock.master", 0600));
}

  /* sets a lock for the given nation id */
void set_lock(id)
     int id;
{
  char fname[PATHLEN];
  FILE *fp, *fopen();
  extern Sworld world;
  extern int ruid;
  long now_secs;

  sprintf(fname, "lock.%d", id);
/*  close(creat(fname, 0666)); */
  if ((fp = fopen(fname, "w")) == NULL) {
    printf("cannot open the lock file file <%s> for writing\n", fname);
    clean_exit();
    exit(1);
  }
  now_secs = time(0L);
    /* now put some titbits of information into the lock file */
  fprintf(fp, "%ld; Nation %s; real uid %d; time: %s", now_secs,
	  world.nations[id].name, ruid, ctime(&now_secs));
  fclose(fp);
}

  /* removes a lock for the given nation id */
void del_lock(id)
     int id;
{
  char fname[PATHLEN];

  sprintf(fname, "lock.%d", id);
  unlink(fname);
}

  /* removes the master lock file */
void del_master_lock()
{
  unlink("lock.master");
}

  /* tries to open the lock file.  returns the file pointer
     it gets, with the file open for reading (if it exists)
   */
FILE *is_locked(id)
     int id;
{
  FILE *fopen();
  char fname[PATHLEN];

  sprintf(fname, "lock.%d", id);
  return fopen(fname, "r");
/*  if ((fp = fopen(fname, "r")) != NULL) {
    fclose(fp);
    return 1;			/* yeah, it is locked */
/*  }
  return 0; */
}

  /* checks if even a single nation has a lock or not */
int is_any_lock()
{
  int i;
  FILE *fp;

  for (i = 0; i < NATIONS; ++i) {
    if (fp = is_locked(i)) {
      fclose(fp);
      return 1;
    }
  }
  return 0;
}

  /* cheks if there is a master lock file */
int is_master_lock()
{
  FILE *fp, *fopen();

  if ((fp = fopen("lock.master", "r")) != NULL) {
    fclose(fp);
    return 1;
  }
  return 0;
}

void set_update_time()
{
  char fname[PATHLEN];
  FILE *fp, *fopen();
  extern Sworld world;
  extern int ruid;
  long now_secs;

  sprintf(fname, UPDATE_FILE);
  if ((fp = fopen(fname, "w")) == NULL) {
    printf("cannot open the update file file <%s> for writing\n", fname);
    clean_exit();
    exit(1);
  }
  now_secs = time(0L);
    /* now put some titbits of information into the lock file */
  fprintf(fp, "%ld; time: %s", now_secs, ctime(&now_secs));
  fclose(fp);
}

char *get_update_time()
{
  char fname[PATHLEN];
  FILE *fp, *fopen();
  extern Sworld world;
  extern int ruid;
  long secs, len;
  char s[300], *rtvl, *tmp;

  sprintf(fname, UPDATE_FILE);
  if ((fp = fopen(fname, "r")) == NULL) {
    set_update_time();
  }
  fgets(s,299,fp);
  if ((rtvl = (char *) malloc(strlen(s) * sizeof(char))) == NULL) mem_error();
  tmp = strchr(s,(int)':');
  strcpy(rtvl,(tmp+1));
  fclose(fp);
  return rtvl;
}

void mem_error()
/* 
  If we can't allocate any more memory, then tell the user that's the
  case, and then die quietly, rather than the horrible death not 
  checking mallocs would cause.
*/
{
  fprintf(stderr,"Error: Couldn't allocate requested memory");
  cleanup();
  clean_exit();
  exit(1);
}     

void load_options(np)
Snation *np;
/* Loads various options form the options file into the user's option record */
{
  char opt_file[PATHLEN], opt_line[100];
  FILE *fopt;
  int len;

  init_options(np);
  sprintf(opt_file, "%s/opt.%d", OPT_DIR, np->id);
  if ((fopt = fopen(opt_file,"r")) == NULL) {
    save_options(np);
    return;
  }
  while (fgets(opt_line, 99, fopt) != NULL) {
    if (strncmp(opt_line, "EXPERT_MODE: ", 13) == 0)
    {
      np->opts->expert_mode = atoi(&(opt_line[13]));
      user.xmode = np->opts->expert_mode;
    } else if (strncmp(opt_line, "CIV_MOVEMODE: ", 14) == 0) {
      np->opts->civ_movemode = atoi(&(opt_line[14]));
    } else if (strncmp(opt_line, "MAIL_FORWARD: ", 14) == 0) {
      if (opt_line[14] == '\n') { 
        np->opts->mail_forward = NULL;
        continue;
      }
      len = strlen(&(opt_line[14]));
      if ((np->opts->mail_forward = (char *)malloc((len + 1)* sizeof(char)))
           == NULL) { mem_error(); }
      strcpy(np->opts->mail_forward,&(opt_line[14]));
      len = strlen(np->opts->mail_forward);
      if (np->opts->mail_forward[len - 1] == '\n') {
        np->opts->mail_forward[len - 1] = '\0';
      }
    } else if (strncmp(opt_line, "MAIL_READER: ", 13) == 0) {
      if (opt_line[13] == '\n') { 
        np->opts->mail_reader = NULL;
        continue;
      }
      len = strlen(&(opt_line[13]));
      if ((np->opts->mail_reader = (char *)malloc((len + 1)* sizeof(char)))
           == NULL) { mem_error(); }
      strcpy(np->opts->mail_reader,&(opt_line[13]));
      len = strlen(np->opts->mail_reader);
      if (np->opts->mail_reader[len - 1] == '\n') {
        np->opts->mail_reader[len - 1] = '\0';
      }
    } else {
      fprintf(stderr,"Error: Bad Option %s\n",opt_line);
    }
  }
  fclose(fopt);
}

void save_options(np)
/* Save all of the user's optoins to a file. */
Snation *np;
{
  FILE *fopt;
  char opt_file[PATHLEN], opt_line[100], *rtvl = NULL;
  int len;

  sprintf(opt_file, "%s/opt.%d", OPT_DIR, np->id);
  if ((fopt = fopen(opt_file,"w")) == NULL) {
    clean_exit();
    fprintf(stderr,"Error: Cannot Open Options File %s\n",opt_file);
    exit(1);
  }

  fprintf(fopt,"EXPERT_MODE: %d\n",np->opts->expert_mode);
  fprintf(fopt,"CIV_MOVEMODE: %d\n", np->opts->civ_movemode);
  if (np->opts->mail_forward == NULL) {
    fprintf(fopt,"MAIL_FORWARD: \n");
  } else {
    fprintf(fopt,"MAIL_FORWARD: %s\n",np->opts->mail_forward);
  }
  if (np->opts->mail_reader == NULL) {
    fprintf(fopt,"MAIL_READER: \n");
  } else {
    fprintf(fopt,"MAIL_READER: %s\n",np->opts->mail_reader);
  }
  fclose (fopt);
}

char *get_char_option(user_num, optname)
/* This function returns a character option for the given user */
int user_num;
char *optname;
{
  FILE *fopt;
  char opt_file[PATHLEN], opt_line[100], *rtvl = NULL;
  int len, optlen = strlen(optname);

  sprintf(opt_file, "%s/opt.%d", OPT_DIR, user_num);
  if ((fopt = fopen(opt_file,"r")) == NULL) {
    return NULL;
  }

  while (fgets(opt_line, 99, fopt) != NULL) {
    if (strncmp(opt_line, optname, optlen) == 0) {
      if (opt_line[optlen + 2] == '\n') { 
        rtvl = NULL;
        break;
      }
      len = strlen(&(opt_line[optlen + 2]));
      if ((rtvl = (char *)malloc((len + 1)* sizeof(char))) == NULL) { 
       mem_error(); }
      strcpy(rtvl,&(opt_line[optlen + 2]));
      len = strlen(rtvl);
      if (rtvl[len - 1] == '\n') {
        rtvl[len - 1] = '\0';
      }
    }
  }
  fclose(fopt);
  return rtvl;
}

int get_int_option(user_num, optname)
/* This function returns an integer option for a given user */
int user_num;
char *optname;
{
  FILE *fopt;
  char opt_file[PATHLEN], opt_line[100];
  int len, optlen = strlen(optname), rtvl;

  sprintf(opt_file, "%s/opt.%d", OPT_DIR, user_num);
  if ((fopt = fopen(opt_file,"r")) == NULL) {
    return NULL;
  }

  while (fgets(opt_line, 99, fopt) != NULL) {
    if (strncmp(opt_line, optname, optlen) == 0) {
      rtvl = atoi(&(opt_line[optlen + 2]));
      break;
    }
  }

  fclose(fopt);
  return rtvl;
}
  
void init_options(np)
/* Init an unused option record */
Snation *np;
{
  if ((np->opts = (Soptions *)malloc(sizeof(Soptions))) == NULL){mem_error();}
  
  np->opts->expert_mode = 0;
  np->opts->civ_movemode = 2; /* Move freely unless restricted by gov */
  np->opts->mail_forward = NULL;
  np->opts->mail_reader = NULL;
}
