/* trade.c -- code to handle the trade board during an update and in the game
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
#include <stdio.h>

#define T_GOLD 1
#define T_METAL 2
#define T_JEWELS 3
#define T_FOOD 4

extern struct item_map tradeitems[];
extern int debug;
FILE *tradef, *bidf;

struct bid { int nation, amount; };

/* Called during update to resolve trades. Bids array is first read
   in. array index is transaction number. bids are read from the bids
   file and placed in the array. old bids are replaced by higher new
   bids. then the trades file is read in, and each is checked against the
   bids array.*/
/* Variables: t=transaction number, n=nation selling, a=amount selling,
   i=item selling, i2=item wanted, a2=min amount wanted, p=privacy info */
dotrades(wp)
     Sworld *wp;
{
  struct bid bids[MAXTRADES];
  int t, n, a, i, i2, a2, p;
  
  tradef = fopen("trades", "r");
  if (!tradef) {
    printf ("Nothing traded this turn\n");
    return(0);
  }
  bidf = fopen("bids", "r");
  if (!bidf) {
    printf ("Nothing bought this turn\n");
    if (tradef) fclose(tradef);
    return(0);
  }
  /* I am going to read in the bids file, picking best bids as we go
     along... i.e. if a bid is read in that is lower than an existing
     bid, it is ignored. */
  while (!feof(bidf)) {
    fscanf(bidf, "%d %d %d\n", &t, &n, &a);
    if (bids[t].amount < a) {
      bids[t].nation = n;
      bids[t].amount = a;
    }
  }
  while (!feof(tradef)) {
    fscanf(tradef, "%d %d %d %d %d %d %d\n", &t, &n, &i, &a, &i2, &a2, &p);
    if (bids[t].amount >= a2) {
      printf ("nation %d sells %d %s to nation %d for %d %s\n", n, a,
	      tradeitems[i].name, bids[t].nation, bids[t].amount, tradeitems[i2].name);
      swapgoods (&wp->nations[n], &wp->nations[bids[t].nation], a, i); /* subtract goods from one nation */
      swapgoods (&wp->nations[bids[t].nation], &wp->nations[n], a2, i2); /* add them to the other */
    }
  }
}



/* This actually adds or subtracts an item from a nation */
swapgoods (fromnat, tonat, amt, item)
     Snation *fromnat, *tonat;
     int amt, item;
{
  switch (tradeitems[item].mark) {
  case 'g':
    fromnat->money -= amt;
    tonat->money += amt;
    break;
  case 'm':
    fromnat->metal -= amt;
    tonat->metal += amt;
    break;
  case 'j':
    fromnat->jewels -= amt;
    tonat->jewels += amt;
    break;
  case 'f':
    fromnat->food -= amt;
    tonat->food += amt;
    break;
  default:
    printf ("TRADE ERROR:  Unknown commodity!\n");
    break;
  }
}  

