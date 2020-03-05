#
# Copyright (C) 2017, 2018, 2020  T+A elektroakustik GmbH & Co. KG
#
# This file is part of TAPSwitch.
#
# TAPSwitch is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License, version 3 as
# published by the Free Software Foundation.
#
# TAPSwitch is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with TAPSwitch.  If not, see <http://www.gnu.org/licenses/>.
#

ACLOCAL_AMFLAGS = -I ../m4

bin_PROGRAMS = tapswitch

tapswitch_SOURCES = \
    tapswitch.cc audiopath.hh dbus_proxy_wrapper.hh appliance.hh maybe.hh \
    messages.h messages.c messages_glib.h messages_glib.c \
    messages_dbus.c messages_dbus.h \
    dbus_iface.cc dbus_iface.h dbus_iface_deep.h gerrorwrapper.hh \
    dbus_handlers.h dbus_handlers.hh \
    backtrace.c backtrace.h os.c os.h

DBUS_IFACES = $(top_srcdir)/dbus_interfaces

AM_CPPFLAGS = -DLOCALEDIR=\"$(localedir)\"
AM_CPPFLAGS += -I$(DBUS_IFACES)
AM_CPPFLAGS += $(TAPSWITCH_DEPENDENCIES_CFLAGS)

AM_CFLAGS = $(CWARNINGS)

AM_CXXFLAGS = $(CXXWARNINGS)

noinst_LTLIBRARIES = \
    libdbus_handlers.la \
    libaudiopath_dbus.la \
    libaudiopath.la \
    libdebug_dbus.la

tapswitch_LDADD = $(noinst_LTLIBRARIES) $(TAPSWITCH_DEPENDENCIES_LIBS)

libaudiopath_la_SOURCES = \
    audiopath.cc audiopath.hh \
    audiopathswitch.cc audiopathswitch.hh \
    appliance.cc appliance.hh maybe.hh \
    gvariantwrapper.cc gvariantwrapper.hh \
    dbus_proxy_wrapper.hh
libaudiopath_la_CFLAGS = $(AM_CFLAGS)
libaudiopath_la_CXXFLAGS = $(AM_CXXFLAGS)

libdbus_handlers_la_SOURCES = \
    dbus_handlers.h dbus_handlers.hh dbus_handlers.cc \
    messages_dbus.h messages_dbus.c
libdbus_handlers_la_CFLAGS = $(AM_CFLAGS)
libdbus_handlers_la_CXXFLAGS = $(AM_CXXFLAGS)

nodist_libaudiopath_dbus_la_SOURCES = de_tahifi_audiopath.c de_tahifi_audiopath.h
libaudiopath_dbus_la_CFLAGS = $(CRELAXEDWARNINGS)

nodist_libdebug_dbus_la_SOURCES = de_tahifi_debug.c de_tahifi_debug.h
libdebug_dbus_la_CFLAGS = $(CRELAXEDWARNINGS)

BUILT_SOURCES = \
    $(nodist_libaudiopath_dbus_la_SOURCES) \
    de_tahifi_audiopath-doc.h \
    de_tahifi_audiopath-doc.md \
    de_tahifi_audiopath.stamp \
    $(nodist_libdebug_dbus_la_SOURCES) \
    de_tahifi_debug-doc.h \
    de_tahifi_debug-doc.md \
    de_tahifi_debug.stamp

CLEANFILES = $(BUILT_SOURCES)

de_tahifi_audiopath-doc.h: de_tahifi_audiopath.stamp
de_tahifi_audiopath-doc.md: de_tahifi_audiopath.stamp
de_tahifi_audiopath.c: de_tahifi_audiopath.stamp
de_tahifi_audiopath.h: de_tahifi_audiopath.stamp
de_tahifi_audiopath.stamp: $(DBUS_IFACES)/de_tahifi_audiopath.xml
	$(GDBUS_CODEGEN) --generate-c-code=de_tahifi_audiopath --c-namespace tdbus_aupath --interface-prefix de.tahifi.AudioPath. $<
	$(DBUS_IFACES)/extract_documentation.py -i $< -o de_tahifi_audiopath-doc.md -H de_tahifi_audiopath-doc.h -c tdbus_aupath -s de.tahifi.AudioPath. -n 'Audio Paths'
	touch $@

de_tahifi_debug-doc.h: de_tahifi_debug.stamp
de_tahifi_debug-doc.md: de_tahifi_debug.stamp
de_tahifi_debug.c: de_tahifi_debug.stamp
de_tahifi_debug.h: de_tahifi_debug.stamp
de_tahifi_debug.stamp: $(DBUS_IFACES)/de_tahifi_debug.xml
	$(GDBUS_CODEGEN) --generate-c-code=de_tahifi_debug --c-namespace tdbus_debug --interface-prefix de.tahifi.Debug. $<
	$(DBUS_IFACES)/extract_documentation.py -i $< -o de_tahifi_debug-doc.md -H de_tahifi_debug-doc.h -c tdbus_debug -s de.tahifi.Debug. -n "Introspection"
	touch $@
