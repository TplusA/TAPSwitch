#
# Copyright (C) 2017, 2020  T+A elektroakustik GmbH & Co. KG
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

if WITH_DOCTEST
check_PROGRAMS = test_audiopath test_audiopathswitch

TESTS = run_tests.sh

if WITH_VALGRIND
AM_TESTS_ENVIRONMENT = VALGRIND="$(VALGRIND)"
AM_TESTS_ENVIRONMENT += VALGRIND_OPTIONS="--suppressions=$(srcdir)/valgrind.suppressions"
TESTS += valgrind.sh
endif

EXTRA_DIST = run_tests.sh valgrind.sh
CLEANFILES = *.junit.xml *.valgrind.xml

AM_CPPFLAGS = -DDOCTEST_CONFIG_TREAT_CHAR_STAR_AS_STRING
AM_CPPFLAGS += -I$(top_srcdir)/src -I$(top_builddir)/src
AM_CPPFLAGS += -I$(top_srcdir)/dbus_interfaces
AM_CFLAGS = $(CWARNINGS)
AM_CXXFLAGS = $(CXXWARNINGS)

noinst_LTLIBRARIES = libtestrunner.la

libtestrunner_la_SOURCES = testrunner.cc

test_audiopath_SOURCES = \
    test_audiopath.cc
test_audiopath_LDADD = \
    libtestrunner.la \
    $(top_builddir)/src/libaudiopath.la
test_audiopath_CPPFLAGS = $(AM_CPPFLAGS)
test_audiopath_CXXFLAGS = $(TAPSWITCH_DEPENDENCIES_CFLAGS) $(AM_CXXFLAGS)

test_audiopathswitch_SOURCES = \
    test_audiopathswitch.cc \
    mock_audiopath_dbus.hh mock_audiopath_dbus.cc \
    mock_messages.hh mock_messages.cc \
    mock_backtrace.hh mock_backtrace.cc \
    mock_os.hh mock_os.cc \
    mock_expectation.hh
test_audiopathswitch_LDADD = \
    libtestrunner.la \
    $(top_builddir)/src/libaudiopath.la \
    $(TAPSWITCH_DEPENDENCIES_LIBS)
test_audiopathswitch_CPPFLAGS = $(AM_CPPFLAGS)
test_audiopathswitch_CXXFLAGS = $(TAPSWITCH_DEPENDENCIES_CFLAGS) $(AM_CXXFLAGS)

doctest: $(check_PROGRAMS)
	for p in $(check_PROGRAMS); do \
	    if ./$$p $(DOCTEST_EXTRA_OPTIONS); then :; \
	    else \
	        echo "*****"; \
	        echo "***** Running $$p again in gdb due to failures"; \
	        echo "*****"; \
	        gdb -ex run --args ./$$p $(DOCTEST_EXTRA_OPTIONS); \
	    fi; \
	done

doctest-valgrind: $(check_PROGRAMS)
	for p in $(check_PROGRAMS); do $(VALGRIND) --leak-check=full --show-reachable=yes --error-limit=no ./$$p $(DOCTEST_EXTRA_OPTIONS); done
endif
