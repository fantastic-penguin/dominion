/* news.h - structures used in the News system */
#ifndef _NEWS_H_
#define _NEWS_H_

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

struct S_Group {
  char name[NAMELEN];   /* The name of the group */
  int  first,last;     /* The numbers of the first and last article numbers */
  char postable;       /* A 'boolean'-Can the players post to the group? */
  struct S_Group *next;
}; 
typedef struct S_Group s_group;

struct S_Article {
  int art_num;
  char date[NAMELEN];
  char sender[2*NAMELEN+4];	/* "leader of nation" */
  char subject[EXECLEN];
  int read;   /* Has the article been read? */
};
typedef struct S_Article s_article;

#endif // _NEWS_H_
