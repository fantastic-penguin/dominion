  /* cinfo.c  --  curses-based info browsing utility/library */

/*
 * Copyright (C) 1990 Free Software Foundation, Inc.
 * Written by Mark Galassi.
 *
 * This file is part of cinfo.
 *
 * cinfo is free software; you can redistribute it and/or
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

#include <stdio.h>
#include <curses.h>
#ifdef SYSV
# include <string.h>
#else
# include <strings.h>
#endif /* SYSV */

/* some systems already have strstr() */
/* #define HAVE_STRSTR */

#define LINE_LEN 80		/* longer than this is truncated */
  /* this is defined in the Makefile */
// /* #define STANDALONE		/* if we run standalone... */
#define INFO_PATH "/usr/local/lib/emacs/info"
#ifndef INFO_INTRO
# define INFO_INTRO "/usr/local/lib/info_intro"
#endif /* INFO_INTRO */
#define INFO_CHAR 0x1F		/* ^_, used to specify an info line */
#define DEL 0x7F		/* DELETE character */

  /* global variables */

  /* current node fields */
char node[LINE_LEN], node_prev[LINE_LEN],
  node_next[LINE_LEN], node_up[LINE_LEN];

  /* seek pointer in info file where the tag table is */
long tag_table_pos = 0;

  /* this tells us that we must show a new page */
int update_info_page = 1;

// Forward declarations
void find_tag_table(FILE *fp);
void clean_exit();
int find_menu_item(FILE *fp, char tag[]);
void parse_info_line(FILE *fp);
int show_page(FILE *fp, WINDOW *infow);
int prompt(char item[]);
void cinfo_init_screen();
void cinfo_intro_screen(FILE *intro_fp, WINDOW *infow);
void cinfo_statline(char s[]);
void cinfo_cleanup();

  /* main routine if this program is run standalone.  Otherwise,
     the cinfo() routine can be called from anywhere.
   */
#ifdef STANDALONE
main(argc, argv)
     int argc;
     char *argv[];
{
  switch (argc) {
  case 2:
    cinfo(argv[1], "Top");
    break;
  case 3:
    cinfo(argv[1], argv[2]);
    break;
  default:
    printf("Useage: %s filename [tag]\n", argv[0]);
    clean_exit();
    exit(1);
  }
  clean_exit();
  exit(0);
}

void clean_exit(){};

#endif /* STANDALONE */

static WINDOW *infow;		/* screen-size window in which it happens */

  /* run curses-based info browsing.
     fname: name of info file
     tag:   topic we should try to search for
   */
void cinfo(fname, tag)
     char fname[], tag[];
{
  char item[LINE_LEN];
  int done = 0;			/* for the input loop */
  long old_pos;
  char c;
  FILE *fp, *intro_fp, *find_info_file();

    /* open up the info file */
  fp = find_info_file(fname);
  find_tag_table(fp);
  if ((intro_fp = fopen(INFO_INTRO, "r")) == NULL) {
    printf("Cannot open info file %s, exiting.\n", INFO_INTRO);
    clean_exit();
    exit(1);
  }
  cinfo_init_screen();
    /* if they start at top level, give them the help screen */
  if (strcmp(tag, "Top") == 0) {
    cinfo_intro_screen(intro_fp, infow);
  }

  find_menu_item(fp, tag);
  parse_info_line(fp);		/* make sure we have the current line parsed */
/*  wclear(infow); */

  while (!done) {
    old_pos = ftell(fp);	/* before this page is shown */
    if (show_page(fp, infow) == -1) {
      /* means that we ran into a new info line */
      parse_info_line(fp);
    }
    cinfo_statline("(SPACE, DEL/BS, m, n, p, u, q, ?)");
    switch (wgetch(infow)) {
    case ' ':
      update_info_page = 1;
      break;
    case DEL:			/* should go back a screen */
    case '\b':
      break;
    case 'm':			/* let user choose a tag */
      if (prompt(item) != -1) {	/* did they type something? */
	fseek(fp, old_pos, 0);	/* in case find_menu_item does not work */
	find_menu_item(fp, item);
	parse_info_line(fp);	/* make sure we have the current line parsed */
	update_info_page = 1;
      }
      break;
    case 'n':
      if (strlen(node_next) > 0) {
	find_menu_item(fp, node_next);
	parse_info_line(fp);
      }
      update_info_page = 1;
      break;
    case 'p':
      if (strlen(node_prev) > 0) {
	find_menu_item(fp, node_prev);
	parse_info_line(fp);
      }
      update_info_page = 1;
      break;
    case 'u':
      if (strlen(node_up) > 0) {
	find_menu_item(fp, node_up);
	parse_info_line(fp);
      }
      break;
    case 'q':
      done = 1;
      break;
    case '?':
      cinfo_intro_screen(intro_fp, infow);
      wclear(infow);
      update_info_page = 1;
      break;
    default:
      fseek(fp, old_pos, 0);
      break;
    }
  }
  fclose(fp);
  fclose(intro_fp);
  cinfo_cleanup();
}

  /* prepare the screen */
void cinfo_init_screen()
{
#ifdef STANDALONE
  printf("initializing screen...\r\n");
  initscr();
  savetty();
  nonl();
  cbreak();
  noecho();
#endif /* STANDALONE */
    /* prepare a full-size window to work in */
  infow = newwin(LINES, COLS, 0, 0);
  wclear(infow);
  wmove(infow, 0, 0);
  wrefresh(infow);
}

  /* clean up curses, if necessary, and free memory */
void cinfo_cleanup()
{
  wclear(infow);
  wrefresh(infow);
  delwin(infow);
#ifdef STANDALONE
  resetty();
  endwin();
#endif /* STANDALONE */
}

  /* shows the introductory page and waits for a key hit */
void cinfo_intro_screen(intro_fp, infow)
     FILE *intro_fp;
     WINDOW *infow;
{
  char line[2*LINE_LEN];
  int n = 0;

  wclear(infow);
  rewind(intro_fp);
  while (fgets(line, 2*LINE_LEN, intro_fp) != NULL) {
    line[LINE_LEN-1] = '\0';	/* truncate it */
    mvwaddstr(infow, n, 0, line);
    wclrtoeol(infow);
    wrefresh(infow);
    ++n;
  }
  wgetch(infow);
}

  /* shows the given string at the bottom of the page */
void cinfo_statline(s)
     char s[];
{
  wmove(infow, LINES-1, 0);
  waddstr(infow, s);
  wclrtoeol(infow);
  wrefresh(infow);
}

void cinfo_statline2()
{
  wmove(infow, LINES-2, 0);
  wstandout(infow);
  wprintw(infow ,"%s", node);
  wstandend(infow);
  wprintw(infow, ", p: %s, u: %s, n: %s", node_prev, node_up, node_next);
  wclrtoeol(infow);
  wrefresh(infow);
}

  /* this is kind of the guts of it:
     setting the file pointer to a menu item.
     this function also returns the seek pointer
   */
int find_menu_item(fp, tag)
     FILE *fp;
     char tag[];
{
  long old_pos = ftell(fp), pos;
  char line[2*LINE_LEN], s[LINE_LEN], item[LINE_LEN];
#ifndef HAVE_STRSTR
#ifdef __STDC__
  extern char *strstr(char *, char *);
#else /* STDC */
  extern char *strstr();
#endif /* STDC */
#endif /* HAVE_STRSTR */

  fseek(fp, tag_table_pos, 0);
    /* prepare a scanning format string */
  sprintf(s, "Node: %%[-A-Z a-z0-9_`\']");
  while (1) {
    if ((fgets(line, 2*LINE_LEN, fp) == NULL) || (sscanf(line, s, item) < 0)) {
      fseek(fp, old_pos, 0);
      return old_pos;
    }
      /* now convert all to lower case */
    str_lower(item);
    str_lower(tag);
    if (strncmp(item, tag, strlen(tag)) == 0) {
        /* ugly kludge, but scanf seems to not do what I want */
      sscanf(line+strlen("Node: ")+strlen(item)+1, "%ld", &pos);
      fseek(fp, pos-1, 0);
      return pos;
    }
  }
}

  /* this function is essential:
     it sets the node[], node_prev[], node_next[] and node_up[] strings
   */
void parse_info_line(fp)
     FILE *fp;
{
  char line[2*LINE_LEN];
  long old_pos = ftell(fp);
  char *ptr;
  int n;

  fgets(line, 2*LINE_LEN, fp);
    /* we can only parse it if it is really a node line */
  if (strncmp(line, "File:", strlen("File:")) != 0) {
    fseek(fp, old_pos, 0);
    return -1;
  }
#define BAD_HACK
#ifdef BAD_HACK
    /* now I play funny pointer games to parse
       this line.  I assume that strstr() returns NULL
       if the second string is not a substring of the
       first.
     */
  if ((ptr = strstr(line, "Node: "))) {
    n = (int) ((long) strchr(ptr, ',') - (long) ptr - strlen("Node: "));
    strncpy(node, ptr+strlen("Node: "), n);
    node[n] = '\0';
  } else {			/* field is not there */
    strcpy(node, "");
  }

  if ((ptr = strstr(line, "Prev: "))) {
    n = (int) ((long) strchr(ptr, ',') - (long) ptr - strlen("Prev: "));
    strncpy(node_prev, ptr+strlen("Prev: "), n);
    node_prev[n] = '\0';
  } else {			/* field is not there */
    strcpy(node_prev, "");
  }

  if ((ptr = strstr(line, "Up: "))) {
    n = (int) ((long) strchr(ptr, ',') - (long) ptr - strlen("Up: "));
    strncpy(node_up, ptr+strlen("Up: "), n);
    node_up[n] = '\0';
  } else {			/* field is not there */
    strcpy(node_up, "");
  }

    /* this last one is easier, because it is
       the last field in an info line.  but
       remember to remove the newline.
     */
  if ((ptr = strstr(line, "Next: "))) {
    strcpy(node_next, ptr+strlen("Next: "));
    node_next[strlen(node_next)-1] = '\0';
  } else {			/* field is not there */
    strcpy(node_next, "");
  }

#endif /* BAD HACK */
  fseek(fp, old_pos, 0);	/* go back to where we were */
  return 1;
}

  /* this is also an essential function:
     it shows the current information page
   */
int show_page(fp, infow)
     FILE *fp;
     WINDOW *infow;
{
  int n = 0;			/* line counter */
  char line[2*LINE_LEN];

  if (!update_info_page) {	/* do we need to redraw? */
    return 0;
  }
  update_info_page = 0;
  wclear(infow);
  for (n = 0; n < LINES-2; ++n) {
    fgets(line, 2*LINE_LEN, fp);
    line[LINE_LEN-1] = '\0';	/* truncate it */
      /* stop if you get to a new info format entry */
    if (line[0] == INFO_CHAR) {
      cinfo_statline2();
      return -1;
    } else {
      mvwaddstr(infow, n, 0, line);
      wclrtoeol(infow);
      wrefresh(infow);
    }
  }
  cinfo_statline2();
  return 1;
}

  /* prompt the user for a tag; return -1 if there is no input */
int prompt(item)
     char item[];
{
  int ret;

  echo();
  nocbreak();
  cinfo_statline("Enter your menu choice: ");
  ret = wscanw(infow, "%[^\n]", item);
  cbreak();
  noecho();
  if (ret <= 0) {
    return -1;
  }
  return 1;
}

  /* called at the start, this finds the location of the tag table */
void find_tag_table(fp)
     FILE *fp;
{
  char line[2*LINE_LEN];
  long pos;

  while (1) {
    pos = ftell(fp);
    if (fgets(line, 2*LINE_LEN, fp) == NULL) {
      break;
    }
    if (line[0] == INFO_CHAR) {
      fgets(line, 2*LINE_LEN, fp);
      if (strncmp(line, "Tag table:", strlen("Tag table:")) == 0) {
	tag_table_pos = ftell(fp); /* got it!!! */
	break;
      }
    }
  }
}

  /* converts a string to lower case */
void str_lower(s)
     char s[];
{
  while (*s) {
    if (*s >= 'A' && *s <= 'Z') { /* is it upper case? */
      *s = *s - ('A' - 'a');	/* if so, convert */
    }
    ++s;
  }
}

  /* tries to find and fopen() the .info file in a variety
     of paths.  returns the file pointer or exits.
   */
FILE *find_info_file(fname)
     char fname[];
{
  char full_fname[LINE_LEN];
  FILE *fp, *fopen();

    /* try several file names, see if one of them works */
  if ((fp = fopen(fname, "r")) == NULL) {
    strcpy(full_fname, fname);
    strcat(full_fname, ".info");	/* try it with a ".info" */
    if ((fp = fopen(full_fname, "r")) == NULL) {
      strcpy(full_fname, INFO_PATH);
      strcat(full_fname, "/");
      strcat(full_fname, fname);
      if ((fp = fopen(full_fname, "r")) == NULL) {
	strcat(full_fname, ".info");
	if ((fp = fopen(full_fname, "r")) == NULL) {
	  printf("Cannot open file %s or %s, exiting.\n", fname, full_fname);
          clean_exit();
	  exit(1);
	}
      }
    }
  }
  return fp;
}


#ifndef HAVE_STRSTR

  /* System V has strstr(), which is useful, but bsd
     does not.  This is a replacement for strstr that
     should make up for that.  The algorithm is a
     total hack, and probably not too fast.
   */
  /* strstr() returns a pointer to the first occurrence of
     s2 inside s1.  A generalization of index() or strchr().
     strstr() returns NULL if s2 is not a substring of s1.
   */
char *
strstr(s1, s2)
     char *s1, *s2;
{
  char *p = NULL;		/* what we are looking for */
  char *s;
  int n = strlen(s2);

    /* run through s1, and see if at any point s2 appears */
  for (s = s1; *s != '\0'; ++s) {
    if (strncmp(s, s2, n) == 0) {
      p = s;			/* we found it!!! */
      break;
    }
  }
  return p;
}

#endif /* HAVE_STRSTR */
