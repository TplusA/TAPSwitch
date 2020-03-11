#
# Copyright (C) 2020  T+A elektroakustik GmbH & Co. KG
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

compiler = meson.get_compiler('cpp')

if not compiler.has_header('doctest.h')
    subdir_done()
endif

testrunner_lib = static_library('testrunner', 'testrunner.cc',
    cpp_args: '-DDOCTEST_CONFIG_TREAT_CHAR_STAR_AS_STRING',
    include_directories: '../src'
)

test('Audio path',
    executable('test_audiopath',
        'test_audiopath.cc',
        include_directories: '../src',
        link_with: [testrunner_lib, audiopath_lib],
        cpp_args: '-DDOCTEST_CONFIG_TREAT_CHAR_STAR_AS_STRING',
        build_by_default: false),
    workdir: meson.current_build_dir(),
    args: ['--reporters=strboxml', '--out=test_connman_service.junit.xml']
)

test('Audio path switching',
    executable('test_audiopathswitch',
        ['test_audiopathswitch.cc', 'mock_audiopath_dbus.cc',
         'mock_messages.cc', 'mock_backtrace.cc', 'mock_os.cc'],
        include_directories: '../src',
        link_with: [testrunner_lib, audiopath_lib],
        cpp_args: '-DDOCTEST_CONFIG_TREAT_CHAR_STAR_AS_STRING',
        dependencies: glib_deps,
        build_by_default: false),
    workdir: meson.current_build_dir(),
    args: ['--reporters=strboxml', '--out=test_connman_service.junit.xml']
)
