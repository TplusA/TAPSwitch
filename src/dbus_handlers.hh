/*
 * Copyright (C) 2017, 2018, 2020  T+A elektroakustik GmbH & Co. KG
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

#ifndef DBUS_HANDLERS_HH
#define DBUS_HANDLERS_HH

/*!
 * \addtogroup dbus_handlers DBus handlers for signals
 * \ingroup dbus
 */
/*!@{*/

#include <vector>

#include "audiopath.hh"
#include "audiopathswitch.hh"
#include "appliance.hh"

namespace DBus
{

/*!
 * Data used in several D-Bus handlers.
 */
class HandlerData
{
  public:
    AudioPath::Paths audio_paths_;
    AudioPath::Switch audio_path_switch_;
    AudioPath::Appliance appliance_state_;

    struct Pending
    {
        void *const object_;
        void *const invocation_;

        constexpr Pending(void *object, void *invocation):
            object_(object),
            invocation_(invocation)
        {}
    };

    std::vector<Pending> pending_audio_source_activations_;

    HandlerData(const HandlerData &) = delete;
    HandlerData &operator=(const HandlerData &) = delete;
    HandlerData(HandlerData &&) = default;

    explicit HandlerData() {}
};

}

/*!@}*/

#endif /* !DBUS_HANDLERS_HH */
