  /* mailer.c -- dominion mail reading system by Stephen Underwood */
  /*                                          and Doug Novellano :) */

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
#include "functions.h"
#include <stdio.h>
#ifdef AMIGA
# include <exec/types.h>
# include <string.h>
#else
# include <sys/types.h>
#endif
#include <unistd.h>
#include <stdlib.h>

#include <time.h>
#ifdef SYSV
# include <string.h>
#else
# include <strings.h>
#endif
#include <ctype.h>

extern Suser user;
extern Sworld world;
extern char *libdir;
extern int ruid, euid;

struct message {
  struct message_body *body;
  char status;
  struct message *next;
};
  
struct message_body {
  char *line;
  struct message_body *next;
};

// Forward declarations
int mail_reader(char *fname);
int load_messages(FILE *fp, struct message **first_mesg);
void display_messages(struct message *first_mesg, int num_mess);
int write_messages(FILE *fp, struct message *first_mesg);
void clear_messages(struct message *first_mesg);
void put_mesg(FILE *fp, struct message_body *mbody);

/* Read the persons mail. Reader is the name
   of the nations whose mail is to be read.
 */
void mail_read(reader)
     int reader;
{
  FILE *lock_fp;
  char mail_command[100],mail_file[PATHLEN];
  char lock_fn[100], *mail_prog, *getenv();
#ifdef UID_SECURITY
      int pid;
#endif

/*
  if ((mail_prog=getenv("DOMINION_MAIL"))==NULL) {
    mail_prog=(char *)malloc(sizeof(DEFAULT_MAIL));
    strcpy(mail_prog, DEFAULT_MAIL);
  }
*/
  mail_prog = world.nations[reader].opts->mail_reader;

/* This is going to be essentially what's in int_mail.c */
  if (mail_prog == NULL) 
  {
    sprintf(mail_file,"%s/mail.%d",MAIL_DIR, reader);
    printf("\n\n");
    fflush(stdin);
    fflush(stdout);
    mail_reader(mail_file);
  }
  else /* We want to use our own mail program */
  {
    char tmp_fname[PATHLEN];
    cleanup(); /* Get ready to leave the system */
    /* we must make the file read/modify-able.  is this a security prob? */
/* Yes it was */
/*      sprintf(mail_command, "chmod 666 \"%s/mail.%d\"", MAIL_DIR, reader);
      system(mail_command);
*/
    strcpy(tmp_fname,"/usr/tmp/dom_XXXXXX");
    mktemp(tmp_fname);
    sprintf(mail_command, "cp %s/mail.%d %s", MAIL_DIR, reader,tmp_fname);
    system(mail_command);
    sprintf(mail_command, "chmod 666 %s", tmp_fname);
    system(mail_command);

#ifdef UID_SECURITY
      /* we must fork, so that in the child we set the
	 real user id, whereas the parent continues with
	 the effective user id.
	 */

      if ((pid=fork()) == 0) {		/* child has fork() == 0 */
	  setuid(getuid());		/* so this user cannot poke around */
#endif
    sprintf(mail_command, "%s -f %s", mail_prog, tmp_fname);
    system(mail_command);
#ifdef UID_SECURITY
	  exit(0);
      }
      else if (pid < 0)
	  perror("Could not fork mailer");
      else
	  while (wait(0) != pid);
#endif UID_SEQURITY
    sprintf(mail_command,"cp %s %s/mail.%d",tmp_fname,MAIL_DIR,reader);
    system(mail_command);
    unlink(tmp_fname);

    init_screen(); /* Resetup the screen after coming back */
  }
}

char *get_line(fp)
/* Gets a single line from the file fp and returns it, or returns NULL */
FILE *fp;
{
  char *temp;

  if ((temp = (char *)malloc((COLS+1) * sizeof(char))) == NULL) mem_error();
  if ( (fgets(temp,COLS-1,fp) == NULL))
  {
    free(temp);
    return NULL;
  } 
  temp[COLS-1] = '\0';
  if (temp[strlen(temp) -1] != '\n') strcat(temp,"\n");
  return temp;
}    

int is_fromline(s)
/* Determines if the string passed it is the start of a new message */
char *s;
{
  if (s == NULL) return 0;
  if (strncmp(s,"From ",5) == 0) return 1;
  return 0;
}

void add_line(mbody, text)
/* Adds a new line onto the end of the message passed to it. */
struct message_body **mbody;
char *text;
{
  struct message_body *last_mbody = *mbody, *temp;

  if (*mbody == NULL)
  {
    if ((*mbody = (struct message_body *) malloc(sizeof(struct message_body)))
             == NULL) { 
      mem_error();
    }
    (*mbody)->next = NULL;
    (*mbody)->line = text;
  } else
  {
    while (last_mbody->next != NULL) last_mbody = last_mbody->next;
    if ((temp = (struct message_body *) malloc(sizeof(struct message_body)))
             == NULL) {
      mem_error();
    }
    last_mbody->next = temp;
    temp->next = NULL;
    temp->line = text;
  }
}

int mail_reader(fname)
/* 
  This routine acts like a very simple mail program, in that it allows
  the user to read their mail, and to delete messages, and even to write
  the messages to a file.  However you are still required with this program
  to proceede linearly one message at a time through all your mail.
*/
char *fname;
{
  FILE *fp, *fpout, *fopen(); /* fp is the mailfile fpout is a temp file */
  char outfname[PATHLEN], cmd[2*PATHLEN];
  int wrote_to_outfile = 0, num_mess;
  struct message *first_mesg = NULL;

  if ((fp = fopen(fname,"r")) == NULL) /* Open the mail file */
  {
    fprintf(stderr,"No Mail\n");
    return 0;
  }
  sprintf(outfname, "%s.out", fname);
  if ((fpout = fopen(outfname,"w")) == NULL) /* Open our temp file */
  {
    fprintf(stderr,"Error: Can't open Temporary Mailfile\n");
    return 0;
  }
  if ((num_mess = load_messages(fp,&first_mesg)) < 1 )
  {
    fprintf(stderr,"No Mail\n");
    return 0;
  }
/*  initscr(); */
  display_messages(first_mesg,num_mess);
  if (write_messages(fpout, first_mesg) != 0)
  {
    fclose(fpout); /* Close the temporary file */
    sprintf(cmd,"mv %s %s",outfname,fname);  /* then replace the mailfile */
  } else
  {
    fclose(fpout); /* Close the temporary file */
    sprintf(cmd,"rm %s",fname); /* Otherwise remove the mailfile */
    unlink(outfname);
  }
#ifdef UID_SECURITY
  if (fork() == 0) {		/* child has fork() == 0 */
    setuid(getuid());
    system(cmd);
    exit(0);
  }
#else
  system(cmd);
#endif
  
  clear_messages(first_mesg);
}

int put_mesg_to_file(mbody)
/*
   This function is to write the given message out to a file the
   user specifies
*/
struct message_body *mbody;
{
  FILE *fp, *fopen();
  char fname[PATHLEN];
  extern int ruid, euid;
#ifdef BSD
 extern char current_dir[];
#else /* BSD */
 extern char *current_dir;
#endif

  printw("Name of File? ");
  refresh();
  wget_string(stdscr,fname,PATHLEN-1);
  if (fname[strlen(fname)-1] == '\n') fname[strlen(fname)-1] = '\0';

#ifdef UID_SECURITY
  if (fork() == 0) {		/* child has fork() == 0 */
      /* first change back to the user's current directory */
    setuid(ruid);
    chdir(current_dir);
    if ((fp = fopen(fname, "w")) == NULL) {
      fprintf(stderr,"Error: could not write to file %d",fname);
      exit(0);
    }
    put_mesg(fp,mbody);
    fclose(fp);
    exit(0);
  }
  wait(0);
#else /* UID_SECURITY */
  chdir(current_dir);
  if ((fp = fopen(fname, "w")) == NULL) {
    fprintf(stderr,"Error: could not write to file %d",fname);
    return -1;
  }
  put_mesg(fp,mbody);
  fclose(fp);
  chdir(libdir);
#endif /* UID_SECURITY */

  return 0;
}

void put_mesg(fp,mbody)
/* This function puts the given message into the file passed to it */
FILE *fp;
struct message_body *mbody;
{
  struct message_body *curr = mbody;

  while (curr != NULL)
  {
    fprintf(fp,"%s",curr->line);
    curr = curr->next;
  }
}

void clear_mesg(mbody)
/* Clean up the memory for a given message */
struct message_body **mbody;
{
  struct message_body *curr = *mbody, *temp;

  while (curr != NULL)
  {
    temp = curr;
    curr = curr->next;
    if (temp->line != NULL) free(temp->line);
    if (temp != NULL) free(temp);
  }
  *mbody = NULL;  /* And clean up the beginning */
}

void add_rest(fpout, fpin)
/* Add the rest of the mailfile to the temporary output file */
FILE *fpout, *fpin;
{
  int c;

  while ((c = fgetc(fpin)) != EOF)
  {
     fputc(c,fpout);
  }
}

int get_message(fp, mbody, first_flag)
int first_flag;
struct message_body **mbody;
FILE *fp;
{
  static char *from_line = NULL;
  char *curr_text;
  int first_line = first_flag;

  while ((curr_text = get_line(fp)) != NULL) {
    if ((is_fromline(curr_text)) && (!first_line))
    { /* If it's the begining of any message but the first */
      from_line = curr_text; /* save the beginning line of the next message*/
      return 0;
    }
    if (from_line != NULL) /* If we have a leftover first line */
    {
      add_line(mbody, from_line);  /* Add it to the message */
      from_line = NULL;
    }
    add_line(mbody, curr_text); /* Add the current line to the mesg */
    first_line = 0;
  }

 /* If it's the end of the file return 1 */
  return 1;
}

void print_message(mbody)
struct message_body *mbody;
{
  int curr_line = 0, i;
  struct message_body *curr = mbody;
  char temp[100];

  clear(); refresh();
  if (curr != NULL) { curr = curr->next; } /* Don't print from line */
  while (curr != NULL)
  {
    printw("%s",curr->line); /* Print out the next line */
    if ( ++curr_line >= (LINES - 1))
    { /* break for the end of the scren */
      printw("---More--- Press return to cont. j to jump to next message : ");
      refresh();
      wget_string(stdscr,temp,39);
      clear();
      curr_line = 0;
      if ((temp[0] == 'j') || (temp[0] == 'J')) 
      {
        return 0;
      }
    }
    curr = curr->next;
  }
  printw("Press return to continue");
  refresh();
  wget_string(stdscr,temp,39);
  curr_line = 0;
}

int load_messages(fp,first_mesg)
FILE *fp;
struct message **first_mesg;
{
  int file_over = 0, num_mess = 0, first_flag = 1;
  struct message *curr_mesg = NULL;
  struct message_body *curr_mbody = NULL;

  do {
    file_over = get_message(fp, &curr_mbody, first_flag);
    if ( curr_mbody == NULL ) { break; }
    num_mess++;
    if (curr_mesg == NULL)
    {
      if ((curr_mesg = (struct message *)malloc(sizeof (struct message)))
          == NULL) { mem_error(); }
      *first_mesg = curr_mesg;
    } else
    {
      if ((curr_mesg->next = (struct message *)malloc(sizeof (struct message)))
          == NULL) { mem_error(); }
      curr_mesg = curr_mesg->next;
    }
    curr_mesg->body = curr_mbody;
    curr_mbody = NULL;
    curr_mesg->next = NULL;
    curr_mesg->status = ' ';
    first_flag = 0; /* Stop special handling for first message */
  } while (file_over != 1);

  return num_mess;
}

int write_messages(fp, first_mesg)
FILE *fp;
struct message *first_mesg;
{
  struct message *curr_mesg = first_mesg;
  int rtvl = 0;

  while ( curr_mesg != NULL )
  {
    switch (curr_mesg->status)
    {
      case 'd': break;
      default: {
	rtvl = 1;
        put_mesg(fp,curr_mesg->body);
        break;
      }
    }
    curr_mesg = curr_mesg->next;
  }
  return rtvl;
}

struct message *get_mesg_by_number(first, num)
struct message *first;
int num;
{
  struct message *curr_mesg = first;
  int i = 0;

  if (num < 0) { return NULL; }
  while (i < num) {
    if (curr_mesg == NULL) { return NULL; }
    curr_mesg = curr_mesg->next;
    i++;
  }

  return curr_mesg;
}

char *get_from(mesg)
struct message *mesg;
{
  struct message_body *curr_line = mesg->body;

  while (curr_line != NULL) {
    if (curr_line->line == NULL) { continue; }
    if (strncmp(curr_line->line, "From: ", strlen("From: ")) == 0) {
      return &(curr_line->line[6]);
    }
    curr_line = curr_line->next;
  }
  return NULL;
}

char *get_subject(mesg)
struct message *mesg;
{
  struct message_body *curr_line = mesg->body;

  while (curr_line != NULL) {
    if (curr_line->line == NULL) { continue; }
    if (strncmp(curr_line->line, "Subject: ", strlen("Subject: ")) == 0) {
      return &(curr_line->line[9]);
    }
    curr_line = curr_line->next;
  }
  return NULL;
}

void display_messages(first_mesg,num_mess)
int num_mess;
struct message *first_mesg;
{
  struct message *curr_mesg = first_mesg, *temp_mesg;
  struct message_body *curr_mbody;
  int user_quit = 0, num_top = 0, num_bottom, num_curr = 0, i, j;
  int chose_option, do_refresh = 1, x, y, oldx, oldy;
  char temp[40], *from, *subject;
#define USED_LINES 8

  while ( user_quit == 0 )
  {
    if (do_refresh == 1) {
      clear(); refresh();
      num_bottom = num_top + (LINES - USED_LINES);
      move(1, 0);
      for (i = num_top;i <= num_bottom; i++)
      {
        if ((temp_mesg = get_mesg_by_number(first_mesg, i)) == NULL) {
          break;
        }
        from = get_from(temp_mesg);
        subject = get_subject(temp_mesg);
        printw("          %c%.2d %c ",(i == num_curr)?'*' : ' ',i,
               temp_mesg->status);
        for(j = 0; (j < 30) && (from[j] != '\0') && (from[j] !='\n') ; j++)
        {
          addch(from[j]);
        }
        for ( j = j - 1; j < 30  ; j++) { addch(' '); }
        addch(' ');
        for(j = 0; (j < 30)&&(subject[j] != '\0')&& (subject[j] !='\n') ; j++)
        {
          addch(subject[j]);
        }
        for ( j = j - 1; j < 30  ; j++) { addch(' '); }
        addch('\n');
      }
      do_refresh = 0;
      addch('\n');
      move(LINES - 4, 0);
      printw("[R]ead curr [W]rite to file, [D]elete, [U]ndelete, [Q]uit\n");
      printw("> for next page < for prev page Enter number to change curr\n");
    } 
    move(LINES - 2, 0);
    printw("[RWDQ><#]:                ");
    move(LINES - 2, 11);
    refresh();
    wget_string(stdscr,temp,39);
    chose_option = 1;
    if (isupper(temp[0])) { temp[0] = (char)tolower(temp[0]); }
    switch (temp[0])
    {
      case 'w': do_refresh = 1; put_mesg_to_file(curr_mesg->body); break;
      case 'd': {
        curr_mesg->status = 'd'; 
        getyx(stdscr,oldy,oldx);
        move(num_curr - num_top + 1, 14);
        addch('d');
        getyx(stdscr, y, x);
        mvcur(y, x, oldy, oldx);
        move(oldy, oldx);
        refresh();
        break;
      }
      case 'u': {
        curr_mesg->status = ' '; 
        getyx(stdscr,oldy,oldx);
        move(num_curr - num_top + 1, 14);
        addch(' ');
        getyx(stdscr, y, x);
        mvcur(y, x, oldy, oldx);
        move(oldy, oldx);
        refresh();
        break;
      }
      case 'r': do_refresh = 1; print_message(curr_mesg->body); break;
      case '>': {
         if ((num_top += (LINES - USED_LINES)) >= num_mess) { 
           num_top -= (LINES - USED_LINES);
         }
         num_curr = num_top;
         curr_mesg = get_mesg_by_number(first_mesg, num_curr);
         do_refresh = 1;
         break;
      }
      case '<': {
         if ((num_top -= (LINES - USED_LINES)) < 0) { 
           num_top = 0;
         }
         num_curr = num_top;
         curr_mesg = get_mesg_by_number(first_mesg, num_curr);
         do_refresh = 1;
         break;
      }
      case 'q': user_quit = 1; break;
      default: {
        getyx(stdscr,oldy,oldx);
        move(num_curr - num_top + 1, 10);
        addch(' ');
        if (isdigit(temp[0])) {
          i = atoi(temp);
          if ((temp_mesg = get_mesg_by_number(first_mesg, i)) != NULL) {
            curr_mesg = temp_mesg;
            num_curr = i;
            if ((i < num_top) || (i > num_bottom)) {
              num_top = i;
              do_refresh = 1;
	    }
	  }
	}
        move(num_curr - num_top + 1, 10);
        addch('*');
        getyx(stdscr, y, x);
        mvcur(y, x, oldy, oldx);
        move(oldy, oldx);
        refresh();
      }
    }
  }
}

void clear_messages(first_mesg)
struct message *first_mesg;
{
  struct message *curr_mesg = first_mesg, *next_mesg;

  while (curr_mesg != NULL)
  {
    next_mesg = curr_mesg->next;
    clear_mesg(&(curr_mesg->body));
    free(curr_mesg);
    curr_mesg = next_mesg;
  }
}

/*#endif*/
