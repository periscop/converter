#
#   /*------------------------------------------------------------------+
#    |                             Converter                            |
#    |------------------------------------------------------------------|
#    |                          autoconf.ac                             |
#    |------------------------------------------------------------------|
#    |                   First version: 18/10/2012                      |
#    +------------------------------------------------------------------+
#
# +------------------------------------------------------------------------+
# |                                                                        |
# |                                 Converter [osl <-> scoplib]            |
# |                                                                        |
# +------------------------------------------------------------------------+
# | Copyright (C) 2012 University of Paris-Sud                             |
# |                                                                        |
# | This library is free software; you can redistribute it and/or modify   |
# | it under the terms of the GNU Lesser General Public License as         |
# | published by the Free Software Foundation; either version 2.1 of the   |
# | License, or (at your option) any later version.                        |
# |                                                                        |
# | This library is distributed in the hope that it will be useful but     |
# | WITHOUT ANY WARRANTY; without even the implied warranty of             |
# | MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU       |
# | Lesser General Public License for more details.                        |
# |                                                                        |
# | You should have received a copy of the GNU Lesser General Public       |
# | License along with this software; if not, write to the Free Software   |
# | Foundation, Inc., 51 Franklin Street, Fifth Floor,                     |
# | Boston, MA  02110-1301  USA                                            |
# |                                                                        |
# | Clay, the Chunky Loop Alteration wizardrY                              |
# | Written by Cedric Bastoul, Cedric.Bastoul@u-psud.fr                    |
# +------------------------------------------------------------------------*/
#
# Makefile.am (or makefile if generated) of Clay.
# Makefile.am is not a makefile, you must run the 'autogen.sh' THEN the
# configure shellscript to generate the Makefile thanks to this file.
#
#############################################################################

if BUNDLED_OSL
    MAYBE_OSL = osl
    OSL_LA = $(top_builddir)/osl/source/libosl.la
endif

SUBDIRS         = $(MAYBE_OSL) doc
DIST_SUBDIRS    = $(MAYBE_OSL) doc
ACLOCAL_AMFLAGS = -I m4

#############################################################################

bin_PROGRAMS    = converter 
lib_LTLIBRARIES = libconverter.la

#############################################################################

pkginclude_HEADERS = \
	include/converter/converter.h

DEFAULT_INCLUDES = -I.
INCLUDES = -I$(top_builddir)/include -I$(top_srcdir)/include
AM_CFLAGS = $(CFLAGS_WARN)
YFLAGS = -y -d

#############################################################################

libconverter_la_LIBADD   = @OSL_LIBS@ @SCOPLIB_LIBS@ $(OSL_LA)
libconverter_la_CPPFLAGS = @OSL_CPPFLAGS@ @SCOPLIB_CPPFLAGS@ -g
libconverter_la_LDFLAGS  = @OSL_LDFLAGS@ @SCOPLIB_LDFLAGS@
libconverter_la_SOURCES  = \
			source/converter.c \
			source/osl2scoplib.c \
			source/scoplib2osl.c \
			source/converter_int.c	

#############################################################################

LDADD             = @OSL_LIBS@ @SCOPLIB_LIBS@ libconverter.la
converter_CPPFLAGS     = @OSL_CPPFLAGS@ @SCOPLIB_CPPFLAGS@ -g
converter_LDFLAGS      = @OSL_LDFLAGS@ @SCOPLIB_LDFLAGS@ # TO BE REMOVED
converter_DEPENDENCIES = libconverter.la
converter_SOURCES      = source/converter.c source/osl2scoplib.c source/scoplib2osl.c source/converter_int.c

#############################################################################

MAINTAINERCLEANFILES = \
	Makefile.in \
	aclocal.m4 \
	configure \
	$(AUX_DIST)

#############################################################################

dist-hook:
	(cd $(distdir) && mkdir -p $(ac_aux_dir))
	for file in $(AUX_DIST); do \
	  cp $$file $(distdir)/$$file; \
	done

#############################################################################
