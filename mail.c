  /* mail.c -- dominion mail system */

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
#include <stdio.h>
#ifdef AMIGA
# include <exec/types.h>
# include <string.h>
#else
# include <sys/types.h>
#endif

#include <time.h>
#ifdef SYSV
# include <string.h>
#else
# include <strings.h>
#endif

#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include "functions.h"

extern Suser user;
extern Sworld world;
extern char *libdir;
extern int ruid, euid;
extern char *get_char_option();

char *mail_forwarding(nation)
     int nation;
{
  return get_char_option(nation,"MAIL_FORWARD");
}

  /* This function checks if mailbox for nation 'id' is locked. */
has_mail_lock(id)
     int id;
{
  FILE *lock_fp;
  char lock_fn[PATHLEN];
  int ret;

/*  ruid = getuid();
  euid = geteuid();
*/
  if (mail_forwarding(id))
      return 0;

  sprintf(lock_fn,"%s/%d.lock", MAIL_DIR, id);
    /* if it's locked: close and return 1 */
  if ((lock_fp = fopen(lock_fn, "r")) != NULL) {
    fclose(lock_fp);
    ret=1;
  } else {
    ret=0;
  }
  return(ret);
} /* has_mail_lock */

  /* This function locks a user's mailbox */
lock_mail(nation)
int nation;
{
  FILE *lock_fp;
  char lock_fn[100];
  
  if (mail_forwarding(nation))
      return;

  sprintf(lock_fn, "%s/%d.lock", MAIL_DIR, nation);
  if ((lock_fp = fopen(lock_fn, "w")) != NULL) {
      fprintf(lock_fp, "%ld; Nation %s\n", time(0L), user.np->name);
      fclose(lock_fp);
    }
}

  /* This unlock's a user's mailbox */
unlock_mail(nation)
int nation;
{
  char lock_fn[100];
  
  if (mail_forwarding(nation))
      return;

  sprintf(lock_fn, "%s/%d.lock", MAIL_DIR, nation);
  unlink(lock_fn);
}

  /* This calls the system to edit the given file with the preferred editor */
void edit(t_fn)
     char *t_fn;
{
  int tmp;
  char *edit_prog, *getenv();
  char edit_command[200], command[200];

  if ((edit_prog=getenv("DOMINION_EDITOR"))==NULL
      && (edit_prog = getenv("VISUAL")) == NULL
      && (edit_prog = getenv("EDITOR")) == NULL) {
      edit_prog = DEFAULT_EDITOR;
    }
#ifdef UID_SECURITY
    /* we must fork, so that in the child we set the
       real user id, whereas the parent continues with
       the effective user id.
     */
  if (fork() == 0) {		/* child has fork() == 0 */
    setuid(ruid);		/* so this user cannot poke around */
    close(creat(t_fn, 0600));
    sprintf(edit_command, "%s %s", edit_prog, t_fn);
    system(edit_command);
    /* change owner so that it can be processed once the uid changes back */
/*
    sprintf(command, "chown %d %s", euid, t_fn);
    system(command);
*/
    chown(t_fn, euid, getgid()); /* use the system call to chown() */
/*
    printf("waiting; type a few returns\n");
    fflush(stdout);
    getchar();
    getchar();
*/
    exit(0);
  }
  wait(0);
#else /* UID_SECURITY */
  creat(t_fn, 0666);
  sprintf(command, "chmod 666 %s", t_fn);
  system(command);
  sprintf(edit_command, "%s %s", edit_prog, t_fn);
  system(edit_command);
#endif /* UID_SECURITY */
}

/* Insert a file (with the name in_name) into the open file stream pointed
   to by out_pntr */
void insert(in_name, out_pntr)
     char *in_name;
     FILE *out_pntr;
{
  FILE *in_pntr;
  int c;

  if ((in_pntr=fopen(in_name, "r"))!=NULL)
    {
      while((c=fgetc(in_pntr))!=EOF)
	fputc(c, out_pntr);
      fclose(in_pntr);
    }
} /* insert */


char *fix_name(s,fixed)
/* Elm and etc. want the name that mail is from to contain no white space */
char *s,*fixed;
{
  char *poss = s, *posf = fixed;

  if (s == NULL) { return NULL; }
  for ( poss = s;(poss != '\0') && (posf - fixed < NAMELEN);  poss++ , posf++)
  {
    if ((*poss == ' ') || (*poss == '\t'))
    {
      *posf = '_';
    } else
    {
      *posf = *poss;
    }
  }
  if (posf - fixed < NAMELEN) { *posf = '\0' ; }
  else { *(fixed + NAMELEN - 1) = '\0'; }
  return fixed;
}
/* Send mail from one nation to another. mailfile is the _name_ of the
   file containing the body of the mail. sender and receiver are the full
   names of the appropriate nations. Guess what subject is... */

int mail_send(mailfile, sender, receiver, subject)
     char mailfile[];
     int sender, receiver;
     char subject[];
{
  time_t now_secs;
  char lock_fn[200], dest_fn[200], tmp_fname[PATHLEN], *now_chars;
  FILE *lock_fp, *dest_fp, *temp_fp;
  char s_name[NAMELEN], r_name[NAMELEN];
  int temp;
  char *forward, fixed_name[NAMELEN];

  if ((forward = get_char_option(receiver,"MAIL_FORWARD")) != NULL) {
      char command[2048];
#ifdef UID_SECURITY
      int pid;
#endif

      strcpy(tmp_fname, "/usr/tmp/domXXXXXX");
      mktemp(tmp_fname);
      temp_fp = fopen(tmp_fname, "w");
      if (!temp_fp) {
	  perror("Could not open temp file");
	  return 1;
      }
      fprintf(temp_fp,"From: %s of %s\n",world.nations[sender].leader,
	      world.nations[sender].name);
      fprintf(temp_fp,"To: %s of %s\n",world.nations[receiver].leader,
	      world.nations[receiver].name);
      fprintf(temp_fp,"Subject: %s\n\n",subject);
      fclose(temp_fp);

#ifdef UID_SECURITY
      sprintf(command, "chmod +r %s", tmp_fname);
      system(command);
#endif

      sprintf(command, "cat %s %s | %s '%s'", tmp_fname, mailfile,
                          MAILER, forward);

/*
      printf("\r\n ready to run the cat command; type some returns \r\n");
      fflush(stdout);
      getchar();
      getchar();
*/
      system(command);
#ifdef CONFUSED_UID
#ifdef UID_SECURITY
      /* we must fork, so that in the child we set the
	 real user id, whereas the parent continues with
	 the effective user id.
       */

      if ((pid=fork()) == 0) {		/* child has fork() == 0 */
	  setuid(getuid());		/* so this user cannot poke around */
#endif
	  system(command);
#ifdef UID_SECURITY
	  exit(0);
      }
/*      else if (pid < 0)
	  perror("Could not fork mailer");
      else
	  while (wait(0) != pid);
*/
      wait(0);
#endif /* UID_SECURITY */
#endif /* CONFUSED_UID */

      unlink(tmp_fname);
      return 0;			/* we've forwarded, so that's all */
  }

  /* Set the names of the files used in mail */
  sprintf(dest_fn, "%s/mail.%d", MAIL_DIR, receiver);

/*  sprintf(lock_fn, "%d.lock", receiver); */
  strcpy(tmp_fname, "dommaXXXXXX");
  mktemp(tmp_fname);

    /* Get the time right now */
  now_secs=time(0L);
  now_chars=ctime(&now_secs);

    /* If not make sure it won't be used */
/*  lock_fp=fopen(lock_fn, "w");
  fprintf(lock_fp, "Mail being sent by %s at %s\n", user.np->name, now_chars);
  fclose(lock_fp);
*/

    /* Copy the mail that's there right now out */
  if ((temp_fp = fopen(tmp_fname, "w")) == NULL) { 
    fprintf(stderr,"Error: Cannot write to mail file %s\n",tmp_fname);
    clean_exit();
    exit(1);
  }
  insert(dest_fn, temp_fp);
  fclose(temp_fp);
    /* Put in the new mail */
  if ((dest_fp=fopen(dest_fn, "w"))!=NULL) {
       /* Some header stuff */
      fix_name(world.nations[sender].leader, fixed_name);
      fprintf(dest_fp, "From %s %s", fixed_name,now_chars);
      fprintf(dest_fp, "Date: %s", now_chars);
      fprintf(dest_fp, "From: %s of %s\n", world.nations[sender].leader,
	      world.nations[sender].name);
      fprintf(dest_fp, "To: %s of %s\n", world.nations[receiver].leader,
	      world.nations[receiver].name);
      fprintf(dest_fp, "Subject: %s\n\n", subject);

        /* Now the body of the message */
      insert(mailfile, dest_fp);
      fprintf(dest_fp, "\n");
        /* and now copy the old mail back in */
      insert(tmp_fname, dest_fp);
      fclose(dest_fp);
    } /* if fopen dest_fp */
  else {
    if (sender==0) {
      fprintf(stderr, "Couldn't open dest_fn\n");
    }
  }
    /* Remove the old unnecessary files */
  unlink(tmp_fname);
  unlink(mailfile);
/*  unlink(lock_fn); */
  unlock_mail(receiver);
  return(0);
} /* mail_send */

