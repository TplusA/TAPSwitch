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

#if HAVE_CONFIG_H
#include <config.h>
#endif /* HAVE_CONFIG_H */

#include "appliance.hh"
#include "messages.h"

static bool set_state(Maybe<bool> &state, bool new_state, const char *what)
{
    if(state == new_state)
    {
        msg_error(0, LOG_WARNING, "Set %s again (unchanged)", what);
        return false;
    }

    state = new_state;

    return true;
}

void AudioPath::Appliance::set_power_state_unknown()
{
    is_up_and_running_.set_unknown();
}

bool AudioPath::Appliance::set_suspend_mode()
{
    return set_state(is_up_and_running_, false, "suspend mode");
}

bool AudioPath::Appliance::set_up_and_running()
{
    return set_state(is_up_and_running_, true, "powered mode");
}

void AudioPath::Appliance::set_audio_path_unknown()
{
    is_ready_for_playback_.set_unknown();
}

bool AudioPath::Appliance::set_audio_path_ready()
{
    return set_state(is_ready_for_playback_, true, "audio path ready");
}

bool AudioPath::Appliance::set_audio_path_blocked()
{
    return set_state(is_ready_for_playback_, false, "audio path blocked");
}
