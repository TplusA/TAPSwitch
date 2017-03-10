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

static void deactivate_player(const AudioPath::Paths &paths,
                              std::string &player_id)
{
    if(player_id.empty())
        return;

    const AudioPath::Player *old_player = paths.lookup_player(player_id);

    GError *error = nullptr;
    tdbus_aupath_player_call_deactivate_sync(old_player->get_dbus_proxy().get_as_nonconst(),
                                             nullptr, &error);
    dbus_handle_error(&error, "Deactivate player");

    player_id.clear();
}

static bool activate_player(const AudioPath::Player &player)
{
    GError *error = nullptr;
    tdbus_aupath_player_call_activate_sync(player.get_dbus_proxy().get_as_nonconst(),
                                           nullptr, &error);
    return dbus_handle_error(&error, "Activate player");
}

static void deselect_source(const AudioPath::Paths &paths,
                            std::string &source_id)
{
    if(source_id.empty())
        return;

    const AudioPath::Source *old_source = paths.lookup_source(source_id);

    GError *error = nullptr;
    tdbus_aupath_source_call_deselected_sync(old_source->get_dbus_proxy().get_as_nonconst(),
                                             source_id.c_str(),
                                             nullptr, &error);
    dbus_handle_error(&error, "Deselect source");

    source_id.clear();
}

static bool select_source(const AudioPath::Source &source)
{
    GError *error = nullptr;
    tdbus_aupath_source_call_selected_sync(source.get_dbus_proxy().get_as_nonconst(),
                                           source.id_.c_str(),
                                           nullptr, &error);
    return dbus_handle_error(&error, "Select source");
}

AudioPath::Switch::ActivateResult
AudioPath::Switch::activate_source(const AudioPath::Paths &paths,
                                   const char *source_id,
                                   const std::string *&player_id)
{
    player_id = nullptr;

    if(source_id[0] == '\0')
        return ActivateResult::ERROR_SOURCE_UNKNOWN;

    if(source_id == current_source_id_)
    {
        player_id = &current_player_id_;
        return ActivateResult::OK_UNCHANGED;
    }

    const auto path(paths.lookup_path(source_id));

    if(path.second == nullptr)
    {
        return path.first == nullptr
            ? ActivateResult::ERROR_SOURCE_UNKNOWN
            : ActivateResult::ERROR_PLAYER_UNKNOWN;
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
    const bool have_delected_source = !current_source_id_.empty();
    const bool have_deactivated_player = kill_player && !current_player_id_.empty();

    if(have_delected_source)
        deselect_source(paths, current_source_id_);

    if(have_deactivated_player)
        deactivate_player(paths, current_player_id_);

    player_id = current_player_id_.empty() ? nullptr : &current_player_id_;

    return (have_delected_source
            ? (have_deactivated_player
               ? AudioPath::Switch::ReleaseResult::COMPLETE_RELEASE
               : AudioPath::Switch::ReleaseResult::SOURCE_DESELECTED)
            : (have_deactivated_player
               ? AudioPath::Switch::ReleaseResult::PLAYER_DEACTIVATED
               : AudioPath::Switch::ReleaseResult::UNCHANGED));
}
