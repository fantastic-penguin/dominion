  /* army.c -- generic, non-visual army stuff;  see also army.c */

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

#include <stdio.h>

  /* army and spirit types */
extern struct army_type *army_types;
extern struct spirit_type *spirit_types;

extern Sworld world;
extern Suser user;
extern int debug;
extern struct army_flags army_flags[];

Sarmy *get_army();

  /* this gets a string with the army status written
     out, and flags if the army is in a magical mode
   */
void get_army_status(ap, s)
     Sarmy *ap;
     char s[];
{
  int has_slash = 0, max_len, len ,offset, i;
  char *pos, *pos2;


  switch (ap->status) {
  case A_ATTACK:
    strcpy(s, "ATTACK");
    break;
  case A_DEFEND:
    strcpy(s, "DEFEND");
    break;
  case A_OCCUPY:
    strcpy(s, "OCCUPY");
    break;
  case A_PATROL:
    strcpy(s, "PATROL");
    break;
  case A_INTERCEPT:
    strcpy(s, "INTERCEPT");
    break;
  case A_GARRISON:
    strcpy(s, "GARRISON");
    break;
  case A_AMBUSH:
    strcpy(s, "AMBUSH");
    break;
  case A_TRADED:
    strcpy(s, "TRADED");
    break;
  default:
    strcpy(s, "UNKNOWN");
    break;
  }
  for (i = 0; i < 32; i++)
  {
    has_slash = add_flag(s,ap->flags,i,has_slash);
  }
  max_len = 11;
  if (ap->id < 100) max_len++;
  if (ap->id < 10) max_len++;
  if (ap->mvpts < 10) max_len++;

  if ((len = strlen(s)) > max_len) { 
    pos = (char *)strchr(s,'/');
    pos2 = s + (max_len - (len - (pos - s)));
    if (pos2 <= s) pos2 = s + 1;
    offset = (pos - pos2);
    while (pos2 + offset <= s + len) {
      *pos2 = *(pos2 + offset);
      pos2++;
    }
  }
}

int add_flag(s,flags,i,has_slash)
/*
   This routine adds to a status line s the flag associated with
   the i'th bit position in the flags, adding a slash if one has not
   already been added.  The flags corresponding to the bit positions
   are in ext.c in a global array.
*/
char *s;
int flags, i, has_slash;
{
  char s2 [2];
  
  if (flags & (0x1 << i)) {
    if (has_slash == 0) {
      strcat(s,"/");
      has_slash = 1;
    }
    sprintf (s2, "%c", army_flags [i].flag);
    strcat (s, s2);
  }
  return has_slash;
}

  /* given a nation and an army id, find the army (or NULL
     if there is no army with that id)
   */
Sarmy *get_army(np, id)
     Snation *np;
     int id;
{
  Sarmy *ap = np->armies;
  char s[100];

  if (id == -1) {
    return NULL;
  }
  while ((ap != NULL) && (ap->id != id)) {
/*    sprintf(s, "(while) ap->id=%d,id=%d", ap->id, id);
    printf("%s", s);
/*     statline2(s, "get_army debug"); */
/*    getch(); */


    ap = ap->next;
  }
/*  sprintf(s, "(out of while) ap->id=%d,id=%d", ap ? ap->id : -1, id);
  printf("%s", s);
/*  statline2(s, "get_army debug"); */
/*  getch(); */

  return ap;			/* should be NULL if none is found */
}

  /* calculate the cost to draft an army */
int army_cost(ap)
     Sarmy *ap;
{
  int index = army_type_index(ap->type);

  if (is_army(ap)) {
    return ap->n_soldiers*army_types[index].money_draft;
  } else {
    return 0;
  }
}

int army_cost_metal(ap)
     Sarmy *ap;
{
  int index = army_type_index(ap->type);
  if (is_army(ap)) {
    return ap->n_soldiers*army_types[index].metal_draft;
  } else {
    return 0;
  }
}

  /* insert the army into the nation's army list.
     if the id parameter is -1, then this function
     decides where to insert it.
   */
int insert_army_nation(np, ap, chosen_id)
     Snation *np;
     Sarmy *ap;
     int chosen_id;
{
  Sarmy *alist = np->armies, *tmp = np->armies, *prev = np->armies;

  if (alist == NULL) {
    return -1;
  }
    /* if the army id given is good, and not taken, then use it */
  if (chosen_id < 0 || get_army(np, chosen_id) != NULL) {
    ap->id = free_army_id(np);
    if (is_in_transport(ap)) {	/* trouble!! */
      printf("bad: this army is in transport, and I had to change its id\n");
    }
  } else {
    ap->id = chosen_id;
  }
#ifdef WAIT_A_WHILE
  /* a little touch-up to the flags to account for ships
     and other things with the inverse altitude flag
   */
  if (ap->flags & AF_INVERSE_ALT) {
    if (np->race.pref_alt >= SEA_LEVEL) {
      ap->flags &= ~AF_LAND;	/* remove the land flag */
      ap->flags |= AF_WATER;	/* give it water flag */
    } else {
      ap->flags &= ~AF_WATER;	/* remove the water flag */
      ap->flags |= AF_LAND;	/* give it the land flag */
    }
  }
#endif /* WAIT_A_WHILE */
  if (alist->id > ap->id) {		/* we must insert at the start */
    ap->next = alist;
    np->armies = (Sarmy *) malloc(sizeof(Sarmy));
    *np->armies = *ap;
    return 1;
  }
  while ((tmp != NULL) && (tmp->id < ap->id)) {
    prev = tmp;			/* keep the previous one */
    tmp = tmp->next;
  }
  ap->next = tmp;
  prev->next = (Sarmy *) malloc(sizeof(Sarmy));
  *prev->next = *ap;		/* copy it on */
  return 1;
}

  /* remove an army from a nation list of armies (if it IS in the list!) */
void delete_army_nation(np, ap)
     Snation *np;
     Sarmy *ap;
{
  Sarmy *tmp = np->armies, *prev = np->armies, *alist = np->armies;

  if (tmp == NULL) {		/* special case for emtpy list */
    np->n_armies = 0;
    return;			/* blah, nothing to delete */
  }
    /* see if the army to be deleted is the head of the list */
  if ((alist->owner == ap->owner) && (alist->id == ap->id)) {
      /* in this special case, copy the second army to
	 the first, then free up the second.
       */
    tmp = alist->next;		/* the next one must be saved for freeing */
    if (alist->next != NULL) {
      *alist = *(alist->next);
      free(tmp);
    } else {
      np->armies = NULL;
      free(alist);
    }
    np->n_armies--;
    return;
  }
    /* get hold of the army id we are looking for */
  while ( (tmp != NULL) &&
	 !( (tmp->owner == ap->owner) && (tmp->id == ap->id) ) ) {
    prev = tmp;			/* remember the previous element */
    tmp = tmp->next;
  }
    /* now we have the same army, so we can remove it */
  if (tmp == NULL) {
    return;			/* blah, we didn't find it */
  }
  prev->next = tmp->next;	/* this is the crucial step:  tmp is removed */
  free(tmp);			/* don't need the space */
  np->n_armies--;
}

  /* remove an army from a list of armies (if it IS in the list!) */
void delete_army_sector(sp, ap)
     Ssector *sp;
     Sarmy *ap;
{
  struct armyid *tmp = sp->alist, *prev = sp->alist;

  if (!sp->alist) {		/* if we have no armies here at all */
    return;
  }
  /* special case it if is the first army;
     also works if it is the ONLY army
   */
  if ((tmp->owner == ap->owner) && (tmp->id == ap->id)) {
    sp->alist = sp->alist->next;
    free(tmp);
    return;			/* done!! */
  }

  while (tmp != NULL) {
    if ((tmp->owner == ap->owner) && (tmp->id == ap->id)) { /* found it! */
      prev->next = tmp->next;
      free(tmp);		/* free the space */
      return;			/* done!! */
    }
    prev = tmp;
    tmp = tmp->next;
  }
}

  /* take a sector and an army, and put the army in the sector's
     army list
   */
int insert_army_sector(sp, ap)
     Ssector *sp;
     Sarmy *ap;
{
  struct armyid *tmp;

  /* first make sure that the army is not yet in this secotr!! */
  if (army_is_in_sector(sp, ap->owner, ap->id)) {
    return 0;
  }
  if (ap->pos.x != sp->loc.x || ap->pos.y != sp->loc.y) {
    printf("major error:  ap->pos != sp->loc in insert_army_sector\r\n");
    refresh();
  }
  ap->pos = sp->loc;		/* make sure army's position is right */
  if (sp->alist == NULL) {	/* special case: empty list */
    sp->alist = (struct armyid *) malloc(sizeof(struct armyid));
    sp->alist->owner = ap->owner;
    sp->alist->id = ap->id;
    sp->alist->next = NULL;
    return 1;
  }
    /* insert at the beginning */
  tmp = (struct armyid *) malloc(sizeof(struct armyid));
  tmp->owner = ap->owner;
  tmp->id = ap->id;
  tmp->next = sp->alist;
  sp->alist = tmp;		/* this is the new beginning */
  return 1;
}

  /* generate an army, in given sector, of
     given type, with given status.  return the army.
     do not add it to the nation's linked list at this point.
   */
Sarmy make_army(type, name, n_soldiers, status, owner, loc)
     char type[], name[];
     int n_soldiers, status, owner;
     Pt loc;
{
  Sarmy new_army;
  Snation *np = &world.nations[owner];
  int aindex, sindex;		/* index in army/spirit type list */
  struct army_type this_atype;
  struct spirit_type this_stype;
  Sspirit *spiritp = user.spirit_list; /* in case it is a spirit */

  aindex = army_type_index(type);
  sindex = spirit_type_index(type);
  if (aindex >= 0) {		/* it is an army!! */
    this_atype = army_types[aindex];
  } else if (sindex >= 0) {	/* it is a spirit */
    this_stype = spirit_types[sindex];
    while (spiritp && (strcmp(type, spiritp->type) != 0)) {
      spiritp = spiritp->next;
    }
    if (spiritp == NULL)  {
      fprintf(stderr,"Error: spirit type %s not available\n",type);
    }
  }

  strcpy(new_army.type, type);
  strcpy(new_army.name, name);
  new_army.n_soldiers = n_soldiers;
  new_army.status = status;
  new_army.owner = owner;
  new_army.mvratio = 100;	/* 100% to start with */
  new_army.pos = loc;
  new_army.id = 0;       /* this is set when army is inserted into list */
  new_army.flags = 0x00L;
    /* when drafted, new armies have only 1/4 of full move points */
  new_army.mvpts = army_move_rate(np, &new_army)/4;
  if (is_army(&new_army)) {	/* case army */
    new_army.sp_bonus = this_atype.bonus;
    new_army.money_maint = this_atype.money_maint;
    new_army.metal_maint = this_atype.metal_maint;
    new_army.jewel_maint = this_atype.jewel_maint;
    new_army.spell_pts_maint = this_atype.spell_pts_maint;
    new_army.flags = this_atype.flags;
  } else if (is_mage(&new_army)) { /* case mage */
    new_army.sp_bonus = 0;
    new_army.money_maint = 0;
    new_army.metal_maint = 0;
    new_army.jewel_maint = MAGE_JEWELS_MAINT;
    new_army.spell_pts_maint = 0;
  } else {			/* if not, it is a spirit */
    new_army.sp_bonus = this_stype.bonus;
    new_army.money_maint = 0;
    new_army.metal_maint = 0;
    new_army.jewel_maint = 0;
/*    new_army.spell_pts_maint = this_stype.spell_pts_draft/4; */
    new_army.spell_pts_maint = (spiritp->cost * 1000000)/(4* this_stype.size );
    new_army.flags |= this_stype.flags;
  }
    /* special handling of the INVERSE_ALT flag:  if they
       are a land race, it becomes a WATER flag;  if they
       are a water race, it becomes a LAND flag.
     */
  if (new_army.flags & AF_INVERSE_ALT) {
    if (np->race.pref_alt >= SEA_LEVEL) {
      new_army.flags |= AF_WATER;
    } else {
      new_army.flags |= AF_LAND;
    }
  }
  new_army.next = NULL;
  memset(&new_army.cargo, '\0', sizeof(Scargo));
  new_army.cargo.army = -1;
  new_army.cargo.title.x = -1;
  new_army.cargo.title.y = -1;

  return new_army;
}

  /* this function tells you the basic army move rate for a nation */
int basic_move_rate(np)
     Snation *np;
{
  return 4 + np->race.speed/8;	/* should not be less than 4 */
}

  /* return 1 if given army can go into occupy mode, 0 otherwise */
int can_occupy(ap)
     Sarmy *ap;
{
  if (ap->n_soldiers >= OCCUPYING_SOLDIERS && !is_in_transport(ap)) {
    return 1;
  } else {
    return 0;
  }
}

int can_patrol(ap)
     Sarmy *ap;
{
  if (ap->n_soldiers >= OCCUPYING_SOLDIERS/2) {
    return 1;
  } else {
    return 0;
  }
}

int can_intercept(ap)
     Sarmy *ap;
{
  if (ap->n_soldiers >= OCCUPYING_SOLDIERS/2) {
    return 1;
  } else {
    return 0;
  }
}

int can_garrison(ap)
     Sarmy *ap;
{
  return 1;
}

  /* loads army types from the army types file */
void load_army_types()
{
  FILE *fp, *fopen();
  char line[210];
  int i;

  if ((fp = fopen(ARMY_TYPES_FILE, "r")) == NULL) {
    printf("cannot open army types file.  quitting.\n");
    clean_exit();
    exit(1);
  }

  do {
    fgets(line, 200, fp);
  } while (line[0] == '#');
    /* we should have the line with the number of army types in file */
  sscanf(line, "%d", &user.n_army_types);

  army_types =
    (struct army_type *)malloc(user.n_army_types*sizeof(struct army_type));

  for (i = 0; i < user.n_army_types; ) {
    fgets(line, 100, fp);
    line[strlen(line)-1] = '\0';
    if (line[0] != '#') {		/* ignore comments */
      sscanf(line,
          "%s : %1s : %f : %d : %d : %d : %d : %d : %d : %d : %d : %s",
	     army_types[i].type, &army_types[i].type_char,
	     &army_types[i].move_factor,
	     &army_types[i].bonus, &army_types[i].money_draft,
	     &army_types[i].metal_draft, &army_types[i].jewel_draft,
	     &army_types[i].money_maint,
	     &army_types[i].metal_maint, &army_types[i].jewel_maint,
	     &army_types[i].flags,
	     army_types[i].draft_places);
      army_types [i].spell_pts_maint = 0; /* Kludge, but ALWAYS so... */
      ++i;
    }
  }

  fclose(fp);
}

  /* returns true if this nation can draft that type of army */
int is_avail_army_type(up, type)
     Suser *up;
     char type[];
{
  int i;
  Savail_army *aa;

    /* run through the list of available armies */
  for (aa = up->avail_armies; aa != NULL; aa = aa->next) {
    if (strncmp(aa->type, type, NAMELEN) == 0) {
      return 1;			/* got it!!! */
    }
  }
  return 0;			/* we did not find that army type */
}

  /* returns the index in army_types, corresponding to
     an army type, or -1 on error (if it is not an army
     but, say, a caravan or spirit or mage).
   */
int army_type_index(type)
     char type[];
{
  int i;
  for (i = 0; i < user.n_army_types; ++i) {
    if (strncmp(army_types[i].type, type, NAMELEN) == 0) {
      return i;			/* got it!! */
    }
  }
  return -1;			/* didn't find it */
}

  /* returns the index in spirit_types, corresponding to
     an army type, or -1 on error (if it is not an army
     but, say, a caravan or army or mage).
   */
int spirit_type_index(type)
     char type[];
{
  int i;
  for (i = 0; i < user.n_spirit_types; ++i) {
    if (strncmp(spirit_types[i].type, type, NAMELEN) == 0) {
      return i;			/* got it!! */
    }
  }
  return -1;			/* did not get it */
}

  /* returns 1 if it is an army (not a navy), 0 otherwise */
int is_army(ap)
     Sarmy *ap;
{
  if (army_type_index(ap->type) >= 0) {
    return 1;
  }
  return 0;
}

  /* returns 1 if it is a ship, 0 otherwise */
int is_navy(ap)
     Sarmy *ap;
{
    /* any army type beginning with "Ships" or "S_" is a navy */
  if (ap && strncmp(ap->type, "S_", strlen("S_")) == 0) {
    return 1;
  }
  if (ap && strncmp(ap->type, "Ships", strlen("Ships")) == 0) {
    return 1;
  }
  return 0;
}

  /* returns 1 if it is an mage, 0 otherwise */
int is_mage(ap)
     Sarmy *ap;
{
  if (ap && (strcmp(ap->type, "Mage") == 0 || is_wizard(ap))) {
    return 1;
  }
  return 0;
}

  /* returns 1 if it is an spirit, 0 otherwise */
int is_spirit(ap)
     Sarmy *ap;
{
  if (spirit_type_index(ap->type) >= 0) {
    return 1;
  }
  return 0;
}

  /* returns the army move rate for a specific army */
int army_move_rate(np, ap)
     Snation *np;
     Sarmy *ap;
{
  int mvpts, i;

  if (is_army(ap)) {
    i = army_type_index(ap->type);
    mvpts = basic_move_rate(np)*army_types[i].move_factor;
  } else if (is_mage(ap)) {
    mvpts = basic_move_rate(np)*MAGE_MOVE_FACTOR;
  } else {			/* must be a spirit */
    if ((i = spirit_type_index(ap->type)) < 0) {
      printf(
	"army_move_rate:  bad error, can't find army type <%s> for army %d.\n",
	     ap->type, ap->id);
      printf("setting move rate to basic.\n");
      mvpts = basic_move_rate(np);
    } else {
      mvpts = basic_move_rate(np)*spirit_types[i].move_factor;
    }
  }
  return mvpts;
}

  /* check if a army "id" belonging to owner "owner" is in "sp: */
int army_is_in_sector(sp, owner, id)
     Ssector *sp;
     int owner, id;		/* owner and id of army we are testing */
{
  struct armyid *alist = sp->alist;

  while (alist != NULL) {
    if (alist->id == id && alist->owner == owner) {
      return 1;
    }
    alist = alist->next;
  }
  return 0;
}

  /* reads in the NEW_ARMY_TYPE lines from the techno_levels
     file, and adds them to the user's list of available army
     types if the user has enough techno skill to deserve them.
     Some of the available army types also come from the "races"
     file, since they are race-specific.
   */
int get_avail_armies(up, skill)
     Suser *up;
     int skill;
{
  FILE *fp, *fopen();
  int level;
  char type[NAMELEN], name[NAMELEN], line[EXECLEN];
  char magfn [EXECLEN];
  int done = 0;

  if ((fp = fopen(TECHNO_FILE, "r")) == NULL) {
    printf("could not open technology file %s\n", TECHNO_FILE);
    return -1;
  }

  while (!done) {
    if (fgets(line, EXECLEN, fp) == NULL) {
      done = 1;
      break;
    }
    if (line[0] != '#') {
      sscanf(line, "%s%d", name, &level); /* a new tech entry */
      if (level <= skill) {
	do {
	  fgets(line, EXECLEN, fp);
	  if (strncmp(line+2,"NEW_ARMY_TYPE:",strlen("NEW_ARMY_TYPE:")) == 0) {
	    add_army_type(up, line+2+strlen("NEW_ARMY_TYPE:"));
	  }
	} while (strncmp(line, "end", strlen("end")) != 0);
      } else {			/* else, we are beyond our skill */
	done = 1;
	break;
      }
    }
  }
  fclose(fp);

  /* Get the magic orders special armies in there too */

  sprintf (magfn, "magic/mag_%s", up->np->mag_order);
  if ((fp = fopen (magfn, "r")) == NULL) {
    printf ("could not open magic file %s\n", magfn);
    return -1;
  }

  done = 0;
  while (!done) {
    if (fgets (line, EXECLEN, fp) == NULL) {
      done = 1;
      break;
    }
    if (strncmp(line,"EXEC:NEW_ARMY_TYPE:",strlen("NEW_ARMY_TYPE:")) == 0) {
      add_army_type(up, line+strlen("EXEC:NEW_ARMY_TYPE:"));
    }
  }

    /* get race-specific army types. for now there is
       only one army type for each race.
     */
  if ((fp = fopen(RACES_FILE, "r")) == NULL) {
    printf("could not open races file %s\n", RACES_FILE);
    return -1;
  }
  done = 0;
  while (!done) {
    if (fgets(line, EXECLEN, fp) == NULL) {
      done = 1;
      break;
    }
    if (strncmp(line, up->np->race.name, strlen(up->np->race.name)) == 0
	&& strncmp(line+strlen(up->np->race.name), "_armies:",
		   strlen("_armies:")) == 0) {
      /* got one!!  in the next line we assume there
	 is just one army type for each race.
       */
      add_army_type(up, strchr(line, ':')+1);
    }
  }
  fclose(fp);
}

  /* add an army type to the user's list */
void add_army_type(up, type)
     Suser *up;
     char type[];
{
  Savail_army *aa = up->avail_armies, *prev_aa = up->avail_armies;
  int falready = 0;
  if (type[strlen(type)-1] == '\n') {
    type[strlen(type)-1] = '\0';
  }
  if (debug) {
    printf("adding army type: <%s>; type return\n", type);
    fflush(stdout);
    getchar();
  }
  if (aa == NULL) {		/* if list is empty... */
    up->avail_armies = (Savail_army *)malloc(sizeof(Savail_army));
    aa = up->avail_armies;
    strcpy(aa->type, type);
    aa->next = NULL;
    return;
  }
  while (aa != NULL) {		/* else, find end of list */
    if (!strcmp (aa->type, type)) { /* If army type already in there... */
      falready = 1;
      break;
    }
    prev_aa = aa;
    aa = aa->next;
  }
  if (!falready) {
    prev_aa->next = (Savail_army *)malloc(sizeof(Savail_army));
    strcpy(prev_aa->next->type, type);
    prev_aa->next->next = NULL;
  }
}

int sect_n_armies(sp)
     Ssector *sp;
{
  int n = 0;
  struct armyid *alist = sp->alist;

  while (alist != NULL) {
    ++n;
    alist = alist->next;
  }
  return n;
}

  /* returns a pointer to the first mage found in a nation */
Sarmy *get_first_mage(np)
     Snation *np;
{
  Sarmy *tmp_ap = np->armies;

  while (tmp_ap) {
    if (is_mage(tmp_ap)) {
      return tmp_ap;
    }
    tmp_ap = tmp_ap->next;
  }
  return tmp_ap;
}

  /* returns the id of the first mage belonging
     to a given nation, found in a given sector.
     returns -1 if there is no mage there.
   */
int first_sect_mage_id(np, sp)
     Snation *np;
     Ssector *sp;
{
  struct armyid *alist = sp->alist;
  Sarmy *ap;

  while (alist) {
    ap = get_army(np, alist->id);
    if (is_mage(ap)) {
      return alist->id;
    }
    alist = alist->next;
  }
  return -1;
}

  /* returns the apparent army type for a disguised army */
void get_apparent_type(ap, type)
     Sarmy *ap;
     char type[];
{
  char *pos;

  if (is_disguised(ap)) {
    pos = (char *)strrchr(ap->name, '/');
    if (pos == NULL) {
      strcpy(type, ap->type);
    } else {
      strcpy(type, pos+1);	/* disguised type is stored in ap->name */
    }
  } else {
    strcpy(type, ap->type);	/* no disguise: army is itself */
  }
}

int get_spell_pts_maint(ap)
  Sarmy *ap;
{

  return (int)(((double)(ap->spell_pts_maint* ap->n_soldiers)/1000000.0)+0.49);
}
