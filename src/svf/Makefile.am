#
# $Id: Makefile.am 1171 2008-04-18 22:48:38Z arniml $
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
# 02111-1307, USA.
#
# Written by A. Laeuger <arniml@sourceforge.net>, 2007.
#

include $(top_srcdir)/Makefile.rules

AM_YFLAGS = -d

noinst_LIBRARIES = libsvf.a

libsvf_a_SOURCES = \
	svf_bison.y \
	svf_flex.l \
	svf.h \
	svf.c

svf_flex.$(OBJEXT) svf.$(OBJEXT): svf_bison.$(OBJEXT)

svf_bison.h: svf_bison.c

MAINTAINERCLEANFILES = \
	svf_bison.c \
	svf_bison.h \
	svf_flex.c
