#!/bin/sh
#
# Copyright (C) 1990 Free Software Foundation, Inc.
# Written by the SBW project.
#
# This file is part of SBW.
#
# SBW is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License as published
# by the Free Software Foundation; either version 1, or (at your option)
# any later version.
#
# This software is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this software; see the file COPYING.  If not, write to
# the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
#

case $# in 
2)
	echo 'Adding  newsgroups' ;;
*)
	echo 'Problem in calling addnews' 1>&2 ; exit 2;;
esac

DIR=$1
DB=$2
if  test ! -d "$DIR"
then
	echo 'Problem with news directory argument' 1>&2
	exit 3
fi
if test ! -f  "$DIR/$DB"
then
	touch $DIR/$DB
	echo 'Creating new database ' $DIR/$DB
else
	echo 'Adding groups to database ' $DIR/$DB
fi

t="y"
while test "$t" = "y"
do
	echo 'Enter the name of the news group: \c'
	read group
	echo 'Do you want it human (default) or just game postable (G=game) \c'
	read postable
	if test "$postable" = "G"
	then
		echo $group ' 1 0 0' >> $DIR/$DB
	else
		echo $group ' 1 0 1' >> $DIR/$DB
	fi
	if test ! -d "$DIR/$group"
	then
		mkdir $DIR/$group
	fi
	echo 'Do you want another group? (y/Y=yes) \c'
	read t
	if test "$t" = "Y"
	then
		t="y"
	fi
done

exit 0
