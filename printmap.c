 /* printmap.c -- print the nation */

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
#include <signal.h>
#include <math.h>

extern Sworld world;
extern Suser user;
extern int debug;
extern int viewall;
extern char libdir[];
extern int (*wrapx)(), (*wrapy)();
#ifdef BSD
extern char current_dir[];
#else /* BSD */
extern char *current_dir, *getcwd();
#endif
extern int euid, ruid;

int reverse_water=0;
int ascii_map=0;
int dash_sectors=0;
#ifdef ANDREW
int beroot = 1;
#endif

Sdiplo **allocate_diplo();
char *des();

main(argc, argv)
     int argc;
     char *argv[];
{
  char nation[NAMELEN], passwd[NAMELEN];
  Snation *np;
  Ssector *sp;
  int i, j, iw, jw, ii, jj;		/* for loops */
  int rel_xcenter, rel_ycenter, abs_xcenter, abs_ycenter;
  int map_size, visibility, c, ascii=0;
  int type1, type2, mark1, mark2;
  extern char *optarg;
  extern int optind;

  /* rel_center vars are map center coords relative to your capital */
  /* abs_center is the map center in absolute coordinates.  Secret! */
  /* map_size is the number of sectors to display across the map    */
  /* type1 is map shading type, type2 is desig || nation_mark text  */

  strcpy(libdir, DEF_LIBDIR);
#ifdef ANDREW
  while ((c = getopt(argc, argv, "d:uxaWb")) != EOF) {
#else
  while ((c = getopt(argc, argv, "d:xaWs")) != EOF) {
#endif
    switch (c) {
#ifdef ANDREW
     case 'u':
       beroot = 0;
       break;
#endif
    case 'a':			/* ascii instead of postscript */
      ascii = 1;
      break;
    case 'x':			/* debug mode */
      debug = 1;
      break;
    case 'd':			/* lib directory */
      strcpy(libdir, optarg);
      break;
    case 'W':			/* reverse above/below water */
      reverse_water = 1;
      break;
    case 's':			/* show invisible sectors dashed */
      dash_sectors = 1;
      break;
    }
  }

  /* now get some unix system information */
#ifdef ANDREW
  if (beroot == 1) {
    Authenticate();
    beGames();
  }
#else
  ruid = getuid();
  euid = geteuid();
#endif

  if (chdir(libdir) == -1) {
    fprintf(stderr,"Error: cannot cd to directory %s\n",libdir);
    clean_exit();
    exit();
  }
  dom_print_intro();
  load_army_types();
  load_spirit_types();
  get_user();			/* select & load nation */

  get_display_type(&type1, &type2); /* display type (nation, desig... */
  get_map_info(&rel_xcenter, &rel_ycenter, &map_size);

  abs_xcenter = user.np->capital.x + rel_xcenter;
  abs_ycenter = user.np->capital.y + rel_ycenter;

#ifdef UID_SECURITY
  setuid(ruid);
#endif
#ifdef ANDREW
  if (beroot == 1) {
    bePlayer();
    unAuth();
    beroot = 0;
  }
#endif
  
  ps_prolog(map_size, type1, type2);

  ii = 0;
  jj = map_size-1;

  for (j = abs_ycenter - map_size/2; j < abs_ycenter + map_size/2; j++) {
    for (i = abs_xcenter - map_size/2; i < abs_xcenter + map_size/2; i++) {
      iw = (*wrapx)(i,j);
      jw = (*wrapy)(i,j);
      visibility = user.visible_sectors[iw][jw];
      if (type1 != 99) {	/* do some kind of shading */
	user.display = type1;
	mark1 = which_mark(iw, jw, &user);
      }
      if (type2 != 99) {	/* do some text in sectors */
	user.display = type2;
	mark2 = which_mark(iw, jw, &user);
      }

      if (ascii_map) asc_mark(mark1, mark2, type1, type2, visibility);
      else ps_mark(ii, jj, mark1, mark2, type1, type2, visibility);
      ii++;
    }

    jj--; ii = 0;
    if (ascii_map) printf("\n");
  }
  if (!ascii_map) {
    draw_axes(map_size, rel_xcenter, rel_ycenter);
    if (type1 != 99)
      draw_key();
    printf("\nshowpage\n\n");
  }
}

asc_mark(m, v)
     int m, v;
{
}


draw_key()
{
  printf("\ngrestore\n");
  printf("/Times-Roman findfont 15 scalefont setfont\n");
  printf("0 .1 1.01 { /i exch def\n");
  printf("  /x 2.1 inch i 300 mul add def\n  /y 2 inch def\n");
  printf("  x y moveto 18 0 rlineto 0 18 rlineto 18 neg 0 rlineto\n");
  printf("  closepath gsave 1 i sub setgray fill grestore stroke\n");
  printf("  x 4 add y .2 inch sub moveto i 10 mul round cvi str cvs show\n");
  printf("  } for\n");
}


draw_axes(size, xc, yc)
     int size, xc, yc;
{
  int x, y, step=1;

  if (size > 50) step = 5;
  else printf("\nsmallfont\n");

  printf("\n/str 15 string def   /size %d def\n", size);
  printf("/minx %d def  /miny %d def\n", xc-size/2, yc-size/2);
  printf("0 %d size 1 sub { /i exch def /i2 i def\n", step);
  printf("   i   -0.8    i minx add str cvs 1 letter  /i i2 def\n");
  printf("   i   size    i minx add str cvs 1 letter  /i i2 def\n");
  printf("  -1   size i sub 1 sub i miny add str cvs 1 letter  /i i2 def\n");
  printf("  size size i sub 1 sub i miny add str cvs 1 letter\n");
  printf("} for\n");
}

ps_prolog(size, type1, type2)
     int size, type1, type2;
{
  printf("%%!\n");
  printf("/inch { 72 mul } def\n");
  printf("/center {dup stringwidth pop 2 div neg 0 rmoveto} bind def\n");
  printf("/smallfont { /Courier findfont 25 scalefont setfont } def\n\n");

  printf("/xaxis { /t exch def /x exch def gsave smallfont\n");
  printf("   x .8 neg t 1 letter   x %d t 1 letter\n", size);
  printf("   grestore } def\n");

  printf("/yaxis { /t exch def /y exch def gsave smallfont\n");
  printf("   1 neg y t 1 letter   %d y t 1 letter\n", size);
  printf("   grestore } def\n\n");

  printf("/box { newpath 0 0 moveto 0 64 lineto 64 64 lineto 64 0 lineto\n");
  printf("     closepath } def\n\n");

  printf("/graybox { /g exch def /y exch def /x exch def gsave\n");
  printf("     x inch y inch translate box g setgray fill grestore } def\n\n");

  printf("/emptybox { /y exch def /x exch def gsave\n");
  printf("   x inch y inch translate box 0 setgray stroke grestore } def\n\n");

  printf("/dashbox { /y exch def /x exch def gsave\n");
  printf("     x inch y inch translate [3 9] 1 setdash box\n");
  printf("     0 setgray stroke grestore } def\n");

  printf("/waterbox { /y exch def /x exch def gsave\n");
  printf("     x inch y inch translate  0 12 64 { /i exch def\n");
  printf("       0 i moveto  64 i lineto stroke \n");
  printf("     } for  grestore } def\n\n");

  printf("/letter { /g exch def /t exch def /y exch def /x exch def\n");
  printf("     gsave  x inch 20 add y inch 15 add translate 0 0 moveto\n");
  printf("     g 0.5 lt { 1 setgray } { 0 setgray } ifelse\n");
  printf("     t show grestore } def\n\n");

  printf("/Times-Roman findfont 20 scalefont setfont\n");
  printf("4.25 inch 10.5 inch moveto (%s) center show\n", user.np->name);
  printf("/Times-Roman findfont 15 scalefont setfont\n");
  printf("4.25 inch 10 inch moveto (%s/%s) center show\n", des(type1), des(type2));
  printf("/Courier findfont 50 scalefont setfont\n\n");
  printf("0.01 setlinewidth  gsave\n");
  printf("1 inch 3 inch translate\n");
  printf("%f %f scale\n\n", 6.5/size, 6.5/size);
}

ps_mark(x, y, m1, m2, t1, t2, vis)
     int x, y, m1, m2, t1, t2, vis;
{
  float gray;
  int other_char = '~';		/* which char represents 'other' world */

  if (debug)
    fprintf(stderr, "x=%d y=%d m1=%c m2=%c t1=%d t2=%d vis=%d\n",
	    x, y, m1, m2, t1, t2, vis);

  if (user.underwater)
    other_char = '.';

  if (vis > SEE_NOTHING) {
    if (m1 == other_char || m2 == other_char) {
      printf("%d %d waterbox\n", x, y);
    } else {
      if (t1 != 99) {		/* if we are doing shading */
	if (m1 == 'I' || m1 == '+') gray = 0.0;
	else gray = 1 - (float)(m1 - '0') / 10.0;
	if (gray == 1.0)	/* make blank white box visible */
	  printf("%d %d emptybox\n", x, y);
	else			/* print a box of proper gray shade */
	  printf("%d %d %3.2f graybox\n", x, y, gray);
      }
      if (t2 != 99) {		/* do some text */
	if (t1 == 99) {		/* haven't done anything yet, make box */
	  printf("%d %d emptybox\n", x, y);
	  gray = 1.0;
	}
	printf("%d %d (%c) %3.2f letter\n", x, y, m2, gray);
      }
    }
  } else {			/* invisible sectors are dashed */
    if (dash_sectors)
      printf("%d %d dashbox\n", x, y);
  }
}

critical()
{
}
noncritical()
{
}
cleanup()
{
}
clean_exit()
{
#ifdef ANDREW
  if (beroot == 1) {
    bePlayer();
    unAuth();
    beroot = 0;
  }
#endif
}

/* GET_MAP_INFO asks the user where to center the map, and what size
   it should be */
get_map_info(x, y, size, type1, type2)
     int *x, *y, *size, *type1, *type2;
{
  char buff[NAMELEN];
  int temp;

  *x = *y = 0;
  *size = 30;
  fprintf(stderr, "\nCenter the map around what sector?\n");
  fprintf(stderr, "       x coordinate [0]: ");
  gets(buff);
  if (strlen(buff)) {
    sscanf(buff, "%d", &temp);
    *x = temp;
  }
  fprintf(stderr, "       y coordinate [0]: ");
  gets(buff);
  if (strlen(buff)) {
    sscanf(buff, "%d", &temp);
    *y = temp;
  }
  fprintf(stderr, "\nHow many sectors across the map [30]: ");
  gets(buff);
  if (strlen(buff)) {
    sscanf(buff, "%d", &temp);
    if ( (float)(temp) / 2.0 != (float)(temp / 2) ) /* even number */
      temp++;
    *size = temp;
  }
  if (*size < 10) *size = 10;
}

/* GET_DISPLAY_TYPE get's the user's choice of the display parameter */
get_display_type(t1, t2)
     int *t1, *t2;
{
  char buff[NAMELEN], a;

  fprintf(stderr, "\np: population\t A: Altitude\t w: Climate\n");
  fprintf(stderr, "t: Terrain\t s: Soil\t m: Metal\n");
  fprintf(stderr, "j: Jewels\t M: Movecost\t N: None\n");
  fprintf(stderr, "\nSelect sector shading from above list [None]: ");

  gets(buff);
  a = buff[0];

  switch (a) {
  case 'p':
    *t1 = POPULATION;
    break;
  case 'A':
    *t1 = ALTITUDE;
    break;
  case 'w':
    *t1 = CLIMATE;
    break;
  case 't':
    *t1 = TERRAIN;
    break;
  case 's':
    *t1 = SOIL;
    break;
  case 'm':
    *t1 = METAL;
    break;
  case 'j':
    *t1 = JEWELS;
    break;
  case 'M':
    *t1 = MOVECOST;
    break;
  default:
    *t1 = 99;
    break;
  }

  fprintf(stderr, "\nn: Nation Mark\t d: Designation\t N: None\n");
  fprintf(stderr, "\nSelect sector text from above list [designation]: ");

  gets(buff);
  a = buff[0];

  switch (a) {
  case 'N':
    *t2 = 99;
    break;
  case 'n':
    *t2 = NATION_MARK;
    break;
  case 'd':
  default:
    *t2 = DESIGNATION;
    break;
  }
}

/* GET_USER actually takes care of reading in the world, selecting and
   loading a nation, and setting up data (such as visibility) */
get_user()
{
  char nation[NAMELEN], passwd[NAMELEN];
  int i;

  read_world(&world, WORLD_FILE);
  fprintf(stderr, "Enter nation name: ");
  getline(nation, NAMELEN);
  user.id = get_nation_id(nation);
  get_crypt_pass("Your password: ", passwd, NULL, NULL);
  if (strcmp(world.nations[user.id].passwd, passwd)) {
    fprintf(stderr, "\nInvalid password.  AND, *I* only give you one try!\n");
    exit(1);
  }
  user.np = &world.nations[user.id];

  user.avail_armies = NULL;
  get_avail_armies(&user, user.np->tech_skill);

  user.spell_list = NULL;
  user.spirit_list = NULL;
  get_spells(&user, user.np->mag_skill);
  get_spirits(&user, user.np->mag_skill);

  if (user.id != 0)		/* Gamemaster is already loaded   */
    load_nation(user.id, user.np); /* load their exec file etc    */

  user.underwater = 0;
  if (user.np->race.pref_alt < 0) /* set underwater preference   */
    user.underwater = 1;
  if (user.id == 0)		/* give Gamemaster clairvoyance */
    viewall = 1;

  user.diplo_matrix = allocate_diplo(world.n_nations);
  read_in_diplo(user.diplo_matrix, world.n_nations);

  user.visible_sectors = (int **) malloc(world.xmax*sizeof(int *));
  for (i = 0; i < world.xmax; ++i)
    user.visible_sectors[i] = (int *) malloc(world.ymax*sizeof(int));
  find_visible_sectors(user.visible_sectors);

  chdir("..");
}


dom_print_intro()
{
  fprintf(stderr, "\ndom_print - postscript map printer\n\n");
  fprintf(stderr, "Redirect output to file or pipe to printer.\n\n");
  fprintf(stderr, " -d dir = set lib directory\n");
  fprintf(stderr, " -W = invert above/below water\n");
  fprintf(stderr, " -s = show invisible sectors dashed\n\n");
}


char *des(x)
     int x;
{
  switch (x) {
  case POPULATION:
    return "Population";
  case ALTITUDE:
    return "Altitude";
  case CLIMATE:
    return "Climate";
  case TERRAIN:
    return "Terrain";
  case SOIL:
    return "Soil";
  case METAL:
    return "Metal";
  case JEWELS:
    return "Jewels";
  case MOVECOST:
    return "Movecost";
  case DESIGNATION:
    return "Designation";
  case NATION_MARK:
    return "Nation Mark";
  default:
    return " ";
  }
}


