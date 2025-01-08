# Makefile.in generated automatically by automake 1.5 from Makefile.am.

# Copyright 1994, 1995, 1996, 1997, 1998, 1999, 2000, 2001
# Free Software Foundation, Inc.
# This Makefile.in is free software; the Free Software Foundation
# gives unlimited permission to copy and/or distribute it,
# with or without modifications, as long as this notice is preserved.

# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY, to the extent permitted by law; without
# even the implied warranty of MERCHANTABILITY or FITNESS FOR A
# PARTICULAR PURPOSE.



SHELL = /bin/sh

srcdir = .
top_srcdir = ../..

prefix = /usr/local
exec_prefix = ${prefix}

bindir = ${exec_prefix}/bin
sbindir = ${exec_prefix}/sbin
libexecdir = ${exec_prefix}/libexec
datadir = ${prefix}/share
sysconfdir = ${prefix}/etc
sharedstatedir = ${prefix}/com
localstatedir = ${prefix}/var
libdir = ${exec_prefix}/lib
infodir = ${prefix}/info
mandir = ${prefix}/man
includedir = ${prefix}/include
oldincludedir = /usr/include
pkgdatadir = $(datadir)/libogg
pkglibdir = $(libdir)/libogg
pkgincludedir = $(includedir)/libogg
top_builddir = ../..

ACLOCAL = ${SHELL} /home/icculus/projects/UT2-Code/Vorbis/ogg/missing --run aclocal
AUTOCONF = ${SHELL} /home/icculus/projects/UT2-Code/Vorbis/ogg/missing --run autoconf
AUTOMAKE = ${SHELL} /home/icculus/projects/UT2-Code/Vorbis/ogg/missing --run automake
AUTOHEADER = ${SHELL} /home/icculus/projects/UT2-Code/Vorbis/ogg/missing --run autoheader

INSTALL = /usr/bin/ginstall -c
INSTALL_PROGRAM = ${INSTALL}
INSTALL_DATA = ${INSTALL} -m 644
INSTALL_SCRIPT = ${INSTALL}
INSTALL_HEADER = $(INSTALL_DATA)
transform = s,x,x,
NORMAL_INSTALL = :
PRE_INSTALL = :
POST_INSTALL = :
NORMAL_UNINSTALL = :
PRE_UNINSTALL = :
POST_UNINSTALL = :
host_alias = 
host_triplet = i686-pc-linux-gnu
AMTAR = ${SHELL} /home/icculus/projects/UT2-Code/Vorbis/ogg/missing --run tar
AS = @AS@
AWK = gawk
CC = gcc
CFLAGS = -O20 -ffast-math -fsigned-char -g -O2
DEBUG = -g -Wall -fsigned-char -g -O2
DEPDIR = .deps
DLLTOOL = @DLLTOOL@
ECHO = echo
EXEEXT = 
INSTALL_STRIP_PROGRAM = ${SHELL} $(install_sh) -c -s
LIBS = 
LIBTOOL = $(SHELL) $(top_builddir)/libtool
LIBTOOL_DEPS = ./ltmain.sh
LIB_AGE = 3
LIB_CURRENT = 3
LIB_REVISION = 0
LN_S = ln -s
OBJDUMP = @OBJDUMP@
OBJEXT = o
OPT = 
PACKAGE = libogg
PROFILE = -Wall -W -pg -g -O20 -ffast-math -fsigned-char -g -O2
RANLIB = ranlib
SIZE16 = int16_t
SIZE32 = int32_t
SIZE64 = int64_t
STRIP = strip
USIZE32 = u_int32_t
VERSION = 1.0rc3
am__include = include
am__quote = 
install_sh = /home/icculus/projects/UT2-Code/Vorbis/ogg/install-sh

AUTOMAKE_OPTIONS = foreign

docdir = $(datadir)/doc/$(PACKAGE)-$(VERSION)/ogg

doc_DATA = bitpacking.html datastructures.html decoding.html encoding.html\
	general.html index.html ogg_packet.html ogg_page.html\
	ogg_page_bos.html ogg_page_continued.html ogg_page_eos.html\
	ogg_page_granulepos.html ogg_page_pageno.html ogg_page_serialno.html\
	ogg_page_version.html ogg_stream_clear.html ogg_stream_destroy.html\
	ogg_page_packets.html ogg_stream_eos.html ogg_stream_flush.html\
	ogg_stream_init.html ogg_stream_packetin.html ogg_stream_packetout.html\
	ogg_stream_pagein.html ogg_stream_pageout.html ogg_stream_reset.html\
	ogg_stream_state.html ogg_sync_buffer.html ogg_sync_clear.html\
	ogg_sync_destroy.html ogg_sync_init.html ogg_sync_pageout.html\
	ogg_sync_pageseek.html ogg_sync_reset.html ogg_sync_state.html\
	ogg_sync_wrote.html oggpack_adv.html oggpack_adv1.html\
	oggpack_adv_huff.html oggpack_bits.html oggpack_buffer.html\
	oggpack_bytes.html oggpack_get_buffer.html oggpack_look.html\
	oggpack_look1.html oggpack_look_huff.html oggpack_read.html\
	oggpack_read1.html oggpack_readinit.html oggpack_reset.html\
	oggpack_write.html oggpack_writeclear.html oggpack_writeinit.html\
	overview.html reference.html style.css vorbis_comment.html\
	vorbis_info.html ogg_packet_clear.html ogg_page_checksum_set.html


EXTRA_DIST = $(doc_DATA)
subdir = doc/ogg
mkinstalldirs = $(SHELL) $(top_srcdir)/mkinstalldirs
CONFIG_CLEAN_FILES =
DIST_SOURCES =
DATA = $(doc_DATA)

DIST_COMMON = Makefile.am Makefile.in
all: all-am

.SUFFIXES:

mostlyclean-libtool:
	-rm -f *.lo

clean-libtool:
	-rm -rf .libs _libs

distclean-libtool:
	-rm -f libtool
$(srcdir)/Makefile.in:  Makefile.am  $(top_srcdir)/configure.in $(ACLOCAL_M4)
	cd $(top_srcdir) && \
	  $(AUTOMAKE) --foreign  doc/ogg/Makefile
Makefile:  $(srcdir)/Makefile.in  $(top_builddir)/config.status
	cd $(top_builddir) && \
	  CONFIG_HEADERS= CONFIG_LINKS= \
	  CONFIG_FILES=$(subdir)/$@ $(SHELL) ./config.status
uninstall-info-am:
install-docDATA: $(doc_DATA)
	@$(NORMAL_INSTALL)
	$(mkinstalldirs) $(DESTDIR)$(docdir)
	@list='$(doc_DATA)'; for p in $$list; do \
	  if test -f "$$p"; then d=; else d="$(srcdir)/"; fi; \
	  f="`echo $$p | sed -e 's|^.*/||'`"; \
	  echo " $(INSTALL_DATA) $$d$$p $(DESTDIR)$(docdir)/$$f"; \
	  $(INSTALL_DATA) $$d$$p $(DESTDIR)$(docdir)/$$f; \
	done

uninstall-docDATA:
	@$(NORMAL_UNINSTALL)
	@list='$(doc_DATA)'; for p in $$list; do \
	  f="`echo $$p | sed -e 's|^.*/||'`"; \
	  echo " rm -f $(DESTDIR)$(docdir)/$$f"; \
	  rm -f $(DESTDIR)$(docdir)/$$f; \
	done
tags: TAGS
TAGS:


DISTFILES = $(DIST_COMMON) $(DIST_SOURCES) $(TEXINFOS) $(EXTRA_DIST)

top_distdir = ../..
distdir = $(top_distdir)/$(PACKAGE)-$(VERSION)

distdir: $(DISTFILES)
	@for file in $(DISTFILES); do \
	  if test -f $$file; then d=.; else d=$(srcdir); fi; \
	  dir=`echo "$$file" | sed -e 's,/[^/]*$$,,'`; \
	  if test "$$dir" != "$$file" && test "$$dir" != "."; then \
	    $(mkinstalldirs) "$(distdir)/$$dir"; \
	  fi; \
	  if test -d $$d/$$file; then \
	    cp -pR $$d/$$file $(distdir) \
	    || exit 1; \
	  else \
	    test -f $(distdir)/$$file \
	    || cp -p $$d/$$file $(distdir)/$$file \
	    || exit 1; \
	  fi; \
	done
check-am: all-am
check: check-am
all-am: Makefile $(DATA)

installdirs:
	$(mkinstalldirs) $(DESTDIR)$(docdir)

install: install-am
install-exec: install-exec-am
install-data: install-data-am
uninstall: uninstall-am

install-am: all-am
	@$(MAKE) $(AM_MAKEFLAGS) install-exec-am install-data-am

installcheck: installcheck-am
install-strip:
	$(MAKE) $(AM_MAKEFLAGS) INSTALL_PROGRAM="$(INSTALL_STRIP_PROGRAM)" \
	  `test -z '$(STRIP)' || \
	    echo "INSTALL_PROGRAM_ENV=STRIPPROG='$(STRIP)'"` install
mostlyclean-generic:

clean-generic:

distclean-generic:
	-rm -f Makefile $(CONFIG_CLEAN_FILES) stamp-h stamp-h[0-9]*

maintainer-clean-generic:
	@echo "This command is intended for maintainers to use"
	@echo "it deletes files that may require special tools to rebuild."
clean: clean-am

clean-am: clean-generic clean-libtool mostlyclean-am

distclean: distclean-am

distclean-am: clean-am distclean-generic distclean-libtool

dvi: dvi-am

dvi-am:

info: info-am

info-am:

install-data-am: install-docDATA

install-exec-am:

install-info: install-info-am

install-man:

installcheck-am:

maintainer-clean: maintainer-clean-am

maintainer-clean-am: distclean-am maintainer-clean-generic

mostlyclean: mostlyclean-am

mostlyclean-am: mostlyclean-generic mostlyclean-libtool

uninstall-am: uninstall-docDATA uninstall-info-am

.PHONY: all all-am check check-am clean clean-generic clean-libtool \
	distclean distclean-generic distclean-libtool distdir dvi \
	dvi-am info info-am install install-am install-data \
	install-data-am install-docDATA install-exec install-exec-am \
	install-info install-info-am install-man install-strip \
	installcheck installcheck-am installdirs maintainer-clean \
	maintainer-clean-generic mostlyclean mostlyclean-generic \
	mostlyclean-libtool uninstall uninstall-am uninstall-docDATA \
	uninstall-info-am

# Tell versions [3.59,3.63) of GNU make to not export all variables.
# Otherwise a system limit (for SysV at least) may be exceeded.
.NOEXPORT:
