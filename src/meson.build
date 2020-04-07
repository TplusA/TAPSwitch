#
# Copyright (C) 2020  T+A elektroakustik GmbH & Co. KG
#
# This file is part of TAPSwitch.
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
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
# MA  02110-1301, USA.
#


config_h = declare_dependency(
    sources: configure_file(input: '../config.h.meson',
                            output: 'config.h',
                            configuration: config_data),
    include_directories: '.'
  )

gnome = import('gnome')

dbus_iface_dir = '../dbus_interfaces'
dbus_iface_defs_includes = include_directories(dbus_iface_dir)

dbus_iface_data = [
    ['de_tahifi_audiopath', 'de.tahifi.AudioPath.', 'tdbus_aupath', 'Audio Paths'],
    ['de_tahifi_debug',     'de.tahifi.Debug.',     'tdbus_debug',  'Debug Levels'],
]

dbus_deps = []
dbus_docs = []

foreach d : dbus_iface_data
    dbus_deps += declare_dependency(
        link_with: static_library(
            d[0].split('_')[-1] + '_dbus',
            gnome.gdbus_codegen(d[0],
                                sources: dbus_iface_dir / d[0] + '.xml',
                                interface_prefix: d[1],
                                namespace: d[2]),
            dependencies: [glib_deps, config_h],
            c_args: relaxed_dbus_warnings),
        include_directories: dbus_iface_defs_includes
    )

    dbus_docs += custom_target(d[0] + '_docs',
        input: dbus_iface_dir / d[0] + '.xml',
        output: ['@BASENAME@-doc.md', '@BASENAME@-doc.h'],
        command: [
            extract_docs, '-i', '@INPUT@', '-o', '@OUTPUT0@', '-H', '@OUTPUT1@',
            '-c', d[2], '-s', d[1], '-n', d[3]
        ])
endforeach

audiopath_lib = static_library('audiopath',
    ['audiopath.cc', 'audiopathswitch.cc', 'appliance.cc', 'gvariantwrapper.cc'],
    dependencies: [glib_deps, config_h]
)

executable(
    'tapswitch',
    [
        'tapswitch.cc', 'messages.c', 'messages_glib.c', 'messages_dbus.c',
        'dbus_iface.cc', 'backtrace.c', 'os.c',
        version_info,
    ],
    dependencies: [dbus_deps, glib_deps, config_h],
    link_with: [
        static_library('dbus_handlers',
            ['dbus_handlers.cc', 'messages_dbus.c'],
            dependencies: [dbus_deps, glib_deps, config_h],
        ),
        audiopath_lib,
    ],
    install: true
)
