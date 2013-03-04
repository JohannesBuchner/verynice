#
#    VeryNice -- a dynamic process re-nicer
#    Copyright (C) 2000 Stephen D. Holland
#
#    This program is free software; you can redistribute it and/or modify
#    it under the terms of the GNU General Public License as published by
#    the Free Software Foundation; version 2 of the License.
#
#    This program is distributed in the hope that it will be useful,
#    but WITHOUT ANY WARRANTY; without even the implied warranty of
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#    GNU General Public License for more details.
#
#    You should have received a copy of the GNU General Public License
#    along with this program; if not, write to the Free Software
#    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

CC = gcc

# PREFIX is usually either /usr or /usr/local
PREFIX=/usr/local
TARGET=linux

# Solaris users probably need to use this:
#INSTALL=/usr/ucb/install

VERSION=0.5

#CFLAGS= -I../include/ -O3 -Wimplicit
CFLAGS= -I../include/ -g -Wimplicit -DPREFIX=\"$(PREFIX)\" -DTARGET_$(TARGET)
LINK = gcc 
AG = /home3/sdh4/anagram/ag_unix_dev/ag



all: verynice

clean:
	rm -f *.o *~ core

distclean: clean
	rm -f verynice

dist: distclean
	(cd .. ; tar cvzf verynice-$(VERSION).tar.gz verynice/ )

install: 
	install -d $(PREFIX)/sbin
	install verynice $(PREFIX)/sbin	
	if [ $(PREFIX) = "/usr" ]; then \
	  mv -f /etc/verynice.conf /etc/verynice.conf~ ; \
	  install verynice.conf /etc ; \
	else \
	  install -d $(PREFIX)/etc ; \
	  mv -f $(PREFIX)/etc/verynice.conf $(PREFIX)/etc/verynice.conf~ ; \
	  install verynice.conf $(PREFIX)/etc ; \
	fi
	install -d $(PREFIX)/doc 
	install -d $(PREFIX)/doc/verynice-$(VERSION)
	install verynice.html $(PREFIX)/doc/verynice-$(VERSION)

%.c: %.syn
	$(AG) -b $<

# convenience rule for analyzing a grammar
anal.%: %.syn
	$(AG) $*

verynice: verynice.o config.o linklist.o stringstack.o 
	$(LINK) -g -o $@ $^ -lm




# dependencies
config.o: config.c linklist.h stringstack.h verynice.h
config.c: config.syn
linklist.o: linklist.c linklist.h
stringstack.o: stringstack.c stringstack.h linklist.h
verynice.o: verynice.c verynice.h linklist.h



