#
# Copyright (C) 2017, 2018  T+A elektroakustik GmbH & Co. KG
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
    dbus_iface.c dbus_iface.h dbus_iface_deep.h dbus_common.h \
    dbus_handlers.h dbus_handlers.hh \
    os.c os.h

DBUS_IFACES = $(top_srcdir)/dbus_interfaces

AM_CPPFLAGS = -DLOCALEDIR=\"$(localedir)\"
AM_CPPFLAGS += -I$(DBUS_IFACES)
AM_CPPFLAGS += $(TAPSWITCH_DEPENDENCIES_CFLAGS)

AM_CFLAGS = $(CWARNINGS)

AM_CXXFLAGS = $(CXXWARNINGS)

noinst_LTLIBRARIES = \
    libdbus_handlers.la \
    libdbus_common.la \
    libaudiopath_dbus.la \
    libaudiopath.la \
    libdebug_dbus.la

tapswitch_LDADD = $(noinst_LTLIBRARIES) $(TAPSWITCH_DEPENDENCIES_LIBS)

libaudiopath_la_SOURCES = \
    audiopath.cc audiopath.hh \
    audiopathswitch.cc audiopathswitch.hh \
    appliance.cc appliance.hh maybe.hh \
    dbus_proxy_wrapper.hh
libaudiopath_la_CFLAGS = $(AM_CFLAGS)
libaudiopath_la_CXXFLAGS = $(AM_CXXFLAGS)

libdbus_handlers_la_SOURCES = \
    dbus_handlers.h dbus_handlers.hh dbus_handlers.cc \
    messages_dbus.h messages_dbus.c
libdbus_handlers_la_CFLAGS = $(AM_CFLAGS)
libdbus_handlers_la_CXXFLAGS = $(AM_CXXFLAGS)

libdbus_common_la_SOURCES = dbus_common.c dbus_common.h
libdbus_common_la_CFLAGS = $(AM_CFLAGS)
libdbus_common_la_CXXFLAGS = $(AM_CXXFLAGS)

nodist_libdebug_dbus_la_SOURCES = debug_dbus.c debug_dbus.h
libdebug_dbus_la_CFLAGS = $(CRELAXEDWARNINGS)

nodist_libaudiopath_dbus_la_SOURCES = audiopath_dbus.c audiopath_dbus.h
libaudiopath_dbus_la_CFLAGS = $(CRELAXEDWARNINGS)

BUILT_SOURCES = \
    $(nodist_libaudiopath_dbus_la_SOURCES) \
    de_tahifi_audiopath.h \
    de_tahifi_audiopath.md \
    audiopath_dbus.stamp \
    $(nodist_libdebug_dbus_la_SOURCES) \
    de_tahifi_debug.h \
    de_tahifi_debug.md \
    debug_dbus.stamp

CLEANFILES = $(BUILT_SOURCES)

de_tahifi_audiopath.h: audiopath_dbus.stamp
de_tahifi_audiopath.md: audiopath_dbus.stamp
audiopath_dbus.c: audiopath_dbus.stamp
audiopath_dbus.h: audiopath_dbus.stamp
audiopath_dbus.stamp: $(DBUS_IFACES)/de_tahifi_audiopath.xml
	$(GDBUS_CODEGEN) --generate-c-code=audiopath_dbus --c-namespace tdbus_aupath --interface-prefix de.tahifi.AudioPath. $<
	$(DBUS_IFACES)/extract_documentation.py -i $< -o de_tahifi_audiopath.md -H de_tahifi_audiopath.h -c tdbus_aupath -s de.tahifi.AudioPath. -n 'Audio Paths'
	touch $@

de_tahifi_debug.h: debug_dbus.stamp
de_tahifi_debug.md: debug_dbus.stamp
debug_dbus.c: debug_dbus.stamp
debug_dbus.h: debug_dbus.stamp
debug_dbus.stamp: $(DBUS_IFACES)/de_tahifi_debug.xml
	$(GDBUS_CODEGEN) --generate-c-code=debug_dbus --c-namespace tdbus_debug --interface-prefix de.tahifi.Debug. $<
	$(DBUS_IFACES)/extract_documentation.py -i $< -o de_tahifi_debug.md -H de_tahifi_debug.h -c tdbus_debug -s de.tahifi.Debug. -n "Introspection"
	touch $@
