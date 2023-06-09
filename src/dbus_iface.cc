/*
 * Copyright (C) 2017, 2018, 2020, 2021, 2023  T+A elektroakustik GmbH & Co. KG
 *
 * This file is part of TAPSwitch.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA  02110-1301, USA.
 */

#if HAVE_CONFIG_H
#include <config.h>
#endif /* HAVE_CONFIG_H */

#include <string.h>

#include "dbus_iface.h"
#include "dbus_iface_deep.h"
#include "dbus_handlers.h"
#include "de_tahifi_audiopath.h"
#include "messages.h"
#include "messages_dbus.h"
#include "gerrorwrapper.hh"

class DBusData
{
  public:
    guint owner_id;
    int name_acquired;

    void *handler_data;

    tdbusaupathManager *audiopath_manager_iface;
    tdbusaupathAppliance *audiopath_appliance_iface;

    tdbusdebugLogging *debug_logging_iface;
    tdbusdebugLoggingConfig *debug_logging_config_proxy;
};

static void try_export_iface(GDBusConnection *connection,
                             GDBusInterfaceSkeleton *iface)
{
    GErrorWrapper error;
    g_dbus_interface_skeleton_export(iface, connection,
                                     "/de/tahifi/TAPSwitch", error.await());
    error.log_failure("Export interface");
}

static void bus_acquired(GDBusConnection *connection,
                         const gchar *name, gpointer user_data)
{
    auto &data = *static_cast<DBusData *>(user_data);

    msg_info("D-Bus \"%s\" acquired", name);

    data.audiopath_manager_iface = tdbus_aupath_manager_skeleton_new();
    data.audiopath_appliance_iface = tdbus_aupath_appliance_skeleton_new();
    data.debug_logging_iface = tdbus_debug_logging_skeleton_new();

    g_signal_connect(data.audiopath_manager_iface, "handle-register-player",
                     G_CALLBACK(dbusmethod_aupath_register_player),
                     data.handler_data);
    g_signal_connect(data.audiopath_manager_iface, "handle-register-source",
                     G_CALLBACK(dbusmethod_aupath_register_source),
                     data.handler_data);
    g_signal_connect(data.audiopath_manager_iface, "handle-request-source",
                     G_CALLBACK(dbusmethod_aupath_request_source),
                     data.handler_data);
    g_signal_connect(data.audiopath_manager_iface, "handle-release-path",
                     G_CALLBACK(dbusmethod_aupath_release_path),
                     data.handler_data);
    g_signal_connect(data.audiopath_manager_iface, "handle-get-active-player",
                     G_CALLBACK(dbusmethod_aupath_get_active_player),
                     data.handler_data);
    g_signal_connect(data.audiopath_manager_iface, "handle-get-paths",
                     G_CALLBACK(dbusmethod_aupath_get_paths),
                     data.handler_data);
    g_signal_connect(data.audiopath_manager_iface, "handle-get-current-path",
                     G_CALLBACK(dbusmethod_aupath_get_current_path),
                     data.handler_data);
    g_signal_connect(data.audiopath_manager_iface, "handle-get-player-info",
                     G_CALLBACK(dbusmethod_aupath_get_player_info),
                     data.handler_data);
    g_signal_connect(data.audiopath_manager_iface, "handle-get-source-info",
                     G_CALLBACK(dbusmethod_aupath_get_source_info),
                     data.handler_data);

    g_signal_connect(data.audiopath_appliance_iface, "handle-set-ready-state",
                     G_CALLBACK(dbusmethod_appliance_set_ready_state),
                     data.handler_data);
    g_signal_connect(data.audiopath_appliance_iface, "handle-get-state",
                     G_CALLBACK(dbusmethod_appliance_get_state),
                     data.handler_data);

    g_signal_connect(data.debug_logging_iface,
                     "handle-debug-level",
                     G_CALLBACK(msg_dbus_handle_debug_level), nullptr);

    try_export_iface(connection, G_DBUS_INTERFACE_SKELETON(data.audiopath_manager_iface));
    try_export_iface(connection, G_DBUS_INTERFACE_SKELETON(data.audiopath_appliance_iface));
    try_export_iface(connection, G_DBUS_INTERFACE_SKELETON(data.debug_logging_iface));
}

static void connect_signals_debug(GDBusConnection *connection,
                                  DBusData &data, GDBusProxyFlags flags,
                                  const char *bus_name, const char *object_path)
{
    GErrorWrapper error;
    data.debug_logging_config_proxy =
        tdbus_debug_logging_config_proxy_new_sync(connection, flags,
                                                  bus_name, object_path,
                                                  nullptr, error.await());
    error.log_failure("Create Debug proxy");
}

static void name_acquired(GDBusConnection *connection,
                          const gchar *name, gpointer user_data)
{
    auto &data = *static_cast<DBusData *>(user_data);

    msg_info("D-Bus name \"%s\" acquired", name);
    data.name_acquired = 1;

    connect_signals_debug(connection, data, G_DBUS_PROXY_FLAGS_NONE,
                          "de.tahifi.TAPSwitch", "/de/tahifi/TAPSwitch");
}

static void name_lost(GDBusConnection *connection,
                      const gchar *name, gpointer user_data)
{
    auto &data = *static_cast<DBusData *>(user_data);

    msg_vinfo(MESSAGE_LEVEL_IMPORTANT, "D-Bus name \"%s\" lost", name);
    data.name_acquired = -1;
}

static void destroy_notification(gpointer data)
{
    msg_vinfo(MESSAGE_LEVEL_IMPORTANT, "Bus destroyed.");
}

static DBusData dbus_data;

int dbus_setup(GMainLoop *loop, bool connect_to_session_bus,
               void *dbus_data_for_dbus_handlers)
{
#if !GLIB_CHECK_VERSION(2, 36, 0)
    g_type_init();
#endif

    memset(&dbus_data, 0, sizeof(dbus_data));

    GBusType bus_type =
        connect_to_session_bus ? G_BUS_TYPE_SESSION : G_BUS_TYPE_SYSTEM;

    static const char bus_name[] = "de.tahifi.TAPSwitch";

    dbus_data.handler_data = dbus_data_for_dbus_handlers;
    dbus_data.owner_id =
        g_bus_own_name(bus_type, bus_name, G_BUS_NAME_OWNER_FLAGS_NONE,
                       bus_acquired, name_acquired, name_lost, &dbus_data,
                       destroy_notification);

    while(dbus_data.name_acquired == 0)
    {
        /* do whatever has to be done behind the scenes until one of the
         * guaranteed callbacks gets called */
        g_main_context_iteration(nullptr, TRUE);
    }

    if(dbus_data.name_acquired < 0)
    {
        msg_error(0, LOG_EMERG, "Failed acquiring D-Bus name");
        return -1;
    }

    msg_log_assert(dbus_data.audiopath_manager_iface != nullptr);
    msg_log_assert(dbus_data.audiopath_appliance_iface != nullptr);
    msg_log_assert(dbus_data.debug_logging_iface != nullptr);
    msg_log_assert(dbus_data.debug_logging_config_proxy != nullptr);

    g_signal_connect(dbus_data.debug_logging_config_proxy, "g-signal",
                     G_CALLBACK(msg_dbus_handle_global_debug_level_changed),
                     nullptr);

    g_main_loop_ref(loop);

    return 0;
}

void dbus_shutdown(GMainLoop *loop)
{
    if(loop == nullptr)
        return;

    g_bus_unown_name(dbus_data.owner_id);

    g_object_unref(dbus_data.audiopath_manager_iface);
    g_object_unref(dbus_data.audiopath_appliance_iface);
    g_object_unref(dbus_data.debug_logging_iface);
    g_object_unref(dbus_data.debug_logging_config_proxy);

    g_main_loop_unref(loop);
}

tdbusaupathManager *dbus_get_audiopath_manager_iface(void)
{
    return dbus_data.audiopath_manager_iface;
}
