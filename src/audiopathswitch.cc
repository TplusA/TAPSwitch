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

#include <glib.h>

#include "audiopathswitch.hh"
#include "audiopath.hh"
#include "gerrorwrapper.hh"
#include "de_tahifi_audiopath.h"
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

    GErrorWrapper error;
    tdbus_aupath_player_call_deactivate_sync(old_player->get_dbus_proxy().get_as_nonconst(),
                                             GVariantWrapper::get(request_data),
                                             nullptr, error.await());

    if(error.log_failure("Deactivate player"))
        msg_error(0, LOG_ERR, "%sDeactivating player %s failed",
                  debug_prefix,  old_player->id_.c_str());

    player_id.clear();
}

static bool activate_player(const AudioPath::Player &player,
                            const GVariantWrapper &request_data)
{
    msg_vinfo(MESSAGE_LEVEL_DEBUG, "%sActivate player %s (%s)",
              debug_prefix, player.id_.c_str(), player.name_.c_str());

    GErrorWrapper error;
    tdbus_aupath_player_call_activate_sync(player.get_dbus_proxy().get_as_nonconst(),
                                           GVariantWrapper::get(request_data),
                                           nullptr, error.await());
    if(error.log_failure("Activate player"))
    {
        msg_error(0, LOG_ERR, "%sActivating player %s failed",
                  debug_prefix, player.id_.c_str());
        return false;
    }

    return true;
}

static AudioPath::Switch::DeselectedAudioSourceResult
deselect_source(const AudioPath::Paths &paths,
                const GVariantWrapper &request_data, std::string &source_id,
                AudioPath::Switch::PendingActivation &pending)
{
    BUG_IF(!source_id.empty() && pending.have_pending_activation(),
           "deselect source: have source ID and pending activation");

    if(source_id.empty() && !pending.have_pending_activation())
        return AudioPath::Switch::DeselectedAudioSourceResult::NONE;

    const std::string &deselected_id(!source_id.empty() ? source_id : pending.get_audio_source_id());
    const AudioPath::Source *old_source = paths.lookup_source(deselected_id);
    const auto result = source_id.empty()
        ? AudioPath::Switch::DeselectedAudioSourceResult::DESELECTED_PENDING
        : AudioPath::Switch::DeselectedAudioSourceResult::DESELECTED_ACTIVE;

    msg_vinfo(MESSAGE_LEVEL_DEBUG,
              "%sDeselect %saudio source %s (%s)", debug_prefix,
              source_id.empty() ? "pending " : "",
              old_source->id_.c_str(), old_source->name_.c_str());

    GErrorWrapper error;
    tdbus_aupath_source_call_deselected_sync(
        old_source->get_dbus_proxy().get_as_nonconst(), deselected_id.c_str(),
        GVariantWrapper::get(request_data), nullptr, error.await());

    if(error.log_failure("Deselect source"))
        msg_error(0, LOG_ERR, "%sDeselecting audio source %s failed",
                  debug_prefix, old_source->id_.c_str());

    source_id.clear();
    pending.clear();

    return result;
}

static bool select_source(const AudioPath::Source &source, bool is_final_select,
                          GVariantWrapper &&request_data)
{
    msg_vinfo(MESSAGE_LEVEL_DEBUG, "%sSelect audio source %s (%s)%s",
              debug_prefix, source.id_.c_str(), source.name_.c_str(),
              is_final_select ? "" : " (deferred)");

    GErrorWrapper error;

    if(is_final_select)
        tdbus_aupath_source_call_selected_sync(source.get_dbus_proxy().get_as_nonconst(),
                                               source.id_.c_str(),
                                               GVariantWrapper::get(request_data),
                                               nullptr, error.await());
    else
        tdbus_aupath_source_call_selected_on_hold_sync(source.get_dbus_proxy().get_as_nonconst(),
                                                       source.id_.c_str(),
                                                       GVariantWrapper::get(request_data),
                                                       nullptr, error.await());

    if(error.log_failure("Select source"))
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
                                   DeselectedAudioSourceResult &deselected_result,
                                   bool select_source_now)
{
    GVariantDict dict;
    g_variant_dict_init(&dict, nullptr);
    auto request_data(GVariantWrapper(g_variant_dict_end(&dict)));

    return activate_source(paths, source_id, player_id, deselected_result,
                           select_source_now, std::move(request_data));
}

AudioPath::Switch::ActivateResult
AudioPath::Switch::activate_source(const AudioPath::Paths &paths,
                                   const char *source_id,
                                   const std::string *&player_id,
                                   DeselectedAudioSourceResult &deselected_result,
                                   bool select_source_now,
                                   GVariantWrapper &&request_data)
{
    player_id = nullptr;
    deselected_result = DeselectedAudioSourceResult::NONE;

    if(source_id[0] == '\0')
    {
        msg_error(EINVAL, LOG_ERR, "%sEmpty audio source ID", debug_prefix);

        if(pending_.have_pending_activation())
            deselected_result = DeselectedAudioSourceResult::DESELECTED_PENDING;

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
        ActivateResult result;

        if(path.first == nullptr)
        {
            msg_error(0, LOG_NOTICE,
                      "%sUnknown audio source %s", debug_prefix, source_id);
            result = ActivateResult::ERROR_SOURCE_UNKNOWN;
        }
        else
        {
            msg_vinfo(MESSAGE_LEVEL_DEBUG,
                      "%sUnknown player %s for audio source %s (%s)",
                      debug_prefix,
                      path.first->player_id_.c_str(),
                      path.first->id_.c_str(), path.first->name_.c_str());
            result = ActivateResult::ERROR_PLAYER_UNKNOWN;
        }

        if(pending_.have_pending_activation())
            deselected_result = DeselectedAudioSourceResult::DESELECTED_PENDING;

        pending_.clear();
        return result;
    }
    else
        log_assert(path.first != nullptr);

    player_id = &path.second->id_;

    deselected_result =
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

AudioPath::Switch::ActivateResult
AudioPath::Switch::cancel_pending_source_activation(const AudioPath::Paths &paths,
                                                    std::string &source_id)
{
    if(!pending_.have_pending_activation())
        return ActivateResult::ERROR_SOURCE_UNKNOWN;

    const auto path(paths.lookup_path(pending_.get_audio_source_id()));

    pending_.take_audio_source_id(source_id);

    const AudioPath::Source *source = paths.lookup_source(source_id);
    auto request_data(pending_.clear());

    GErrorWrapper error;
    tdbus_aupath_source_call_deselected_sync(source->get_dbus_proxy().get_as_nonconst(),
                                             source_id.c_str(),
                                             GVariantWrapper::get(request_data),
                                             nullptr, error.await());

    current_source_id_.clear();

    if(error.log_failure("Deselect source (canceled)"))
    {
        msg_error(0, LOG_ERR,
                  "%sDeselecting audio source %s (canceled) failed",
                  debug_prefix, source->id_.c_str());
        return ActivateResult::ERROR_SOURCE_FAILED;
    }

    return ActivateResult::OK_PLAYER_SWITCHED;
}

AudioPath::Switch::ReleaseResult
AudioPath::Switch::release_path(const AudioPath::Paths &paths, bool kill_player,
                                const std::string *&player_id,
                                DeselectedAudioSourceResult &deselected_result)
{
    GVariantDict dict;
    g_variant_dict_init(&dict, nullptr);
    auto request_data(GVariantWrapper(g_variant_dict_end(&dict)));

    return release_path(paths, kill_player, player_id, deselected_result,
                        std::move(request_data));
}

AudioPath::Switch::ReleaseResult
AudioPath::Switch::release_path(const AudioPath::Paths &paths, bool kill_player,
                                const std::string *&player_id,
                                DeselectedAudioSourceResult &deselected_result,
                                GVariantWrapper &&request_data)
{
    const bool have_deselected_source = !current_source_id_.empty();
    const bool have_deactivated_player = kill_player && !current_player_id_.empty();

    msg_vinfo(MESSAGE_LEVEL_DEBUG,
              "%sRelease current audio path (%s), %s player",
              debug_prefix,
              have_deselected_source ? "<NONE>" : current_source_id_.c_str(),
              kill_player ? "deactivate" : "keep");

    deselected_result =
        deselect_source(paths, request_data, current_source_id_, pending_);

    if(have_deactivated_player)
        deactivate_player(paths, request_data, current_player_id_);

    player_id = current_player_id_.empty() ? nullptr : &current_player_id_;

    return (have_deselected_source
            ? (have_deactivated_player
               ? ReleaseResult::COMPLETE_RELEASE
               : ReleaseResult::SOURCE_DESELECTED)
            : (have_deactivated_player
               ? ReleaseResult::PLAYER_DEACTIVATED
               : ReleaseResult::UNCHANGED));
}
