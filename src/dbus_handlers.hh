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

#ifndef DBUS_HANDLERS_HH
#define DBUS_HANDLERS_HH

/*!
 * \addtogroup dbus_handlers DBus handlers for signals
 * \ingroup dbus
 */
/*!@{*/

#include "audiopath.hh"

namespace DBus
{

/*!
 * Data used in several D-Bus handlers.
 */
class HandlerData
{
  public:
    AudioPath::Paths audio_paths_;

    HandlerData(const HandlerData &) = delete;
    HandlerData &operator=(const HandlerData &) = delete;
    HandlerData(HandlerData &&) = default;

    explicit HandlerData() {}
};

}

/*!@}*/

#endif /* !DBUS_HANDLERS_HH */
