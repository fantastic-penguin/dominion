######################################################################
#
# Copyright (C) 1990 Free Software Foundation, Inc.
# Written by the dominion project.
#
# This file is part of dominion.
#
# dominion is free software; you can redistribute it and/or modify it
# under the terms of the GNU General Public License as published by the Free
# Software Foundation; either version 1, or (at your option) any later version.
# 
# dominion is distributed in the hope that it will be useful, but WITHOUT
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
# FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
# details.
# 
# You should have received a copy of the GNU General Public License along with
# dominion; see the file COPYING.  If not, write to the Free Software
# Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.  
#
#######################################################################

SHELL=/bin/sh

# Here is my own terminal rule for RCS, so that it DOES check
# out the file if the RCS version is newer.  Comment this out
# and suffer, if you don't have GNU make.  Or you can use the
# "user" Makefile.

% :: RCS/%,v
	co $<

# version number
VERSION=1.06
# directories for binaries and lib files
BINDIR = /usr/games
#BINDIR = .
#LIBDIR = /usr/games/lib/dominion
LIBDIR = lib
THIS_GAME_LIBDIR = $(LIBDIR)
# MAILDIR is a subdir of THIS_GAME_LIBDIR
MAILDIR = mail
NEWSDIR = news
NGDB_FILE = groups
# NEWS_GROUP is the name of the news group with general update information
NEWS_GROUP = News
#This is the newsgroup database. Where news info is stored.
EXECDIR = exec
DOCDIR = doc
MISCDIR = misc
MAGDIR = magic
OPTDIR = options

# DEFAULT_MAIL is a basic mail program, to be used if the user has
# no environment variable or option for DOMINION_MAIL.  This should be
# "Mail" or "mailx", or any mail program that allows you to choose
# whic message to read.  The old "binmail" is out.
DEFAULT_MAIL = elm

#MAILER is the mail program that will be sending forwarded mail.
#It should be set up in such a way that you can just put addresses
#after it and then type the message to send mail.  
#MAILER = "/usr/ucb/mail -s Dominion_Mail"
MAILER = /bin/mail

# DEFAULT_EDITOR is a basic visual editor, to be used if the user has
# no environment variable or option for VISUAL.  This could be
# micro-gnu-emacs (mg), or emacs, or vi, or anything your users use.
DEFAULT_EDITOR = mg

# location of the info_intro file, used for the online manual,
# with respect to the lib directory
INFO_INTRO=info_intro

# choose a graphics interface.  choice is only cur_stuff.o for now
GRAPHICS_STUFF = cur_stuff.o
GRAPHICS = -DCURSES

CC = cc
# put -g if you want to debug, or -O if you want to optimize
DEBUGFLAGS = -g
LDFLAGS = $(DEBUGFLAGS)
LIBS = -lcurses -ltermcap -lm

# put -DBSD instead of -DSYSV if you are on Berkeley UNIX

# put -DUID_SECURITY if you are running unix and you don't
#  want users to poke around game files with shell escapes.
#  This requires the UNIX system setuid() calls and such, and
#  is quite necessary on UNIX.  Massive cheating can go on
#  otherwise.  Without UNIX, the super user will have to trust
#  the players to not look at the game files.

# put -DHAVE_STRSTR if your system has strstr().  We provide
#  a replacement which is just a quick hack.  SGI and HP-UX
#  are both known to have strstr().

# put -DINTERNAL_MAIL if you want to use the internal mail program
# (in case external mail programs don't interface well to dominion
# on your system).  If you want to use the DEFAULT_MAIL defined
# above, remove -DINTERNAL_MAIL

# put -DPMAX if you are using ultrix. This will fix the mvwprintw
# curses bug that occurs under ultrix.

# put -DANDREW if you are using andrew games authentication

CFLAGS = $(DEBUGFLAGS) $(GRAPHICS) -DDEF_LIBDIR=\"$(LIBDIR)\" -DDEFAULT_MAIL=\"$(DEFAULT_MAIL)\"  -DMAILER=\"$(MAILER)\" -DDEFAULT_EDITOR=\"$(DEFAULT_EDITOR)\" -DNEWS_GROUP=\"$(NEWS_GROUP)\" -DINFO_INTRO=\"$(INFO_INTRO)\" -DVERSION=\"$(VERSION)\" -DSYSV -DUID_SECURITY -DINTERNAL_MAIL

# files relating to documentation, history of the game...
DOC_FILES = README ChangeLog TASKS THE_STORY dominion.tex gm.tex dominion.6 \
	dominion.info refcard info_intro

# all files that go in MAGDIR
MAG_FILES = mag_orders mag_Master mag_Diana mag_Necromancy mag_Yavanna \
	mag_Demonology mag_Monsters mag_Neptune mag_Chess mag_Aule \
	mag_Inferno mag_Avian mag_Unity mag_Time mag_Insects

# all files that go in MISCDIR
MISC_FILES = races npcs army_types spirit_types techno_levels

LIB_FILES = $(DOC_FILES) $(MAG_FILES) $(MISC_FILES)

# all files that are part of the distribution
DIST_FILES = Makefile $(DOC_FILES) \
		$(MAG_FILES) $(MISC_FILES) dominion.h misc.h army.h news.h \
		costs.h cur_stuff.h dominion.c world.c nation.c user.c ext.c \
		file.c misc.c init.c commands.c help.c menus.c cur_stuff.c \
		diplomacy.c army.c reports.c spy.c makeworld.c addnation.c \
		update.c armylib.c mail.c techno.c diplolib.c battle.c \
		maglib.c trade.c trademenu.c news.c c_news.c npc.c npclib.c \
		economy.c transport.c spells.c spelllib.c root.c cinfo.c \
		budget.c construct.c int_mail.c movement.c printmap.c \
		addnews.sh COPYING

all: dominion dom_make dom_add dom_update dom_print $(MAG_FILES) $(MISC_FILES) \
		refcard info_intro

lib-files: all $(MAG_FILES) $(MISC_FILES) refcard info_intro \
		dominion.info addnews.sh
	-find $(THIS_GAME_LIBDIR) -exec chmod u+w {} \;
	-mkdir $(THIS_GAME_LIBDIR)
	-mkdir $(THIS_GAME_LIBDIR)/$(MAILDIR)
	-mkdir $(THIS_GAME_LIBDIR)/$(NEWSDIR)
	-mkdir $(THIS_GAME_LIBDIR)/$(EXECDIR)
	-mkdir $(THIS_GAME_LIBDIR)/$(DOCDIR)
	-mkdir $(THIS_GAME_LIBDIR)/$(MISCDIR)
	-mkdir $(THIS_GAME_LIBDIR)/$(MAGDIR)
	-mkdir $(THIS_GAME_LIBDIR)/$(OPTDIR)
#	-cp $(DOC_FILES) $(THIS_GAME_LIBDIR)/$(DOCDIR)
	-cp refcard dominion.info $(THIS_GAME_LIBDIR)/$(DOCDIR)
	-cp info_intro $(THIS_GAME_LIBDIR)
	-cp $(MAG_FILES) $(THIS_GAME_LIBDIR)/$(MAGDIR)
	-cp $(MISC_FILES) $(THIS_GAME_LIBDIR)/$(MISCDIR)
	find $(THIS_GAME_LIBDIR) -exec chmod g-rw {} \; -exec chmod o-rw {} \;

install: all
	-csh -c "cp dominion dom_make dom_add dom_update $(BINDIR)"
	cp dominion.6 /usr/man/man6
	echo "remember to make the program *dominion* setuid to you"
	echo "you can do this with"
	echo "	chmod u+s dominion"

# the name of GNU emacs on your system
# (not necessary unless you build dominion.info; see rule below)
EMACS = emacs
# startup file for the latexinfo package on your system
# (not necessary unless you build dominion.info; see rule below)
LTI_START = /umax/rosalia/el/ltistart.el
# where the emacs info files go
# (not really important unless you run cinfo() as standalone)
INFO_DIR = /usr/local/lib/emacs/info

install-info: dominion.info
	cp dominion.info $(INFO_DIR)

clean-lib:
	-rm -rf $(THIS_GAME_LIBDIR)

new-world: all clean-lib lib-files
	@echo
	@echo 'all data cleared, and lib files installed'
	@echo
	./dom_make -d $(THIS_GAME_LIBDIR)
	@echo 'newsgroup ' $(NEWS_GROUP) ' created automatically'
	-mkdir $(THIS_GAME_LIBDIR)/$(NEWSDIR)/$(NEWS_GROUP)
	@echo $(NEWS_GROUP) ' 1 0 0' > $(THIS_GAME_LIBDIR)/$(NEWSDIR)/$(NGDB_FILE)
	/bin/sh ./addnews.sh $(THIS_GAME_LIBDIR)/$(NEWSDIR) $(NGDB_FILE)

# use dominiontmp so you can load the program while it is being used
#dominion: dominiontmp
#	cp dominiontmp dominion

dominion: dominion.o world.o nation.o user.o ext.o file.o misc.o init.o \
		trademenu.o budget.o \
		commands.o help.o menus.o diplomacy.o reports.o spy.o trade.o \
		army.o armylib.o mail.o diplolib.o maglib.o $(GRAPHICS_STUFF) \
		news.o c_news.o economy.o transport.o spells.o spelllib.o \
		root.o cinfo.o construct.o int_mail.o movement.o
	$(CC) $(LDFLAGS) -o dominion dominion.o world.o nation.o user.o ext.o \
		file.o budget.o \
		misc.o init.o trademenu.o commands.o help.o menus.o \
		diplomacy.o reports.o spy.o trade.o army.o armylib.o mail.o \
		diplolib.o maglib.o $(GRAPHICS_STUFF) news.o c_news.o \
		economy.o transport.o spells.o spelllib.o root.o cinfo.o \
		construct.o int_mail.o movement.o $(LIBS)

lint:
	lint $(CFLAGS) *.c

dom_make: makeworld.o file.o misc.o ext.o armylib.o diplolib.o world.o \
		economy.o movement.o
	$(CC) $(LDFLAGS) -o dom_make makeworld.o file.o misc.o ext.o \
		armylib.o diplolib.o world.o economy.o movement.o $(LIBS)

dom_add: addnation.o file.o misc.o ext.o armylib.o diplolib.o world.o \
		economy.o movement.o
	$(CC) $(LDFLAGS) -o dom_add addnation.o file.o misc.o ext.o armylib.o \
		diplolib.o world.o economy.o movement.o \
		$(LIBS)

dom_update: update.o file.o misc.o nation.o armylib.o ext.o mail.o \
		techno.o battle.o diplolib.o world.o maglib.o trade.o npc.o \
		economy.o spelllib.o news.o movement.o npclib.o
	$(CC) $(LDFLAGS) -o dom_update update.o file.o misc.o nation.o ext.o \
		armylib.o mail.o techno.o battle.o diplolib.o world.o \
		maglib.o trade.o npc.o economy.o spelllib.o news.o movement.o \
		npclib.o $(LIBS)


dom_print: printmap.o file.o misc.o nation.o armylib.o ext.o maglib.o\
		diplolib.o world.o economy.o movement.o
	$(CC) $(LDFLAGS) -o dom_print printmap.o file.o misc.o nation.o \
		ext.o armylib.o diplolib.o world.o economy.o movement.o \
		maglib.o $(LIBS)

#dom_trans: trans.o file.o misc.o nation.o armylib.o ext.o mail.o \
#		techno.o battle.o diplolib.o world.o maglib.o trade.o npc.o \
#		economy.o spelllib.o news.o movement.o
#	$(CC) $(LDFLAGS) -o dom_trans trans.o file.o misc.o nation.o ext.o \
#		armylib.o mail.o techno.o battle.o diplolib.o world.o \
#		maglib.o trade.o npc.o economy.o spelllib.o news.o \
#		movement.o $(LIBS)

# you might want to comment this out if you don't have LaTeXinfo.
# the second part will unpack dominion.info from a mail/news distribution.
dominion.info: dominion.tex
	-/bin/rm -f info.bak
	-mv dominion.info info.bak
	-cp dominion.tex junk.tex
	-chmod 644 junk.tex
	-$(EMACS) junk.tex -l $(LTI_START) -f nodify-file -f latexinfo-format-buffer -f save-buffer -kill
	-rm junk.tex
	-touch dominion.info

# Now that I use "shar2" I don't need all this stuff with uuencode
# and split.  it will be done automatically while unpacking.

# DO NOT DELETE THIS LINE -- make depend uses it
addnation.o: addnation.c army.h dominion.h misc.h
army.o: army.c army.h cur_stuff.h dominion.h misc.h
armylib.o: army.h armylib.c dominion.h misc.h
battle.o: army.h battle.c dominion.h misc.h
c_news.o: c_news.c dominion.h news.h
cinfo.o: cinfo.c
commands.o: army.h commands.c dominion.h misc.h
cur_stuff.o: cur_stuff.c cur_stuff.h dominion.h misc.h
diplolib.o: diplolib.c dominion.h misc.h
diplomacy.o: diplomacy.c dominion.h misc.h
dominion.o: dominion.c dominion.h misc.h
economy.o: army.h costs.h dominion.h economy.c misc.h
ext.o: dominion.h ext.c misc.h
file.o: dominion.h file.c misc.h
help.o: cur_stuff.h dominion.h help.c
init.o: dominion.h init.c
int_mail.o: dominion.h int_mail.c
maglib.o: dominion.h maglib.c misc.h
mail.o: dominion.h mail.c
makeworld.o: dominion.h makeworld.c misc.h
menus.o: army.h dominion.h menus.c misc.h
misc.o: army.h cur_stuff.h dominion.h misc.c misc.h
movement.o: army.h dominion.h movement.c misc.h
nation.o: army.h dominion.h misc.h nation.c
news.o: dominion.h news.c news.h
npc.o: army.h dominion.h misc.h npc.c
npclib.o: army.h dominion.h misc.h npclib.c
reports.o: dominion.h misc.h reports.c
budget.o: dominion.h misc.h budget.c
root.o: dominion.h root.c
spelllib.o: army.h dominion.h misc.h spelllib.c
spells.o: army.h dominion.h misc.h spells.c
spy.o: dominion.h misc.h spy.c
techno.o: dominion.h misc.h techno.c
trade.o: dominion.h misc.h trade.c
trademenu.o: trademenu.c
transport.o: army.h costs.h dominion.h misc.h transport.c
construct.o: army.h costs.h dominion.h misc.h construct.c
update.o: army.h dominion.h misc.h update.c
user.o: dominion.h misc.h user.c
world.o: army.h dominion.h misc.h world.c
# DO NOT DELETE THIS 2nd LINE -- make depend uses it

# rules for RCS

# check everything in
ci_all:
	ci $(DIST_FILES)

co_all:
	co $(DIST_FILES)

clean:
	/bin/rm -f *.o dom_add dom_make dominion dominiontmp dom_update \
		a.out core TAGS info.bak *.dvi *.log *.aux *.err *.toc \
		*.jep *.cp *.fn *.pg *.ky *.vr *.tp *~

#.DEFAULT:
#	co $@

#menus.c: RCS/menus.c,v
#	co menus.c

# everything after this is used to make the ftp distribution,
# or other such stuff.

#dominion.info.Z: dominion.info
#	compress -c dominion.info > dominion.info.Z

#info.Z.uue: dominion.info.Z
#	uuencode dominion.info.Z dominion.info.Z > info.Z.uue

dominion.shr.Z: dominion.shr
	-/bin/rm -f dominion.shr.Z
	compress dominion.shr

dominion.shr: $(DIST_FILES)
	shar $(DIST_FILES) > dominion.shr

dominion.tar.Z: $(DIST_FILES) dominion.info
	tar cvf dominion.tar $(DIST_FILES)
	compress dominion.tar

dominion.dvi.Z: dominion.dvi
	compress dominion.dvi

dominion.dvi: dominion.tex
	latex dominion

dominion.PS.Z: dominion.PS
	compress dominion.PS

dominion.PS: dominion.dvi
	dvips dominion > dominion.PS

# for mail (and news) distribution
mail-dist: $(DIST_FILES)
#	split -1000 dominion.tex dominion.tex
#	makekit -s60k -nDOM-$(VERSION)- $(DIST_FILES) info.Z.uue
# now use the "shar2" program which can split large files if
# necessary (> 60K).  call with verbose and mixed-ascii-binary
# options.
	shar2 -v -M -oDOM-$(VERSION)- -l60 $(DIST_FILES)
	touch mail-dist

# this is to make it available by anonymous ftp
FTP_DIR=/usr/ftp/pub/dominion-$(VERSION)

ftp-update: dominion.tar.Z dominion.dvi dominion.PS mail-dist dominion.info
	mv dominion.tar.Z $(FTP_DIR)/dominion-$(VERSION).tar.Z
	compress DOM-$(VERSION)-*
	mv DOM-$(VERSION)-*.Z $(FTP_DIR)
	compress dominion.PS
	mv dominion.PS.Z $(FTP_DIR)
	compress dominion.dvi
	mv dominion.dvi.Z $(FTP_DIR)
	compress -c dominion.info > $(FTP_DIR)/dominion.info.Z
	compress -c /usr/local/lib/tex/macros/latexinfo.sty > \
		$(FTP_DIR)/latexinfo.sty.Z
	compress -c /usr/local/lib/tex/macros/tabular.sty > \
		$(FTP_DIR)/tabular.sty.Z

UUCPP=/usr/spool/uucppublic

to-mozart: dominion.shr
	compress dominion.shr
	mv dominion.shr.Z $(UUCPP)
	uucp $(UUCPP)/dominion.shr.Z mozart!$(UUCPP)
