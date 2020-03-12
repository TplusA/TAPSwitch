/*
 * Copyright (C) 2018, 2020  T+A elektroakustik GmbH & Co. KG
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

#ifndef APPLIANCE_HH
#define APPLIANCE_HH

/*!
 * \addtogroup audiopath Audio path management
 */
/*!@{*/

#include "maybe.hh"

namespace AudioPath
{

class Appliance
{
  private:
    Maybe<bool> is_up_and_running_;
    Maybe<bool> is_ready_for_playback_;

  public:
    Appliance(const Appliance &) = delete;
    Appliance &operator=(const Appliance &) = delete;

    explicit Appliance() {}

    const Maybe<bool> &is_up_and_running() const { return is_up_and_running_; }
    const Maybe<bool> &is_audio_path_ready() const { return is_ready_for_playback_; }

    void set_power_state_unknown();
    bool set_suspend_mode();
    bool set_up_and_running();

    void set_audio_path_unknown();
    bool set_audio_path_ready();
    bool set_audio_path_blocked();
};

}

/*!@}*/

#endif /* !APPLIANCE_HH */
