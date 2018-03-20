/*
 * Copyright (C) 2017, 2018  T+A elektroakustik GmbH & Co. KG
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

static void enter_audiopath_manager_handler(GDBusMethodInvocation *invocation)
{
    static const char iface_name[] = "de.tahifi.AudioPath.Manager";

    msg_vinfo(MESSAGE_LEVEL_TRACE, "%s method invocation from '%s': %s",
              iface_name, g_dbus_method_invocation_get_sender(invocation),
              g_dbus_method_invocation_get_method_name(invocation));
}

gboolean dbusmethod_aupath_register_player(tdbusaupathManager *object,
                                           GDBusMethodInvocation *invocation,
                                           const gchar *player_id,
                                           const gchar *player_name,
                                           const gchar *path,
                                           gpointer user_data)
{
    enter_audiopath_manager_handler(invocation);

    if(player_id[0] == '\0' || player_name[0] == '\0' || path[0] == '\0')
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

    const auto add_result(
        data->audio_paths_.add_player(std::move(
            AudioPath::Player(player_id, player_name,
                              DBus::mk_proxy<AudioPath::Player::PType>(dest, path)))));

    tdbus_aupath_manager_complete_register_player(object, invocation);

    tdbus_aupath_manager_emit_player_registered(object, player_id, player_name);

    switch(add_result)
    {
      case AudioPath::Paths::AddResult::NEW_COMPONENT:
      case AudioPath::Paths::AddResult::UPDATED_COMPONENT:
        break;

      case AudioPath::Paths::AddResult::NEW_PATH:
      case AudioPath::Paths::AddResult::UPDATED_PATH:
        data->audio_paths_.for_each(
            [object, player_id]
            (const AudioPath::Paths::Path &p)
            {
                if(p.second->id_ == player_id)
                    tdbus_aupath_manager_emit_path_available(object, p.first->id_.c_str(), player_id);
            });

        break;
    }

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
    enter_audiopath_manager_handler(invocation);

    if(source_id[0] == '\0' || source_name[0] == '\0' || player_id[0] == '\0' ||
       path[0] == '\0')
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

    const auto add_result(
        data->audio_paths_.add_source(std::move(
            AudioPath::Source(source_id, source_name, player_id,
                              DBus::mk_proxy<AudioPath::Source::PType>(dest, path)))));

    tdbus_aupath_manager_complete_register_source(object, invocation);

    switch(add_result)
    {
      case AudioPath::Paths::AddResult::NEW_COMPONENT:
      case AudioPath::Paths::AddResult::UPDATED_COMPONENT:
        break;

      case AudioPath::Paths::AddResult::NEW_PATH:
      case AudioPath::Paths::AddResult::UPDATED_PATH:
        tdbus_aupath_manager_emit_path_available(object, source_id, player_id);
        break;
    }

    return TRUE;
}

gboolean dbusmethod_aupath_request_source(tdbusaupathManager *object,
                                          GDBusMethodInvocation *invocation,
                                          const gchar *source_id,
                                          gpointer user_data)
{
    enter_audiopath_manager_handler(invocation);

    auto *data = static_cast<DBus::HandlerData *>(user_data);
    const bool select_source_now(data->appliance_state_.is_audio_path_ready() == true);
    const std::string *player_id;
    bool success = false;
    bool suppress_signal = false;

    msg_vinfo(MESSAGE_LEVEL_DIAG, "Requested audio source \"%s\"", source_id);

    switch(data->audio_path_switch_.activate_source(data->audio_paths_,
                                                    source_id, player_id,
                                                    select_source_now))
    {
      case AudioPath::Switch::ActivateResult::ERROR_SOURCE_UNKNOWN:
        g_dbus_method_invocation_return_error_literal(invocation,
                                                      G_DBUS_ERROR, G_DBUS_ERROR_INVALID_ARGS,
                                                      "Source unknown");
        suppress_signal = true;
        break;

      case AudioPath::Switch::ActivateResult::ERROR_SOURCE_FAILED:
        g_dbus_method_invocation_return_error_literal(invocation,
                                                      G_DBUS_ERROR, G_DBUS_ERROR_INVALID_ARGS,
                                                      "Source process failed");
        break;

      case AudioPath::Switch::ActivateResult::ERROR_PLAYER_UNKNOWN:
        g_dbus_method_invocation_return_error_literal(invocation,
                                                      G_DBUS_ERROR, G_DBUS_ERROR_INVALID_ARGS,
                                                      "No player associated");
        suppress_signal = true;
        break;

      case AudioPath::Switch::ActivateResult::ERROR_PLAYER_FAILED:
        g_dbus_method_invocation_return_error_literal(invocation,
                                                      G_DBUS_ERROR, G_DBUS_ERROR_INVALID_ARGS,
                                                      "Player process failed");
        break;

      case AudioPath::Switch::ActivateResult::OK_UNCHANGED:
        suppress_signal = true;

        /* fall-through */

      case AudioPath::Switch::ActivateResult::OK_PLAYER_SAME:
        if(select_source_now)
            tdbus_aupath_manager_complete_request_source(object, invocation,
                                                         player_id->c_str(), false);

        success = true;
        break;

      case AudioPath::Switch::ActivateResult::OK_PLAYER_SWITCHED:
        if(select_source_now)
            tdbus_aupath_manager_complete_request_source(object, invocation,
                                                         player_id->c_str(), true);

        success = true;
        break;
    }

    if(success)
    {
        if(select_source_now)
            msg_vinfo(MESSAGE_LEVEL_DIAG,
                      "Activated audio source %s, %semitting signal",
                      source_id, suppress_signal ? "not " : "");
        else
        {
            msg_vinfo(MESSAGE_LEVEL_DIAG,
                      "Activation of audio source %s deferred until appliance is ready",
                      source_id);
            suppress_signal = true;

            g_object_ref(G_OBJECT(object));
            g_object_ref(G_OBJECT(invocation));
            data->pending_audio_source_activation_.set(object, invocation);
        }
    }
    else
        msg_error(0, LOG_ERR,
                  "Failed activating audio source %s, %semitting signal",
                  source_id, suppress_signal ? "not " : "");

    if(suppress_signal)
        return TRUE;

    if(success)
        tdbus_aupath_manager_emit_path_activated(object, source_id,
                                                 player_id->c_str());
    else
        tdbus_aupath_manager_emit_path_activated(object, "",
                                                 (player_id != nullptr)
                                                 ? player_id->c_str()
                                                 : "");

    return TRUE;
}

gboolean dbusmethod_aupath_release_path(tdbusaupathManager *object,
                                        GDBusMethodInvocation *invocation,
                                        gboolean deactivate_player,
                                        gpointer user_data)
{
    enter_audiopath_manager_handler(invocation);

    auto *data = static_cast<DBus::HandlerData *>(user_data);
    const std::string *player_id;
    bool suppress_signal = false;

    switch(data->audio_path_switch_.release_path(data->audio_paths_,
                                                 deactivate_player, player_id))
    {
      case AudioPath::Switch::ReleaseResult::SOURCE_DESELECTED:
      case AudioPath::Switch::ReleaseResult::PLAYER_DEACTIVATED:
      case AudioPath::Switch::ReleaseResult::COMPLETE_RELEASE:
        break;

      case AudioPath::Switch::ReleaseResult::UNCHANGED:
        suppress_signal = true;
        break;
    }

    tdbus_aupath_manager_complete_release_path(object, invocation);

    if(!suppress_signal)
        tdbus_aupath_manager_emit_path_activated(object, "",
                                                 (player_id != nullptr)
                                                 ? player_id->c_str()
                                                 : "");

    return TRUE;
}

gboolean dbusmethod_aupath_get_active_player(tdbusaupathManager *object,
                                             GDBusMethodInvocation *invocation,
                                             const gchar *source_id,
                                             gpointer user_data)
{
    enter_audiopath_manager_handler(invocation);

    auto *data = static_cast<DBus::HandlerData *>(user_data);

    tdbus_aupath_manager_complete_get_active_player(object, invocation,
                                                    data->audio_path_switch_.get_player_id().c_str());

    return TRUE;
}

gboolean dbusmethod_aupath_get_paths(tdbusaupathManager *object,
                                     GDBusMethodInvocation *invocation,
                                     gpointer user_data)
{
    enter_audiopath_manager_handler(invocation);

    GVariantBuilder usable;
    g_variant_builder_init(&usable, G_VARIANT_TYPE("a(ss)"));

    GVariantBuilder incomplete;
    g_variant_builder_init(&incomplete, G_VARIANT_TYPE("a(ss)"));

    auto *data = static_cast<DBus::HandlerData *>(user_data);

    data->audio_paths_.for_each(
        [&usable, &incomplete]
        (const AudioPath::Paths::Path &p)
        {
            GVariantBuilder *const vb =
                (p.first != nullptr && p.second != nullptr) ? &usable : &incomplete;

            g_variant_builder_add(vb, "(ss)",
                                  p.first != nullptr ? p.first->id_.c_str() : "",
                                  p.second != nullptr ? p.second->id_.c_str() : "");
        },
        AudioPath::Paths::ForEach::ANY);

    tdbus_aupath_manager_complete_get_paths(object, invocation,
                                            g_variant_builder_end(&usable),
                                            g_variant_builder_end(&incomplete));

    return TRUE;
}

gboolean dbusmethod_aupath_get_player_info(tdbusaupathManager *object,
                                           GDBusMethodInvocation *invocation,
                                           const gchar *player_id,
                                           gpointer user_data)
{
    enter_audiopath_manager_handler(invocation);

    auto *data = static_cast<DBus::HandlerData *>(user_data);
    const auto *const p(data->audio_paths_.lookup_player(player_id));

    if(p == nullptr)
        g_dbus_method_invocation_return_error(invocation, G_DBUS_ERROR,
                                              G_DBUS_ERROR_FAILED,
                                              "Audio player \"%s\" no registered",
                                              player_id);
    else
    {
        auto *proxy = G_DBUS_PROXY(p->get_dbus_proxy().get());
        tdbus_aupath_manager_complete_get_player_info(object, invocation,
                                                      p->name_.c_str(),
                                                      g_dbus_proxy_get_name(proxy),
                                                      g_dbus_proxy_get_object_path(proxy));
    }

    return TRUE;
}

gboolean dbusmethod_aupath_get_source_info(tdbusaupathManager *object,
                                           GDBusMethodInvocation *invocation,
                                           const gchar *source_id,
                                           gpointer user_data)
{
    enter_audiopath_manager_handler(invocation);

    auto *data = static_cast<DBus::HandlerData *>(user_data);
    const auto *const s(data->audio_paths_.lookup_source(source_id));

    if(s == nullptr)
        g_dbus_method_invocation_return_error(invocation, G_DBUS_ERROR,
                                              G_DBUS_ERROR_FAILED,
                                              "Audio source \"%s\" no registered",
                                              source_id);
    else
    {
        auto *proxy = G_DBUS_PROXY(s->get_dbus_proxy().get());
        tdbus_aupath_manager_complete_get_source_info(object, invocation,
                                                      s->name_.c_str(),
                                                      s->player_id_.c_str(),
                                                      g_dbus_proxy_get_name(proxy),
                                                      g_dbus_proxy_get_object_path(proxy));
    }

    return TRUE;
}

static void enter_audiopath_appliance_handler(GDBusMethodInvocation *invocation)
{
    static const char iface_name[] = "de.tahifi.AudioPath.Appliance";

    msg_vinfo(MESSAGE_LEVEL_TRACE, "%s method invocation from '%s': %s",
              iface_name, g_dbus_method_invocation_get_sender(invocation),
              g_dbus_method_invocation_get_method_name(invocation));
}

static tdbusaupathManager *
complete_pending(DBus::HandlerData::Pending &pending,
                 const std::string &player_id, bool have_switched,
                 const char *error_message = nullptr)
{
    auto *object = static_cast<tdbusaupathManager *>(pending.object_);
    auto *invocation = static_cast<GDBusMethodInvocation *>(pending.invocation_);

    log_assert(object != nullptr && invocation != nullptr);

    if(error_message == nullptr)
        tdbus_aupath_manager_complete_request_source(object, invocation,
                                                     player_id.c_str(),
                                                     have_switched);
    else
        g_dbus_method_invocation_return_error_literal(invocation,
                                                      G_DBUS_ERROR, G_DBUS_ERROR_INVALID_ARGS,
                                                      error_message);

    g_object_unref(G_OBJECT(invocation));
    pending.clear();

    return object;
}

static void process_pending_audio_source_activation(tdbusaupathAppliance *object,
                                                    GDBusMethodInvocation *invocation,
                                                    DBus::HandlerData &data)
{
    bool success = false;
    bool suppress_signal = false;

    tdbusaupathManager *pending_object = nullptr;
    std::string source_id;
    const auto result =
        data.audio_path_switch_.complete_pending_source_activation(data.audio_paths_,
                                                                   &source_id);

    switch(result)
    {
      case AudioPath::Switch::ActivateResult::ERROR_SOURCE_UNKNOWN:
        /* had no pending activation */
        success = true;
        suppress_signal = true;
        tdbus_aupath_appliance_complete_set_ready_state(object, invocation);
        break;

      case AudioPath::Switch::ActivateResult::ERROR_SOURCE_FAILED:
        pending_object =
            complete_pending(data.pending_audio_source_activation_,
                             data.audio_path_switch_.get_player_id(),
                             false, "Source process failed");
        break;

      case AudioPath::Switch::ActivateResult::ERROR_PLAYER_UNKNOWN:
      case AudioPath::Switch::ActivateResult::ERROR_PLAYER_FAILED:
        pending_object =
            complete_pending(data.pending_audio_source_activation_,
                             data.audio_path_switch_.get_player_id(),
                             false,
                             "Unexpected result while completing pending audio source activation");
        g_dbus_method_invocation_return_error(
                invocation, G_DBUS_ERROR, G_DBUS_ERROR_INVALID_ARGS,
                "Unexpected result %d while completing pending audio source activation",
                static_cast<int>(result));
        break;

      case AudioPath::Switch::ActivateResult::OK_UNCHANGED:
        suppress_signal = true;

        /* fall-through */

      case AudioPath::Switch::ActivateResult::OK_PLAYER_SAME:
        pending_object =
            complete_pending(data.pending_audio_source_activation_,
                             data.audio_path_switch_.get_player_id(),
                             false);
        tdbus_aupath_appliance_complete_set_ready_state(object, invocation);
        success = true;
        break;

      case AudioPath::Switch::ActivateResult::OK_PLAYER_SWITCHED:
        pending_object =
            complete_pending(data.pending_audio_source_activation_,
                             data.audio_path_switch_.get_player_id(),
                             true);
        tdbus_aupath_appliance_complete_set_ready_state(object, invocation);
        success = true;
        break;
    }

    if(!source_id.empty())
    {
        if(success)
            msg_vinfo(MESSAGE_LEVEL_DIAG,
                      "Deferred activation of audio source %s, %semitting signal",
                      source_id.c_str(), suppress_signal ? "not " : "");
        else
            msg_error(0, LOG_ERR,
                      "Deferred activation of audio source %s failed, %semitting signal",
                      source_id.c_str(), suppress_signal ? "not " : "");
    }

    if(pending_object != nullptr)
    {
        if(!suppress_signal)
        {
            if(success)
                tdbus_aupath_manager_emit_path_activated(pending_object, source_id.c_str(),
                                                         data.audio_path_switch_.get_player_id().c_str());
            else
                tdbus_aupath_manager_emit_path_activated(pending_object, "",
                                                         data.audio_path_switch_.get_player_id().c_str());
        }

        g_object_unref(G_OBJECT(pending_object));
    }
}

gboolean dbusmethod_appliance_set_ready_state(tdbusaupathAppliance *object,
                                              GDBusMethodInvocation *invocation,
                                              const guchar state,
                                              gpointer user_data)
{
    enter_audiopath_appliance_handler(invocation);

    auto *data = static_cast<DBus::HandlerData *>(user_data);

    switch(state)
    {
      case 0:
        msg_info("Appliance is not ready to play");
        data->appliance_state_.set_audio_path_blocked();
        break;

      case 1:
        msg_info("Appliance is ready to play");
        data->appliance_state_.set_audio_path_ready();
        break;

      case 2:
      default:
        msg_info("Appliance ready-to-play state unknown");
        data->appliance_state_.set_audio_path_unknown();
        break;
    }

    if(data->appliance_state_.is_audio_path_ready() == true)
        process_pending_audio_source_activation(object, invocation, *data);
    else
        tdbus_aupath_appliance_complete_set_ready_state(object, invocation);

    return TRUE;
}

gboolean dbusmethod_appliance_get_state(tdbusaupathAppliance *object,
                                        GDBusMethodInvocation *invocation,
                                        gpointer user_data)
{
    enter_audiopath_appliance_handler(invocation);

    auto *data = static_cast<DBus::HandlerData *>(user_data);
    const auto &state(data->appliance_state_.is_audio_path_ready());

    guchar result;

    if(state == false)
        result = 0;
    else if(state == true)
        result = 1;
    else
        result = 2;

    tdbus_aupath_appliance_complete_get_state(object, invocation, result);

    return TRUE;
}
