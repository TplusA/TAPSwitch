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

#ifndef DBUS_PROXY_WRAPPER_HH
#define DBUS_PROXY_WRAPPER_HH

#include <memory>

namespace DBus
{

/*!
 * GLib D-Bus proxy wrapped into a class for ressource management.
 */
template <typename T>
class Proxy
{
  private:
    T *proxy_;

  public:
    Proxy(const Proxy &) = delete;
    Proxy &operator=(const Proxy &) = delete;

    explicit Proxy(T *proxy):
        proxy_(proxy)
    {}

    ~Proxy();

    const T *get() const { return proxy_; }
};

template <typename T>
std::unique_ptr<T> mk_proxy(const char *dest, const char *obj_path);

}

#endif /* !DBUS_PROXY_WRAPPER_HH */
