/*
 * Copyright (C) 2017  T+A elektroakustik GmbH & Co. KG
 *
 * This file is part of TAPSwitch.
 *
 * TAPSwitch is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 3 as
 * published by the Free Software Foundation.
 *
 * TAPSwitch is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with TAPSwitch.  If not, see <http://www.gnu.org/licenses/>.
 */

#if HAVE_CONFIG_H
#include <config.h>
#endif /* HAVE_CONFIG_H */

#include "dbus_handlers.hh"
#include "dbus_handlers.h"
#include "dbus_iface_deep.h"
#include "messages.h"

namespace DBus
{

template <>
std::unique_ptr<AudioPath::Player::PType>
mk_proxy(const char *dest, const char *obj_path)
{
    GDBusConnection *connection =
        g_dbus_interface_skeleton_get_connection(G_DBUS_INTERFACE_SKELETON(dbus_get_audiopath_manager_iface()));

    GError *error = nullptr;

    tdbusaupathPlayer *proxy =
        tdbus_aupath_player_proxy_new_sync(connection, G_DBUS_PROXY_FLAGS_NONE,
                                           dest, obj_path, nullptr, &error);
    dbus_handle_error(&error, "Create AudioPath.Player proxy");

    return std::unique_ptr<AudioPath::Player::PType>(new AudioPath::Player::PType(proxy));
}

template<>
Proxy<tdbusaupathPlayer>::~Proxy()
{
    if(proxy_ == nullptr)
        return;

    g_object_unref(proxy_);
    proxy_ = nullptr;
}

template <>
std::unique_ptr<AudioPath::Source::PType>
mk_proxy(const char *dest, const char *obj_path)
{
    GDBusConnection *connection =
        g_dbus_interface_skeleton_get_connection(G_DBUS_INTERFACE_SKELETON(dbus_get_audiopath_manager_iface()));

    GError *error = nullptr;

    tdbusaupathSource *proxy =
        tdbus_aupath_source_proxy_new_sync(connection, G_DBUS_PROXY_FLAGS_NONE,
                                           dest, obj_path, nullptr, &error);
    dbus_handle_error(&error, "Create AudioPath.Source proxy");

    return std::unique_ptr<AudioPath::Source::PType>(new AudioPath::Source::PType(proxy));
}

template<>
Proxy<tdbusaupathSource>::~Proxy()
{
    if(proxy_ == nullptr)
        return;

    g_object_unref(proxy_);
    proxy_ = nullptr;
}

}

gboolean dbusmethod_aupath_register_player(tdbusaupathManager *object,
                                           GDBusMethodInvocation *invocation,
                                           const gchar *player_id,
                                           const gchar *player_name,
                                           const gchar *path,
                                           gpointer user_data)
{
    if(player_id[0] == '\0' || player_name == '\0' || path == '\0')
    {
        g_dbus_method_invocation_return_error_literal(invocation,
                                                      G_DBUS_ERROR, G_DBUS_ERROR_INVALID_ARGS,
                                                      "Empty argument");
        return TRUE;
    }

    auto *data = static_cast<DBus::HandlerData *>(user_data);

    const char *dest =
        g_dbus_message_get_sender(g_dbus_method_invocation_get_message(invocation));

    msg_vinfo(MESSAGE_LEVEL_DIAG,
              "Register player %s (\"%s\") running on %s, object %s",
              player_id, player_name, dest, path);

    data->audio_paths_.add_player(std::move(
        AudioPath::Player(player_id, player_name,
                          DBus::mk_proxy<AudioPath::Player::PType>(dest, path))));

    tdbus_aupath_manager_complete_register_player(object, invocation);

    return TRUE;
}

gboolean dbusmethod_aupath_register_source(tdbusaupathManager *object,
                                           GDBusMethodInvocation *invocation,
                                           const gchar *source_id,
                                           const gchar *source_name,
                                           const gchar *player_id,
                                           const gchar *path,
                                           gpointer user_data)
{
    if(source_id[0] == '\0' || source_name == '\0' || player_id[0] == '\0' ||
       path == '\0')
    {
        g_dbus_method_invocation_return_error_literal(invocation,
                                                      G_DBUS_ERROR, G_DBUS_ERROR_INVALID_ARGS,
                                                      "Empty argument");
        return TRUE;
    }

    auto *data = static_cast<DBus::HandlerData *>(user_data);

    const char *dest =
        g_dbus_message_get_sender(g_dbus_method_invocation_get_message(invocation));

    msg_vinfo(MESSAGE_LEVEL_DIAG,
              "Register source %s (\"%s\") for player %s running on %s, object %s",
              source_id, source_name, player_id, dest, path);

    data->audio_paths_.add_source(std::move(
        AudioPath::Source(source_id, source_name, player_id,
                          DBus::mk_proxy<AudioPath::Source::PType>(dest, path))));

    tdbus_aupath_manager_complete_register_source(object, invocation);

    return TRUE;
}

gboolean dbusmethod_aupath_request_source(tdbusaupathManager *object,
                                          GDBusMethodInvocation *invocation,
                                          const gchar *source_id,
                                          gpointer user_data)
{
    BUG("%s(): not implemented yet", __func__);
    return FALSE;
}

gboolean dbusmethod_aupath_release_path(tdbusaupathManager *object,
                                        GDBusMethodInvocation *invocation,
                                        gboolean deactivate_player,
                                        gpointer user_data)
{
    BUG("%s(): not implemented yet", __func__);
    return FALSE;
}

gboolean dbusmethod_aupath_get_active_player(tdbusaupathManager *object,
                                             GDBusMethodInvocation *invocation,
                                             const gchar *source_id,
                                             gpointer user_data)
{
    BUG("%s(): not implemented yet", __func__);
    return FALSE;
}
