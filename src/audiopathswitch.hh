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

#ifndef AUDIOPATHSWITCH_HH
#define AUDIOPATHSWITCH_HH

#include <string>

/*!
 * \addtogroup audiopath
 */
/*!@{*/

namespace AudioPath
{

class Paths;

class Switch
{
  public:
    enum class ActivateResult
    {
        ERROR_SOURCE_UNKNOWN,
        ERROR_SOURCE_FAILED,
        ERROR_PLAYER_UNKNOWN,
        ERROR_PLAYER_FAILED,
        OK_UNCHANGED,
        OK_PLAYER_SAME,
        OK_PLAYER_SWITCHED,
    };

    enum class ReleaseResult
    {
        SOURCE_DESELECTED,
        PLAYER_DEACTIVATED,
        COMPLETE_RELEASE,
        UNCHANGED,
    };

  private:
    std::string current_source_id_;
    std::string current_player_id_;

  public:
    Switch(const Switch &) = delete;
    Switch &operator=(const Switch &) = delete;

    explicit Switch() {}

    ActivateResult activate_source(const Paths &paths, const char *source_id,
                                   const std::string *&player_id);
    ReleaseResult release_path(const Paths &paths, bool kill_player,
                               const std::string *&player_id);

    const std::string &get_player_id() const { return current_player_id_; }
};

}

/*!@}*/

#endif /* !AUDIOPATHSWITCH_HH */
