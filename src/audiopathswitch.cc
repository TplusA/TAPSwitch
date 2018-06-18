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

#include <glib.h>

#include "audiopathswitch.hh"
#include "audiopath.hh"
#include "dbus_iface_deep.h"
#include "messages.h"

static const char debug_prefix[] = "AUDIO SOURCE SWITCH: ";

static void deactivate_player(const AudioPath::Paths &paths,
                              const GVariantWrapper &request_data,
                              std::string &player_id)
{
    if(player_id.empty())
        return;

    const AudioPath::Player *old_player = paths.lookup_player(player_id);

    msg_vinfo(MESSAGE_LEVEL_DEBUG,
              "%sDeactivate player %s (%s)", debug_prefix,
              old_player->id_.c_str(), old_player->name_.c_str());

    GError *error = nullptr;
    tdbus_aupath_player_call_deactivate_sync(old_player->get_dbus_proxy().get_as_nonconst(),
                                             GVariantWrapper::get(request_data),
                                             nullptr, &error);

    if(!dbus_handle_error(&error, "Deactivate player"))
        msg_error(0, LOG_ERR, "%sDeactivating player %s failed",
                  debug_prefix,  old_player->id_.c_str());

    player_id.clear();
}

static bool activate_player(const AudioPath::Player &player,
                            const GVariantWrapper &request_data)
{
    msg_vinfo(MESSAGE_LEVEL_DEBUG, "%sActivate player %s (%s)",
              debug_prefix, player.id_.c_str(), player.name_.c_str());

    GError *error = nullptr;
    tdbus_aupath_player_call_activate_sync(player.get_dbus_proxy().get_as_nonconst(),
                                           GVariantWrapper::get(request_data),
                                           nullptr, &error);
    if(!dbus_handle_error(&error, "Activate player"))
    {
        msg_error(0, LOG_ERR, "%sActivating player %s failed",
                  debug_prefix, player.id_.c_str());
        return false;
    }

    return true;
}

static void deselect_source(const AudioPath::Paths &paths,
                            const GVariantWrapper &request_data,
                            std::string &source_id,
                            AudioPath::Switch::PendingActivation &pending)
{
    if(source_id.empty() && !pending.have_pending_activation())
        return;

    const std::string &deselected_id(!source_id.empty() ? source_id : pending.get_audio_source_id());
    const AudioPath::Source *old_source = paths.lookup_source(deselected_id);

    msg_vinfo(MESSAGE_LEVEL_DEBUG,
              "%sDeselect audio source %s (%s)", debug_prefix,
              old_source->id_.c_str(), old_source->name_.c_str());

    GError *error = nullptr;
    tdbus_aupath_source_call_deselected_sync(old_source->get_dbus_proxy().get_as_nonconst(),
                                             deselected_id.c_str(),
                                             GVariantWrapper::get(request_data),
                                             nullptr, &error);

    if(!dbus_handle_error(&error, "Deselect source"))
        msg_error(0, LOG_ERR, "%sDeselecting audio source %s failed",
                  debug_prefix, old_source->id_.c_str());

    source_id.clear();
    pending.clear();
}

static bool select_source(const AudioPath::Source &source, bool is_final_select,
                          GVariantWrapper &&request_data)
{
    msg_vinfo(MESSAGE_LEVEL_DEBUG, "%sSelect audio source %s (%s)%s",
              debug_prefix, source.id_.c_str(), source.name_.c_str(),
              is_final_select ? "" : " (deferred)");

    GError *error = nullptr;

    if(is_final_select)
        tdbus_aupath_source_call_selected_sync(source.get_dbus_proxy().get_as_nonconst(),
                                               source.id_.c_str(),
                                               GVariantWrapper::get(request_data),
                                               nullptr, &error);
    else
        tdbus_aupath_source_call_selected_on_hold_sync(source.get_dbus_proxy().get_as_nonconst(),
                                                       source.id_.c_str(),
                                                       GVariantWrapper::get(request_data),
                                                       nullptr, &error);

    if(!dbus_handle_error(&error, "Select source"))
    {
        msg_error(0, LOG_ERR, "%sSelecting audio source %s%s failed",
                  debug_prefix, source.id_.c_str(),
                  is_final_select ? "" : " (deferred)");
        return false;
    }

    return true;
}

AudioPath::Switch::ActivateResult
AudioPath::Switch::activate_source(const AudioPath::Paths &paths,
                                   const char *source_id,
                                   const std::string *&player_id,
                                   bool select_source_now)
{
    GVariantDict dict;
    g_variant_dict_init(&dict, nullptr);
    auto request_data(GVariantWrapper(g_variant_dict_end(&dict)));

    return activate_source(paths, source_id, player_id, select_source_now,
                           std::move(request_data));
}

AudioPath::Switch::ActivateResult
AudioPath::Switch::activate_source(const AudioPath::Paths &paths,
                                   const char *source_id,
                                   const std::string *&player_id,
                                   bool select_source_now,
                                   GVariantWrapper &&request_data)
{
    player_id = nullptr;

    if(source_id[0] == '\0')
    {
        msg_error(EINVAL, LOG_ERR, "%sEmpty audio source ID", debug_prefix);
        pending_.clear();
        return ActivateResult::ERROR_SOURCE_UNKNOWN;
    }

    if(source_id == current_source_id_)
    {
        msg_vinfo(MESSAGE_LEVEL_DEBUG,
                  "%sAudio source not changed", debug_prefix);
        player_id = &current_player_id_;
        log_assert(!pending_.have_pending_activation());
        return ActivateResult::OK_UNCHANGED;
    }

    if(pending_.have_pending_activation() &&
       source_id == pending_.get_audio_source_id())
    {
        msg_vinfo(MESSAGE_LEVEL_DEBUG,
                  "%sAudio source activation for %s already pending",
                  debug_prefix, source_id);
        player_id = &current_player_id_;
        return ActivateResult::OK_PLAYER_SWITCHED_SOURCE_DEFERRED;
    }

    const auto path(paths.lookup_path(source_id));

    if(path.second == nullptr)
    {
        if(path.first == nullptr)
        {
            msg_error(0, LOG_NOTICE,
                      "%sUnknown audio source %s", debug_prefix, source_id);
            pending_.clear();
            return ActivateResult::ERROR_SOURCE_UNKNOWN;
        }
        else
        {
            msg_vinfo(MESSAGE_LEVEL_DEBUG,
                      "%sUnknown player %s for audio source %s (%s)",
                      debug_prefix,
                      path.first->player_id_.c_str(),
                      path.first->id_.c_str(), path.first->name_.c_str());
            pending_.clear();
            return ActivateResult::ERROR_PLAYER_UNKNOWN;
        }
    }
    else
        log_assert(path.first != nullptr);

    player_id = &path.second->id_;

    deselect_source(paths, request_data, current_source_id_, pending_);

    const bool players_changed = (*player_id != current_player_id_);

    if(players_changed)
    {
        deactivate_player(paths, request_data, current_player_id_);

        if(!activate_player(*path.second, request_data))
            return ActivateResult::ERROR_PLAYER_FAILED;

        current_player_id_ = path.second->id_;
    }

    if(!select_source(*path.first, select_source_now, GVariantWrapper(request_data)))
        return ActivateResult::ERROR_SOURCE_FAILED;

    const auto result = players_changed
        ? (select_source_now
           ? ActivateResult::OK_PLAYER_SWITCHED
           : ActivateResult::OK_PLAYER_SWITCHED_SOURCE_DEFERRED)
        : (select_source_now
           ? ActivateResult::OK_PLAYER_SAME
           : ActivateResult::OK_PLAYER_SAME_SOURCE_DEFERRED);

    if(select_source_now)
        current_source_id_ = path.first->id_;
    else
        pending_.set(path.first->id_, std::move(request_data), result);

    return result;
}

AudioPath::Switch::ActivateResult
AudioPath::Switch::complete_pending_source_activation(const AudioPath::Paths &paths,
                                                      std::string *source_id)
{
    if(!pending_.have_pending_activation())
        return ActivateResult::ERROR_SOURCE_UNKNOWN;

    /*
     * Essentially, this is the tail of #AudioPath::Switch::activate_source().
     * We can assume that the head of that function has already been executed,
     * i.e., the previous audio source has already been deselected and the
     * correct player has been activated. All we really need to do in here is
     * to activate the audio source so that the player raring to go can start
     * playing.
     */
    const auto path(paths.lookup_path(pending_.get_audio_source_id()));
    const auto result(pending_.get_phase_one_result());

    if(source_id != nullptr)
        pending_.take_audio_source_id(*source_id);

    auto request_data(pending_.clear());

    if(!select_source(*path.first, true, std::move(request_data)))
        return ActivateResult::ERROR_SOURCE_FAILED;

    current_source_id_ = path.first->id_;

    return result;
}

AudioPath::Switch::ReleaseResult
AudioPath::Switch::release_path(const AudioPath::Paths &paths, bool kill_player,
                                const std::string *&player_id)
{
    GVariantDict dict;
    g_variant_dict_init(&dict, nullptr);
    auto request_data(GVariantWrapper(g_variant_dict_end(&dict)));

    return release_path(paths, kill_player, player_id, std::move(request_data));
}

AudioPath::Switch::ReleaseResult
AudioPath::Switch::release_path(const AudioPath::Paths &paths, bool kill_player,
                                const std::string *&player_id,
                                GVariantWrapper &&request_data)
{
    const bool have_deselected_source = !current_source_id_.empty();
    const bool have_deactivated_player = kill_player && !current_player_id_.empty();

    msg_vinfo(MESSAGE_LEVEL_DEBUG,
              "%sRelease current audio path (%s), %s player",
              debug_prefix,
              have_deselected_source ? "<NONE>" : current_source_id_.c_str(),
              kill_player ? "deactivate" : "keep");

    deselect_source(paths, request_data, current_source_id_, pending_);

    if(have_deactivated_player)
        deactivate_player(paths, request_data, current_player_id_);

    player_id = current_player_id_.empty() ? nullptr : &current_player_id_;

    return (have_deselected_source
            ? (have_deactivated_player
               ? AudioPath::Switch::ReleaseResult::COMPLETE_RELEASE
               : AudioPath::Switch::ReleaseResult::SOURCE_DESELECTED)
            : (have_deactivated_player
               ? AudioPath::Switch::ReleaseResult::PLAYER_DEACTIVATED
               : AudioPath::Switch::ReleaseResult::UNCHANGED));
}
