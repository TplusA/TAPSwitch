/*
 * Copyright (C) 2018, 2020  T+A elektroakustik GmbH & Co. KG
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
