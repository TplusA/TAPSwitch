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

#include "audiopathswitch.hh"
#include "audiopath.hh"
#include "dbus_iface_deep.h"
#include "messages.h"

static const char debug_prefix[] = "AUDIO SOURCE SWITCH: ";

static void deactivate_player(const AudioPath::Paths &paths,
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
                                             nullptr, &error);

    if(!dbus_handle_error(&error, "Deactivate player"))
        msg_error(0, LOG_ERR, "%sDeactivating player %s failed",
                  debug_prefix,  old_player->id_.c_str());

    player_id.clear();
}

static bool activate_player(const AudioPath::Player &player)
{
    msg_vinfo(MESSAGE_LEVEL_DEBUG, "%sActivate player %s (%s)",
              debug_prefix, player.id_.c_str(), player.name_.c_str());

    GError *error = nullptr;
    tdbus_aupath_player_call_activate_sync(player.get_dbus_proxy().get_as_nonconst(),
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
                            std::string &source_id)
{
    if(source_id.empty())
        return;

    const AudioPath::Source *old_source = paths.lookup_source(source_id);

    msg_vinfo(MESSAGE_LEVEL_DEBUG,
              "%sDeselect audio source %s (%s)", debug_prefix,
              old_source->id_.c_str(), old_source->name_.c_str());

    GError *error = nullptr;
    tdbus_aupath_source_call_deselected_sync(old_source->get_dbus_proxy().get_as_nonconst(),
                                             source_id.c_str(),
                                             nullptr, &error);

    if(!dbus_handle_error(&error, "Deselect source"))
        msg_error(0, LOG_ERR, "%sDeselecting audio source %s failed",
                  debug_prefix, old_source->id_.c_str());

    source_id.clear();
}

static bool select_source(const AudioPath::Source &source)
{
    msg_vinfo(MESSAGE_LEVEL_DEBUG, "%sSelect audio source %s (%s)",
              debug_prefix, source.id_.c_str(), source.name_.c_str());

    GError *error = nullptr;
    tdbus_aupath_source_call_selected_sync(source.get_dbus_proxy().get_as_nonconst(),
                                           source.id_.c_str(),
                                           nullptr, &error);

    if(!dbus_handle_error(&error, "Select source"))
    {
        msg_error(0, LOG_ERR, "%sSelecting audio source %s failed",
                  debug_prefix, source.id_.c_str());
        return false;
    }

    return true;
}

AudioPath::Switch::ActivateResult
AudioPath::Switch::activate_source(const AudioPath::Paths &paths,
                                   const char *source_id,
                                   const std::string *&player_id)
{
    player_id = nullptr;

    if(source_id[0] == '\0')
    {
        msg_error(EINVAL, LOG_ERR, "%sEmpty audio source ID", debug_prefix);
        return ActivateResult::ERROR_SOURCE_UNKNOWN;
    }

    if(source_id == current_source_id_)
    {
        msg_vinfo(MESSAGE_LEVEL_DEBUG,
                  "%sAudio source not changed", debug_prefix);
        player_id = &current_player_id_;
        return ActivateResult::OK_UNCHANGED;
    }

    const auto path(paths.lookup_path(source_id));

    if(path.second == nullptr)
    {
        if(path.first == nullptr)
        {
            msg_error(0, LOG_NOTICE,
                      "%sUnknown audio source %s", debug_prefix, source_id);
            return ActivateResult::ERROR_SOURCE_UNKNOWN;
        }
        else
        {
            msg_vinfo(MESSAGE_LEVEL_DEBUG,
                      "%sUnknown player %s for audio source %s (%s)",
                      debug_prefix,
                      path.first->player_id_.c_str(),
                      path.first->id_.c_str(), path.first->name_.c_str());
            return ActivateResult::ERROR_PLAYER_UNKNOWN;
        }
    }
    else
        log_assert(path.first != nullptr);

    player_id = &path.second->id_;

    deselect_source(paths, current_source_id_);

    const bool players_changed = (*player_id != current_player_id_);

    if(players_changed)
    {
        deactivate_player(paths, current_player_id_);

        if(!activate_player(*path.second))
            return ActivateResult::ERROR_PLAYER_FAILED;

        current_player_id_ = path.second->id_;
    }

    if(!select_source(*path.first))
        return ActivateResult::ERROR_SOURCE_FAILED;
    else
    {
        current_source_id_ = path.first->id_;

        return players_changed
            ? ActivateResult::OK_PLAYER_SWITCHED
            : ActivateResult::OK_PLAYER_SAME;
    }
}

AudioPath::Switch::ReleaseResult
AudioPath::Switch::release_path(const AudioPath::Paths &paths, bool kill_player,
                                const std::string *&player_id)
{
    const bool have_deselected_source = !current_source_id_.empty();
    const bool have_deactivated_player = kill_player && !current_player_id_.empty();

    msg_vinfo(MESSAGE_LEVEL_DEBUG,
              "%sRelease current audio path (%s), %s player",
              debug_prefix,
              have_deselected_source ? "<NONE>" : current_source_id_.c_str(),
              kill_player ? "deactivate" : "keep");

    if(have_deselected_source)
        deselect_source(paths, current_source_id_);

    if(have_deactivated_player)
        deactivate_player(paths, current_player_id_);

    player_id = current_player_id_.empty() ? nullptr : &current_player_id_;

    return (have_deselected_source
            ? (have_deactivated_player
               ? AudioPath::Switch::ReleaseResult::COMPLETE_RELEASE
               : AudioPath::Switch::ReleaseResult::SOURCE_DESELECTED)
            : (have_deactivated_player
               ? AudioPath::Switch::ReleaseResult::PLAYER_DEACTIVATED
               : AudioPath::Switch::ReleaseResult::UNCHANGED));
}
