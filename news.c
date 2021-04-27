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
#include "news.h"
#include "functions.h"
#include <stdio.h>
#ifdef AMIGA
#  include <exec/types.h>
#  define sleep(x) Delay(x*50)
#else
#  include <sys/types.h>
#endif
#include <time.h>
#ifdef SYSV
#  include <string.h>
#else
#  include <strings.h>
#endif
#include <unistd.h>
#include <stdlib.h>

extern Sworld world;
extern Suser user;

char *news_post()
{
    FILE *file;
    static char buffer[1024];
    char *ptr;

    sprintf(buffer, "news.post");
    if ((file = fopen(buffer, "r"))) {
	fgets(buffer, sizeof(buffer), file);
	fclose(file);
	ptr = buffer + strlen(buffer) - 1;
	if (*ptr == '\n')
	    *ptr = '\0';
	return buffer;
    }
    else
	return NULL;
}

int valid_group(name,posting)
char *name;
int posting;
{
  char news_dir[100],g_fn[100],in[100];
  char post_in;
  FILE *g_fp;
  int ret = 0;
  char out[100];
  char s_in[100];

  sprintf(news_dir,"%s",NEWS_DIR);
  sprintf(g_fn,"%s/%s",news_dir,NGDB_FILE);
  
  g_fp=fopen(g_fn,"r");
  if (g_fp!=NULL)
/*    while((fscanf(g_fp,"%s %*d %*d %*c",in))>0)*/
    while((fgets(s_in,100,g_fp))!=NULL)
      {
	s_in[strlen(s_in)-1] = '\0';
	sscanf(s_in,"%s %*d %*d %c",in,&post_in);
	if((!posting)&&(!strcmp(in,name)))
	  ret=1;
	else
	  if((posting)&&(!strcmp(in,name))&&(post_in=='1'))
	    ret=1;
      }
  if (g_fp != NULL) { fclose(g_fp); }
  return(ret);
} /* valid_group */
	
s_group *group_find(first,name)
s_group *first;
char *name;
{
  s_group *loop,*ret;
  ret=NULL;
  for (loop=first;loop!=NULL;loop=loop->next)
    if (!strcmp(name,loop->name))
      ret=loop;
  return(ret);
} /* group_find */
      
/* This checks if someone else is adding an article (ie it's locked) */

int check_news_lock()
{
  FILE *lock_fp;
  char lock_fn[100];
  char news_dir[100];
  int ret;

  sprintf(news_dir,"%s",NEWS_DIR);
  sprintf(lock_fn,"%s/%s.lock",news_dir,NGDB_FILE);
  if ((lock_fp=fopen(lock_fn,"r"))!=NULL) {
    ret=0;
    fclose(lock_fp);
  } else {
    ret=1;
  }
  return(ret);
}   /* check_news_lock */

void lock_news()
{
  FILE *lock_fp;
  char news_dir[100],lock_fn[100];

  sprintf(news_dir,"%s",NEWS_DIR);
  sprintf(lock_fn,"%s/%s.lock",news_dir,NGDB_FILE);
  if ((lock_fp=fopen(lock_fn,"w"))!=NULL)
    {
      fprintf(lock_fp,"locked\n");
      fclose(lock_fp);
    }
}  /* lock_news */

void unlock_news()
{
  char news_dir[100],lock_fn[100];
  sprintf(news_dir,"%s",NEWS_DIR);
  sprintf(lock_fn,"%s/%s.lock",news_dir,NGDB_FILE);
  unlink(lock_fn);
}  /* unlock_news */

void group_insert(first,temp)
s_group **first,*temp;
{
  s_group *loop;
  
  if (*first==NULL)
    {
      *first=temp;
    }
  else
    {
      for (loop=(*first);loop->next!=NULL;loop=loop->next);
      loop->next=temp;
    }
} /* group_insert */

post_news_file(news_file,group_name,subject,id)
char *news_file,*group_name,*subject;
int id;
{
  FILE *in,*out;
  FILE *g_fp;
  char news_dir[100],g_fn[100],g_dir[100],a_fn[100];
  s_group *g_first,*g_temp;
  char g_name_in[100],post_in;
  int first_in,last_in;
  int news_locked;
  char cmd[1000];
  char s_in[100];
  FILE *a_fp;
  char *forward;

  g_first=NULL;

  sprintf(news_dir,"%s",NEWS_DIR);
  sprintf(g_fn,"%s/%s",news_dir,NGDB_FILE);
  sprintf(g_dir,"%s/%s",news_dir,group_name);
  if (!(news_locked=check_news_lock()))
    {
      sleep(2);
      news_locked=check_news_lock();
    }
  if (news_locked==1)
    {
      lock_news();
      if ((g_fp=fopen(g_fn,"r")) == NULL) {
	printf(
            "\nCould not open file %s for reading; you should check it out\n",
	       g_fn);
      } else {
/*while((fscanf(g_fp,"%s %d %d %c",g_name_in,&first_in,&last_in,&post_in))>0)*/
	while ((fgets(s_in,100,g_fp))!=NULL) {
	  s_in[strlen(s_in)-1] = '\0';
	  sscanf(s_in,"%s %d %d %c",g_name_in,&first_in,&last_in,&post_in);
	  g_temp=(s_group *)malloc(sizeof(s_group));
	  strcpy(g_temp->name,g_name_in);
	  /*	  sprintf(*g_temp->name,"%s",g_name_in); */
	  g_temp->first=first_in;
	  g_temp->last=last_in;
	  g_temp->postable=post_in;
	  g_temp->next=NULL;
	  group_insert(&g_first,g_temp);
	}
	fclose(g_fp);
      }
      g_temp=group_find(g_first,group_name);
      if (g_temp==NULL) {
	fprintf(stderr,"Bad group name: %s\n",group_name);
      } else {
	  /* sprintf(a_fn,"%s/%d",g_dir,g_temp->last+1);
	  sprintf(cmd,"mv %s %s",news_file,a_fn);
	  system(cmd);
	  */
	sprintf(a_fn,"%s/%d",g_dir,g_temp->last+1);
	a_fp=fopen(a_fn,"w");
	if (a_fp==NULL) {
	  fprintf(stderr,"Error Opening New Article File Pointer\n");
	  return(1);
	}
	fprintf(a_fp,"Date: Thon %d\n",world.turn);
	if (id==0) {
	  fprintf(a_fp,"From: Update\n");
	} else {
	  fprintf(a_fp,"From: %s\n",world.nations[id].name);
	  fprintf(a_fp,"Author: %s of %s\n",world.nations[id].leader,
		  world.nations[id].name);
	}
	fprintf(a_fp,"Subj: %s\n\n",subject);
	insert(news_file,a_fp);
	if (a_fp != NULL) { fclose(a_fp); }
	unlink(news_file);
	(g_temp->last)++;
      }
      if ( (forward = news_post()) != NULL) {
	sprintf(cmd, "cat %s | %s '%s'", a_fn, MAILER, forward);
	system(cmd);
      }
      g_fp=fopen(g_fn,"w");
      for (g_temp=g_first;g_temp!=NULL;g_temp=g_temp->next) {
	fprintf(g_fp,"%s %d %d %c\n",g_temp->name,g_temp->first,
		g_temp->last,g_temp->postable);
      }
      if (g_fp != NULL) { fclose(g_fp); }
      unlock_news();
    } /* News_locked==1 */
} /* Post_news_file */
