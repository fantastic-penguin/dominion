  /* cur_stuff.h - various definitions for the curses interface */

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

  /* BSD seems to not have beep(), or at least sunOS4.0 does not */
#ifdef BSD
# define beep() (putchar(CTL('G')))
#endif /* BSD */

  /* the sector display window originally goes at these locations */
#define SECTW_SIZE_X 24
#define SECTW_SIZE_Y 8
#define ARMYW_SIZE_X 25
#define ARMYW_SIZE_Y 10
#define SECTW_X (COLS - 1 - SECTW_SIZE_X)
#define SECTW_Y (LINES - SECTW_SIZE_Y - 2)
#define ARMYW_X (COLS - ARMYW_SIZE_X)
#define ARMYW_Y (SECTW_Y - ARMYW_SIZE_Y - 2)

Pt drag_cursor();
#define DRAG_ABS 0
#define DRAG_REL 1

  /* two macros that help us center the map around a user's
     "user.center" point, rather than with absolute coordinates
   */
#define xoff() (user.center.x-(COLS-2)/4)
#define xoff_compact() (user.center.x-(COLS-2)/2) /* for compact map */
#define yoff() (user.center.y-(LINES-2)/2)
