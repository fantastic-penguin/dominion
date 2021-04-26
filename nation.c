/* nation.c -- commands needed to remember changes to a nation;
               basically a lot of routines to parse the exec file
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
extern Sworld world;
extern int debug;
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

struct exec_cmd { char name[NAMELEN]; void (*func)(); };

void cmd_amove(), cmd_astat(), cmd_aflag_set(), cmd_aflag_clear(),
  cmd_flag_set_sector(), cmd_flag_clear_sector(),
  cmd_acargo(), cmd_sname(),  cmd_aname(), cmd_charity(),
  cmd_taxrate(), cmd_desig_sector(), cmd_amake(),
  cmd_cpeople_sector(), cmd_cowner_sector(), cmd_csoil_sector(),
  cmd_cmetal_sector(), cmd_cjewels_sector(), cmd_caltitude_sector(),
  cmd_cmoney(), cmd_cmetal(), cmd_cjewels(),
  cmd_cur_tech_money (), cmd_cur_mag_money (), cmd_cur_mag_jewels (),
  cmd_cur_tech_metal (),
  cmd_cur_spy_money (),
  cmd_cspell_pts(), cmd_ctech_skill(), cmd_cmag_skill(),
  cmd_cfood(), cmd_tech_money(), cmd_tech_metal(),
  cmd_spy_money(),
  cmd_mag_money(), cmd_mag_jewels(), cmd_cmine(), cmd_cfarm(),
  cmd_cspeed(), cmd_nation_name(), cmd_nation_leader(), cmd_nation_order(),
  cmd_nation_mark(), cmd_set_npc(), cmd_clear_npc(), cmd_npc_param(),
  cmd_ccombat(), cmd_cattack(), cmd_cdefense(), cmd_crepro(), cmd_cmortality(),
  cmd_cintel(), cmd_cmag_apt(), cmd_cstrength(),
  cmd_new_army_type(), cmd_aincrease(), cmd_amerge(), cmd_asplit(),
  cmd_adisband(),
  cmd_cabonus(), cmd_cfort_sector(), cmd_croads_sector(), cmd_cpass(),
  cmd_destroy(), cmd_acastle ();

static struct exec_cmd commands[] = {
  {"AMOVE", cmd_amove} , {"ASTAT", cmd_astat} , {"AMAKE", cmd_amake} ,
  {"AFLAG_SET", cmd_aflag_set} , {"AFLAG_CLEAR", cmd_aflag_clear} ,
  {"FLAG_SET_SECTOR", cmd_flag_set_sector} ,
  {"FLAG_CLEAR_SECTOR", cmd_flag_clear_sector} ,
  {"ACARGO", cmd_acargo} ,
  {"SNAME", cmd_sname} , {"ANAME", cmd_aname},
  {"DESIG_SECTOR", cmd_desig_sector} ,
  {"CHARITY", cmd_charity} , {"TAXRATE", cmd_taxrate},
  {"CMONEY", cmd_cmoney} , {"CJEWELS", cmd_cjewels}, {"CMETAL", cmd_cmetal},
  {"CUR_MAGR&Dmoney", cmd_cur_mag_money},
  {"CUR_TECHR&Dmoney", cmd_cur_tech_money},
  {"CUR_TECHR&Dmetal", cmd_cur_tech_metal},
  {"CUR_MAGR&Djewels", cmd_cur_mag_jewels},
  {"CUR_SPYR&Dmoney", cmd_cur_spy_money},
  {"CSPELL_PTS", cmd_cspell_pts}, {"CTECH_SKILL", cmd_ctech_skill} ,
  {"CMAG_SKILL", cmd_cmag_skill} ,
  {"CFOOD", cmd_cfood} , {"TECHR&Dmoney", cmd_tech_money} ,
  {"TECHR&Dmetal", cmd_tech_metal} , {"SPYR&Dmoney", cmd_spy_money} ,
  {"MAGR&Dmoney" , cmd_mag_money} , {"MAGR&Djewels" , cmd_mag_jewels} ,
  {"CMINE" , cmd_cmine} , {"CFARM" , cmd_cfarm} , {"CSPEED", cmd_cspeed} ,
  {"NATION_NAME", cmd_nation_name} , {"NATION_LEADER", cmd_nation_leader} ,
  {"NATION_ORDER", cmd_nation_order} , {"NATION_MARK", cmd_nation_mark} ,
  {"SET_NPC", cmd_set_npc} , {"CLEAR_NPC", cmd_clear_npc} ,
  {"NPC_PARAM", cmd_npc_param} ,
  {"CATTACK" , cmd_cattack} , {"CDEFENSE" , cmd_cdefense} , {"CCOMBAT" , cmd_ccombat} ,
  {"CREPRO" , cmd_crepro} , {"CINTEL", cmd_cintel} ,
  {"CMAG_APT", cmd_cmag_apt} , {"CSTRENGTH", cmd_cstrength} ,
  {"CMORTALITY" , cmd_cmortality} , {"CPEOPLE_SECTOR" , cmd_cpeople_sector} ,
  {"COWNER_SECTOR" , cmd_cowner_sector} , {"CSOIL_SECTOR", cmd_csoil_sector} ,
  {"CMETAL_SECTOR", cmd_cmetal_sector} ,
  {"CJEWELS_SECTOR", cmd_cjewels_sector} ,
  {"CALTITUDE_SECTOR", cmd_caltitude_sector} ,
  {"NEW_ARMY_TYPE", cmd_new_army_type} , {"AMERGE", cmd_amerge} ,
  {"AINCREASE", cmd_aincrease},
  {"ASPLIT", cmd_asplit} , {"ADISBAND", cmd_adisband} ,
  {"CABONUS", cmd_cabonus} ,
  {"CFORT_SECTOR", cmd_cfort_sector} , {"CROADS_SECTOR", cmd_croads_sector} ,
  {"CPASS", cmd_cpass} , {"DESTROY", cmd_destroy},
  {"ACASTLE", cmd_acastle}
};

// forward declarations
int getexec(FILE *fp, struct argument args[]);
void load_options(Snation* np);
void run_exec_line(Snation *np, struct argument args[]);
int parse_exec_line(char line[], struct argument args[]);
int insert_army_nation(Snation *np, Sarmy *ap, int chosen_id);
int insert_army_sector(Ssector *sp, Sarmy *ap);
void delete_army_nation(Snation *np, Sarmy *ap);
void delete_army_sector(Ssector *sp, Sarmy *ap);

/*******************************************************/
/* gets nation data for nation 'id', put it into '*np' */
/* takes in an integer nation id, and a pointer to a   */
/* nation structure.                                   */
/*******************************************************/
void load_nation(id, np)
     int id;
     Snation *np;
{
  FILE *exec_file;
  struct argument args[N_EXEC_ARGS];
  char filename[NAMELEN];
  int argc, i;
  extern Suser user;

  *np = world.nations[id];
  user.np = np;		/* hmm.. must cleanup use of "Suser" in update */
    /* remember what the initial values were, before reading the exec file */
  user.init_money = user.np->money;
  user.init_metal = user.np->metal;
  user.init_jewels = user.np->jewels;
  user.init_food = user.np->food;
  user.id = np->id;
  load_options(np);

  sprintf(filename, "exec/exec%d", np->id);
  exec_file = fopen(filename, "r");
  if (!exec_file && np->npc_flag == 0) {
    fprintf(stderr, "Nation %d (%s) did not move this turn.\n", np->id, np->name); }
  else {
    while (argc = getexec(exec_file, args) != -1) {
      run_exec_line(np, args);
    }
    if (exec_file != NULL) { fclose(exec_file); }
  }
}

/****************************************************************/
/* this takes a pointer to the exec file and a pointer to an    */
/* array of argument structures.  The structures are filled in  */
/* with the arguments of the next line of the file.  The        */
/* function returns an integer which is the number of arguments */
/****************************************************************/
int getexec(fp, args)
FILE *fp;
struct argument args[];
{
  char buff[EXECLEN];
  if (fp == NULL) {
    return (-1);
  }
  if (fgets(buff, EXECLEN, fp) != NULL) {
    parse_exec_line(buff, args);
  }
  else return(-1);
}



  /* this takes the exec line args (already parsed) and runs it */
void run_exec_line(np, args)
     Snation *np;
     struct argument args[];
{
  int i, found = 0;

  for (i=0; i < sizeof(commands)/sizeof(struct exec_cmd); i++) {
    if (!(strcmp(commands[i].name, args[0].data.str))) {
      found = 1;
      (*commands[i].func)(np, args);
    }
  }
  if (!found) {
    printf("Error: exec command <%s> not implemented\n", args[0].data.str);
  }
}

#ifdef AMIGA
#define sscanf sscanf2
void sscanf2(data, terminate, result)
     char *data, *terminate, *result;
{
  int i;
  
  terminate += 3;
  for (;;) {
    for (i=0; i<strlen(terminate); i++)
      if (*data == terminate[i]) return;
    *result = *data;
    data++;
    result++;
    *result = '\0';
  }
}
#endif


  /* this parses a single exec line */
int parse_exec_line(line, args)
     char line[];
     struct argument args[];
{
  char cmd_str[NAMELEN], arg_tmp[NAMELEN];
  int count = 0, place;

    /* make sure there is a newline at the end of this line */
  if (line[strlen(line)-1] != '\n') {
    line[strlen(line)+1] = '\0'; /* extend the string */
    line[strlen(line)] = '\n'; /* put the newline in */
  }
  sscanf(line, "%[^:]", args[0].data.str);
  place = strlen(args[0].data.str)+1;
  args[0].type = TXT;
  do {
    sscanf(&line[place], "%[^:\n]", arg_tmp);
    place += strlen(arg_tmp)+1;
    count++;
    if (isdigit(arg_tmp[0]) || (arg_tmp[0] == '-')) {
      args[count].type = NUM;
      args[count].data.num = atoi(arg_tmp);
    }
    else {
      args[count].type = TXT;
      strcpy(args[count].data.str, arg_tmp);
    }
  } while (place+1 < strlen(line));
  return(count);
}



/******************************************************************/
/* this takes the name of a nation (a pointer to an array of      */
/* characters) and returns the id number of that nation.          */
/******************************************************************/
int get_nation_id(name)		/* finds id, given name */
     char name[];
{
  int i;

  for (i = 0; i < world.n_nations; ++i) {
    if (!strcmp(world.nations[i].name, name)) {
      return i;		/* found it! */
    }
  }
  return -1;			/* it's not there */
}

  /* rename a sector */
void cmd_sname(np, args)
     Snation *np;
     struct argument args[];
{
  int x, y;
  char name[NAMELEN];

  x = args[1].data.num;
  y = args[2].data.num;
  strcpy (name, args[3].data.str);
  
  if (debug) printf ("Naming sector %d,%d as %s.\n", x, y, name); 
  strcpy(world.map[x][y].name, name);
}

  /* rename an army */
void cmd_aname(np, args)
     Snation *np;
     struct argument args[];
{
  int id;
  char *name;
  Sarmy *ap, *get_army();

  id = args[1].data.num;
  name = args[2].data.str;
  
  if (ap = get_army(np, id)) {
    if (debug) printf ("Naming army %d as %s.\n", id, name);
    strcpy(ap->name, name);
  } else {
    printf("funny:  cannot find army %d\n", id);
  }
}


/* Move an army... change it's move points to the move points left, which
   is given in the exec string. */
void cmd_amove(np, args)
     Snation *np;
     struct argument args[];
{
  Sarmy *armypt;
  Ssector *sp;
  int a, x, y, fm;
  
  a = args[1].data.num;		/* army number */
  x = args[2].data.num;		/* new x coordinate */
  y = args[3].data.num;		/* new y coordinate */
  fm = args[4].data.num;	/* final moves */
  armypt = np->armies;
  while (armypt->id != a && armypt->next != NULL) armypt = armypt->next;
  if (armypt->id == a) {
    if (debug) {
      printf ("Moving army %d from %d,%d to %d,%d\n", a, armypt->pos.x,
	    armypt->pos.y, x, y);
    }
    sp = &(world.map[armypt->pos.x][armypt->pos.y]);
    delete_army_sector(sp, armypt);

    armypt->pos.x = x;
    armypt->pos.y = y;
    armypt->mvpts = fm;
    sp = &(world.map[x][y]);
    insert_army_sector(sp, armypt);
  }
  else printf ("Error! Could not find army!\n"); 
}


/* Change status of an army... if the army is being changed to OCCUPY mode,
   and it is _already_ in occupy mode, DO NOT change it's move ratio!
   Otherwise set the move ratio, change the moves left to zero, and
   change the status. */
void cmd_astat(np, args)
     Snation *np;
     struct argument args[];
{
  int a, s;
  Sarmy *armypt, *get_army();
  
  a = args[1].data.num;
  s = args[2].data.num;
  if (debug) printf ("Changing status of army %d to %d\n", a, s);
  armypt = get_army(np, a);
  if (armypt) {
    if (s == A_OCCUPY && armypt->status != A_OCCUPY) {
      armypt->mvratio =
	(int)(100 * (float)(armypt->mvpts) / basic_move_rate(np) );
      armypt->mvpts = 0;
    }
    armypt->status = s;
  } else {
    printf("Trying to change status on army %d which does not exist\n", a);
  }
}

  /* Set/clear a flag of an army, such as AF_HIDDEN, AF_FLIGHT and so on... */
void cmd_aflag_set(np, args)
     Snation *np;
     struct argument args[];
{
  int id, flag;
  Sarmy *armypt, *get_army();
  
  id = args[1].data.num;
  flag = args[2].data.num;
  if (debug) printf ("Setting bit %x of army %d\n", flag, id);
  if (armypt = get_army(np, id)) {
    armypt->flags |= flag;
  }
}

void cmd_aflag_clear(np, args)
     Snation *np;
     struct argument args[];
{
  int id, flag;
  Sarmy *armypt, *get_army();
  
  id = args[1].data.num;
  flag = args[2].data.num;
  if (debug) printf ("Clearing bit %x of army %d\n", flag, id);
  if (armypt = get_army(np, id)) {
    armypt->flags &= ~flag;
  }
}

  /* Set/clear a flag of a sector, such as SF_HIDDEN, SF_BUBBLE and so on... */
void cmd_flag_set_sector(np, args)
     Snation *np;
     struct argument args[];
{
  int x, y, flag;
  Ssector *sp;
  
  x = args[1].data.num;
  y = args[2].data.num;
  flag = args[3].data.num;
  sp = &world.map[x][y];
  sp->flags |= flag;
}

void cmd_flag_clear_sector(np, args)
     Snation *np;
     struct argument args[];
{
  int x, y, flag;
  Ssector *sp;
  
  x = args[1].data.num;
  y = args[2].data.num;
  flag = args[3].data.num;
  sp = &world.map[x][y];
  sp->flags &= ~flag;
}

  /* Change cargo of an army */
void cmd_acargo(np, args)
     Snation *np;
     struct argument args[];
{
  int id;
  Sarmy *armypt, *get_army();
  
  id = args[1].data.num;

  if (debug) printf ("Changing cargo of army %d\n", id);
  if ((armypt = get_army(np, id)) != NULL) {
    armypt->cargo.money = args[2].data.num;
    armypt->cargo.metal = args[3].data.num;
    armypt->cargo.jewels = args[4].data.num;
    armypt->cargo.food = args[5].data.num;
    armypt->cargo.people = args[6].data.num;
    armypt->cargo.army = args[7].data.num;
    armypt->cargo.title.x = args[8].data.num;
    armypt->cargo.title.y = args[9].data.num;
  } else {
    printf("[%s] Could not find army %d, nation %d\n", args[0].data.str,
	   id, np->id);
  }
}

void cmd_taxrate(np, args)
     struct argument args[];
     Snation *np;
{
  int t;

  t = args[1].data.num;  
  if (debug) printf ("Taxes changed from %d to %d.\n", np->taxes, t);
  np->taxes = t;
}



void cmd_charity(np, args)
     Snation *np;
     struct argument args[];
{
  int c;
  
  c = args[1].data.num;
  if (debug) printf ("Charity changed from %d to %d.\n", np->charity, c);
  np->charity = c;
}


void cmd_desig_sector(np, args)
     Snation *np;
     struct argument args[];
{
  int x, y, d;
  Snation *owner_np;

  x = args[1].data.num;
  y = args[2].data.num;
  d = args[3].data.num;		/* the new designation */
  owner_np = &world.nations[world.map[x][y].owner];
  if (debug) printf ("Redesignating %d,%d to %d.\n", x, y, d);
  world.map[x][y].designation = d;
    /* special case for capital: */
  if (d == D_CAPITAL) {
    owner_np->capital.x = x;	/* make sure Gamemaster does not get it */
    owner_np->capital.y = y;
  }
}


void cmd_amake(np, args)
     Snation *np;
     struct argument args[];
{
  Sarmy army, make_army();
  Ssector *sp;
  int a, no_sold, x, y;
  char *type, *name;
  
  a = args[1].data.num;		/* army number */
  no_sold = args[2].data.num;	/* number of soldiers drafted */
  x = args[3].data.num;		/* x coordinate where army is drafted */
  y = args[4].data.num;		/* y coordinate where army is drafted */
  type = args[5].data.str;	/* army type */
  name = args[6].data.str;	/* army name */

  sp = &(world.map[x][y]);
  army = make_army(type,name,no_sold,A_DEFEND,np->id,sp->loc);
/*  army.id  = free_army_id(np); */
  army.id = a;
  army.next = NULL;

  if (debug) printf("Permanently drafting army #%d\n",army.id);
  if (np->armies == NULL) {
    np->armies = (Sarmy *) malloc(sizeof(Sarmy));
    *(np->armies) = army;
  } else {
    insert_army_nation(np, &army, a);
  }
  insert_army_sector(sp, &army);
  ++np->n_armies;
}

void cmd_cmoney(np, args)
     Snation *np;
     struct argument args[];
{
  int change, nat_id;

  nat_id = args[1].data.num;
  change = args[2].data.num;	/* changed money */
  np = &world.nations[nat_id];
     
  np->money = np->money + change;
  if (debug) printf("New nation money (after difference of %d) = %d\n",
	 change,np->money);
}

void cmd_cmetal(np, args)
     Snation *np;
     struct argument args[];
{
  int change, nat_id;

  nat_id = args[1].data.num;
  change = args[2].data.num;   /* changed money */
  np = &world.nations[nat_id];
  
  np->metal = np->metal + change;
  if (debug) printf("New nation metal (after difference %d) = %d\n",
	 change,np->metal);
}

void cmd_cjewels(np, args)
     Snation *np;
     struct argument args[];
{
  int change, nat_id;

  nat_id = args[1].data.num;
  change = args[2].data.num;   /* changed money */
  np = &world.nations[nat_id];
  
  np->jewels = np->jewels + change;
  if (debug) printf("New nation jewel (after difference %d) = %d\n",
	 change,np->jewels);
}

void cmd_cspell_pts(np, args)
     Snation *np;
     struct argument args[];
{
  int change, nat_id;

  nat_id = args[1].data.num;
  change = args[2].data.num;   /* changed money */
  np = &world.nations[nat_id];
  
  np->spell_pts = np->spell_pts + change;
  if (debug) printf("New nation spell pts (after difference %d) = %d\n",
		    change, np->spell_pts);
}

void cmd_ctech_skill(np, args)
     Snation *np;
     struct argument args[];
{
  int change, nat_id;

  nat_id = args[1].data.num;
  change = args[2].data.num;   /* changed money */
  np = &world.nations[nat_id];
  
  np->tech_skill = np->tech_skill + change;
}

void cmd_cmag_skill(np, args)
     Snation *np;
     struct argument args[];
{
  int change, nat_id;

  nat_id = args[1].data.num;
  change = args[2].data.num;   /* changed money */
  np = &world.nations[nat_id];
  
  np->mag_skill = np->mag_skill + change;
}

void cmd_cfood(np, args)
     Snation *np;
     struct argument args[];
{
  int change, nat_id;

  nat_id = args[1].data.num;
  change = args[2].data.num;   /* changed money */
  np = &world.nations[nat_id];
  
  np->food = np->food + change;
  if (debug) printf("New nation food (after difference %d) = %d\n",
	 change,np->food);
}

void cmd_tech_money(np,args)
     Snation *np;
     struct argument args[];
{
  np->tech_r_d = args[1].data.num;
  if (debug) printf("Tech R&D money changed to %d\n",np->tech_r_d);
}


void cmd_tech_metal(np,args)
     Snation *np;
     struct argument args[];
{
  np->tech_r_d_metal = args[1].data.num;
  if (debug) printf("Tech R&D metal changed to %d\n",np->tech_r_d_metal);
}


void cmd_spy_money(np,args)
     Snation *np;
     struct argument args[];
{
     np->spy_r_d = args[1].data.num;
   }


void cmd_mag_money(np,args)
     Snation *np;
     struct argument args[];
{
  np->mag_r_d = args[1].data.num;
  if (debug) printf("Magic money changed to %d\n",np->mag_r_d);
}

void cmd_mag_jewels(np,args)
     Snation *np;
     struct argument args[];
{
  np->mag_r_d_jewels = args[1].data.num;
  if (debug) printf("Magic jewels changed to %d\n",np->mag_r_d_jewels);
}

void cmd_cmine(np, args)
     Snation *np;
     struct argument args[];
{
  np->mine_skill += args[1].data.num;
}
void cmd_cfarm(np, args)
     Snation *np;
     struct argument args[];
{
  np->farm_skill += args[1].data.num;
}
void cmd_cspeed(np, args)
     Snation *np;
     struct argument args[];
{
  np->race.speed += args[1].data.num;
}

void cmd_nation_name(np, args)
     Snation *np;
     struct argument args[];
{
  Snation *changed_np = &world.nations[args[1].data.num];
  strcpy(changed_np->name, args[2].data.str);
}

void cmd_nation_leader(np, args)
     Snation *np;
     struct argument args[];
{
  Snation *changed_np = &world.nations[args[1].data.num];
  strcpy(changed_np->leader, args[2].data.str);
}

void cmd_nation_order(np, args)	/* change magic order */
     Snation *np;
     struct argument args[];
{
  Snation *changed_np = &world.nations[args[1].data.num];
  strcpy(changed_np->mag_order, args[2].data.str);
}

void cmd_nation_mark(np, args)	/* change a nation's mark */
     Snation *np;
     struct argument args[];
{
  Snation *changed_np = &world.nations[args[1].data.num];
  changed_np->mark = args[2].data.str[0];
}

  /* set and clear the npc flag on a nation */
void cmd_set_npc(np, args)
     Snation *np;
     struct argument args[];
{
  Snation *changed_nation = &world.nations[args[1].data.num];

  changed_nation->npc_flag = 1;
}

void cmd_clear_npc(np, args)
     Snation *np;
     struct argument args[];
{
  Snation *changed_nation = &world.nations[args[1].data.num];

  changed_nation->npc_flag = 0;
}

void cmd_npc_param(np, args)
     Snation *np;
     struct argument args[];
{
  Snation *changed_nation = &world.nations[args[1].data.num];

  changed_nation->npc_agg = args[2].data.num;
  changed_nation->npc_exp = args[3].data.num;
  changed_nation->npc_iso = args[4].data.num;
}

  /* change in a nation's fight bonus */
void cmd_cattack(np, args)
     Snation *np;
     struct argument args[];
{
  np->attack += args[1].data.num;
}

void cmd_cdefense(np, args)
     Snation *np;
     struct argument args[];
{
  np->defense += args[1].data.num;
}

void cmd_ccombat(np, args)
     Snation *np;
     struct argument args[];
{
  np->attack += args[1].data.num;
  np->defense += args[1].data.num;
}

void cmd_crepro(np, args)
     Snation *np;
     struct argument args[];
{
  np->race.repro += args[1].data.num;
}

void cmd_cmortality(np, args)
     Snation *np;
     struct argument args[];
{
  if ((np->race.mortality += args[1].data.num) < 1) {
    np->race.mortality = 1;
  }
}

void cmd_cintel(np, args)
     Snation *np;
     struct argument args[];
{
  if ((np->race.intel += args[1].data.num) < 1) {
    np->race.intel = 1;
  }
}

void cmd_cmag_apt(np, args)
     Snation *np;
     struct argument args[];
{
  if ((np->race.mag_apt += args[1].data.num) < 1) {
    np->race.mag_apt = 1;
  }
}

void cmd_cstrength(np, args)
     Snation *np;
     struct argument args[];
{
  if ((np->race.strength += args[1].data.num) < 1) {
    np->race.strength = 1;
  }
}

  /* change the number of people in that sector */
void cmd_cpeople_sector(np, args)
     Snation *np;
     struct argument args[];
{
  Ssector *sp;
  int x, y;

  x = args[1].data.num;
  y = args[2].data.num;
  sp = &world.map[x][y];
  sp->n_people += args[3].data.num;
}
  /* change the owner in that sector */
void cmd_cowner_sector(np, args)
     Snation *np;
     struct argument args[];
{
  Ssector *sp;
  int x, y;

  x = args[1].data.num;
  y = args[2].data.num;
  sp = &world.map[x][y];
  sp->owner = args[3].data.num;
  addsector(&world.nations[sp->owner], x, y);
  if (debug)
    printf("sector (%d,%d) owned by %s\n", x, y,
	   world.nations[sp->owner].name);
}

  /* change the soil in that sector */
void cmd_csoil_sector(np, args)
     Snation *np;
     struct argument args[];
{
  Ssector *sp;
  int x, y;

  x = args[1].data.num;
  y = args[2].data.num;
  sp = &world.map[x][y];
  sp->soil += args[3].data.num;
}

  /* change the metal in that sector */
void cmd_cmetal_sector(np, args)
     Snation *np;
     struct argument args[];
{
  Ssector *sp;
  int x, y;

  x = args[1].data.num;
  y = args[2].data.num;
  sp = &world.map[x][y];
  sp->metal += args[3].data.num;
}
  /* change the jewels in that sector */
void cmd_cjewels_sector(np, args)
     Snation *np;
     struct argument args[];
{
  Ssector *sp;
  int x, y;

  x = args[1].data.num;
  y = args[2].data.num;
  sp = &world.map[x][y];
  sp->jewels += args[3].data.num;
}

  /* change the altitude in that sector */
void cmd_caltitude_sector(np, args)
     Snation *np;
     struct argument args[];
{
  Ssector *sp;
  int x, y;

  x = args[1].data.num;
  y = args[2].data.num;
  sp = &world.map[x][y];
  sp->altitude += args[3].data.num;
}

/* adds a new type of army to the nation's list of available army types */
void cmd_new_army_type(np, args)
     Snation *np;
     struct argument args[];
{
  Savail_army *aa;
  extern Suser user;
  int falready=0;

  /* Check to see that they don't already have the army type */
  aa = user.avail_armies;
  if (aa != NULL) {
    do {
      if (!strcmp (aa->type, args[1].data.str)) {
	falready = 1;
	break;
      }
    } while (aa->next && (aa=aa->next));
  }
  if (aa == NULL) {
    aa = (Savail_army *) malloc (sizeof(Savail_army));
    strcpy (aa->type, args[1].data.str);
  } else if (!falready) {
    do { ; } while (aa->next && (aa = aa->next));
    aa->next = (Savail_army *) malloc (sizeof (Savail_army));
    aa->next->next = NULL;
    strcpy(aa->next->type, args[1].data.str);
  }
  user.n_army_types++;
}

/* allows nation to construct a new type of object, such
   as increasing roads in a sector, or making land into water
   or vice-versa, or ships, or caravans.
*/
void cmd_new_construct(np, args)
     Snation *np;
     struct argument args[];
{
}

  /* adds the first army to the second.  Requires the two army ids. */
void cmd_amerge(np, args)
     Snation *np;
     struct argument args[];
{
  Sarmy *ap, *ap2;
  Ssector *sp;
  ap = get_army(np, args[1].data.num);
  ap2 = get_army(np, args[2].data.num);
  sp = &world.map[ap->pos.x][ap->pos.y];
  ap2->n_soldiers += ap->n_soldiers;
  ap2->sp_bonus = min(ap->sp_bonus, ap2->sp_bonus);
  ap2->mvpts = min(ap->mvpts, ap2->mvpts);
  ap2->mvratio = min(ap->mvratio, ap2->mvratio);
  delete_army_sector(sp, ap);
  delete_army_nation(np, ap);
}

  /* increases the army (of given id) by the given number of people */
void cmd_aincrease(np, args)
     Snation *np;
     struct argument args[];
{
  Sarmy *ap;

  ap = get_army(np, args[1].data.num);
  ap->n_soldiers += args[2].data.num;
  ap->mvpts = 0;
  ap->mvratio = 0;
}

  /* splits an army up.  requires armyid and number of men to split */
void cmd_asplit(np, args)
     Snation *np;
     struct argument args[];
{
  Sarmy *ap, army2, make_army(), *get_army();

  ap = get_army(np,args[1].data.num);
  army2 = make_army(ap->type, "", args[2].data.num, ap->status,
		    ap->owner, ap->pos);
  ap->n_soldiers -= args[2].data.num;
  army2.mvpts = ap->mvpts;
  army2.mvratio = ap->mvratio;
  army2.flags = ap->flags;
  army2.sp_bonus = ap->sp_bonus;
  army2.money_maint = ap->money_maint;
  army2.metal_maint = ap->metal_maint;
  army2.jewel_maint = ap->jewel_maint;
  army2.spell_pts_maint = ap->spell_pts_maint;
  sprintf(army2.name, "%s", army2.type, army2.id);
  np->n_armies++;
  insert_army_nation(np, &army2, -1);
  insert_army_sector(&world.map[army2.pos.x][army2.pos.y], &army2);
    /* to give them a name, we must know their id *after* they are inserted */
  ap = get_army(np, army2.id);
  sprintf(ap->name, "%s", army2.type, army2.id);
}

void cmd_adisband(np,args)
     Snation *np;
     struct argument args[];
{
  Sarmy *ap;
  Ssector *sp;
  ap = get_army(np,args[1].data.num);
  sp = &world.map[ap->pos.x][ap->pos.y];
    /* you don't get the population back from a spirit */
  if (!is_spirit(ap) && !is_in_transport(ap)) {
    sp->n_people += ap->n_soldiers;
  }
  delete_army_sector(sp, ap);
  delete_army_nation(np, ap);
}

void cmd_cabonus(np,args)		/* give an army more/less special bonus */
     Snation *np;
     struct argument args[];
{
  Sarmy *ap;
  int bonus_change;

  bonus_change = args[2].data.num;
  if (ap = get_army(np,args[1].data.num)) {
    ap->sp_bonus += bonus_change;
  }
}

  /* change fortification level of a sector */
void cmd_cfort_sector(np, args)
     Snation *np;
     struct argument args[];
{
  Ssector *sp;
  int x, y;

  x = args[1].data.num;
  y = args[2].data.num;
  sp = &world.map[x][y];
  sp->defense += args[3].data.num;
}

  /* change number of roads in a sector */
void cmd_croads_sector(np, args)
     Snation *np;
     struct argument args[];
{
  Ssector *sp;
  int x, y;

  x = args[1].data.num;
  y = args[2].data.num;
  sp = &world.map[x][y];
  sp->roads += args[3].data.num;
}

void cmd_cpass(np, args)
     Snation *np;
     struct argument args[];
{
  Snation *dest_np;
  int id;
  char *new_pass;

  id = args[1].data.num;
  new_pass = args[2].data.str;
  dest_np = &world.nations[id];
  strcpy(dest_np->passwd, new_pass);
}

void cmd_cur_mag_money (np, args)

Snation * np;
struct argument args [];
{
  np->cur_mag_r_d = args[1].data.num;
}

void cmd_cur_tech_money (np, args)

Snation * np;
struct argument args [];
{
  np->cur_tech_r_d = args[1].data.num;
}

void cmd_cur_mag_jewels (np, args)

Snation * np;
struct argument args [];
{
  np->cur_mag_r_d_jewels = args[1].data.num;
}

void cmd_cur_tech_metal (np, args)

Snation * np;
struct argument args [];
{
  np->cur_tech_r_d_metal = args[1].data.num;
}

void cmd_cur_spy_money (np, args)

Snation * np;
struct argument args [];
{
  np->cur_spy_r_d = args[1].data.num;
}

void cmd_destroy(np, args)
     Snation *np;
     struct argument args[];
{
  int id;

  id = args[1].data.num;
  destroy_nation(id);
}

void cmd_acastle (np, args)
     
Snation * np;
struct argument args [];
{
  int army_id;
  Ssector * sp;
  Ssector * capital = &world.map [np->capital.x][np->capital.y];
  Sarmy * ap;

  army_id = args[1].data.num;
  if ((ap = get_army (np, army_id)) == NULL) {
    printf ("funny, can't find army %d\n", army_id);
  }
  sp = &world.map [ap->pos.x][ap->pos.y];

  delete_army_sector (sp, ap);

  ap->pos.x = capital->loc.x;
  ap->pos.y = capital->loc.y;
  
  insert_army_sector (capital, ap);
}

  /* read in the exec file for the master nation; this is done by all
     users, in case the master has changed things for them.  It is not
     done by the master nation, since that happens automaticaly.
   */
void load_master_execs()
{
  int argc;
  Snation *np = &world.nations[0];
  FILE *exec_file, *fopen();
  struct argument args[N_EXEC_ARGS];
  char filename[NAMELEN];

  sprintf(filename, "exec/exec%d", np->id);
  exec_file = fopen(filename, "r");
  if (!exec_file && np->npc_flag == 0) {
    /* printf("Nation %d (%s) did not move this turn.\n", np->id, np->name); */
  } else {
    while (argc = getexec(exec_file, args) != -1) {
      run_exec_line(np, args);
    }
    if (exec_file != NULL) { fclose(exec_file); }
  }
}

