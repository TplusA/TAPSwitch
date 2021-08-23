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

#if HAVE_CONFIG_H
#include <config.h>
#endif /* HAVE_CONFIG_H */

#include <unordered_map>

#include <glib.h>

#include "dbus_handlers.hh"
#include "dbus_handlers.h"
#include "dbus_iface_deep.h"
#include "gerrorwrapper.hh"
#include "messages.h"

namespace DBus
{

using RegisterPlayerCallback =
    std::function<void(std::unique_ptr<AudioPath::Player::PType>)>;

template <>
void mk_proxy_done<RegisterPlayerCallback>(GObject *source_object, GAsyncResult *res,
                                           gpointer user_data)
{
    auto proxy_done_cb(reinterpret_cast<RegisterPlayerCallback *>(user_data));
    GErrorWrapper error;
    auto *proxy = tdbus_aupath_player_proxy_new_finish(res, error.await());

    try
    {
        if(error.log_failure("Create AudioPath.Player proxy"))
            (*proxy_done_cb)(nullptr);
        else
            (*proxy_done_cb)(std::make_unique<AudioPath::Player::PType>(proxy));
    }
    catch(...)
    {
        BUG("Exception from AudioPath.Player proxy-done callback (ignored)");
    }

    delete proxy_done_cb;
}

template <>
void mk_proxy_async(const char *dest, const char *obj_path,
                    RegisterPlayerCallback *proxy_done_cb)
{
    GDBusConnection *connection =
        g_dbus_interface_skeleton_get_connection(G_DBUS_INTERFACE_SKELETON(dbus_get_audiopath_manager_iface()));
    tdbus_aupath_player_proxy_new(
        connection, G_DBUS_PROXY_FLAGS_NONE, dest, obj_path, nullptr,
        mk_proxy_done<RegisterPlayerCallback>,
        reinterpret_cast<void *>(proxy_done_cb));
}

template <>
Proxy<tdbusaupathPlayer>::~Proxy()
{
    if(proxy_ == nullptr)
        return;

    g_object_unref(proxy_);
    proxy_ = nullptr;
}

using RegisterSourceCallback =
    std::function<void(std::unique_ptr<AudioPath::Source::PType>)>;

template <>
void mk_proxy_done<RegisterSourceCallback>(GObject *source_object, GAsyncResult *res,
                                           gpointer user_data)
{
    auto proxy_done_cb(reinterpret_cast<RegisterSourceCallback *>(user_data));
    GErrorWrapper error;
    auto *proxy = tdbus_aupath_source_proxy_new_finish(res, error.await());

    try
    {
        if(error.log_failure("Create AudioPath.Source proxy"))
            (*proxy_done_cb)(nullptr);
        else
            (*proxy_done_cb)(std::make_unique<AudioPath::Source::PType>(proxy));
    }
    catch(...)
    {
        BUG("Exception from AudioPath.Source proxy-done callback (ignored)");
    }

    delete proxy_done_cb;
}

template <>
void mk_proxy_async(const char *dest, const char *obj_path,
                    RegisterSourceCallback *proxy_done_cb)
{
    GDBusConnection *connection =
        g_dbus_interface_skeleton_get_connection(G_DBUS_INTERFACE_SKELETON(dbus_get_audiopath_manager_iface()));
    tdbus_aupath_source_proxy_new(
        connection, G_DBUS_PROXY_FLAGS_NONE, dest, obj_path, nullptr,
        mk_proxy_done<RegisterSourceCallback>,
        reinterpret_cast<void *>(proxy_done_cb));
}

template <>
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

static void register_player_bottom_half(
        tdbusaupathManager *object, GDBusMethodInvocation *invocation,
        std::unique_ptr<AudioPath::Player::PType> proxy,
        std::string &&player_id, std::string &&player_name,
        DBus::HandlerData &handler_data)
{
    const auto add_result(
        handler_data.audio_paths_.add_player(
            AudioPath::Player(player_id.c_str(), player_name.c_str(),
                              std::move(proxy))));

    tdbus_aupath_manager_complete_register_player(object, invocation);

    tdbus_aupath_manager_emit_player_registered(object, player_id.c_str(),
                                                player_name.c_str());

    switch(add_result)
    {
      case AudioPath::Paths::AddResult::NEW_COMPONENT:
      case AudioPath::Paths::AddResult::UPDATED_COMPONENT:
        break;

      case AudioPath::Paths::AddResult::NEW_PATH:
      case AudioPath::Paths::AddResult::UPDATED_PATH:
        handler_data.audio_paths_.for_each(
            [object, player_id]
            (const AudioPath::Paths::Path &p)
            {
                if(p.second->id_ == player_id)
                    tdbus_aupath_manager_emit_path_available(
                        object, p.first->id_.c_str(), player_id.c_str());
            });

        break;
    }
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
        g_dbus_method_invocation_return_error_literal(
            invocation, G_DBUS_ERROR, G_DBUS_ERROR_INVALID_ARGS,
            "Empty argument");
        return TRUE;
    }

    const char *dest =
        g_dbus_message_get_sender(g_dbus_method_invocation_get_message(invocation));

    msg_vinfo(MESSAGE_LEVEL_DIAG,
              "Register player %s (\"%s\") running on %s, object %s",
              player_id, player_name, dest, path);

    auto *done_fn =
        new DBus::RegisterPlayerCallback(
            [object, invocation,
             pid = std::move(std::string(player_id)),
             pname = std::move(std::string(player_name)),
             hd = static_cast<DBus::HandlerData *>(user_data)]
            (std::unique_ptr<AudioPath::Player::PType> proxy) mutable
            {
                register_player_bottom_half(object, invocation, std::move(proxy),
                                            std::move(pid), std::move(pname), *hd);
            });

    DBus::mk_proxy_async<AudioPath::Player::PType>(dest, path, done_fn);

    return TRUE;
}

static void register_source_bottom_half(
        tdbusaupathManager *object, GDBusMethodInvocation *invocation,
        std::unique_ptr<AudioPath::Source::PType> proxy,
        std::string &&source_id, std::string &&source_name,
        std::string &&player_id, DBus::HandlerData &handler_data)
{
    const auto add_result(
        handler_data.audio_paths_.add_source(
            AudioPath::Source(source_id.c_str(), source_name.c_str(),
                              player_id.c_str(), std::move(proxy))));

    tdbus_aupath_manager_complete_register_source(object, invocation);

    switch(add_result)
    {
      case AudioPath::Paths::AddResult::NEW_COMPONENT:
      case AudioPath::Paths::AddResult::UPDATED_COMPONENT:
        break;

      case AudioPath::Paths::AddResult::NEW_PATH:
      case AudioPath::Paths::AddResult::UPDATED_PATH:
        tdbus_aupath_manager_emit_path_available(object, source_id.c_str(),
                                                 player_id.c_str());
        break;
    }
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
        g_dbus_method_invocation_return_error_literal(
            invocation, G_DBUS_ERROR, G_DBUS_ERROR_INVALID_ARGS,
            "Empty argument");
        return TRUE;
    }

    const char *dest =
        g_dbus_message_get_sender(g_dbus_method_invocation_get_message(invocation));

    msg_vinfo(MESSAGE_LEVEL_DIAG,
              "Register source %s (\"%s\") for player %s running on %s, object %s",
              source_id, source_name, player_id, dest, path);

    auto *done_fn =
        new DBus::RegisterSourceCallback(
            [object, invocation,
             srcid = std::move(std::string(source_id)),
             srcname = std::move(std::string(source_name)),
             pid = std::move(std::string(player_id)),
             hd = static_cast<DBus::HandlerData *>(user_data)]
            (std::unique_ptr<AudioPath::Source::PType> proxy) mutable
            {
                register_source_bottom_half(object, invocation, std::move(proxy),
                                            std::move(srcid), std::move(srcname),
                                            std::move(pid), *hd);
            });

    DBus::mk_proxy_async<AudioPath::Source::PType>(dest, path, done_fn);

    return TRUE;
}

static void emit_path_switch_signal(tdbusaupathManager *object,
                                    const gchar *source_id,
                                    const std::string *const player_id,
                                    GVariantWrapper &&request_data,
                                    bool success, bool is_deferred)
{
    if(success)
    {
        log_assert(player_id != nullptr);

        if(is_deferred)
            tdbus_aupath_manager_emit_path_deferred(object, source_id,
                                                    player_id->c_str());
        else
            tdbus_aupath_manager_emit_path_activated(object, source_id,
                                                     player_id->c_str(),
                                                     GVariantWrapper::get(request_data));
    }
    else
    {
        if(is_deferred)
            tdbus_aupath_manager_emit_path_deferred(object, "",
                                                    (player_id != nullptr)
                                                    ? player_id->c_str()
                                                    : "");
        else
            tdbus_aupath_manager_emit_path_activated(object, "",
                                                     (player_id != nullptr)
                                                     ? player_id->c_str()
                                                     : "",
                                                     GVariantWrapper::get(request_data));
    }
}

static void complete_pending_call(
        const DBus::HandlerData::Pending &pending, const std::string &player_id,
        bool have_switched, GDBusError error_code, const char *error_message,
        std::unordered_map<tdbusaupathManager *, GVariantWrapper> *manager_objects,
        bool suppress_activated_signal)
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
                                                      G_DBUS_ERROR, error_code,
                                                      error_message);

    if(!suppress_activated_signal &&
       manager_objects != nullptr &&
       manager_objects->find(object) == manager_objects->end())
    {
        g_object_ref(G_OBJECT(object));
        (*manager_objects)[object] = std::move(pending.request_data_);
    }

    g_object_unref(G_OBJECT(object));
    g_object_unref(G_OBJECT(invocation));
}

static void fail_all_pending_calls(
        std::vector<DBus::HandlerData::Pending> &pending,
        const AudioPath::Switch &audio_path_switch,
        const char *error_message)
{
    for(const auto &p : pending)
        complete_pending_call(p, audio_path_switch.get_player_id(),
                              false, G_DBUS_ERROR_FAILED, error_message,
                              nullptr, true);

    pending.clear();
}

/*!
 * System policy: We assume completion of audio path switching is allowed in
 *                case we don't know for sure.
 */
static bool is_audio_path_enable_allowed(const Maybe<bool> &power_state,
                                         const Maybe<bool> &audio_state)
{
    return !(power_state == false || audio_state == false);
}

gboolean dbusmethod_aupath_request_source(tdbusaupathManager *object,
                                          GDBusMethodInvocation *invocation,
                                          const gchar *source_id,
                                          GVariant *arg_request_data,
                                          gpointer user_data)
{
    enter_audiopath_manager_handler(invocation);

    auto *data = static_cast<DBus::HandlerData *>(user_data);
    bool select_source_now =
        is_audio_path_enable_allowed(data->appliance_state_.is_up_and_running(),
                                     data->appliance_state_.is_audio_path_ready());
    GVariantWrapper request_data(arg_request_data);
    const std::string *player_id;
    AudioPath::Switch::DeselectedAudioSourceResult deselected_result;
    bool success = false;
    bool suppress_activated_signal = false;
    bool is_activation_deferred = false;
    bool emit_reactivation = false;

    msg_vinfo(MESSAGE_LEVEL_DIAG, "Requested audio source \"%s\"", source_id);

    switch(data->audio_path_switch_.activate_source(data->audio_paths_,
                                                    source_id, player_id,
                                                    deselected_result,
                                                    select_source_now,
                                                    GVariantWrapper(request_data)))
    {
      case AudioPath::Switch::ActivateResult::ERROR_SOURCE_UNKNOWN:
        g_dbus_method_invocation_return_error_literal(invocation,
                                                      G_DBUS_ERROR, G_DBUS_ERROR_INVALID_ARGS,
                                                      "Source unknown");
        suppress_activated_signal = true;
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
        suppress_activated_signal = true;
        break;

      case AudioPath::Switch::ActivateResult::ERROR_PLAYER_FAILED:
        g_dbus_method_invocation_return_error_literal(invocation,
                                                      G_DBUS_ERROR, G_DBUS_ERROR_INVALID_ARGS,
                                                      "Player process failed");
        break;

      case AudioPath::Switch::ActivateResult::OK_UNCHANGED:
        tdbus_aupath_manager_complete_request_source(object, invocation,
                                                     player_id->c_str(), false);
        success = true;
        select_source_now = true;
        suppress_activated_signal = true;
        emit_reactivation = true;
        break;

      case AudioPath::Switch::ActivateResult::OK_PLAYER_SAME_SOURCE_DEFERRED:
      case AudioPath::Switch::ActivateResult::OK_PLAYER_SWITCHED_SOURCE_DEFERRED:
        success = true;
        is_activation_deferred = true;
        break;

      case AudioPath::Switch::ActivateResult::OK_PLAYER_SAME:
        tdbus_aupath_manager_complete_request_source(object, invocation,
                                                     player_id->c_str(), false);
        success = true;
        break;

      case AudioPath::Switch::ActivateResult::OK_PLAYER_SWITCHED:
        tdbus_aupath_manager_complete_request_source(object, invocation,
                                                     player_id->c_str(), true);
        success = true;
        break;
    }

    switch(deselected_result)
    {
      case AudioPath::Switch::DeselectedAudioSourceResult::NONE:
      case AudioPath::Switch::DeselectedAudioSourceResult::DESELECTED_ACTIVE:
        break;

      case AudioPath::Switch::DeselectedAudioSourceResult::DESELECTED_PENDING:
        fail_all_pending_calls(
            data->pending_audio_source_activations_, data->audio_path_switch_,
            "Canceled pending audio source activation because "
            "a different source has been requested");
        break;
    }

    if(success)
    {
        if(select_source_now)
        {
            if(emit_reactivation)
            {
                msg_vinfo(MESSAGE_LEVEL_DIAG,
                          "Reactivated audio source %s", source_id);
                tdbus_aupath_manager_emit_path_reactivated(
                    object, source_id, player_id->c_str(),
                    GVariantWrapper::get(request_data));
            }
            else
                msg_vinfo(MESSAGE_LEVEL_DIAG,
                          "Activated audio source %s, %semitting signal",
                          source_id, suppress_activated_signal ? "not " : "");

        }
        else
        {
            msg_vinfo(MESSAGE_LEVEL_DIAG,
                      "Activation of audio source %s deferred until appliance is ready",
                      source_id);

            g_object_ref(G_OBJECT(object));
            g_object_ref(G_OBJECT(invocation));
            data->pending_audio_source_activations_.emplace_back(
                            object, invocation, GVariantWrapper(request_data));
        }
    }
    else
        msg_error(0, LOG_ERR,
                  "Failed activating audio source %s, %semitting signal",
                  source_id, suppress_activated_signal ? "not " : "");

    if(!suppress_activated_signal)
        emit_path_switch_signal(object, source_id, player_id,
                                std::move(request_data),
                                success, is_activation_deferred);

    return TRUE;
}

gboolean dbusmethod_aupath_release_path(tdbusaupathManager *object,
                                        GDBusMethodInvocation *invocation,
                                        gboolean deactivate_player,
                                        GVariant *arg_request_data,
                                        gpointer user_data)
{
    enter_audiopath_manager_handler(invocation);

    auto *data = static_cast<DBus::HandlerData *>(user_data);
    GVariantWrapper request_data(arg_request_data);
    const std::string *player_id;
    AudioPath::Switch::DeselectedAudioSourceResult deselected_result;
    bool suppress_activated_signal = false;

    switch(data->audio_path_switch_.release_path(data->audio_paths_,
                                                 deactivate_player, player_id,
                                                 deselected_result,
                                                 GVariantWrapper(request_data)))
    {
      case AudioPath::Switch::ReleaseResult::SOURCE_DESELECTED:
      case AudioPath::Switch::ReleaseResult::PLAYER_DEACTIVATED:
      case AudioPath::Switch::ReleaseResult::COMPLETE_RELEASE:
        break;

      case AudioPath::Switch::ReleaseResult::UNCHANGED:
        suppress_activated_signal = true;
        break;
    }

    switch(deselected_result)
    {
      case AudioPath::Switch::DeselectedAudioSourceResult::NONE:
      case AudioPath::Switch::DeselectedAudioSourceResult::DESELECTED_ACTIVE:
        break;

      case AudioPath::Switch::DeselectedAudioSourceResult::DESELECTED_PENDING:
        fail_all_pending_calls(
            data->pending_audio_source_activations_, data->audio_path_switch_,
            "Canceled pending audio source activation because "
            "the audio path been released");
        break;
    }

    tdbus_aupath_manager_complete_release_path(object, invocation);

    if(!suppress_activated_signal)
        tdbus_aupath_manager_emit_path_activated(object, "",
                                                 (player_id != nullptr)
                                                 ? player_id->c_str()
                                                 : "",
                                                 GVariantWrapper::get(request_data));

    return TRUE;
}

gboolean dbusmethod_aupath_get_active_player(tdbusaupathManager *object,
                                             GDBusMethodInvocation *invocation,
                                             const gchar *source_id,
                                             gpointer user_data)
{
    enter_audiopath_manager_handler(invocation);

    auto *data = static_cast<DBus::HandlerData *>(user_data);

    if(source_id != nullptr && source_id[0] != '\0' &&
       data->audio_path_switch_.get_source_id() != source_id)
        tdbus_aupath_manager_complete_get_active_player(object, invocation, "");
    else
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

gboolean dbusmethod_aupath_get_current_path(tdbusaupathManager *object,
                                            GDBusMethodInvocation *invocation,
                                            gpointer user_data)
{
    enter_audiopath_manager_handler(invocation);

    const auto *const data = static_cast<DBus::HandlerData *>(user_data);
    tdbus_aupath_manager_complete_get_current_path(
            object, invocation,
            data->audio_path_switch_.get_source_id().c_str(),
            data->audio_path_switch_.get_player_id().c_str());

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
                                              "Audio player \"%s\" not registered",
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
                                              "Audio source \"%s\" not registered",
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

static void log_deferred_activation(const std::string &source_id,
                                    bool success, bool suppress_activated_signal)
{
    if(!source_id.empty())
    {
        if(success)
            msg_vinfo(MESSAGE_LEVEL_DIAG,
                      "Deferred activation of audio source %s, %semitting signal",
                      source_id.c_str(), suppress_activated_signal ? "not " : "");
        else
            msg_error(0, LOG_ERR,
                      "Deferred activation of audio source %s failed, %semitting signal",
                      source_id.c_str(), suppress_activated_signal ? "not " : "");
    }
}

static void emit_path_activated(tdbusaupathManager *manager_object,
                                const std::string &source_id,
                                const AudioPath::Switch &audio_path_switch,
                                GVariantWrapper &&request_data,
                                bool success)
{
    if(manager_object == nullptr)
        return;

    if(success)
        tdbus_aupath_manager_emit_path_activated(manager_object, source_id.c_str(),
                                                 audio_path_switch.get_player_id().c_str(),
                                                 GVariantWrapper::get(request_data));
    else
        tdbus_aupath_manager_emit_path_activated(manager_object, "",
                                                 audio_path_switch.get_player_id().c_str(),
                                                 GVariantWrapper::get(request_data));

    g_object_unref(G_OBJECT(manager_object));
}

static void complete_all_pending_calls(
        std::vector<DBus::HandlerData::Pending> &pending,
        const std::string &source_id,
        const AudioPath::Switch &audio_path_switch,
        bool success, bool suppress_activated_signal, bool have_switched,
        GDBusError error_code = G_DBUS_ERROR_FAILED,
        const char *error_message = nullptr,
        std::function<void()> &&after_completion = nullptr)
{
    log_deferred_activation(source_id, success, suppress_activated_signal);

    std::unordered_map<tdbusaupathManager *, GVariantWrapper> manager_objects;

    for(auto &p : pending)
        complete_pending_call(p, audio_path_switch.get_player_id(),
                              have_switched, error_code, error_message,
                              &manager_objects, suppress_activated_signal);

    if(after_completion != nullptr)
        after_completion();

    pending.clear();

    for(auto &m : manager_objects)
        emit_path_activated(m.first, source_id, audio_path_switch,
                            std::move(m.second), success);
}

static void process_pending_audio_source_activation(tdbusaupathAppliance *object,
                                                    GDBusMethodInvocation *invocation,
                                                    DBus::HandlerData &data)
{
    std::string source_id;
    const auto result =
        data.audio_path_switch_.complete_pending_source_activation(data.audio_paths_,
                                                                   &source_id);

    switch(result)
    {
      case AudioPath::Switch::ActivateResult::ERROR_SOURCE_UNKNOWN:
        /* had no pending activation */
        tdbus_aupath_appliance_complete_set_ready_state(object, invocation);
        log_deferred_activation(source_id, true, true);
        break;

      case AudioPath::Switch::ActivateResult::ERROR_SOURCE_FAILED:
        complete_all_pending_calls(
            data.pending_audio_source_activations_, source_id,
            data.audio_path_switch_, false, false, false,
            G_DBUS_ERROR_INVALID_ARGS, "Source process failed");
        break;

      case AudioPath::Switch::ActivateResult::ERROR_PLAYER_UNKNOWN:
      case AudioPath::Switch::ActivateResult::ERROR_PLAYER_FAILED:
        complete_all_pending_calls(
            data.pending_audio_source_activations_, source_id,
            data.audio_path_switch_, false, false, false,
            G_DBUS_ERROR_INVALID_ARGS,
            "Unexpected result while completing pending audio source activation",
            [invocation, result] ()
            {
                g_dbus_method_invocation_return_error(
                    invocation, G_DBUS_ERROR, G_DBUS_ERROR_INVALID_ARGS,
                    "Unexpected result %d while completing pending audio source activation",
                    static_cast<int>(result));
            });
        break;

      case AudioPath::Switch::ActivateResult::OK_UNCHANGED:
      case AudioPath::Switch::ActivateResult::OK_PLAYER_SAME:
      case AudioPath::Switch::ActivateResult::OK_PLAYER_SAME_SOURCE_DEFERRED:
        complete_all_pending_calls(
            data.pending_audio_source_activations_, source_id,
            data.audio_path_switch_, true,
            result == AudioPath::Switch::ActivateResult::OK_UNCHANGED,
            false, G_DBUS_ERROR_FAILED, nullptr,
            [object, invocation] ()
            {
                tdbus_aupath_appliance_complete_set_ready_state(object, invocation);
            });
        break;

      case AudioPath::Switch::ActivateResult::OK_PLAYER_SWITCHED:
      case AudioPath::Switch::ActivateResult::OK_PLAYER_SWITCHED_SOURCE_DEFERRED:
        complete_all_pending_calls(
            data.pending_audio_source_activations_, source_id,
            data.audio_path_switch_, true, false, true,
            G_DBUS_ERROR_FAILED, nullptr,
            [object, invocation] ()
            {
                tdbus_aupath_appliance_complete_set_ready_state(object, invocation);
            });
        break;
    }
}

static void cancel_pending_audio_source_activation(GDBusMethodInvocation *invocation,
                                                   DBus::HandlerData &data)
{
    std::string source_id;
    const auto result =
        data.audio_path_switch_.cancel_pending_source_activation(data.audio_paths_,
                                                                 source_id);

    switch(result)
    {
      case AudioPath::Switch::ActivateResult::ERROR_SOURCE_UNKNOWN:
        /* had no pending activation */
        break;

      case AudioPath::Switch::ActivateResult::ERROR_SOURCE_FAILED:
      case AudioPath::Switch::ActivateResult::OK_PLAYER_SWITCHED:
        complete_all_pending_calls(
            data.pending_audio_source_activations_, source_id,
            data.audio_path_switch_, false, true, false,
            G_DBUS_ERROR_ACCESS_DENIED,
            "Canceled audio source activation");
        break;

      case AudioPath::Switch::ActivateResult::ERROR_PLAYER_UNKNOWN:
      case AudioPath::Switch::ActivateResult::ERROR_PLAYER_FAILED:
      case AudioPath::Switch::ActivateResult::OK_UNCHANGED:
      case AudioPath::Switch::ActivateResult::OK_PLAYER_SAME:
      case AudioPath::Switch::ActivateResult::OK_PLAYER_SAME_SOURCE_DEFERRED:
      case AudioPath::Switch::ActivateResult::OK_PLAYER_SWITCHED_SOURCE_DEFERRED:
        BUG("Unexpected cancel result %d", int(result));
        break;
    }
}

gboolean dbusmethod_appliance_set_ready_state(tdbusaupathAppliance *object,
                                              GDBusMethodInvocation *invocation,
                                              const guchar audio_state,
                                              const guchar power_state,
                                              gpointer user_data)
{
    enter_audiopath_appliance_handler(invocation);

    auto *data = static_cast<DBus::HandlerData *>(user_data);
    bool suspended;

    switch(power_state)
    {
      case 1:
        msg_info("Appliance suspended");
        suspended = data->appliance_state_.set_suspend_mode();
        break;

      case 2:
        msg_info("Appliance powered");
        data->appliance_state_.set_up_and_running();
        suspended = false;
        break;

      case 0:
      default:
        msg_info("Appliance power state unknown");
        data->appliance_state_.set_power_state_unknown();
        suspended = false;
        break;
    }

    switch(audio_state)
    {
      case 1:
        msg_info("Appliance is not ready to play");
        data->appliance_state_.set_audio_path_blocked();
        break;

      case 2:
        msg_info("Appliance is ready to play");
        data->appliance_state_.set_audio_path_ready();
        break;

      case 0:
      default:
        msg_info("Appliance ready-to-play state unknown");
        data->appliance_state_.set_audio_path_unknown();
        break;
    }

    if(is_audio_path_enable_allowed(data->appliance_state_.is_up_and_running(),
                                    data->appliance_state_.is_audio_path_ready()))
        process_pending_audio_source_activation(object, invocation, *data);
    else
    {
        if(suspended)
            cancel_pending_audio_source_activation(invocation, *data);

        tdbus_aupath_appliance_complete_set_ready_state(object, invocation);
    }

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
