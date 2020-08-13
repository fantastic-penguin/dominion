/* trademenu.c - displays trade board and allows buying and selling of items */

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
#ifdef TRADEMENU

#include "dominion.h"
#include "misc.h"
#include <stdio.h>
#include <ctype.h>

extern Sworld world;
extern Suser user;
extern struct item_map tradeitems[];
FILE *trades, *bids;

struct trade { int natn, item1, amt1, item2, amt2, priv; };

trade_menu(nation)
{
  WINDOW *tw;
  char c;
  int done=0, done2=0, start, curn, i, num_trades;
  int num, amt;
  struct trade tlist[MAXTRADES], tr;
  int blist[MAXTRADES];
  
  while (!done) {
    num_trades = read_trades(tlist, blist);
    statline("hit space when done", "trade board");
    tw = newwin(LINES-2, COLS, 0, 0);
    wmove(tw, 0, COLS/2-10);
    wstandout(tw);
    wprintw(tw, "Trade Board for nation %s", world.nations[user.id].name);
    wstandend(tw);
    
    mvwprintw(tw, 2, 1, "T# | NATION           ITEM FOR SALE     |     MINIMUM BID       | YOUR BID");
    mvwprintw(tw, 3, 1, "== | ============  ==================== | ===================== | ========");
    
    start = 0;
    for (i = 0; i < (num_trades-start); i++) {
      tr = tlist[i+start];
      mvwprintw(tw, i+4, 0, "%3d | %-12s %8d %-12s | %8d %-12s | %8d",
		i+start, world.nations[tr.natn].name, tr.amt1,
		tradeitems[tr.item1].name, tr.amt2,
		tradeitems[tr.item2].name, blist[i+start]);
    }
    mvwprintw(tw, 19, 2, "Do you wish to (s)ell an item, or (b)uy an item? ");
    wrefresh(tw);
    
    done2 = 0;
    while (!done2) {
      c = getch();
      c = isupper(c) ? tolower(c) : c;
      switch(c) {
      case ' ':
	done = done2 = 1;
	break;
      case 's':
	tr.natn = user.id;
	tr.item1 = tr.item2 = tr.amt1 = tr.amt2 = tr.priv = 0;
	sell_what(tw, &tr);    /* find what to sell and amount */
	append_sale(tw, tr, num_trades);  /* write new trade to trade file */
	done2 = 1;
	break;
      case 'b':
	buy_what(tw, &num, &amt);	/* get users bid */
	append_bid(tw, num, amt); /* write new bid to bids file */
	done2 = 1;
	break;
      default:
	bad_key();
	break;
      }
    }
  }
  delwin(tw);
  touch_all_wins();
  refresh();
}


/* reads the list of current trades from the trades file into an array. */
int read_trades(list, list2)
     struct trade list[];
     int list2[];
{
  int n, i, t, a, nt;

  trades = fopen(TRADES_FILE, "r");
  i = 0;
  if (trades) {
    while (!feof(trades)) {
      fscanf(trades, "%d", &n);
      fscanf(trades, "%d %d %d %d %d %d\n", &list[n].natn, &list[n].item1,
	     &list[n].amt1, &list[n].item2, &list[n].amt2, &list[n].priv);
      i++;
    }
    fclose(trades);
  }
  nt = i;

  for (i = 0; i < MAXTRADES; i++)
    list2[i] = 0;
  bids = fopen(BIDS_FILE, "r");
  if (bids) {
    while (!feof(bids)) {
      fscanf(bids, "%d %d %d\n", &t, &n, &a);
      if (n == user.id)
	list2[t] = a;
    }
    fclose(bids);  
  }
  return(nt);
}


sell_what(tw, desc)
     WINDOW *tw;
     struct trade *desc;
{
  char str[80], ic1, ic2;
  int i, it1, am1, it2, am2, ret;

  i = 0;
  str[0] = '\0';

  while (strncmp(tradeitems[i].name, "END", 3)) {
    sprintf(&str[strlen(str)], "(%c) %s, ", tradeitems[i].mark, tradeitems[i].name);
    i++;
  }
  mvwprintw(tw, 19, 2, "%s", str);
  echo();
  mvwprintw(tw, 21, 2, "Which item type do you wish to sell? ");
  wrefresh(tw);
  ic1 = getch();
  it1 = whichitem(ic1);
  if (ret > 0) {
    mvwprintw(tw, 21, 2, "How much %s to sell? ", tradeitems[it1].name);
    wclrtoeol(tw);
    nocbreak();
    wrefresh(tw);
    ret = wscanw(tw, "%d", &am1);
    if (ret > 0) {
      mvwprintw(tw, 21, 2, "Which item do you want in return? ");
      wclrtoeol(tw);
      cbreak();
      wrefresh(tw);
      ic2 = getch();
      it2 = whichitem(ic2);
      if (ret > 0) {
	mvwprintw(tw, 21, 2, "Minimum %s you will accept? ", tradeitems[it2].name);
	wclrtoeol(tw);
	nocbreak();
	ret = wscanw(tw, "%d", &am2);
	if (ret > 0) {
	  desc->item1 = it1;
	  desc->amt1 = am1;
	  desc->item2 = it2;
	  desc->amt2 = am2;
	  desc->priv = 0;
	}
      }
    }
  }
  noecho();
  cbreak();
}


int whichitem(ch)
     char ch;
{
  int i=0;

  while (tradeitems[i].mark != ch) i++;
  return i;
}


buy_what(tw, t, a)
     WINDOW *tw;
     int *t, *a;
{
  int num, am, ret;

  wmove(tw, 19, 2);
  wclrtoeol(tw);
  mvwprintw(tw, 19, 2, "Which trade item do you wish to bid on? ");
  echo();
  nocbreak();
  wrefresh(tw);
  ret = wscanw(tw, "%d", &num);
  if (ret > 0) {
    mvwprintw(tw, 19, 2, "How much do you wish to bid? ");
    wclrtoeol(tw);
    ret = wscanw(tw, "%d", &am);
    if (ret > 0) {
      *t = num;
      *a = am;
    }
  }
  noecho();
  cbreak();
}


append_sale(tw, desc, number)
     WINDOW *tw;
     struct trade desc;
     int number;
{
  if ((trades = fopen (TRADES_FILE, "a")) == NULL)
  {
    fprintf(stderr,"Error: Cannot write to file %s\n",TRADES_FILE);
    clean_exit();
    exit(1);
  }
  fprintf(trades, "%d %d %d %d %d %d %d\n", number, desc.natn,
	  desc.item1, desc.amt1, desc.item2, desc.amt2, desc.priv);
  fclose(trades);
}


append_bid(tw, num, amt)
     int num, amt;
{
  if ( (bids = fopen(BIDS_FILE, "a") ) == NULL)
  {
    fprintf(stderr,"Error: Cannot write to file %s\n",BIDS_FILE);
    clean_exit();
    exit(1);
  }
  fprintf(bids, "%d %d %d\n", num, user.id, amt);
  fclose(bids);
}
#endif /* TRADEMENU */
