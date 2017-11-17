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

CC ?= gcc


RPM_BUILD_ROOT=

# PREFIX is usually either /usr or /usr/local
PREFIX=/usr/local
BINDIR=$(PREFIX)/sbin
ETCDIR=/etc
TARGET=linux


INSTALL=install
# Solaris users probably need to use this:
#INSTALL=/usr/ucb/install

VERSION=1.1.1

#CFLAGS= -I../include/ -O3 -Wimplicit
CFLAGS += -Wimplicit
CPPFLAGS = -I../include/ -DPREFIX=\"$(PREFIX)\" -DTARGET_$(TARGET) -DVERSION=\"$(VERSION)\"
AG = /home3/sdh4/anagram/ag_unix_dev/ag



all: verynice

clean:
	rm -f *.o *~ core

distclean: clean
	rm -f verynice

# distribution build procedure:
# make dist
# as root: make buildrpm
# make postbuildrpm

dist: distclean
	(cd .. ; tar cvzf verynice-$(VERSION).tar.gz verynice/ )

install:
	$(INSTALL) -d $(RPM_BUILD_ROOT)$(BINDIR)
	$(INSTALL) verynice $(RPM_BUILD_ROOT)$(BINDIR)
	$(INSTALL) -d $(RPM_BUILD_ROOT)$(ETCDIR)
	$(INSTALL) -m 644 verynice.conf $(RPM_BUILD_ROOT)$(ETCDIR)
	$(INSTALL) -d $(RPM_BUILD_ROOT)$(PREFIX)/share
	$(INSTALL) -d $(RPM_BUILD_ROOT)$(PREFIX)/share/doc
	$(INSTALL) -d $(RPM_BUILD_ROOT)$(PREFIX)/share/doc/verynice-$(VERSION)
	$(INSTALL) -d $(RPM_BUILD_ROOT)$(PREFIX)/share/doc/verynice-$(VERSION)/html
	$(INSTALL) -m 644 verynice.html $(RPM_BUILD_ROOT)$(PREFIX)/share/doc/verynice-$(VERSION)/html
	$(INSTALL) -m 644 README $(RPM_BUILD_ROOT)$(PREFIX)/share/doc/verynice-$(VERSION)
	$(INSTALL) -m 644 README.SYN $(RPM_BUILD_ROOT)$(PREFIX)/share/doc/verynice-$(VERSION)
	$(INSTALL) -m 644 COPYING $(RPM_BUILD_ROOT)$(PREFIX)/share/doc/verynice-$(VERSION)
	$(INSTALL) -m 644 CHANGELOG $(RPM_BUILD_ROOT)$(PREFIX)/share/doc/verynice-$(VERSION)


buildrpm:
	cp ../verynice-$(VERSION).tar.gz /usr/src/redhat/SOURCES
	rm -f /usr/src/redhat/SRPMS/verynice-$(VERSION)-1.src.rpm
	rm -f /usr/src/redhat/RPMS/*/verynice-$(VERSION)-1.*.rpm

	sed 's/VERSION/$(VERSION)/' <verynice.spec >/usr/src/redhat/SOURCES/verynice.spec
	rpm -ba /usr/src/redhat/SOURCES/verynice.spec

postbuildrpm:
	cp /usr/src/redhat/SRPMS/verynice-$(VERSION)-1.src.rpm ..
	cp /usr/src/redhat/RPMS/*/verynice-$(VERSION)-1.*.rpm ..

%.c: %.syn
	$(AG) -b $<

# convenience rule for analyzing a grammar
anal.%: %.syn
	$(AG) $*

verynice: verynice.o config.o linklist.o stringstack.o
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^ -lm




# dependencies
config.o: config.c linklist.h stringstack.h verynice.h
config.c: config.syn
linklist.o: linklist.c linklist.h
stringstack.o: stringstack.c stringstack.h linklist.h
verynice.o: verynice.c verynice.h linklist.h



