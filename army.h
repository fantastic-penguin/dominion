  /* army.h -- constants for armies, caravans, ships....
               there are also some macros declared below
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

  /* army statuses */
#define A_DEFEND 1		/* army status: defend */
#define A_ATTACK 2		/*              attack */
#define A_OCCUPY 3		/*              occupy current sector */
#define A_PATROL 4		/*              patrol surrounding sectors */
#define A_INTERCEPT 5		/*              intercept nearby armies */
#define A_GARRISON 6		/*              man city/town/fort/sctr def. */
#define A_AMBUSH 7		/*              perform ambushes from towns */
#define A_TRADED 8		/*              this army will be traded */

  /* fraction of the dead that vampire units raise from the dead */
#define VAMPIRE_FRACT 0.33	/* 1/3 of the dead rise */
#define SACRIFICED_FRACT 125	/* 1 pt. per 125 people */

  /* bit definitions for Sarmy.flags */
#define AF_FLIGHT  0x01		/* army is flying!! (1) */
#define AF_HIDDEN  0x02		/* army is magically cloaked (2) */
#define AF_VAMPIRE 0x04		/* army is sucking blood (4) */
#define AF_IN_TRANSPORT 0x08	/* army is on a caravan/ship (8) */
#define AF_MISSILES 0x10	/* army shoots arrows and such */
#define AF_WATER 0x20		/* army is walking on water (32) */
#define AF_FRONT_LINE 0x40	/* front line type of army (64) */
#define AF_KAMIKAZE 0x80	/* will die after fighting (128) */
#define AF_MACHINE 0x100	/* war carts, catapults, siege eng. (256) */
#define AF_DISGUISED 0x200	/* army is disguised (512) */
#define AF_WIZARD 0x400		/* can summon and cast spells (1024) */
#define AF_SORCERER 0x800	/* can use sorcery (2048) */
#define AF_CARGO 0x1000		/* can hold a cargo (4096) */
#define AF_UNDERGROUND 0x2000	/* can burrow under ground (8192) */
#define AF_LAND 0x4000		/* army walks on land (16384) */
  /* this next one is kind of special, and requires some special
     handling.  this flag means that (at draft/summon/reset time)
     the army will be given the WATER flag if it is a land race,
     and the LAND flag if it is a water race.  The special-purpose
     code for this flag should be in make_army() and reset_armies().
   */
#define AF_INVERSE_ALT 0x8000	/* opposite of race pref (32768) */

  /* a couple of useful macros */
#define is_hidden(ap) (ap->flags & AF_HIDDEN)
#define is_vampire(ap) (ap->flags & AF_VAMPIRE)
#define is_flight(ap) (ap->flags & AF_FLIGHT)
#define is_in_transport(ap) (ap->flags & AF_IN_TRANSPORT)
#define is_missiles(ap) (ap->flags & AF_MISSILES)
#define is_water(ap) (ap->flags & AF_WATER)
#define is_front_line(ap) (ap->flags & AF_FRONT_LINE)
#define is_kamikaze(ap) (ap->flags & AF_KAMIKAZE)
#define is_machine(ap) (ap->flags & AF_MACHINE)
#define is_disguised(ap) (ap->flags & AF_DISGUISED)
#define is_wizard(ap) (ap->flags & AF_WIZARD)
#define is_sorcerer(ap) (ap->flags & AF_SORCERER)
#define is_cargo(ap) (ap->flags & AF_CARGO)
#define is_underground(ap) (ap->flags & AF_UNDERGROUND)
#define is_land(ap) (ap->flags & AF_LAND)

#define INITIATION_JEWELS 5000	/* cost to initiate a mage (jewels) */
#define MAGE_JEWELS_MAINT 1000  /* cost to maintain a mage (jewels) */
#define MAGE_MOVE_FACTOR 2	/* mages move 2*basic_rate */
#define SORCERER_MOVE_FACTOR 1  /* sorcerers move at basic rate */
#define MEN_PER_MACHINE 10
