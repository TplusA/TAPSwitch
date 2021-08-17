/*
 * Copyright (C) 2017, 2018, 2020, 2021  T+A elektroakustik GmbH & Co. KG
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

#ifndef DBUS_HANDLERS_H
#define DBUS_HANDLERS_H

#include <gio/gio.h>

#include "de_tahifi_audiopath.h"

/*!
 * \addtogroup dbus_handlers DBus handlers for signals
 * \ingroup dbus
 */
/*!@{*/

#ifdef __cplusplus
extern "C" {
#endif

gboolean dbusmethod_aupath_register_player(tdbusaupathManager *object,
                                           GDBusMethodInvocation *invocation,
                                           const gchar *player_id,
                                           const gchar *player_name,
                                           const gchar *path,
                                           gpointer user_data);
gboolean dbusmethod_aupath_register_source(tdbusaupathManager *object,
                                           GDBusMethodInvocation *invocation,
                                           const gchar *source_id,
                                           const gchar *source_name,
                                           const gchar *player_id,
                                           const gchar *path,
                                           gpointer user_data);
gboolean dbusmethod_aupath_request_source(tdbusaupathManager *object,
                                          GDBusMethodInvocation *invocation,
                                          const gchar *source_id,
                                          GVariant *arg_request_data,
                                          gpointer user_data);
gboolean dbusmethod_aupath_release_path(tdbusaupathManager *object,
                                        GDBusMethodInvocation *invocation,
                                        gboolean deactivate_player,
                                        GVariant *arg_request_data,
                                        gpointer user_data);
gboolean dbusmethod_aupath_get_active_player(tdbusaupathManager *object,
                                             GDBusMethodInvocation *invocation,
                                             const gchar *source_id,
                                             gpointer user_data);
gboolean dbusmethod_aupath_get_paths(tdbusaupathManager *object,
                                     GDBusMethodInvocation *invocation,
                                     gpointer user_data);
gboolean dbusmethod_aupath_get_current_path(tdbusaupathManager *object,
                                            GDBusMethodInvocation *invocation,
                                            gpointer user_data);
gboolean dbusmethod_aupath_get_player_info(tdbusaupathManager *object,
                                           GDBusMethodInvocation *invocation,
                                           const gchar *player_id,
                                           gpointer user_data);
gboolean dbusmethod_aupath_get_source_info(tdbusaupathManager *object,
                                           GDBusMethodInvocation *invocation,
                                           const gchar *source_id,
                                           gpointer user_data);
gboolean dbusmethod_appliance_set_ready_state(tdbusaupathAppliance *object,
                                              GDBusMethodInvocation *invocation,
                                              const guchar audio_state,
                                              const guchar power_state,
                                              gpointer user_data);
gboolean dbusmethod_appliance_get_state(tdbusaupathAppliance *object,
                                        GDBusMethodInvocation *invocation,
                                        gpointer user_data);

#ifdef __cplusplus
}
#endif

/*!@}*/

#endif /* !DBUS_HANDLERS_H */
